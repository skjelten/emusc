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


#include "biquad_filter.h"

namespace EmuSC {


BiquadFilter::BiquadFilter()
{
  _in[0] = 0;
  _in[1] = 0;

  _out[0] = 0;
  _out[1] = 0;
}


BiquadFilter::~BiquadFilter()
{}


float BiquadFilter::apply(float input)
{
  double out = 0;
  out += _n[0] * input + _n[1] *  _in[0] + _n[2] *  _in[1];
  out -=                 _d[1] * _out[0] + _d[2] * _out[1];

  // Shift the delay line
  _in[1]  = _in[0];
  _in[0]  = input;

  _out[1] = _out[0];
  _out[0] = out;

  return (float) out;
}

}
