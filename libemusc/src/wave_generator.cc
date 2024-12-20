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
// All models in the Sound Canvas line has 2 LFOs:
//  - LFO1 is definde per instrument and is shared with both partials
//  - LFO2 is defined per instrument partial
// This gives a maximum of 3 separate LFOs per note

// Each LFO has 4 parameters defined in the control ROM:
//   - Waveform and phase shift
//   - Rate
//   - Delay
//   - Fade
// LFO rate, delay and fade parameters can be changed by clients or SysEx
// messages, but only rate can be changed after a "note on" event.

// The exact way the Sound Canvas produces LFO signals is currently unknown,
// but we know that the following waveforms are used:
//  0: Sine
//  1: Square
//  2: Sawtooth
//  3: Triangle
//  8: Random Level (S&H)
//  9: Random Slope

// All waveforms can be 0, 90, 180 or 270 degrees phase shifted.


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


WaveGenerator::WaveGenerator(struct ControlRom::Instrument &instrument,
                             Settings *settings, int partId)
  : _id(0),
    _currentValue(0),
    _random(std::nanf("")),
    _randomStart(std::nanf("")),
    _settings(settings),
    _partId(partId)
{
  _sampleRate = settings->sample_rate();
  _sampleFactor = 1.0 / (_sampleRate * 10.0);

  _waveForm = (enum Waveform) (instrument.LFO1Waveform & 0x0f);
  _rate = instrument.LFO1Rate;

  // Note: Vibrato Delay equals the double in ROM values
  int delayInput = instrument.LFO1Delay +
    (settings->get_param(PatchParam::VibratoDelay, _partId) - 0x40) * 2;
  if (delayInput < 0) delayInput = 0;
  if (delayInput > 127) delayInput = 127;
  _delay = _delayTable[delayInput] * _sampleRate;

  _fade = instrument.LFO1Fade * 0.01 * _sampleRate;
  _fadeMax = _fade;

  _useLUT = true;
  _interpolate = false;

  _update_params();

  _index = _phase_shift_to_index((instrument.LFO1Waveform & 0xf0) >> 4);
}


WaveGenerator::WaveGenerator(struct ControlRom::InstPartial &instPartial,
                             Settings *settings, int partId)
  : _id(1),
    _currentValue(0),
    _random(std::nanf("")),
    _randomStart(std::nanf("")),
    _settings(settings),
    _partId(partId)
{
  _sampleRate = settings->sample_rate();
  _sampleFactor = 1.0 / (_sampleRate * 10.0);

  _waveForm = (enum Waveform) (instPartial.LFO2Waveform & 0x0f);
  _rate = instPartial.LFO2Rate;

  uint8_t delayInput = instPartial.LFO2Delay;
  if (delayInput > 127) delayInput = 127;
  _delay = _delayTable[(int) delayInput] * _sampleRate;

  _fade = instPartial.LFO2Fade * 0.01 * _sampleRate;
  _fadeMax = _fade;

  _useLUT = true;
  _interpolate = false;

  _update_params();

  _index = _phase_shift_to_index((instPartial.LFO2Waveform & 0xf0) >> 4);
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

  _index = (_index + 1 < _period) ? _index + 1 : 0;

  double LFOValue;
  if (_waveForm == Waveform::Sine) {
    if (!_useLUT) {
      LFOValue = std::sin(_index * 2.0 * M_PI / (float) _period);

    } else {
      if (!_interpolate) {
	LFOValue = _sineTable[(int) _index * _sineTable.size() /(float)_period];

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
    LFOValue = _sineTable[(int) _index * _sineTable.size() /(float)_period] > 0 ? 1.0 : -1.0;

  } else if (_waveForm == Waveform::Sawtooth) {
    LFOValue = (fmod(_index, _period) / _period) * 2 - 1.0;

  } else if (_waveForm == Waveform::Triangle) {
    float pos = _index / (float) _period;

    if (pos < 0.25)
      LFOValue = pos * 4;
    else if (pos < 0.75)
      LFOValue = 2.0 - (pos * 4.0);
    else
      LFOValue = pos * 4 - 4.0;

  } else if (_waveForm == Waveform::RandomLevel) {
    // Random changes twice per LFO period
    if (_index == 0 || _index == (std::floor(_period / 2.0)) || std::isnan(_random))
      _random = -1+ static_cast<float>(rand()) / static_cast<float>(RAND_MAX/2);

    LFOValue = _random;

  } else if (_waveForm == Waveform::RandomSlope) {
    int halfPeriod = std::floor(_period / 2.0);

    // First iteration
    if (std::isnan(_randomStart)) {
      _randomStart = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
      _random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }

    // Random changes twice per LFO period
    if (_index == 0 || _index == halfPeriod) {
      _randomStart = _random;
      _random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }

    float pos;
    if (_index < halfPeriod)
      pos = _index / (float) halfPeriod;
    else
      pos = (_index - halfPeriod) / (float) halfPeriod;

    if (_random > _randomStart)
      LFOValue = _randomStart + pos * (_random - _randomStart);
    else
      LFOValue = _randomStart - pos * (_randomStart - _random);
  }

  // Apply fade
  if (_fade > 0) {
    _currentValue = (double)(( _fadeMax - _fade) / (float) _fadeMax) * LFOValue;
    _fade --;

  } else {
    _currentValue = LFOValue;
  }
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

  if (_rate + _rateChange <= 0)
    _period = 0;
  else
    _period = _sampleRate * 10 / (_rate + _rateChange);
}


float WaveGenerator::_phase_shift_to_index(int phaseShift)
{
  if (phaseShift >= 0 && phaseShift < 4)
    return 0;
  else if (phaseShift >= 4 && phaseShift < 8)
    return _period * 0.25;
  else if (phaseShift >= 8 && phaseShift < 12)
    return _period * 0.5;
  else // (phaseShift >= 12 && phaseShift < 16)
    return _period * 0.75;
}

}
