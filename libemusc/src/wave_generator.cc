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

// Wave generator class used to generate low frequency oscialltors (LFOs).
// The SC-55 uses only a few LFO waveforms to generate instrument audio,
// but reverse engineering has shown that the hardware at least supports:
// 0: Sine
// 1: Square
// 2: Sawtooth
// 3: Triangle
// 8: Random

// The exact way the SC-55 generates the waveforms is currently unknown.


#include "wave_generator.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include <stdint.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif


namespace EmuSC {

// Make Clang compiler happy
constexpr std::array<float, 128> WaveGenerator::_delayTable;
constexpr std::array<float, 256> WaveGenerator::_sineTable;


WaveGenerator::WaveGenerator(bool id, struct ControlRom::Instrument &instrument,
                             Settings *settings, int partId)
  : _id(id),
    _settings(settings),
    _partId(partId),
    _index(0)
{
  _sampleRate = settings->get_param_uint32(SystemParam::SampleRate);
  _sampleFactor = 1.0 / (_sampleRate * 10.0);

  if (id == 0) {        // Initialize LFO1
    _waveForm = (enum Waveform) instrument.LFO1Waveform;
    _rate = instrument.LFO1Rate;

    if (instrument.LFO1Delay < _delayTable.size()) {
      _delay = _delayTable[(int) instrument.LFO1Delay] * _sampleRate;
    } else {
      std::cerr << "libEmuSC internal error: LFO delay set to illegal value ("
                << instrument.LFO1Delay << ")" << std::endl;
      _delay = 0;
    }

    _fade = instrument.LFO1Fade * 0.01 * _sampleRate;
    _fadeMax = _fade;

  } else {              // Initialize LFO2
    // LFO2 initialization is not found in the control ROM yet, but some
    // instruments seems to have specific configuration also for LFO2.
    // Just adding some common defaults for now.
    _waveForm = Waveform::Sine;
    _rate = 0;
    _delay = 0;
    _fade = 0;
    _fadeMax = _fade;
  }

  _useLUT = true;
  _interpolate = false;

  _update_params();
}


WaveGenerator::~WaveGenerator()
{}


void WaveGenerator::next(void)
{
  if (++_updateParams % 100 == 0)
    _update_params();

  if (_rate + _rateChange <= 0) {       // rate = 0 Hz => keep previous output
    return;
  }

  if (_delay > 0) {                     // delay > 0 => no output
    _delay--;
    _currentValue = 0;
    return;
  }

  // Calculate waveform value
  double LFOValue;
  if (_waveForm == Waveform::Sine) {
    if (!_useLUT) {
      _index += (float) (_rate + _rateChange) * _sampleFactor;
      while (_index > 2.0 * M_PI) _index -=  2.0 * M_PI;

      LFOValue = std::sin(2.0 * M_PI * _index);

    } else {
      _index += (float) _sineTable.size() * (_rate + _rateChange)*_sampleFactor;

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

  } else if (_waveForm == Waveform::Square) {
    _index += (float) _sineTable.size() * (_rate + _rateChange) * _sampleFactor;
    while (_index >= _sineTable.size())
      _index -= _sineTable.size();

    LFOValue = (_sineTable[(int) _index] > 0) ? 1.0 : -1.0;

  // TODO: 180 deg phase shift on sawtooth?
  } else if (_waveForm == Waveform::Sawtooth) {
    double period = _sampleRate * 10 / (_rate + _rateChange);
    double pos = fmod(_index, period);

    LFOValue = (pos / period) * 2 - 1.0;

    _index = (_index + 1.0 < period) ? _index + 1 : 0;

  } else if (_waveForm == Waveform::Triangle) {
    double period = _sampleRate * 10 / (_rate + _rateChange);
    double pos = _index / period;

    if (pos < 0.25)
      LFOValue = pos * 4;
    else if (pos < 0.75)
      LFOValue = 2.0 - (pos * 4.0);
    else
      LFOValue = pos * 4 - 4.0;

    _index = (_index + 1.0 < period) ? _index + 1 : 0;

  // Random is observed to be square waveform with random amplitude changes
  } else if (_waveForm == Waveform::Random) {
    double period = _sampleRate * 10 / (_rate + _rateChange);
    double pos1 = _index / period;
    double pos2 = (_index + 1) / period;

    // Random changes twice per l
    if (_index == 0 || (pos1 <= 0.5 && pos2 > 0.5))
      _random = -1+ static_cast<float>(rand()) / static_cast<float>(RAND_MAX/2);

    LFOValue = _random;

    _index = (_index + 1.0 < period) ? _index + 1 : 0;
  }

  // Apply fade
  if (_fade > 0) {
    _currentValue = (double)(( _fadeMax - _fade) / (float) _fadeMax) * LFOValue;
    _fade --;

  } else {
    _currentValue = LFOValue;
  }

//  std::cout << "LFO" << (int) _id << " rate=" << (float) (_rate + _rateChange) / 10 << " value=" << _currentValue << std::endl;
}


void WaveGenerator::_update_params(void)
{
  if (_id == 0) {
    _rateChange =
      _settings->get_param(PatchParam::Acc_LFO1RateControl, _partId) - 0x40 +
      _settings->get_param(PatchParam::VibratoRate, _partId) - 0x40;

  } else {
    _rateChange =
      _settings->get_param(PatchParam::Acc_LFO2RateControl, _partId) - 0x40;
  }
}

}
