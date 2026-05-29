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

// TVF - Time Variant Filter
// The Sound Canvas is using a 2nd. order low or high pass filter for TVF.
// The "TVF Type" variable in partial definitions specifies the filter type or
// whether the TVF filter is disabled.

// If TVF filter is enabled, the "TVF Cutoff Frequency" in partial definition
// sets the base cutoff frequency, and envelope values are modifiers to this
// base frequency. The significance of the TVF Envelope on the cutoff frequency
// is controlled by the TVF Envelope Depth parameter.

// Cutoff Freq Key Follow scales filter freq with LUT index = "ROM / 10" keys.
// The Cutoff Freq Key Follow Direction has 4 modes:
//   0 => Adjust all keys with center on key 64
//   1 => Only adjust keys > C4
//   2 => Only adjust keys > C7
//   3 => Only adjust keys < C7

// To calculate the filter resonance there are two calculations needed:
//  * Read resonance value from partial def. in ROM and add 2x SysEx value
//  * Calculate the correct index and read the value from LUT.TVFResonanceFreq
// Whichever of the two values are lowest will be used for calculating the
// resonance from the LUT.Resonance lookup table.


#include "tvf.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string.h>


namespace EmuSC {


TVF::TVF(ControlRom::InstPartial &instPartial, uint8_t key, uint8_t velocity,
         WaveGenerator *LFO1, WaveGenerator *LFO2,ControlRom::LookupTables &LUT,
         Settings *settings, int8_t partId)
  : Envelope(LUT),
    _sampleRate(settings->sample_rate()),
    _LFO1(LFO1),
    _LFO2(LFO2),
    _lfo1FadeComplete(false),
    _lfo2FadeComplete(false),
    _LUT(LUT),
    _instPartial(instPartial),
    _resonance(0x40),
    _envLevel(0),
    _key(key),
    _settings(settings),
    _partId(partId)
{
  if (instPartial.TVFType == 0)
    _svf = new SVF(SVF::Mode::LowPass);
  else if (instPartial.TVFType == 1)
    _svf = new SVF(SVF::Mode::HighPass);
  else
    _svf = nullptr;

  if (_svf == nullptr)                       // TVF disabled
    return;

  _velocity = _get_velocity_from_vcurve(velocity);

  // TODO: RENAME TO _cofKeyFollow? Any relation to timeKeyFollow?
  _keyFollow = _get_cof_key_follow(_instPartial.TVFCFKeyFlw - 0x40);
  _coFreqVSens = _read_cutoff_freq_vel_sens(_instPartial.TVFCOFVSens - 0x40);
  _envDepth = _LUT.TVFEnvDepth[_instPartial.TVFEnvDepth];

  _init_freq_and_res();
  _init_envelope();

  update();
}


TVF::~TVF()
{
  delete _svf;
}


// TODO: Add suport for Cutoff freq V-sens
// TVF consists of two values: Filter cutoff frequency and resonance.
// The cutoff frequency is controlled by the envelope generator and have a
// "mode" variable controlling how the frequency values are interpolated /
// smoothed across and inside control loops of 256 samples. Resonance is a more
// static variable controlled by instrument definition and SySEx messsages.
void TVF::apply_sample_set(std::array<float, 256> &dryBus)
{
  // Skip filter calculation if filter is disabled for this partial 
  if (_svf == nullptr)
    return;

  for (int i = 0; i < 256; i++) {
    dryBus[i] = _svf->process_sample(dryBus[i]);
  }
}


void TVF::note_off()
{
  set_phase(Envelope::Phase::Release);
}


// Run regularly for every 256 samples @32k sample rate => 125Hz
void TVF::update(void)
{
  if (_svf == nullptr)                       // TVF disabled
    return;

  // Update LFO depth parameters based on fade-in status
  if (!_lfo1FadeComplete)
    _update_lfo_depth(1);
  if (!_lfo2FadeComplete)
    _update_lfo_depth(2);

  _iterate_phase();

  // Update filter coefficients
  _svf->set_cutoff_freq(_envLevel);
  _svf->set_resonance(_resonance);

  if (0)
    std::cout << std::hex << "0x" << _envLevel << "  -  " << _resonance
              << std::endl;
}


void TVF::_update_lfo_depth(int lfo)
{
  if (lfo == 1) {
    if (_LFO1->fade() != UINT16_MAX) {
      _lfo1Depth = (_LFO1->fade() *
		    _LUT.LFOTVFDepth[_instPartial.TVFLFO1Depth & 0x7f]) >> 16;
    } else {
      _lfo1Depth = _LUT.LFOTVFDepth[_instPartial.TVFLFO1Depth & 0x7f];
      _lfo1FadeComplete = true;
    }

  } else if (lfo == 2) {
    if (_LFO2->fade() != UINT16_MAX) {
      _lfo2Depth = (_LFO2->fade() *
		    _LUT.LFOTVFDepth[_instPartial.TVFLFO2Depth & 0x7f]) >> 16;
    } else {
      _lfo2Depth = _LUT.LFOTVFDepth[_instPartial.TVFLFO2Depth & 0x7f];
      _lfo2FadeComplete = true;
    }
  }
}


void TVF::_init_envelope(void)
{
  _phaseLevel[0] = 0x40;
  _phaseLevel[1] = _instPartial.TVFEnvL1;
  _phaseLevel[2] = _instPartial.TVFEnvL2;
  _phaseLevel[3] = _instPartial.TVFEnvL3;
  _phaseLevel[4] = _instPartial.TVFEnvL4;
  _phaseLevel[5] = _instPartial.TVFEnvL5;

  _phaseTime[0] = 0;                                     // Never used
  _phaseTime[1] = _instPartial.TVFEnvT1 & 0x7F;
  _phaseTime[2] = _instPartial.TVFEnvT2 & 0x7F;
  _phaseTime[3] = _instPartial.TVFEnvT3 & 0x7F;
  _phaseTime[4] = _instPartial.TVFEnvT4 & 0x7F;
  _phaseTime[5] = _instPartial.TVFEnvT5 & 0x7F;

  // Adjust time for Envelope Time Key Follow including Envelope Time Key Preset
  set_time_key_follow(Envelope::Type::TVF, 0, _key,
                      _instPartial.TVFETKeyF14 - 0x40, _instPartial.TVFETKeyFP14);
  set_time_key_follow(Envelope::Type::TVF, 1, _key,
                      _instPartial.TVFETKeyF5 - 0x40, _instPartial.TVFETKeyFP5);

  // Adjust time for Envelope Time Velocity Sensitivity
  set_time_velocity_sensitivity(Envelope::Type::TVF, 0,
                                _instPartial.TVFETVSens12 - 0x40, _velocity);
  set_time_velocity_sensitivity(Envelope::Type::TVF, 1,
                                _instPartial.TVFETVSens35 - 0x40, _velocity);

  if (0) {
    std::cout << "\nNew TVF envelope [" << std::dec << (int) _key << "]\n"
	      << " Attack 1: L=0 -> L=" << _phaseLevel[1]
	      << " T=" << _phaseTime[1] << std::endl
	      << " Attack 2: L=" << _phaseLevel[1] << " -> L=" << _phaseLevel[2]
	      << " T=" << _phaseTime[2] << std::endl
	      << " Decay 1: L=" << _phaseLevel[2] << " -> L=" << _phaseLevel[3]
	      << " T=" << _phaseTime[3] << std::endl
	      << " Decay 2: L=" << _phaseLevel[3] << " -> L=" << _phaseLevel[4]
	      << " T=" << _phaseTime[4] << std::endl
              << " Sustain: -- L=" << _phaseLevel[4] << std::endl
              << " Release: --> L=" << _phaseLevel[5]
	      << " T=" << _phaseTime[5] << std::endl;
  }

  // Initialization run in Phase=Off and with manually update LFO1 depth
  _lfo1Depth = (_LFO1->fade() * _lfo1Depth) >> 16;
  _iterate_phase();

  _init_new_phase(Phase::Attack1);
}


int TVF::_get_velocity_from_vcurve(uint8_t velocity)
{
  unsigned int address = _instPartial.TVFCOFVelCur * 128 + velocity;
  if (address > _LUT.VelocityCurves.size()) {
    std::cerr << "libEmuSC internal error: Illegal velocity curve used"
              << std::endl;
    return 0;
  }

  return _LUT.VelocityCurves[address];
}


int TVF::_read_cutoff_freq_vel_sens(int cofvsROM)
{
  int v = 127 - _velocity;
  int res = 0x7fff;
  if (cofvsROM != 0)
    res -= ((v * _LUT.TVFCutoffVSens[std::abs(cofvsROM)]) & 0xffff);

  return res;
}


int TVF::_get_level_init(int level)
{
  int depth = (_envDepth * _coFreqVSens) * 2;
  int scale = _LUT.TVFEnvScale[std::clamp(std::abs(level - 0x40), 0, 63)];
  int tmp = (scale * ((depth & 0xffff0000) >> 16)) * 2;
  int res = ((tmp & 0x0000ff00) >> 8) + ((tmp & 0x00ff0000) >> 8);

  if (level >= 0x40)
    res += _keyFollow;
  else
    res = _keyFollow - res;

  return res;
}


void TVF::_init_freq_and_res(void)
{
  _L1Init = _get_level_init(_instPartial.TVFEnvL1);
  _L2Init = _get_level_init(_instPartial.TVFEnvL2);
  _L3Init = _get_level_init(_instPartial.TVFEnvL3);
  _L4Init = _get_level_init(_instPartial.TVFEnvL4);
  _L5Init = _get_level_init(_instPartial.TVFEnvL5);

  int envLevelMax = _keyFollow;
  envLevelMax = std::max(envLevelMax, _L1Init);
  envLevelMax = std::max(envLevelMax, _L2Init);
  envLevelMax = std::max(envLevelMax, _L3Init);
  envLevelMax = std::max(envLevelMax, _L4Init);
  envLevelMax = std::max(envLevelMax, _L5Init);

  int tm3 = _settings->get_param(PatchParam::TVFCutoffFreq, _partId) - 0x40;
  tm3 = std::clamp(tm3, -0x32, 0x10);
  int bptm3 = tm3 < 0 ? _instPartial.TVFBaseFlt + tm3 : _instPartial.TVFBaseFlt;
  int cofIndex = std::clamp(envLevelMax + (bptm3 << 8), 0, 0x7fff);
  cofIndex = std::min(cofIndex + 0xff, 0x7fff);

  int cof = _LUT.TVFCutoffFreq[cofIndex >> 8];
  int rfIndex = (2 * cof) + 0xff;
  rfIndex = rfIndex > 0xffff ? 0xff00 : rfIndex;
  _resIndexFreq = _LUT.TVFResonanceFreq[(rfIndex >> 8)];

  int tm4 = _settings->get_param(PatchParam::TVFResonance, _partId) - 0x40;
  tm4 = std::clamp(tm4, -0x32, 0x32);
  _resonance = std::min(_instPartial.TVFResonance - (tm4 * 2), _resIndexFreq);
  _resonance = std::max(_resonance, 0);
}


int TVF::_get_cof_key_follow(int cofkfROM)
{
  int kmIndex = _LUT.KeyMapperIndex[48 + _instPartial.TVFCFKeyFlwC] - _LUT.KeyMapperOffset;
  int km = static_cast<int>(_native_endian_uint16((uint8_t *) &_LUT.KeyMapper[kmIndex + _key * 2]));
  int cofkf = _LUT.TVFCutoffFreqKF[std::abs(cofkfROM)];
  int res = ((km - 0x4000) * cofkf) >> 8;

  if (0)
    std::cout << "km=0x" << std::hex << km
	      << " cofkfROM=" << std::dec << cofkfROM
	      << " mulxu.w=0x" << std::hex << ((km - 0x4000) * cofkf)
	      << " res=" << res << std::dec
	      << std::endl;

  if (cofkfROM < 0)
    return  -res;

  return res;
}


uint16_t TVF::_native_endian_uint16(uint8_t *ptr)
{
  if (_le_native())
    return (ptr[0] << 8 | ptr[1]);

  return (ptr[1] << 8 | ptr[0]);
}


void TVF::_iterate_phase(void)
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

