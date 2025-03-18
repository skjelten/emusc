/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2024  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
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

// The pitch corrections for each sample can be diveded into 4 components:
//  - Static pitch corrections. Does not change and calculated once.
//  - Dynamic parameters. E.g. pitch bend and LFO rate. Updated approx 100Hz.
//  - Pitch modulation by LFOs (vibrato). Adjusted by dynamic paramters.
//  - Pitch envelope. Also adjusted by dynamic paramters.

// Pitch envelopes have a linear correlation between pitch envelope value and
// multiplier value: Pitch change in cents = 0.3 * multiplier * phase value



#include "tvp.h"

#include <cmath>
#include <iostream>
#include <string.h>


namespace EmuSC {


TVP::TVP(ControlRom::InstPartial &instPartial, uint8_t key, uint8_t velocity,
	 int keyShift, ControlRom::Sample *ctrlSample,
	 WaveGenerator *LFO1, WaveGenerator *LFO2, int pitchCurve,
         ControlRom::LookupTables &LUT, Settings *settings,int8_t partId)
  : _sampleRate(settings->sample_rate()),
    _key(key),
    _keyFreq(440 * exp(log(2) * (key - 69) / 12)),
    _LFO1(LFO1),
    _LFO2(LFO2),
    _LUT(LUT),
    _LFO1Depth(instPartial.TVPLFO1Depth & 0x7f),
    _LFO2Depth(instPartial.TVPLFO2Depth & 0x7f),
    _expFactor(log(2) / 12000),
    _envelope(NULL),
    _multiplier(instPartial.pitchEnvDepth & 0x7f),
    _instPartial(instPartial),
    _settings(settings),
    _partId(partId)
{
  _set_static_params(keyShift, ctrlSample, pitchCurve);
  update_dynamic_params();

  _init_envelope(velocity);
  if (_envelope)
    _envelope->start();
}


TVP::~TVP()
{
  delete _envelope;
}


double TVP::get_next_value()
{
  float fixedPitchAdj = _staticPitchCorr * _pitchOffsetHz * _pitchExp;

  float vibrato1 = (_LFO1->value() * _LUT.LFOTVPDepth[_accLFO1Depth]) / 3650;
  float vibrato2 = (_LFO2->value() * _LUT.LFOTVPDepth[_accLFO2Depth]) / 3650;
  float envelope = _envelope->get_next_value() * 0.3 * _multiplier * log(2);
  float dynPitchAdj = exp((envelope + vibrato1 + vibrato2) / 1200.0);

  return fixedPitchAdj * dynPitchAdj;
}


void TVP::note_off()
{
  if (_envelope)
    _envelope->release();
}


void TVP::update_dynamic_params(void)
{
  _accLFO1Depth = _LFO1Depth +
    (_settings->get_param(PatchParam::VibratoDepth, _partId) - 0x40) * 2 +
    _settings->get_param(PatchParam::Acc_LFO1PitchDepth, _partId);
  if (_accLFO1Depth < 0) _accLFO1Depth = 0;
  if (_accLFO1Depth > 127) _accLFO1Depth = 127;

  _accLFO2Depth = _LFO2Depth +
    _settings->get_param(PatchParam::Acc_LFO2PitchDepth, _partId);
  if (_accLFO2Depth < 0) _accLFO2Depth = 0;
  if (_accLFO2Depth > 127) _accLFO2Depth = 127;

  float freqKeyTuned = _keyFreq +
    (_settings->get_param_nib16(PatchParam::PitchOffsetFine,_partId) -0x080)/10;
  _pitchOffsetHz = freqKeyTuned / _keyFreq;

  _pitchExp = exp((_settings->get_param_32nib(SystemParam::Tune) - 0x400 +
                   (_settings->get_patch_param((int) PatchParam::ScaleTuningC +
                                               (_key % 12),_partId) - 0x40)*10 +
                   ((_settings->get_param_uint16(PatchParam::PitchFineTune,
                                                 _partId) - 16384)
                    / 16.384)) *
                  _expFactor);
}


void TVP::_set_static_params(int keyShift, ControlRom::Sample *ctrlSample,
                             int pitchCurve)
{
  int drumSet = _settings->get_param(PatchParam::UseForRhythm, _partId);

  int randomPitchDepth = 0;
  if (_instPartial.randPitch != 0) {
    randomPitchDepth =
      std::rand() % (_instPartial.randPitch * 2 + 1) - _instPartial.randPitch;
    if (randomPitchDepth < -100) randomPitchDepth = -100;
    if (randomPitchDepth > 100) randomPitchDepth = 100;
  }

  float pitchKeyFollow = 1;
  if (_instPartial.pitchKeyFlw - 0x40 != 10)
    pitchKeyFollow += ((float) _instPartial.pitchKeyFlw - 0x4a) / 10.0;

  // Find actual difference in key between NoteOn and sample
  int keyDiff = 0;
  if (drumSet) {
    keyDiff = keyShift + _settings->get_param(DrumParam::PlayKeyNumber,
                                              drumSet - 1, _key) - 0x3c;

  } else {                                        // Regular instrument
    keyDiff = _key + keyShift - ctrlSample->rootKey;
  }

  // Pitch correction table (in decicents)
  int pitchScaleCurve = 0;
  if (pitchCurve == 1)
    pitchScaleCurve = _LUT.PitchScale1[_key] - 0x8000;
  else if (pitchCurve == 2)
    pitchScaleCurve = _LUT.PitchScale2[_key] - 0x8000;
  else if (pitchCurve == 3)
    pitchScaleCurve = _LUT.PitchScale3[_key] - 0x8000;

  _staticPitchCorr =
    (exp(
         ( // Corrections in octaves (100 cents)
           ( _instPartial.coarsePitch - 0x40 +
             (float) keyDiff * pitchKeyFollow +
             (60 - ctrlSample->rootKey) * (1 - pitchKeyFollow) ) * 100 +

           // Correction in cents
           (pitchScaleCurve / 10) +
           _instPartial.finePitch - 0x40 +
           randomPitchDepth +
           ((ctrlSample->pitch - 1024) / 16)
         ) * log(2) / 1200)
    ) * 32000.0 / _settings->sample_rate();
}


void TVP::_init_envelope(uint8_t velocity)
{
  float phasePitch[6];
  uint8_t phaseDuration[6];

  if (_instPartial.pitchEnvVSens >= 0x40) {
    phasePitch[0] = _instPartial.pitchEnvL0 - 0x40;
    phasePitch[1] = _instPartial.pitchEnvL1 - 0x40;
    phasePitch[2] = _instPartial.pitchEnvL2 - 0x40;
    phasePitch[3] = _instPartial.pitchEnvL3 - 0x40;
    phasePitch[4] = 0;
    phasePitch[5] = _instPartial.pitchEnvL5 - 0x40;

  } else {      // Negative Pitch Envelope Velocity Sensitivty inverts envelope
    phasePitch[0] = 0x40 - _instPartial.pitchEnvL0;
    phasePitch[1] = 0x40 - _instPartial.pitchEnvL1;
    phasePitch[2] = 0x40 - _instPartial.pitchEnvL2;
    phasePitch[3] = 0x40 - _instPartial.pitchEnvL3;
    phasePitch[4] = 0;
    phasePitch[5] = 0x40 - _instPartial.pitchEnvL5;
  }

  // Scale envelope output by Pitch Envelope Velocity Sensitivty
  if (_instPartial.pitchEnvVSens != 0x40 && velocity != 127) {
    float vs = std::abs(_instPartial.pitchEnvVSens - 0x40) * (127 - velocity)
      * _envVSensLUT[std::abs(_instPartial.pitchEnvVSens - 0x40)];
    float vsFactor = pow(2, -vs / 1200.0);

    phasePitch[0] *= vsFactor;
    phasePitch[1] *= vsFactor;
    phasePitch[2] *= vsFactor;
    phasePitch[3] *= vsFactor;
    phasePitch[5] *= vsFactor;
  }

  phaseDuration[0] = 0;
  phaseDuration[1] = _instPartial.pitchEnvT1 & 0x7F;
  phaseDuration[2] = _instPartial.pitchEnvT2 & 0x7F;
  phaseDuration[3] = _instPartial.pitchEnvT3 & 0x7F;
  phaseDuration[4] = _instPartial.pitchEnvT4 & 0x7F;
  phaseDuration[5] = _instPartial.pitchEnvT5 & 0x7F;

  bool phaseShape[6] = { 0, 0, 0, 0, 0, 0 };

  _envelope = new Envelope(phasePitch, phaseDuration, phaseShape, _key, _LUT,
                           _settings, _partId, Envelope::Type::Pitch);

  // Adjust time for Envelope Time Key Follow
  if (_instPartial.pitchETKeyF14 != 0x40) { std::cout << "Pitch correction: " << (int) _instPartial.pitchETKeyF14 - 0x40 << std::endl;
    _envelope->set_time_key_follow(0, _instPartial.pitchETKeyF14 - 0x40);
  }
  if (_instPartial.pitchETKeyF5 != 0x40)
    _envelope->set_time_key_follow(1, _instPartial.pitchETKeyF5 - 0x40);
}

}
