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


#include "riaa_filter.h"

#include <cmath>

#include <stdint.h>
#include <stdlib.h>

namespace EmuSC {


RiaaFilter::RiaaFilter(int sampleRate, long double dcgain)
{
  // Calculate biquad for RIAA filter
  // https://en.wikipedia.org/wiki/RIAA_equalization
  long double z1 = exp(-1.0 / ((long double) (318e-6) * sampleRate));   // t1
  long double z2 = exp(-1.0 / ((long double) (3.18e-6) * sampleRate));  // t2
  long double p1 = exp(-1.0 / ((long double) (3180e-6) * sampleRate));  // t3
  long double p2 = exp(-1.0 / ((long double) (75e-6) * sampleRate));    // t4

  _n[0] = 1;
  _n[1] = -z1 - z2;
  _n[2] = z1 * z2;

  _d[0] = 1;
  _d[1] = -p1 - p2;
  _d[2] = p1 * p2;

  long double gain = (_n[0] + _n[1] + _n[2]) / (_d[0] + _d[1] + _d[2]);
  long double gainAttenuation = (dcgain / gain);

  for(int i = 0; i < 3; i++) {
    _n[i] *= gainAttenuation;
  }
}


RiaaFilter::~RiaaFilter()
{}

}