  int segmentCurveIndex = 0;          // -0x30  TODO: FIX SUSTAIN -> 8 always!

  if (_phase == Phase::Init) {                     // Initialization run
    _ipLevelInit = _keyFollow & 0xffff;

  } else if (_phase == Phase::Sustain) {
    _phaseRemainder = 0;
    segmentCurveIndex = 8;
    _ipLevelInit = _L5Init & 0xffff;

  } else if (_phaseDuration <= 8) {               // Very short phase duration
    _phasePosition = 0xffff;
    segmentCurveIndex = _phaseDuration;
    _ipLevelInit = _currentLevelInit & 0xffff;

  } else {                                        // Normal phase duration
    _phaseStepSize = (8 << 16) / _phaseDuration;
    segmentCurveIndex = 8;

    int loadScale = 1;    // TODO: Move to central location (Settings?) for
                          //       coordination between notes
    int step = loadScale + _phaseRemainder;
    _phaseRemainder = 0;

    int mul = _phaseStepSize * step;
    int phaseStepDelta = (mul >> 16);
    int phaseAccInc = (mul & 0xffff) + _phasePosition;

    if (phaseAccInc > 0xffff) {
      phaseAccInc &= 0xffff;
      phaseStepDelta += 1;
    }

    if (phaseStepDelta != 0) {
      if (phaseAccInc < 0xffff)
        phaseStepDelta -= 1;

      _phaseRemainder = (phaseStepDelta / _phaseStepSize) & 0xffff;
      _phasePosition = 0xffff;

      _ipLevelInit = _currentLevelInit & 0xffff;

    } else {
      _phasePosition = phaseAccInc;

      // Interpolation of Level Init values
      int prev = _prevLevelInit & 0xffff;
      int curr = _currentLevelInit & 0xffff;
      int phase = _phasePosition;

      if (prev == curr) {
        _ipLevelInit = prev;

      } else if ((prev ^ curr) & 0x8000) {             // Different signs
        int16_t absPrev = std::abs(prev);
        int16_t absCurr = std::abs(curr);
        int16_t mag = std::abs(absCurr - absPrev);
        if (mag < 0) mag = std::numeric_limits<int16_t>::max();

        int16_t scaled = (int16_t) ((uint32_t(uint16_t(mag)) * phase) >> 16);
        int16_t magResult = (absCurr >= absPrev) ? absPrev + scaled : absPrev - scaled;
        _ipLevelInit = (curr < 0) ? -magResult : magResult;

      } else {                                         // Same signs
        int16_t mag = std::abs(curr - prev);
        if (mag < 0) mag = std::numeric_limits<int16_t>::max();

        int16_t scaled = (int16_t) ((uint32_t(uint16_t(mag)) * phase) >> 16);
        _ipLevelInit = (curr >= prev) ? prev + scaled : prev - scaled;
      }
    }
  }

