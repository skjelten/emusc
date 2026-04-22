/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2026  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  libEmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libEmuSC. If not, see <http://www.gnu.org/licenses/>.
 */

// TODO: We need to be able to get the list of active notes so that we can
// check for active keys. And we need to know their env. phase
// Portamento seems to need to know the following:
//  Active key, phase 1-5 (attack -> sustain) -> legato mode
//  Active key, release phase -> portamento without legato
//  No active key, no portamento / legato at all


#include "pitch.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string.h>


namespace EmuSC {


int Pitch::_portaTargetPitch = 0;
std::array<int, 28> Pitch::_portaBasePitch = {};
int Pitch::_pbpIndex = -1;


Pitch::Pitch(ControlRom &ctrlRom, uint16_t instrumentIndex, int partialId,
             uint8_t key, uint8_t velocity, WaveGenerator *LFO1,
             WaveGenerator *LFO2, Settings *settings, int8_t partId)
  : Envelope(ctrlRom.lookupTables),
    _sampleRateScale((float) 32000 / (float) settings->sample_rate()),
    _key(key),
    _ctrlRom(ctrlRom),
    _instrumentIndex(instrumentIndex),
    _instPartial(ctrlRom.instrument(instrumentIndex).partials[partialId]),
    _LUT(ctrlRom.lookupTables),
    _LFO1(LFO1),
    _LFO2(LFO2),
    _lfo1FadeComplete(false),
    _lfo2FadeComplete(false),
    _sampleIndex(0xffff),
    _cachedPFineTune(0),
    _settings(settings),
    _partId(partId)
{
  _keyFollowOffset = (std::abs(_key - 60) *
      _LUT.PitchParamScale[std::abs(_instPartial.pitchKeyFlw - 0x49)] & 0xffff);

  _init_base_pitch();

  // Find sample index based on base pitch
  uint16_t pIndex = _instPartial.partialIndex;
  int breakPitch = _basePitchC - _instPartial.rootKeyOffset + 0x40;

  for (int j = 0; j < 16; j ++) {
    if (_ctrlRom.partial(pIndex).breaks[j] >= breakPitch ||
	_ctrlRom.partial(pIndex).breaks[j] == 0x7f) {
      _sampleIndex = _ctrlRom.partial(pIndex).samples[j];

      // If sample for a defined break turns out not to be defined, the partial
      // must be ignored. Difficult to say whether this is a bug in the Control
      // ROM or by design.
      if (_sampleIndex == 0xffff)
	throw(std::string("Undefined sample"));

      break;
    }
  }

  _init_envelope(velocity);

  update();
}


Pitch::~Pitch()
{}


int Pitch::_get_env_key_follow(int kfpRom, int kfRom)
{
  // This feature seems never to be used, but implement it anyways just in case
  int kmIndex = _LUT.KeyMapperIndex[std::min(0x78 + kfpRom, 0x87)] -
                _LUT.KeyMapperOffset;
  int8_t key = _LUT.KeyMapper[kmIndex + _key];

  if (kfRom == 0) {
    return 0x100;

  } else if (kfRom < 0) {
    kfRom = -kfRom;
    key = -key;

    if (key == 0)
      key = static_cast<int8_t>(0xff);
  }

  bool negativeKey = false;
  key -= 0x80;
  if (key == 0)
    return 0x100;

  if (key < 0) {
    key = -key;
    negativeKey = true;
  }

  int prod = _LUT.EnvTimeKeyFollowSens[kfRom] * (key & 0xff) * 2;
  int etsIndex = negativeKey ? 0x80 - (prod >> 8) : 0x80 + (prod >> 8);

  return _LUT.EnvTimeScale[etsIndex];
}


int Pitch::_get_env_time_velocity_sensitivity(int etvsRom, int velocity)
{
  if (etvsRom == 0)
    return 0x100;

  int prod = _LUT.EnvTimeKeyFollowSens[std::min(std::abs(etvsRom), 20)] *
             (127 - velocity);

  if (etvsRom > 0) {
    int tmp = 0x1fc0 - prod;
    if (tmp < 0x20)
      return 0xffff;
    else
      return (0x1fc000 / tmp) & 0xffff;
  }

  if (prod >= 0x1f41)
    return 4;

  return (0x810 * (0x1fc0 - prod)) >> 16;
}


int Pitch::_get_env_phase_rate(int etRom, int phase)
{
  int envTime;
  if (phase <= 4)
    envTime = _LUT.envelopeTime[etRom] * _envTimeKeyFlwT14;
  else
    envTime = _LUT.envelopeTime[etRom] * _envTimeKeyFlwT5;

  if (envTime >= 0xff0000)
    envTime = 0xffff;
  else
    envTime = (envTime >> 8) & 0xffff;

  envTime *= _envTimeVelSens;
  if (envTime >= 0xff0000)
    return 8;

  envTime = (envTime >> 8) & 0xffff;
  if (envTime <= 8)
    return 0xffff;

    return (0x80000 / envTime);// & 0xffff;
}


void Pitch::note_off()
{
  set_phase(Phase::Release);
}


void Pitch::update(void)
{
  // Update LFO depth parameters based on fade-in status
  int index = std::clamp((_instPartial.TVPLFO1Depth & 0x7f) - 0x80 + 2 *
                         _settings->get_param(PatchParam::VibratoDepth,_partId),
                         0, 0x7f);
  if (_lfo1FadeComplete) {
    _lfo1Depth = _LUT.LFOTVPDepth[index];
  } else {
    _lfo1Depth = (_LFO1->fade() * _LUT.LFOTVPDepth[index]) >> 16;
    if (_LFO1->fade() == UINT16_MAX)
      _lfo1FadeComplete = true;
  }

  index = _instPartial.TVPLFO2Depth & 0x7f;
  if (_lfo2FadeComplete) {
    _lfo2Depth = _LUT.LFOTVPDepth[index];
  } else {
    _lfo2Depth = (_LFO1->fade() * _LUT.LFOTVPDepth[index]) >> 16;
    if (_LFO2->fade() == UINT16_MAX)
      _lfo2FadeComplete = true;
  }

  _iterate_phase();
}


void Pitch::_init_base_pitch(void)
{
  _basePitchF = 0;
  _basePitchC = _instPartial.coarsePitch - 0x40;

  _apply_key_shifts_bp();
  _apply_key_follow_bp();
  _apply_scale_tuning_bp();
}


void Pitch::_apply_key_shifts_bp(void)
{
  // Base pitch is based on MIDI key + key shifts
  int drumSet = _settings->get_param(PatchParam::UseForRhythm, _partId);
  if (drumSet == 0) {
    int mKeyShift = _settings->get_param(SystemParam::KeyShift);
    int pKeyShift = _settings->get_param(PatchParam::PitchKeyShift, _partId);
    _basePitchC = std::clamp(_basePitchC + mKeyShift - 0x40, 0, 0x7f);
    _basePitchC = std::clamp(_basePitchC + pKeyShift - 0x40, 0, 0x7f);

  } else {
    int dKey = _settings->get_param(DrumParam::PlayKeyNumber, drumSet, _key);
    _basePitchC = std::clamp(_basePitchC + dKey, 0, 0x7f);

    if (0) { // TODO: Verify if this is only supposed to happen on mk2 models
      int pKeyShift = _settings->get_param(PatchParam::PitchKeyShift, _partId);
      _basePitchC = std::clamp(_basePitchC + pKeyShift - 0x40, 0, 0x7f);
    }
  }
}


void Pitch::_apply_key_follow_bp(void)
{
  int initBasePitchC = _basePitchC;
  _basePitchC = 0;

  int keyCenterDist;
  int drumSet = _settings->get_param(PatchParam::UseForRhythm, _partId);
  if (drumSet) {
    int dKey = _settings->get_param(DrumParam::PlayKeyNumber, drumSet, _key);
    keyCenterDist = dKey - 60;
  } else {
    keyCenterDist = _key - 60;
  }

  // Adjust for Pitch Key Follow parameter in Partial definition
  int keyFollow = _instPartial.pitchKeyFlw - 0x40;
  int sign = (keyCenterDist ^ keyFollow) < 0 ? -1 : 1;

  uint16_t ppScale = _LUT.PitchParamScale[std::abs(keyFollow)];
  int distCenter = ((std::abs(keyCenterDist) << 1)  * ppScale);

  if ((distCenter & 0xffff) < 0xfde8) {
    if (sign > 0) {
      _basePitchF = ((distCenter & 0xffff) * 999) / 65000;
      _basePitchC += ((distCenter >> 16) & 0xff);
    } else {
      _basePitchF = 1000 - ((distCenter & 0xffff) * 999) / 65000;
      _basePitchC += -(_basePitchC + ((distCenter >> 16) & 0xff) + 1);
    }

  } else {
    _basePitchF = 0;
    _basePitchC += ((distCenter >> 16) & 0xff) + 1;
    if (sign < 0) _basePitchC = -_basePitchC;
  }

  if (drumSet)
    _basePitchC = std::clamp(_basePitchC - keyCenterDist + initBasePitchC, 0, 0x7f);
  else
    _basePitchC = std::clamp(_basePitchC + 0x3c + initBasePitchC, 0, 0x7f);
}


void Pitch::_apply_scale_tuning_bp(void)
{
  int scaleTuning = _settings->get_patch_param((int) PatchParam::ScaleTuningC +
                                               (_key % 12), _partId) - 0x40;
  if (scaleTuning == 0)
    return;

  int keyFollow = _instPartial.pitchKeyFlw - 0x40;
  if (keyFollow == 0x40)  // Weired, but this is checked for..
    return;

  int sign = (scaleTuning ^ keyFollow) < 0 ? -1 : 1;
  uint16_t ppScale = _LUT.PitchParamScale[std::abs(keyFollow)];
  int distCenter = (std::abs(scaleTuning) << 1) * ppScale;

  if ((distCenter & 0xffff) < 65000) {
    int tmp = ((distCenter & 0xffff) / 6500);
    distCenter = (distCenter >> 16) * 10 + tmp;
  } else {
    distCenter = ((distCenter >> 16) + 1) * 10;
  }

  if (sign < 0)
    distCenter = -distCenter;

  _basePitchF += distCenter;
  if (_basePitchF < 0) {
    _basePitchF += 1000;
    _basePitchC -= 1;
  } else if (_basePitchF >= 1000) {
    _basePitchF -= 1000;
    _basePitchC += 1;
  }
}


int Pitch::_init_portamento(bool portamento, bool legato)
{
  int delta = 0;

  if (portamento) {
    int pControl = _settings->get_param(PatchParam::PortamentoControl, _partId);
    if (pControl != 0xFF) {
      uint32_t scaled = pControl * 1000;
      delta = (int32_t) scaled - _portaBasePitch[_pbpIndex];
    }

  } else {
    delta = _portaTargetPitch - _portaBasePitch[_pbpIndex];
  }

  if (0)
    std::cout << "Pitch init portamento(portamento=" << portamento
              << ", legato=" << legato << ") delta=" << delta << std::endl;

  // If legato, accumulate instead of reset
  if (legato)
    return _portamentoDelta + delta;

  return delta;
}


void Pitch::_init_envelope(uint8_t velocity)
{
  // Portamento Target Pitch is a static / global variable for target pitch.
  if (++_pbpIndex >= 24) _pbpIndex = 0;
  _portaTargetPitch = _portaBasePitch[_pbpIndex];
  _portaBasePitch[_pbpIndex] = (_basePitchC * 1000) + _basePitchF;

  int srKey = _ctrlRom.sample(_sampleIndex).rootKey * 1000;
  int sfPitch =  _ctrlRom.sample(_sampleIndex).pitchInit - 1024;
  _samplePitchOffsetInit = srKey - sfPitch;

  _samplePitchOffsetSust =
    _samplePitchOffsetInit - (_ctrlRom.sample(_sampleIndex).pitchSust - 1024);
  _samplePitchOffsetActive = _samplePitchOffsetInit;

  int pitchCurve = _ctrlRom.instrument(_instrumentIndex).pitchCurve;
  _portaBasePitch[_pbpIndex] += _get_pitch_curve_correction(pitchCurve);

  _portaBasePitch[_pbpIndex] += (_instPartial.finePitch - 0x40) * 10;
  _portaBasePitch[_pbpIndex] = std::max(0, _portaBasePitch[_pbpIndex]);

  int8_t rnd = static_cast<int8_t>((std::rand() % 0xffff) >> 8);
  int16_t delta = ((rnd < 0 ? -rnd : rnd) * _instPartial.randPitch + 0x80) >> 8;
  _portaBasePitch[_pbpIndex] += (rnd < 0 ? -delta : delta) * 10;
  _phaseLevel[4] += (rnd < 0 ? -delta : delta) * 10;

  // calculate the 0xffc5: Portamento needs to be on, mono mode
  int pOn = _settings->get_param(PatchParam::Portamento, _partId); // 1 = On
  int pMode = _settings->get_param(PatchParam::PolyMode, _partId); // 0 = mono
  // But how do we know of other keys being down? legato is trouble..
  bool usePortamento = (pOn && !pMode) ? true : false;
  _portamentoDelta = _init_portamento(usePortamento, 0);

  int pCtrl = _settings->get_param(PatchParam::PortamentoControl, _partId) +
              _instPartial.coarsePitch - 0x40;
  pCtrl = std::clamp(pCtrl, 0, 0x7f);

  if (0)
    std::cout << "TMP PORTAMENTO CTRL=" << pCtrl
              << " On=" << pOn
              << " polyMode=" << pMode
              << std::endl;

  // TODO: PORTAMENTO NOT COMPLETE!


  // Find Pitch Envelope Velocity Sensitvity
  int pitchEnvVSensRom = _instPartial.pitchEnvVSens - 0x40;
  if (pitchEnvVSensRom == 0) {
    _envVelSens = _LUT.PitchEnvDepth[_instPartial.pitchEnvDepth & 0x7f];

  } else {
    _envVelSens = _LUT.PitchEnvVelSens1[std::abs(pitchEnvVSensRom)] +
                 (_LUT.PitchEnvVelSens2[std::abs(pitchEnvVSensRom)] * velocity);
    _envVelSens *= _LUT.PitchEnvDepth[_instPartial.pitchEnvDepth & 0x7f];
    _envVelSens = (_envVelSens + 0x8000) >> 16;
  }

  // Find all initial phase levels from Instrument Partial def. in Control ROM
  int phaseLevelRom[6];
  phaseLevelRom[0] = _instPartial.pitchEnvL0 - 0x40;
  phaseLevelRom[1] = _instPartial.pitchEnvL1 - 0x40;
  phaseLevelRom[2] = _instPartial.pitchEnvL2 - 0x40;
  phaseLevelRom[3] = _instPartial.pitchEnvL3 - 0x40;
  phaseLevelRom[4] = 0;
  phaseLevelRom[5] = _instPartial.pitchEnvL5 - 0x40;

  // Negative Pitch Envelope Velocity Sensitivty inverts envelope
  if (pitchEnvVSensRom < 0) {
    for (int i = 0; i < 6; i ++)
      phaseLevelRom[i] = -phaseLevelRom[i];
  }

  // Convert phase levels to usable pitch levels
  for (int i = 0; i < 6; i ++) {
    int prod;
    if (std::abs(phaseLevelRom[i]) < 0x40)
      prod = (_envVelSens * _LUT.TVFEnvScale[std::abs(phaseLevelRom[i])]) << 1;
    else
      prod =  (_envVelSens * 0xff) << 1;
    int res = (prod >> 8) & 0xffff;

    _phaseLevel[i] = (phaseLevelRom[i] >= 0) ? _portaBasePitch[_pbpIndex] + res
                                             : _portaBasePitch[_pbpIndex] - res;
    _phaseLevel[i] = std::max(_phaseLevel[i], 0);
  }

  _phaseTime[0] = 0;
  _phaseTime[1] = _instPartial.pitchEnvT1 & 0x7F;
  _phaseTime[2] = _instPartial.pitchEnvT2 & 0x7F;
  _phaseTime[3] = _instPartial.pitchEnvT3 & 0x7F;
  _phaseTime[4] = _instPartial.pitchEnvT4 & 0x7F;
  _phaseTime[5] = _instPartial.pitchEnvT5 & 0x7F;

  _envTimeKeyFlwT14 = _get_env_key_follow(_instPartial.pitchETKeyFP14,
                                          _instPartial.pitchETKeyF14 - 0x40);
  _envTimeKeyFlwT5 = _get_env_key_follow(_instPartial.pitchETKeyFP5,
                                         _instPartial.pitchETKeyF5 - 0x40);

  _envTimeVelSens =
    _get_env_time_velocity_sensitivity(_instPartial.pitchEnvTVSens - 0x40,
                                       velocity);

  _envPhaseRate[0] = _get_env_phase_rate(_phaseTime[1], 1);
  _envPhaseRate[1] = _get_env_phase_rate(_phaseTime[1], 1);
  _envPhaseRate[2] = _get_env_phase_rate(_phaseTime[2], 2);
  _envPhaseRate[3] = _get_env_phase_rate(_phaseTime[3], 3);
  _envPhaseRate[4] = _get_env_phase_rate(_phaseTime[4], 4);
  _envPhaseRate[5] = _get_env_phase_rate(_phaseTime[5], 5);

  _init_new_phase(Phase::Attack1);
}


void Pitch::first_sample_run_complete(void)
{
  _samplePitchOffsetActive = _samplePitchOffsetSust;
  _cachedPFineTune = 0;                   // Reset fine tune cache
}


int Pitch::_get_pitch_curve_correction(int pitchCurve)
{
  // If pitch curve is 0 in the instrument definition -> no correction
  if (!pitchCurve)
    return 0;

  int lutIndex = _LUT.KeyMapperIndex[96 + pitchCurve] - _LUT.KeyMapperOffset;
  int key = _key + _instPartial.coarsePitch - 0x40;

  return (_LUT.KeyMapper[lutIndex + 2 * key] << 8) +
         _LUT.KeyMapper[lutIndex + 2 * key + 1] - 0x8000;
}


void Pitch::_iterate_phase(void)
{
  if (_phasePosition >= 0xffff) {
    if (_phase == Phase::Attack1) {
      _init_new_phase(Phase::Attack2);
    } else if (_phase == Phase::Attack2) {
      _init_new_phase(Phase::Decay1);
    } else if (_phase == Phase::Decay1) {
      _init_new_phase(Phase::Decay2);
    } else if (_phase == Phase::Decay2) {
      _init_new_phase(Phase::Sustain);
    } else if (_phase == Phase::Release) {
      _phase = Phase::Terminated;
      return;
    }
  }

  if (_phase == Phase::Sustain) {
    _phaseRemainder = 0;
    _releasePitch = _phaseLevel[4];
    _targetPitch =  _phaseLevel[4];

  } else {
    int loadScale = 1;    // TODO: Move to central location (Settings?) for
                          //       coordination between notes
    int step = loadScale + _phaseRemainder;
    _phaseRemainder = 0;

    int mul = _envPhaseRate[static_cast<int>(_phase)] * step;
    int phaseStepDelta = (mul >> 16);
    int phaseAccInc = (mul & 0xffff) + _phasePosition;

    if (phaseAccInc > 0xffff) {
      phaseAccInc &= 0xffff;
      phaseStepDelta += 1;
    }

    if (phaseStepDelta == 0) {
      _phasePosition = phaseAccInc;

    } else {
      if (phaseAccInc < 0xffff)
        phaseStepDelta -= 1;

      _phaseRemainder = ((phaseStepDelta << 16) / _envPhaseRate[0]) & 0xffff;
      _phasePosition = 0xffff;
    }

    uint32_t interp;
    int phaseStartValueF = _phaseStartValue & 0xffff;
    int phaseEndValueF = _phaseEndValue & 0xffff;
    if (_isAscending) {
      interp = _phaseStartValue +
               ((((phaseEndValueF - phaseStartValueF) & 0xffff) * _phasePosition) >> 16);
    } else {
      interp = _phaseStartValue -
               ((((phaseStartValueF - phaseEndValueF) & 0xffff) * _phasePosition) >> 16);
    }

    _releasePitch = interp;
    _targetPitch = interp;
  }

  // Apply global pitch modulation
  _targetPitch +=
    _settings->get_acc_control_param(Settings::ControllerParam::Pitch, _partId);
  _targetPitch = std::clamp(_targetPitch, 0, 0x1f018);

  // This is the envelope part of the pitch calculation
  _envelopeOut = _targetPitch;

  int lfoDepth = _lfo1Depth +
    _settings->get_acc_control_param(Settings::ControllerParam::LFO1PitchDepth,
                                     _partId);

  // Add LFO modulations
  lfoDepth = std::min(lfoDepth, 0x1770);
  int lfoProd = (_LFO1->value() << 1) * lfoDepth;
  int lfoMod = (lfoProd + 0x8000) >> 16;
  _targetPitch = std::max(_targetPitch + lfoMod, 0);

  lfoDepth = _lfo2Depth +
    _settings->get_acc_control_param(Settings::ControllerParam::LFO2PitchDepth,
                                     _partId);
  lfoDepth = std::min(lfoDepth, 0x1770);
  lfoProd = int32_t(_LFO2->value() << 1) * lfoDepth;
  lfoMod = (lfoProd + 0x8000) >> 16;
  _targetPitch = std::max(_targetPitch + lfoMod, 0);

  int masterTune = _settings->get_param_32nib(SystemParam::Tune) - 0x400;
  _targetPitch = std::max(_targetPitch + masterTune, 0);

  if (_portamentoDelta != 0) {
    int pTime = _settings->get_param(PatchParam::PortamentoTime, _partId);
    int pRate = _LUT.PortamentoRate[pTime];

    int pStep = 1 * pRate; //TODO: Move to central location (Settings?) for
                           //       coordination between notes

    if ((_portamentoDelta >> 16) >= 0) {
      _portamentoDelta = std::max(_portamentoDelta - pStep, 0);
    } else {
      _portamentoDelta = std::min(_portamentoDelta + pStep, 0);
    }

    _targetPitch += _portamentoDelta;
  }

  // Convert accumulated pitch values to oscillator frequency / phase increment
  int relativePitch = _targetPitch - _samplePitchOffsetActive - 12000;
  int octave = relativePitch / 12000;
  int frac   = relativePitch % 12000;
  uint16_t freq;

  if (relativePitch >= 0) {
    if(octave != 0)
      freq = 0xffff;
    else
      freq = _calc_phase_inc_from_pitch(frac);

  } else {
    if (frac != 0) {
      octave --;
      frac = 12000 + frac;
    }

    freq = _calc_phase_inc_from_pitch(frac);

    if (octave != 0)
      freq >>= std::abs(octave);
  }

  int phaseIncrement = freq;

  // Calculate new "Fine Tune" correction if it has changed
  int fineTune =
    _settings->get_param_nib16(PatchParam::PitchOffsetFine, _partId);

  if (fineTune != _cachedPFineTune) {
    _cachedPFineTune = fineTune;
    relativePitch = _samplePitchOffsetActive - 0x13c68;
    octave = relativePitch / 12000;
    frac   = relativePitch % 12000;

    if (relativePitch >= 0) {
      if (octave != 0)
        freq = 0xFFFF;
      else
        freq = _calc_phase_inc_from_pitch(frac);

    } else {
      if (frac != 0) {
        octave --;
        frac = 12000 + frac;
      }

      freq = _calc_phase_inc_from_pitch(frac);

      if (octave != 0)
        freq >>= std::abs(octave);
    }

    if (freq == 0) freq = 1;
    uint32_t res =
      (uint32_t) std::min(((std::abs(fineTune - 0x80)) << 16) / freq, 0x7fff);

    _cachedPFineTuneOffset = (fineTune < 0x80) ? -res : res;
  }

  _phaseIncrement =
    std::clamp(phaseIncrement + _cachedPFineTuneOffset, 0, 0xffff);
}


void Pitch::_init_new_phase(enum Phase newPhase)
{
  if (newPhase == Phase::Init) {
    std::cerr << "libEmuSC: Internal error, envelope in illegal state"
	      << std::endl;
    return;

  } else if (newPhase == Phase::Attack1 || newPhase == Phase::Attack2 ||
	     newPhase == Phase::Decay1 || newPhase == Phase::Decay2) {
    _phaseStartValue = _phaseLevel[static_cast<int>(_phase)];
    _phaseEndValue = _phaseLevel[static_cast<int>(newPhase)];
    _isAscending = (_phaseStartValue < _phaseEndValue) ? 1 : 0;

  } else if (newPhase == Phase::Sustain) {
    _phase = newPhase;

    _phaseStartValue = _phaseLevel[static_cast<int>(_phase)];
    _phaseEndValue = _phaseLevel[static_cast<int>(newPhase)];
    return;

  } else if (newPhase == Phase::Release) {
    _phaseStartValue = _releasePitch;
    _phaseEndValue = _phaseLevel[static_cast<int>(newPhase)];
    _isAscending = (_phaseStartValue < _phaseEndValue) ? 1 : 0;
  }

  _phaseDuration = _phaseTime[static_cast<int>(newPhase)];

  // TODO: Investigate this part
  _phaseDuration = _LUT.envelopeTime[std::clamp(_phaseDuration, 0, 127)];
  _phasePosition = 0;
  _phaseRemainder = 0;

  // Correct phase duration for Time Key Follow
  if (newPhase != Phase::Release)
    _phaseDuration = (_phaseDuration * _envTimeKeyFlwT14) >> 8;
  else
    _phaseDuration = (_phaseDuration * _envTimeKeyFlwT5) >> 8;

  // Correct phase duration for Time Velocity Sensitivity
  _phaseDuration = (_phaseDuration * _envTimeVelSens) >> 8;

  /*
  // Correct phase duration for Time Velocity Sensitivity
  if (newPhase == Phase::Attack1 || newPhase == Phase::Attack2)
    _phaseDuration = (_phaseDuration * _timeVelSensT1T2) >> 8;
  else
    _phaseDuration = (_phaseDuration * _timeVelSensT3T5) >> 8;
  */

  if (0) {
    std::cout << "New Pitch envelope phase: -> "
              << std::dec << static_cast<int>(newPhase)
	      << " (" << _phaseName[static_cast<int>(newPhase)] << "): Level = "
              << _phaseStartValue << " -> " << _phaseEndValue
	      << " | Time = 0x" << std::hex << _phaseDuration << " => "
	      << std::dec << (_phaseDuration * 8) / 32000.0 << "s" << std::endl;
  }

  _phase = newPhase;
}


int Pitch::_calc_phase_inc_from_pitch(int frac)
{
  uint16_t fine   = _LUT.PitchFineExp[frac & 0xff];
  uint16_t coarse = _LUT.PitchCoarseExp[(frac >> 8) & 0xff];

  unsigned int mul = ((uint32_t(fine) * coarse) >> 16) & 0xffff;
  unsigned int rotl2 = (mul << 2) | (mul >> 14);
  int and3 = (rotl2 & 0xff00) | (rotl2 & 3);

  return (and3 << 8 | and3 >> 8) + coarse;
}

}
