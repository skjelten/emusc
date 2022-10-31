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


#ifndef __BIQUAD_FILTER_H__
#define __BIQUAD_FILTER_H__


class BiquadFilter
{
private:

protected:
  long double _n[3];          // Numerator
  long double _d[3];          // Denominator

  float _in[2];               // Next input values 
  long double _out[2];        // Previous output values
  
public:
  BiquadFilter();
  virtual ~BiquadFilter() = 0;

};


#endif  // __BIQUAD_FILTER_H__