  int tmCF = _settings->get_param(PatchParam::TVFCutoffFreq, _partId);
  tmCF = std::clamp(tmCF, 0xe, 0x50);
  int accCoFreq = std::clamp(_instPartial.TVFBaseFlt + (tmCF - 0x40), 0,
                          (int) _instPartial.TVFBaseFlt);

  uint16_t accDepth = _ipLevelInit + (accCoFreq << 8);
  accDepth = std::clamp((int) accDepth, 0, INT16_MAX);

  int accCutoffCtrl = _settings->get_acc_control_param(Settings::ControllerParam::TVFCutoff, _partId);

  accDepth = std::clamp(accDepth + std::abs(accCutoffCtrl), 0, (int) INT16_MAX);

  // This is how far the "TVF envelope" goes
  _envelopeOut = accDepth >> 8;

  // Add LFO modulations to cutoff frequency
  int lfoDepth = std::abs(_lfo1Depth +
                          _settings->get_acc_control_param(Settings::ControllerParam::LFO1TVFDepth, _partId));
  lfoDepth = std::min(lfoDepth, 0x1800);
  int lfoProd = (_LFO1->value() << 1) * lfoDepth;
  int lfoMod = (lfoProd + 0x8000) >> 16;
  accDepth = std::clamp(accDepth + lfoMod, 0, INT16_MAX);

