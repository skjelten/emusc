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


#include "allpass_filter.h"

#include <iostream>


namespace EmuSC {


AllPassFilter::AllPassFilter(int maxDelay, int delay)
  : Delay(maxDelay, delay),
    _coefficient(0.7)
{}


AllPassFilter::~AllPassFilter()
{}


// Should there be any feedback involved?
float AllPassFilter::process_sample(float input)
{
  float lastOutput =
    (_readIndex != 0) ? _delayLine[_readIndex] : _delayLine[_maxDelay - 1];

  float fbInput = input + (lastOutput * _coefficient);
  _delayLine[_writeIndex] = fbInput;
  
  // Move the read and write positions
  _readIndex = (_readIndex + 1) % _maxDelay;
  _writeIndex = (_writeIndex + 1) % _maxDelay;

  return lastOutput - (fbInput * _coefficient);
}

}
