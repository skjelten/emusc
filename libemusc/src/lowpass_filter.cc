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


#include "lowpass_filter.h"

#include <cmath>

#include <stdint.h>
#include <stdlib.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

namespace EmuSC {


LowPassFilter::LowPassFilter(int sampleRate)
  : _sampleRate(sampleRate)
{}


LowPassFilter::~LowPassFilter()
{}


//  q = 0.707 -> no resonance
//  frequency = 1000.0;
void LowPassFilter::calculate_coefficients(float frequency, float q)
{
  float w = frequency * 2.0 * M_PI;
  float t = 1.0 / _sampleRate;

  // Calculate coefficients (redo if sampleRate, frequency or q changes)
  _d[0] = 4.0 + ((w / q)* 2.0 * t) + pow(w, 2.0) * pow(t, 2.0);
  _d[1] = ((2.0 * pow(t, 2.0) * pow(w, 2.0)) -8.0) / _d[0];
  _d[2] = (4.0 - (w / q * 2.0 * t) + (pow(w, 2.0) * pow(t, 2.0))) / _d[0];

  _n[0] = pow(w, 2.0) * pow(t, 2.0) / _d[0];
  _n[1] = pow(w, 2.0) * 2.0 * pow(t, 2.0) / _d[0];
  _n[2] = pow(w, 2.0) * pow(t, 2.0) / _d[0];
}

}