  lfoDepth = std::abs(_lfo2Depth +
                      _settings->get_acc_control_param(Settings::ControllerParam::LFO2TVFDepth, _partId));
  lfoDepth = std::min(lfoDepth, 0x1800);
  lfoProd = int32_t(_LFO2->value() << 1) * lfoDepth;
  lfoMod = (lfoProd + 0x8000) >> 16;
  accDepth = std::clamp(accDepth + lfoMod, 0, INT16_MAX);

  int tmRes = _settings->get_param(PatchParam::TVFResonance, _partId) - 0x40;
  tmRes = std::clamp(_instPartial.TVFResonance - tmRes * 2,
                     0, _resIndexFreq);

  if ((tmRes & 0xff) != _resonance) {
    if ((tmRes & 0xff) < _resonance)
      _resonance -= 1;
    else
      _resonance += 1;

    int resFreqIndex = std::min(_envLevel + 0xff, 0xff00);
    int resFreq = _LUT.TVFResonanceFreq[resFreqIndex >> 8];
    _resonance = std::min(_resonance, resFreq);
  }

  int ipCoFreq;
  int coFreq1 = _LUT.TVFCutoffFreq[(accDepth >> 8)];

  if (accDepth == 0) {
    ipCoFreq = coFreq1;

  } else {
    int coFreq2 = _LUT.TVFCutoffFreq[(accDepth >> 8) + 1];
    ipCoFreq = coFreq1 + (((coFreq2 - coFreq1) * (accDepth & 0xff)) >> 8);
  }

