/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2023  Håkon Skjelten
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


TVP::TVP(ControlRom::InstPartial &instPartial, Settings *settings,int8_t partId)
  : _settings(settings),
    _partId(partId),
    _sampleRate(settings->get_param_uint32(SystemParam::SampleRate)),
    _LFO1(_sampleRate),
    _LFO2(_sampleRate),
    _ahdsr(NULL),
    _fade(0)
{
  // TODO: Figure out how the sine wave for pitch modulation is found on the
  //       Sound Canvas. In the meantime utilize a simple wavetable.
  _vibratoBaseFreq = lfo1RateTable[settings->get_param(PatchParam::ToneNumber2,
						       partId)];

  _LFO1DepthPartial = (instPartial.TVPLFODepth & 0x7f);

  // TODO:
  _delay = 0;//sampleRate / 2;

  // Fade in is measured on SC-55nmkII to be approx ~4 LFO periods in length
  _fadeIn = (4 / _vibratoBaseFreq) * _sampleRate;
  _fadeInStep = 1.0 / _fadeIn;

  // If TVP envelope phase durations are all 0 we only have a static filter
  // TODO: Verify that this is correct - what to do when P1-5 value != 0?
  if ((instPartial.pitchDurP1 + instPartial.pitchDurP2 + instPartial.pitchDurP3+
       instPartial.pitchDurP4 + instPartial.pitchDurP5) == 0)
    return;

  double phasePitchInit;        // Initial pitch for phase 1
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

  std::string id = "TVP (" + std::to_string(instPartial.partialIndex) + ")";

  _ahdsr = new AHDSR(phasePitchInit, phasePitch, phaseDuration, settings, partId, id);
  _ahdsr->start();
}


TVP::~TVP()
{
  delete _ahdsr;
}


double TVP::get_pitch()
{
  // LFO1
  float freq = _vibratoBaseFreq +
    ((_settings->get_param(PatchParam::VibratoRate, _partId) - 0x80 +
      _settings->get_param(PatchParam::Acc_LFO1RateControl, _partId)) * 0.1);
  if (freq > 0)
    _LFO1.set_frequency(freq);
  else
    _LFO1.set_frequency(0);
  int lfo1DepthParam = _LFO1DepthPartial +
    _settings->get_param(PatchParam::VibratoDepth, _partId) - 0x40 +
    _settings->get_param(PatchParam::Acc_LFO1PitchDepth, _partId);
  float lfo1Depth = lfo1DepthParam * 0.0011;
  if (lfo1Depth < 0) lfo1Depth = 0;

  // LFO2
  freq = 0 +                // FIXME: Add default frequency for LFO2
    (_settings->get_param(PatchParam::Acc_LFO2RateControl, _partId)-0x40) * 0.1;
  if (freq > 0)
    _LFO2.set_frequency(freq);
  else
    _LFO2.set_frequency(0);
  int lfo2DepthParam = _settings->get_param(PatchParam::Acc_LFO2PitchDepth,
					    _partId);
  float lfo2Depth = lfo2DepthParam * 0.0011;
  if (lfo2Depth < 0) lfo2Depth = 0;

  // TODO: Rewrite delay to a number of 32kHz samples
  // TODO: Does the LFO2 have a different delay / fade in period?
  double vibrato;
  if (_delay > 0) {                                           // Delay
    _delay--;
    vibrato = 1;

  } else {
    if (_fadeIn > 0) {                                        // Fade in
      _fadeIn--;
      _fade += _fadeInStep;
      vibrato = (1 + (_LFO1.next_sample() * _fade * lfo1Depth)) *
	(1 + (_LFO2.next_sample() * _fade * lfo2Depth));

    } else {                                                  // Full vibrato
      vibrato = (1 + (_LFO1.next_sample() * lfo1Depth)) *
	(1 + (_LFO2.next_sample() * lfo2Depth));
    }
  }

  // TODO: Delay function -> seconds
  // sec = 0.5 * exp(_LFODelay * log(10.23 / 0.5) / 50)

  // Pitch envelope
  // Envelope pitch values in ROM seems to be in percent.
  double pEnv = 1;
  if (_ahdsr)
    pEnv += _ahdsr->get_next_value() * 0.01;

  return vibrato * pEnv;
}


void TVP::note_off()
{
  if (_ahdsr)
    _ahdsr->release();
}

}
