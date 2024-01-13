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

// This wave generation class is used to generate low frequency oscialltor
// (LFO) signals. The waveforms used by the SC-55 seems to be:
// - sine     for vibrato, TVF and TVA
// - triangle for chorus
//
// The exact way the SC-55 generates the wave forms is unknown, but EmuSC is
// currently using either a simple lookup table or direct calculation.


#include "wave_generator.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include <stdint.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif


namespace EmuSC {


// Make Clang compiler happy
constexpr std::array<float, 128> WaveGenerator::_delayTable;
constexpr std::array<float, 256> WaveGenerator::_sineTable;


WaveGenerator::WaveGenerator(enum Waveform waveForm, uint32_t sampleRate,
			     uint8_t baseFrequency, bool useLUT, bool interpolate)
  : _waveForm(waveForm),
    _sampleRate(sampleRate),
    _baseFrequency(baseFrequency),
    _useLUT(useLUT),
    _interpolate(interpolate),
    _frequency(0),
    _delay(0),
    _fade(0),
    _index(0)
{
  _sampleFactor = 1.0 / sampleRate;
}


WaveGenerator::~WaveGenerator()
{}


void WaveGenerator::update_frequency(int changeRate)
{
  int newRate = std::min(std::max(_baseFrequency + changeRate, 0), 127);
  _frequency = newRate / 10.0;
}


void WaveGenerator::set_delay(int newDelay)
{
  newDelay = std::min(std::max(newDelay, 0), 127);
  _delay = _delayTable[newDelay] * _sampleRate;
}


void WaveGenerator::set_fade(int newFade)
{
  newFade = std::min(std::max(newFade, 0), 127);
  _fade = newFade * 0.01 * _sampleRate;
  _fadeMax = _fade;
}


void WaveGenerator::next(void)
{
  if (_frequency <= 0) {                     // 0 freq => no output
    _currentValue = 0;
    return;
  }

  if (_delay > 0) {                          // delay > 0 => no output
    _delay--;
    _currentValue = 0;
    return;
  }

  // Calculate waveform value
  double LFOValue;
  if (_waveForm == Waveform::sine) {
    if (!_useLUT) {
      _index += (float) _frequency * _sampleFactor;
      while (_index > 2.0 * M_PI) _index -=  2.0 * M_PI;

      LFOValue = std::sin(2.0 * M_PI * _index);

    } else {
      _index += (float) _sineTable.size() * _frequency * _sampleFactor;

      while (_index >= _sineTable.size())
	_index -= _sineTable.size();

      if (!_interpolate) {
	LFOValue = _sineTable[(int) _index];

      } else {
	int nextIndex = (int) _index + 1;
	if (nextIndex >= _sineTable.size())
	  nextIndex = 0;

	float fractionNext = _index - ((int) _index);
	float fractionPrev = 1.0 - fractionNext;

	LFOValue = fractionPrev * _sineTable[(int) _index] +
	           fractionNext * _sineTable[nextIndex];
      }
    }

  } else {     // TODO: Triangle waveform



  }

  // Apply fade
  if (_fade > 0) {
    _currentValue = (double)(( _fadeMax - _fade) / (float) _fadeMax) * LFOValue;
    _fade --;

  } else {
    _currentValue = LFOValue;
  }
}

}