  ipCoFreq *= 2;
  _resonance = std::max(_resonance, 8);

  int res = _LUT.TVFResonance[_resonance] << 8;
  if (res < ipCoFreq)
    ipCoFreq = res;

  ipCoFreq = std::min(ipCoFreq, 0xe600);

  _prevEnvLevel = _envLevel;
  _envLevel = ipCoFreq;

  int intEnvValue = _envLevel;
  if (_envLevel == _prevEnvLevel) {
    _envLevelMode = 0xff00;
    return;

  } else if (_envLevel > _prevEnvLevel) {
    intEnvValue &= 0xff00;
    if (intEnvValue == (_prevEnvLevel & 0xff00)) {
      intEnvValue += 0x100;
      intEnvValue = std::max(intEnvValue,
                             _LUT.TVFCutoffFreq[_resonance] << 8);
    }
  }

  intEnvValue = std::min(intEnvValue, 0xe600);

  if (segmentCurveIndex == 0) {
    _envLevelMode = (intEnvValue & 0xff00) | 0xaf;      // Env. segment acc. preload
    return;
  }

  int segmentStepIndex = _LUT.EnvSegmentCurve[segmentCurveIndex];
  int phaseAccumulator = _envLevel - _prevEnvLevel;
  if (phaseAccumulator < 0) phaseAccumulator = -phaseAccumulator;

  for (int i = 0; i < 8; i++) {
    uint16_t prev = phaseAccumulator;
    phaseAccumulator <<= 1;
    if(0)
      std::cout << "prev=" << prev << std::endl;
    if (prev & 0x8000)
      break;

    segmentStepIndex --;
  }

  if (segmentStepIndex < 0) {
    segmentStepIndex = 0;
    phaseAccumulator >>= 1;
  }

  int tvfLow = (phaseAccumulator >> 8) & 0xff;
  tvfLow = ((tvfLow >> 3) + 1) >> 1;
  tvfLow |= _LUT.EnvSegmentStep[segmentStepIndex];

  _envLevelMode = (intEnvValue & 0xff00) + tvfLow;
}


