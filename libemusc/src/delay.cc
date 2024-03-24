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


#include "delay.h"

#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>


namespace EmuSC {


Delay::Delay(int maxDelay, int delay)
  : _delayLine(maxDelay),
    _maxDelay(maxDelay),
    _delay(delay),
    _feedbackFactor(0.0)
{
  _writeIndex = 0;
  _readIndex = _writeIndex - delay - 1;
  while (_readIndex < 0)
    _readIndex += _maxDelay;
}


Delay::~Delay()
{}


float Delay::process_sample(float input)
{
  // Calculate output from the delay line
  float output = _delayLine[_readIndex];

  _delayLine[_writeIndex] = input + _feedbackFactor * output;

  // Move the read and write indices
  _readIndex = (_readIndex + 1) % _maxDelay;
  _writeIndex = (_writeIndex + 1) % _maxDelay;

  return output;
}


void Delay::set_feedback(float feedback)
{
  _feedbackFactor = feedback;
}


void Delay::set_delay(int delay)
{
  if (delay != _delay) {
    _delay = delay;
    std::fill_n(_delayLine.begin(), delay, 0);
    
    _readIndex = _writeIndex - _delay;
    while (_readIndex < 0)
      _readIndex += _maxDelay;
  }
}

}
