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

// This class implements a 2nd. order biquad lowpass filter in direct form 1 


#include "lowpass_filter2.h"

#include <cmath>

#include <stdint.h>
#include <stdlib.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif


namespace EmuSC {


LowPassFilter2::LowPassFilter2(int sampleRate)
  : _sampleRate(sampleRate)
{}


LowPassFilter2::~LowPassFilter2()
{}


//  q = 0.707 -> no resonance
void LowPassFilter2::calculate_coefficients(float frequency, float q)
{
  double omega = 2.0 * M_PI * frequency / _sampleRate;
  double alpha = sin(omega) / (2.0 * q);
  double cos_omega = cos(omega);

  _d[0] = 1.0 + alpha;
  _d[1] = -2.0 * cos_omega / _d[0];
  _d[2] = (1.0 - alpha) / _d[0];

  _n[0] = (1.0 - cos_omega) / (2.0 * _d[0]);
  _n[1] = (1.0 - cos_omega) / _d[0];
  _n[2] = _n[0];
}

}
