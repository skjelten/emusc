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

// NOTES:
// Study of the output from a SC-55mkII shows that vibrato rate patch parameter
// modifies the LFO rate with 1/10 Hz, so that e.g. +10 increases LFO with 1 Hz.
// This is linear up to at least 9Hz before it becomes an exponential increase.

// Vibrato delay is hard to accurately measure. Seems to suppress the LFO
// following an exponential function, and then fade in from 0 to full depth
// over a period of 4 LFO frequency periods.

// Vibrato depth seems to be a linear function of vibrato depth in partial
// definition + vibrato depth in patch parameters up to the sum of 55.
//   Pitch adj(vibrato depth) = 0.0011 * (VD partial + VD patch param)
// When accumulated sum of vibrato depth exceeds 55, it seems to start following
// an exponential function.

// Pitch envelopes have a linear correlation between pitch envelope value and
// multiplier value: Pitch change in cents = 0.3 * multiplier * phase value


#include "tvp.h"

#include <cmath>
#include <iostream>
#include <string.h>


namespace EmuSC {


TVP::TVP(ControlRom::InstPartial &instPartial, uint8_t key,
         WaveGenerator *LFO[2], Settings *settings,int8_t partId)
  : _sampleRate(settings->get_param_uint32(SystemParam::SampleRate)),
    _key(key),
    _keyFreq(440 * exp(log(2) * (key - 69) / 12)),
    _LFO1(LFO[0]),
    _LFO2(LFO[1]),
    _LFO1DepthPartial(instPartial.TVPLFODepth & 0x7f),
    _expFactor(log(2) / 12000),
    _ahdsr(NULL),
    _multiplier(instPartial.pitchMult & 0x7f),
    _instPartial(instPartial),
    _settings(settings),
    _partId(partId)
{
  // If TVP envelope phase durations are all 0 we only have a static filter
  // TODO: Verify that this is correct - what to do when P1-5 value != 0?
  if ((!instPartial.pitchDurP1 && !instPartial.pitchDurP2 &&
       !instPartial.pitchDurP3 && !instPartial.pitchDurP4 &&
       !instPartial.pitchDurP5))
    return;

  _init_envelope();
  if (_ahdsr)
    _ahdsr->start();

  update_params();
}


TVP::~TVP()
{
  delete _ahdsr;
}


double TVP::get_pitch()
{
  float pitchAdj = _pitchOffsetHz * _pitchExp;

  double vibrato = (1 + (_LFO1->value() * _accLFO1Depth * 0.0011)) *
                   (1 + (_LFO2->value() * _accLFO2Depth * 0.0011));

  // TODO: Delay function -> seconds
  // sec = 0.5 * exp(_LFODelay * log(10.23 / 0.5) / 50)

  // Return only vibrato if pitch envelope is not present in this partial
  if (!_ahdsr)
    return pitchAdj * vibrato;

  // TODO: Make this calculation more CPU efficient
  float envelope = exp(_ahdsr->get_next_value() * 0.3 * _multiplier *
		       (log(2) / 1200.0));

  return pitchAdj * vibrato * envelope;
}


void TVP::note_off()
{
  if (_ahdsr)
    _ahdsr->release();
}


void TVP::update_params(void)
{
  // LFOs
  int lfoDepth = _LFO1DepthPartial +
    _settings->get_param(PatchParam::VibratoDepth, _partId) - 0x40 +
    _settings->get_param(PatchParam::Acc_LFO1PitchDepth, _partId);
  _accLFO1Depth = (lfoDepth > 0) ? (uint8_t) lfoDepth : 0;

  lfoDepth = _settings->get_param(PatchParam::Acc_LFO2PitchDepth, _partId);
  _accLFO2Depth = (lfoDepth > 0) ? (uint8_t) lfoDepth : 0;

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


void TVP::_init_envelope(void)
{
  int phasePitchInit;           // Initial pitch for phase 1
  double phasePitch[5];         // Target phase pitch for phase 1-5
  uint8_t phaseDuration[5];     // Phase duration for phase 1-5

  phasePitchInit = _instPartial.pitchLvlP0 - 0x40;
  phasePitch[0] = _instPartial.pitchLvlP1 - 0x40;
  phasePitch[1] = _instPartial.pitchLvlP2 - 0x40;
  phasePitch[2] = _instPartial.pitchLvlP3 - 0x40;
  phasePitch[3] = _instPartial.pitchLvlP4 - 0x40;
  phasePitch[4] = 0;

  phaseDuration[0] = _instPartial.pitchDurP1 & 0x7F;
  phaseDuration[1] = _instPartial.pitchDurP2 & 0x7F;
  phaseDuration[2] = _instPartial.pitchDurP3 & 0x7F;
  phaseDuration[3] = _instPartial.pitchDurP4 & 0x7F;
  phaseDuration[4] = _instPartial.pitchDurP5 & 0x7F;

  bool phaseShape[5] = { 0, 0, 0, 0, 0 };

  _ahdsr = new AHDSR(phasePitch, phaseDuration, phaseShape, 0,
		     _settings, _partId, AHDSR::Type::TVP, phasePitchInit);
}

}
