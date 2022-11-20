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


#include "tvf.h"

#include <cmath>
#include <iostream>
#include <string.h>

namespace EmuSC {


TVF::TVF(ControlRom::InstPartial instPartial, uint32_t sampleRate)
  : _sampleRate(sampleRate),
    _ahdsr(NULL)
{
  /*
  double phaseLevel[5];         // Phase volume for phase 1-5
  double phaseDuration[5];      // Phase duration for phase 1-5
  bool   phaseShape[5];         // Phase shape for phase 1-5

  // Set adjusted values for volume (0-127) and time (seconds)
  phaseLevel[0] = _convert_volume(instPartial.TVFLvlP1);
  phaseLevel[1] = _convert_volume(instPartial.TVFLvlP2);
  phaseLevel[2] = _convert_volume(instPartial.TVFLvlP3);
  phaseLevel[3] = _convert_volume(instPartial.TVFLvlP4);
  phaseLevel[4] = 0;

  phaseDuration[0] = _convert_time_to_sec(instPartial.TVFDurP1 & 0x7F);
  phaseDuration[1] = _convert_time_to_sec(instPartial.TVFDurP2 & 0x7F);
  phaseDuration[2] = _convert_time_to_sec(instPartial.TVFDurP3 & 0x7F);
  phaseDuration[3] = _convert_time_to_sec(instPartial.TVFDurP4 & 0x7F);
  phaseDuration[4] = 0; //_convert_time_to_sec(instPartial.TVFDurP5 & 0x7F);

  phaseShape[0] = (instPartial.TVFDurP1 & 0x80) ? 0 : 1;
  phaseShape[1] = (instPartial.TVFDurP2 & 0x80) ? 0 : 1;
  phaseShape[2] = (instPartial.TVFDurP3 & 0x80) ? 0 : 1;
  phaseShape[3] = (instPartial.TVFDurP4 & 0x80) ? 0 : 1;
  phaseShape[4] = 0; //(instPartial.TVFDurP5 & 0x80) ? 0 : 1;

//  _ahdsr = new AHDSR(phaseLevel, phaseDuration, phaseShape, sampleRate);
//  _ahdsr->start();
*/

//  _lpFilter = new LowPassFilter(sampleRate);
//  _lpFilter->calculate_coefficients(1000, 0.707);// FIXME: Freq. & q (resonance)
}


TVF::~TVF()
{
  if (_ahdsr)
    delete _ahdsr;
}


double TVF::_convert_time_to_sec(uint8_t time)
{
  if (time == 0)
    return 0;

  return (pow(2.0, (double)(time) / 18.0) / 5.45 - 0.183);
}


double TVF::apply(double input)
{
//  if (_ahdsr)
//    return _ahdsr->get_next_value();
  double output = input;

  // Disable low-pass filter
  //output = _lpFilter->apply(input);
  
  if (0)
    std::cout << "TVF: input=" << input << " ,output=" << output << std::endl;

  return output;
}


void TVF::note_off()
{
  _ahdsr->release();
}

}
