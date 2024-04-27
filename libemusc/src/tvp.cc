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


#include "tvp.h"

#include <cmath>
#include <iostream>
#include <string.h>


namespace EmuSC {


TVP::TVP(ControlRom::InstPartial &instPartial, WaveGenerator *LFO[2],
	 Settings *settings,int8_t partId)
  : _sampleRate(settings->get_param_uint32(SystemParam::SampleRate)),
    _LFO1(LFO[0]),
    _LFO2(LFO[1]),
    _LFO1DepthPartial(instPartial.TVPLFODepth & 0x7f),
    _ahdsr(NULL),
    _multiplier(instPartial.pitchMult & 0x7f),
    _settings(settings),
    _partId(partId)
{
  // If TVP envelope phase durations are all 0 we only have a static filter
  // TODO: Verify that this is correct - what to do when P1-5 value != 0?
  if ((!instPartial.pitchDurP1 && !instPartial.pitchDurP2 &&
       !instPartial.pitchDurP3 && !instPartial.pitchDurP4 &&
       !instPartial.pitchDurP5))
    return;

  int phasePitchInit;           // Initial pitch for phase 1
  double phasePitch[5];         // Target phase pitch for phase 1-5
  uint8_t phaseDuration[5];     // Phase duration for phase 1-5

  phasePitchInit = instPartial.pitchLvlP0 - 0x40;
  phasePitch[0] = instPartial.pitchLvlP1 - 0x40;
  phasePitch[1] = instPartial.pitchLvlP2 - 0x40;
  phasePitch[2] = instPartial.pitchLvlP3 - 0x40;
  phasePitch[3] = instPartial.pitchLvlP4 - 0x40;
  phasePitch[4] = 0;

  phaseDuration[0] = instPartial.pitchDurP1 & 0x7F;
  phaseDuration[1] = instPartial.pitchDurP2 & 0x7F;
  phaseDuration[2] = instPartial.pitchDurP3 & 0x7F;
  phaseDuration[3] = instPartial.pitchDurP4 & 0x7F;
  phaseDuration[4] = instPartial.pitchDurP5 & 0x7F;

  bool phaseShape[5] = { 0, 0, 0, 0, 0 };

  _ahdsr = new AHDSR(phasePitch, phaseDuration, phaseShape, 0,
		     settings, partId, AHDSR::Type::TVP, phasePitchInit);
  _ahdsr->start();
}


TVP::~TVP()
{
  delete _ahdsr;
}


double TVP::get_pitch()
{
  // LFO1
  int lfo1DepthParam = _LFO1DepthPartial +
    _settings->get_param(PatchParam::VibratoDepth, _partId) - 0x40 +
    _settings->get_param(PatchParam::Acc_LFO1PitchDepth, _partId);
  float lfo1Depth = lfo1DepthParam * 0.0011;
  if (lfo1Depth < 0) lfo1Depth = 0;

  // LFO2
  int lfo2DepthParam = _settings->get_param(PatchParam::Acc_LFO2PitchDepth,
					    _partId);
  float lfo2Depth = lfo2DepthParam * 0.0011;
  if (lfo2Depth < 0) lfo2Depth = 0;

  double vibrato = (1 + (_LFO1->value() * lfo1Depth)) *
                   (1 + (_LFO2->value() * lfo2Depth));

  // TODO: Delay function -> seconds
  // sec = 0.5 * exp(_LFODelay * log(10.23 / 0.5) / 50)

  // Envelope pitch values in ROM seems to be in percent
  double pEnvelope = 1;
  if (_ahdsr)
    pEnvelope += (_ahdsr->get_next_value() * ((float) _multiplier / 6400));

  return vibrato * pEnvelope;
}


void TVP::note_off()
{
  if (_ahdsr)
    _ahdsr->release();
}

}
