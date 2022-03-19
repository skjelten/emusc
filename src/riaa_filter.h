/*  
 *  EmuSC - Sound Canvas emulator
 *  Copyright (C) 2022  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __RIAA_FILTER_H__
#define __RIAA_FILTER_H__


#include "biquad_filter.h"

#include <stdint.h>


class RiaaFilter: public BiquadFilter
{
private:
  RiaaFilter();
  
public:
  RiaaFilter(int sampleRate, long double dcgain);
  ~RiaaFilter();

  float apply(float sample);

};


#endif  // __RIAA_FILTER_H__
