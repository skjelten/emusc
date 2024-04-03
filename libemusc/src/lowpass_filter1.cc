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


#include "lowpass_filter1.h"

#include <cmath>

#include <stdint.h>
#include <stdlib.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif


namespace EmuSC {


LowPassFilter1::LowPassFilter1(int sampleRate)
  : _sampleRate(sampleRate),
    a(0),
    prevOutput(0)
{}


LowPassFilter1::~LowPassFilter1()
{}


void LowPassFilter1::calculate_alpha(float frequency)
{
  a = (2 * M_PI * frequency) / (_sampleRate + (2 * M_PI * frequency));
}


float LowPassFilter1::apply(float input)
{
  double filteredSample = a * input + (1 - a) * prevOutput;
  prevOutput = filteredSample;

  return (float) filteredSample;
}

}
