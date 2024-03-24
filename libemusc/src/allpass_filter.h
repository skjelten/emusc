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


#ifndef __ALLPASS_FILTER_H__
#define __ALLPASS_FILTER_H__


#include "delay.h"


namespace EmuSC {


class AllPassFilter : public Delay
{
public:
  AllPassFilter(int bufferSize, int delay);
 ~AllPassFilter();

  float process_sample(float input);
  float process_sample_delay(float input);

private:
  float _coefficient;
  float _lastOutput;

  AllPassFilter();
};

}

#endif  // __ALLPASS_FILTER_H__
