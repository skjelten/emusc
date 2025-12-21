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
// All models in the Sound Canvas line have 2 LFOs:
//  - LFO1 is definde per instrument and is shared with both partials
//  - LFO2 is defined per instrument partial
// This gives a maximum of 3 separate LFOs per note.

// Each LFO has 5 parameters in the instrument [partial] definition:
//   - Waveform and phase shift
//   - Rate
//   - Delay
//   - Fade (fade-in)
// LFO rate and delay parameters can be changed by clients or SysEx messages,
// but only rate can be changed after a "note on" event.

// The folllowing waveforms are supported by the SC-55 family:
//  0: Sine
//  1: Square
//  2: Sawtooth
//  3: Triangle
//  8: Sample & Hold (random sample)
//  9: Random (sample & glide)
// 10: Random (same as 9, but most likely intended to have longer step size)

// All waveforms can be 0, 90, 180 or 270 degrees phase shifted.
// Rate, delay and fade values are all defined by lookup tables in the CPU ROM.


#include "wave_generator.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include <stdint.h>


namespace EmuSC {


WaveGenerator::WaveGenerator(struct ControlRom::Instrument &instrument,
                             struct ControlRom::LookupTables &LUT,
                             Settings *settings, int partId)
  : _id(0),
    _LUT(LUT),
    _delay(0),
    _fade(0),
    _currentValue(0),
    _currentValueNorm(0),
    _random(0),
    _randomFirstRun(true),
    _settings(settings),
    _partId(partId)
{
  _waveform = (enum Waveform) (instrument.LFO1Waveform & 0x0f);
  _instRate = instrument.LFO1Rate;

  // Note: Vibrato Delay equals 2x in ROM values
  int delayInput = instrument.LFO1Delay +
    (settings->get_param(PatchParam::VibratoDelay, _partId) - 0x40) * 2;
  delayInput = std::clamp(delayInput, 0, 127);

  _delayIncLUT = LUT.LFODelayTime[delayInput];
  _fadeIncLUT = LUT.LFODelayTime[instrument.LFO1Fade];

  // Phase shift is done by moving the start off accumulated rate
  _accRate = ((instrument.LFO1Waveform & 0xf0) << 8);

  if (0)
    std::cout << "New LFO1: Waveform=" << (instrument.LFO1Waveform & 0x0f)
              << " Phase=" << (instrument.LFO1Waveform & 0xf0)
              << " Rate=" << _instRate
              << " Delay=" << (int) instrument.LFO1Delay << " -> "
              << _delayIncLUT << " -> " << 512.0 / _delayIncLUT << " s"
              << " Fade=" << (int) instrument.LFO1Fade << " -> " << _fadeIncLUT
              << " -> " << 512.0 / _fadeIncLUT << " s" << std::endl;
}


WaveGenerator::WaveGenerator(struct ControlRom::InstPartial &instPartial,
                             struct ControlRom::LookupTables &LUT,
                             Settings *settings, int partId)
  : _id(1),
    _LUT(LUT),
    _delay(0),
    _fade(0),
    _currentValue(0),
    _currentValueNorm(0),
    _random(0),
    _randomFirstRun(true),
    _settings(settings),
    _partId(partId)
{
  _waveform = (enum Waveform) (instPartial.LFO2Waveform & 0x0f);
  _instRate = instPartial.LFO2Rate;

  _delayIncLUT = LUT.LFODelayTime[instPartial.LFO2Delay];
  _fadeIncLUT = LUT.LFODelayTime[instPartial.LFO2Fade];

  // Phase shift is done by moving the start off accumulated rate
  _accRate = ((instPartial.LFO2Waveform & 0xf0) << 8);

  if (0)
    std::cout << "New LFO2: Waveform=" << (instPartial.LFO2Waveform & 0x0f)
              << " Phase=" << (instPartial.LFO2Waveform & 0xf0)
              << " Rate=" << _instRate
              << " Delay=" << (int) instPartial.LFO2Delay
              << " -> " << 512.0 / _delayIncLUT << "s"
              << " Fade=" << (int) instPartial.LFO2Fade
              << " -> " << 512.0 / _fadeIncLUT << "s" << std::endl;
}


WaveGenerator::~WaveGenerator()
{}


// This function is called at ~125Hz and has 256 samples between each run @32k
void WaveGenerator::update(void)
{
  // Check if we are in the delay phase (no LFO output)
  if (_delay < 0xffff) {
    _delay += _delayIncLUT;

    if (_delay < 0xffff)
      return;
  }

  // Check if we are in the fade-in phase (scaled LFO output)
  if (_fade < 0xffff) {
    _fade += _fadeIncLUT;

    if (_fade > 0xffff)
      _fade = 0xffff;
  }

  // To calculate the rate we need to use the LUT for converting ROM and
  // "Vibrato Rate" (only LFO1) values. Controller values for LFO1/2 rates are
  // pre-caclculated and just needs to be added. Max rate is 0x28f6.
  int index = _instRate;
  if (_id == 0)
    index += _settings->get_param(PatchParam::VibratoRate, _partId) - 0x40;

  int rate =  _LUT.LFORate[std::clamp(index, 0, 127)];
  if (_id == 0)
    rate += _settings->get_acc_control_param(Settings::ControllerParam::LFO1Rate, _partId);
  else
    rate += _settings->get_acc_control_param(Settings::ControllerParam::LFO2Rate, _partId);

  rate = std::clamp(rate, 0, 0x28f6);

  // FIXME: There is an unknwon multiplication that we are missing
  //        Seems to only be affected with multiple simultaneous notes
  //        -> ROM:3BE1                 mulxu.w @0xAC5A:16, r4

  int LFOValue;
  switch (_waveform) {
    case Waveform::Sine:       LFOValue = _generate_sine(rate);        break;
    case Waveform::Square:     LFOValue = _generate_square(rate);      break;
    case Waveform::Sawtooth:   LFOValue = _generate_sawtooth(rate);    break;
    case Waveform::Triangle:   LFOValue = _generate_triangle(rate);    break;
    case Waveform::SampleHold: LFOValue = _generate_sample_hold(rate); break;
    case Waveform::Random:     LFOValue = _generate_random(rate);      break;
    default:
      std::cerr << "libEmuSC: Internal error! Waveform generator called with "
                << "illegal waveform ID: " << (int) _waveform << std::endl;
      return;
  }

  _currentValue = LFOValue;

  if (LFOValue < 0x8000)
    _currentValueNorm = LFOValue * (_fade / 65535.0);
  else if (LFOValue > 0x8000)
    _currentValueNorm = (LFOValue - 0xffff) * (_fade / 65535.0);
  else
    _currentValueNorm = 0;
}


int WaveGenerator::_generate_sine(int rate)
{
  _accRate += rate;

  int v = _accRate - 0x8000;
  if (v & 0x8000)
    v = -v;

  int index = (v >> 8) & 0xff;
  int a = _LUT.LFOSine[index];
  int b = _LUT.LFOSine[index + 1];

  int diff = b - a;
  int tmp = (diff < 0) ? -(-diff & 0xff) * (v & 0xff) : diff * (v & 0xff);
  uint16_t result = ((a & 0xff) << 8) + tmp;
  result >>= 1;

  if (_accRate > 0x8000)
    result = -result;

  return result;
}


int WaveGenerator::_generate_square(int rate)
{
  _accRate += rate;

  return (_accRate < 0x8000) ? 0x7fff : 0x8001;
}


int WaveGenerator::_generate_sawtooth(int rate)
{
  _accRate += rate;

  return (_accRate - 0x8000) & 0xffff;
}


int WaveGenerator::_generate_triangle(int rate)
{
  _accRate += rate;

  int result;
  if (_accRate == 0x8000) {
    result = _accRate;

  } else if (_accRate < 0x8000u) {
    int32_t v = (int32_t)_accRate - 0x4000;
    if ((uint16_t) v < 0x4000u)
      v = - (int16_t)v;
    result = (uint16_t)(v + v) - 0x8000;

  } else {
    int16_t foldIn = -(int16_t) _accRate;
    int32_t v = (int32_t) foldIn - 0x4000;
    if ((uint16_t) v < 0x4000u)
      v = - (int16_t) v;

    uint16_t vv = (uint16_t) (v + v);
    result = (vv == 0) ? 0x7fff : 0x8000 - vv;
  }

  return result;
}


int WaveGenerator::_generate_sample_hold(int rate)
{
  uint32_t sum = _accRate + (uint32_t) (rate * 2);
  bool overflow = (sum >> 16) & 1;
  _accRate = (uint16_t) sum;

  if (overflow)
    _random = (uint16_t) (rand() & 0xFFFF);

  return _random;
}


int WaveGenerator::_generate_random(int rate)
{
  constexpr int step = 0x50;

  uint32_t sum = _accRate + (uint32_t) (rate * 2);
  bool overflow = (sum >> 16) & 1;
  _accRate = (uint16_t) sum;

  if (overflow || _randomFirstRun) {
    _randomFirstRun = false;
    _random = (uint16_t) (rand() & 0xFFFF);
  }

  int result;
  if (_currentValue == _random)
    result = _currentValue;
  else if (_currentValue < _random)
    result = (_currentValue + step < _random) ? _currentValue + step : _random;
  else
    result = (_currentValue - step > _random) ? _currentValue - step : _random;

  return result;
}

}
