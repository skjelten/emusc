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
    _ahdsr(NULL)
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

  // TODO: tremolo / LFO

  _LFODepth = instPartial.TVPLFODepth;

  _frequency = 5;

  _readPointer = 0;
  
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


double TVP::get_pitch(void)
{
  // Disable TVP - work in progress
  return 1;


  // LFO hack -> vibrato
  float x = 1.0 / (float) _sampleRate;
  _readPointer += _sinus.size() * _frequency * x;
  while(_readPointer >= _sinus.size())
    _readPointer -= _sinus.size();

  //  std::cout << "LFO Depth=" << (int) _LFODepth << std::endl;

  // Vibrato
  double vibratoLFO = (double) _sinus[(int) _readPointer] / 128.0 * _LFODepth;
  double v = exp((vibratoLFO) * 2 * log(2) / 1200);

  static int k = 0;
//  if (k++ < 20)
//  std::cout << "t=" << t << " vLFODepth=" << (int) _LFODepth << std::endl;
  
  // Envelope
//  double e = _ahdsr->get_next_value();

  
  if (0)
    std::cout << "v=" << v << std::endl;

  return v;
}


void TVP::note_off()
{
  _ahdsr->release();
}

}
