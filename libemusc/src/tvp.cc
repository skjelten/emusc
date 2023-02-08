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
    _LFO(_sampleRate),
    _ahdsr(NULL),
    _fade(0)
{
  /*
  double phasePitch[5];         // Phase volume for phase 1-5
  double phaseDuration[5];      // Phase duration for phase 1-5
  bool   phaseShape[5];         // Phase shape for phase 1-5

  // Set adjusted values for volume (0-127) and time (seconds)
  phasePitch[0] = instPartial.pitchLvlP1;
  phasePitch[1] = instPartial.pitchLvlP2;
  phasePitch[2] = instPartial.pitchLvlP3;
  phasePitch[3] = instPartial.pitchLvlP4;
  phasePitch[4] = 0;

  phaseDuration[0] = _convert_time_to_sec(instPartial.pitchDurP1 & 0x7F);
  phaseDuration[1] = _convert_time_to_sec(instPartial.pitchDurP2 & 0x7F);
  phaseDuration[2] = _convert_time_to_sec(instPartial.pitchDurP3 & 0x7F);
  phaseDuration[3] = _convert_time_to_sec(instPartial.pitchDurP4 & 0x7F);
  phaseDuration[4] = _convert_time_to_sec(instPartial.pitchDurRel & 0x7F);

  phaseShape[0] = (instPartial.pitchDurP1 & 0x80) ? 0 : 1;
  phaseShape[1] = (instPartial.pitchDurP2 & 0x80) ? 0 : 1;
  phaseShape[2] = (instPartial.pitchDurP3 & 0x80) ? 0 : 1;
  phaseShape[3] = (instPartial.pitchDurP4 & 0x80) ? 0 : 1;
  phaseShape[4] = (instPartial.pitchDurRel & 0x80) ? 0 : 1;
*/
//  _ahdsr = new AHDSR(phasePitch, phaseDuration, phaseShape, sampleRate);
//  _ahdsr->start();

  // TODO: Figure out how the sine wave for pitch modulation is found on the
  //       Sound Canvas. In the meantime utilize a simple wavetable.
  _vibratoBaseFreq = lfoRateTable[settings->get_param(PatchParam::ToneNumber2,
						      partId)];

  _LFODepth = (instPartial.TVPLFODepth & 0x7f);       // LFOPartDepth

  // TODO:
  _delay = 0;//sampleRate / 2;

  // Fade in is measured on SC-55nmkII to be approx ~4 LFO periods in length
  _fadeIn = (4 / _vibratoBaseFreq) * _sampleRate;
  _fadeInStep = 1.0 / _fadeIn;
}


TVP::~TVP()
{
  delete _ahdsr;
}


double TVP::_convert_time_to_sec(uint8_t time)
{
  if (time == 0)
    return 0;

  return (pow(2.0, (double)(time) / 18.0) / 5.45 - 0.183);
}


double TVP::get_pitch()
{
  // LFO rate: Linear between 0 and 9 Hz.
  // TODO: Find a function or LUT to also correctly handle higher frequencies
  float freq = _vibratoBaseFreq + (_settings->get_param(PatchParam::VibratoRate,
							_partId) - 0x40) * 0.1;
  if (freq > 0)
    _LFO.set_frequency(freq);
  else
    _LFO.set_frequency(0);

  // TODO: Properly handle ModWheel with all paramters from
  //       "Modulation Controller #1"
  float modWheelPitch = 0;
  uint8_t modWheel = _settings->get_param(PatchParam::Modulation, _partId);
  if (modWheel)
    modWheelPitch = 0.001;

  // TODO: Move recalculation of vibrato depth to separate function when values
  // change
  double vibrato;
  if (_delay > 0) {                                         // Delay
    _delay--;
    vibrato = 1;

  } else {

    int lfoDepth2 = _LFODepth +
      _settings->get_param(PatchParam::VibratoDepth, _partId) - 0x40;
    float lfoDepth3 = 0.0011 * lfoDepth2;
    if (lfoDepth3 < 0) lfoDepth3 = 0;

    if (_fadeIn > 0) {                                 // Fade in
      _fadeIn--;
      _fade += _fadeInStep;
      vibrato = 1 + (_LFO.next_sample() * _fade * lfoDepth3);

    } else {                                                  // Full vibrato
      vibrato = 1 + (_LFO.next_sample() * lfoDepth3); // + modWheelPitch));
    }
  }

  // TODO: Delay function -> seconds
  // sec = 0.5 * exp(_LFODelay * log(10.23 / 0.5) / 50)

  // TODO: Implement pitch envelope
  //  double e = _ahdsr->get_next_value();

  if (0)
    std::cout << "v=" << vibrato << ", mw=" << modWheel
	      << " depth=" << _LFODepth <<  std::endl;

  return vibrato;
}


void TVP::note_off()
{
  _ahdsr->release();
}

}