void TVF::_init_new_phase(enum Phase newPhase)
{
  if (newPhase == Phase::Terminated) {
    std::cerr << "libEmuSC: Internal error, envelope in illegal state"
	      << std::endl;
    return;

  } else if (newPhase == Phase::Attack1) {
    _prevLevelInit = _L2Init;
    _currentLevelInit = _L1Init;

    _currentEnvTime = _phaseTime[static_cast<int>(newPhase)];

    _phaseStartValue = _phaseLevel[static_cast<int>(_phase)];
    _phaseEndValue = _phaseLevel[static_cast<int>(newPhase)];

  } else if (newPhase == Phase::Attack2) {
    _prevLevelInit = _L1Init;
    _currentLevelInit = _L2Init;

    _currentEnvTime = _phaseTime[static_cast<int>(newPhase)];

    _phaseStartValue = _phaseLevel[static_cast<int>(_phase)];
    _phaseEndValue = _phaseLevel[static_cast<int>(newPhase)];

  } else if (newPhase == Phase::Decay1) {
    _prevLevelInit = _L2Init;
    _currentLevelInit = _L3Init;

    _currentEnvTime = _phaseTime[static_cast<int>(newPhase)];

    _phaseStartValue = _phaseLevel[static_cast<int>(_phase)];
    _phaseEndValue = _phaseLevel[static_cast<int>(newPhase)];

  } else if (newPhase == Phase::Decay2) {
    _prevLevelInit = _L3Init;
    _currentLevelInit = _L4Init;

    _currentEnvTime = _phaseTime[static_cast<int>(newPhase)];

    _phaseStartValue = _phaseLevel[static_cast<int>(_phase)];
    _phaseEndValue = _phaseLevel[static_cast<int>(newPhase)];

  } else if (newPhase == Phase::Sustain) {
    _phase = newPhase;

    if (_envLevel == 0)
      _finished = true;

    return;

  } else if (newPhase == Phase::Release) {
    _prevLevelInit = _ipLevelInit;
    _currentLevelInit = _L5Init;
    _currentEnvTime = _phaseTime[static_cast<int>(newPhase)];

    _phaseStartValue = _envLevel >> 8;  //_envLevelMode; // (_envLevelMode >> 8);
    _phaseEndValue = _phaseLevel[static_cast<int>(newPhase)];
  }

  _phaseDuration = _phaseTime[static_cast<int>(newPhase)];

  // TODO: Add test for PD#4 on TVF envelopes
  if (newPhase == Phase::Attack1 || newPhase == Phase::Attack2) {
    _phaseDuration +=
      (_settings->get_param(PatchParam::TVFAEnvAttack, _partId) - 0x40) * 2;

  } else if (newPhase == Phase::Decay1 || newPhase == Phase::Decay2) {
    _phaseDuration +=
      (_settings->get_param(PatchParam::TVFAEnvDecay, _partId) - 0x40) * 2;

  } else if (newPhase == Phase::Release) {
    _phaseDuration +=
      (_settings->get_param(PatchParam::TVFAEnvRelease, _partId) - 0x40) * 2;
  }

  _phaseDuration = _LUT.envelopeTime[std::clamp(_phaseDuration, 0, 127)];
  _phasePosition = 0;
  _phaseRemainder = 0;

  // Correct phase duration for Time Key Follow
  if (newPhase != Phase::Release)
    _phaseDuration = (_phaseDuration * _timeKeyFlwT1T4) >> 8;
  else
    _phaseDuration = (_phaseDuration * _timeKeyFlwT5) >> 8;

  // Correct phase duration for Time Velocity Sensitivity
  if (newPhase == Phase::Attack1 || newPhase == Phase::Attack2)
    _phaseDuration = (_phaseDuration * _timeVelSensT1T2) >> 8;
  else
    _phaseDuration = (_phaseDuration * _timeVelSensT3T5) >> 8;

  if (0) {
    std::cout << "New TVF envelope phase: -> "
              << std::dec << static_cast<int>(newPhase)
	      << " (" << _phaseName[static_cast<int>(newPhase)] << "): Level = "
              << _phaseStartValue << " -> " << _phaseEndValue
	      << " | Time = 0x" << std::hex << _phaseDuration << " => "
	      << std::dec << (_phaseDuration * 8) / 32000.0 << "s" << std::endl;
  }

  _phase = newPhase;
}

}
