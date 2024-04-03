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


#ifndef __LOWPASS_FILTER1_H__
#define __LOWPASS_FILTER1_H__


#include <stdint.h>


namespace EmuSC {


class LowPassFilter1
{
public:
  LowPassFilter1(int sampleRate);
  ~LowPassFilter1();

  void calculate_alpha(float frequency);
  float apply(float input);

private:
  int _sampleRate;

  float a;
  float prevOutput;

  LowPassFilter1();
};

}

#endif  // __LOWPASS_FILTER_H__
