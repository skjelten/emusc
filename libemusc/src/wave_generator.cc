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
// LFO rate and delay parameters can be changed by clients or SysEx messages,
// but only rate can be changed after a "note on" event.

// The exact way the Sound Canvas produces LFO signals is currently unknown,
// but we know that the following waveforms are used:
//  0: Sine
//  1: Square
//  2: Sawtooth
//  3: Triangle
//  8: Random Level (S&H)
//  9: Random Slope

// All waveforms can be 0, 90, 180 or 270 degrees phase shifted.
// Rate, delay and fade are all defined by lookup tables in the CPU ROM.


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


WaveGenerator::WaveGenerator(struct ControlRom::Instrument &instrument,
                             struct ControlRom::LookupTables &LUT,
                             Settings *settings, int partId)
  : _id(0),
    _sampleRate(settings->sample_rate()),
    _LUT(LUT),
    _currentValue(0),
    _random(std::nanf("")),
    _randomStart(std::nanf("")),
    _settings(settings),
    _partId(partId)
{
  _waveForm = (enum Waveform) (instrument.LFO1Waveform & 0x0f);
  _rate = instrument.LFO1Rate;

  // Note: Vibrato Delay equals the double in ROM values
  int delayInput = instrument.LFO1Delay +
    (settings->get_param(PatchParam::VibratoDelay, _partId) - 0x40) * 2;
  if (delayInput < 0) delayInput = 0;
  if (delayInput > 127) delayInput = 127;

  _delay = (32000.0 / (61 * LUT.LFODelayTime[delayInput])) * 125.0;
  _fade = (32000.0 / (61 * LUT.LFODelayTime[instrument.LFO1Fade])) * 125.0;
  _fadeMax = _fade;

  if (0)
    std::cout << "New LFO1: rate=" << _rate
              << " delay(rom)=" << delayInput
              << " -> " << _delay << " samples / "
              << (float) _delay / settings->sample_rate() << "s"
              << ", fade(rom)" << (int) instrument.LFO1Fade
              << " -> " << _fade << " samples / "
              << (float) _fade / settings->sample_rate() << "s" << std::endl;

  _useLUT = true;
  _interpolate = true;

  _update_params();

  _index = _phase_shift_to_index((instrument.LFO1Waveform & 0xf0) >> 4);
}


WaveGenerator::WaveGenerator(struct ControlRom::InstPartial &instPartial,
                             struct ControlRom::LookupTables &LUT,
                             Settings *settings, int partId)
  : _id(1),
    _sampleRate(settings->sample_rate()),
    _LUT(LUT),
    _currentValue(0),
    _random(std::nanf("")),
    _randomStart(std::nanf("")),
    _settings(settings),
    _partId(partId)
{
  _waveForm = (enum Waveform) (instPartial.LFO2Waveform & 0x0f);
  _rate = instPartial.LFO2Rate;

  _delay = (32000.0 / (61 * LUT.LFODelayTime[instPartial.LFO2Delay])) * 125.0;
  _fade = (32000.0 / (61 * LUT.LFODelayTime[instPartial.LFO2Fade])) * 125.0;
  _fadeMax = _fade;

  if (0)
    std::cout << "New LFO2: delay(rom)=" << (int) instPartial.LFO2Delay
              << " -> " << _delay << " samples / "
              << (float) _delay / settings->sample_rate() << "s"
              << ", fade(rom)" << (int) instPartial.LFO2Fade
              << " -> " << _fade << " samples / "
              << (float) _fade / settings->sample_rate() << "s" << std::endl;

  _useLUT = true;
  _interpolate = true;

  _update_params();

  _index = _phase_shift_to_index((instPartial.LFO2Waveform & 0xf0) >> 4);
}


WaveGenerator::~WaveGenerator()
{}


// This function is called at ~125Hz and has 256 samples between each run @32k
void WaveGenerator::update(void)
{
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
  int squareWave = (_index < _period / 2) ? 1 : -1;

  float LFOValue;
  if (_waveForm == Waveform::Sine) {
    if (!_useLUT) {
      LFOValue = std::sin(_index * 2.0 * M_PI / (float) _period);

    } else { // Sine is created by using a 1/2 sine LUT and a square wave
      float pos = (int) _index * 256 / (float) _period;
      if (pos >= 128)
        pos -= 128;

      if (!_interpolate) {
        LFOValue = (squareWave * _LUT.LFOSine[pos]) / 255.0;

      } else {
        int p0 = squareWave * _LUT.LFOSine[(int) pos];
        int p1 = squareWave * _LUT.LFOSine[(int) (pos + 1) % 128];

        float fractionP1 = pos - (int) pos;
        float fractionP0 = 1.0 - fractionP1;
	LFOValue = (fractionP0 * p0 + fractionP1 * p1) / 255.0;
      }
    }

  } else if (_waveForm == Waveform::Square) {
    LFOValue = squareWave;

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
    _period = (32000.0 / (61 * _LUT.LFORate[_rate + _rateChange])) * 125.0;
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
