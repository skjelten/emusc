/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2023  Håkon Skjelten
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


#include "wavetable.h"

#include <cmath>
#include <iostream>

#include <stdint.h>

namespace EmuSC {


Wavetable::Wavetable(uint32_t sampleRate, bool interpolate)
  : _sampleFactor(1.0 / sampleRate),
    _interpolate(interpolate),
    _frequency(0),
    _index(0)
{}


Wavetable::~Wavetable()
{}


double Wavetable::next_sample(void)
{
  if (_frequency <= 0)
    return 0;

  float output = 0;

  _index += (float) _sineTable.size() * _frequency * _sampleFactor;

  while (_index >= _sineTable.size())
    _index -= _sineTable.size();

  if (!_interpolate) {
    output = _sineTable[(int) _index];

  } else {
    int nextIndex = (int) _index + 1;
    if (nextIndex >= _sineTable.size())
      nextIndex = 0;

    float fractionNext = _index - ((int) _index);
    float fractionPrev = 1.0 - fractionNext;

    output = fractionPrev * _sineTable[(int) _index] +
             fractionNext * _sineTable[nextIndex];
  }

  return output;
}

}
