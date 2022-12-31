/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022  Håkon Skjelten
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


#include "tvp.h"

#include <cmath>
#include <iostream>
#include <string.h>

namespace EmuSC {


TVP::TVP(ControlRom::InstPartial instPartial, uint32_t sampleRate)
  : _sampleRate(sampleRate),
    _LFO(sampleRate),
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
  //       Sound Canvas. In the meantime utilize a simple wavetable with 6Hz.
  _LFO.set_frequency(6);

  // TODO: Find LUT or formula for using Pitch LFO Depth. For now just using a
  //       static approximation.
  _LFODepth = (instPartial.TVPLFODepth & 0x7f) * 0.0009;
  std::cout << "LFODepth = " << _LFODepth << std::endl;
  // TODO: Figure out where the control data for controlling vibrato delay and
  //       fade-in. In the meantime use static no delay and 1s for fade-in.
  _delay = 0;//sampleRate / 2;
  _fadeIn = sampleRate;
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


double TVP::get_pitch(float modWheel)
{
  // LFO hack
  // TODO: Figure out how the LFOs are implemented and used on the Sound Canvas
  //       3 LFOs, one for TVP/F/A? 6 LFOs due to individual partials?
  //       Sound tests indicates that individual notes even on the same MIDI
  //       channel are out of phase -> unlimited number of LFOs? And how does
  //       this relate to LFO1 & LFO2 as referred to in the SysEx implementation
  //       chart?

  // For now just use a simple sine wavetable class; one instance per tvp/f/a
  // per partial.

  double vibrato;
  if (_delay > 0) {                                         // Delay
    _delay--;
    vibrato = 1;

  } else if (_fadeIn > 0) {                                 // Fade in
    _fadeIn--;
    _fade += _fadeInStep;
    vibrato = 1 + (_LFO.next_sample() * ((_LFODepth * _fade) + modWheel));

  } else {                                                  // Full vibrato
    vibrato = 1 + (_LFO.next_sample() * (_LFODepth + modWheel));
  }

  // TODO: Implement pitch envelope
  //  double e = _ahdsr->get_next_value();
  
  if (0)
    std::cout << "v=" << vibrato << ", mw=" << modWheel << std::endl;

  return vibrato;
}


void TVP::note_off()
{
  _ahdsr->release();
}

}
