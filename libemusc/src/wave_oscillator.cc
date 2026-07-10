/*
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2026  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 2.1 of the License, or
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


#include "wave_oscillator.h"

#include <algorithm>
#include <iostream>


namespace EmuSC {


WaveOscillator::WaveOscillator(ControlRom::Sample *ctrlSample,
                               std::vector<float> *pcmSamples,
                               std::function<void(void)> cb)
  : _pcmSamples(pcmSamples),
    _phase(0.0f),
    _loopMode{ctrlSample->loopMode},
    _firstRunCompleteCallback(cb),
    _firstRunComplete(false)
{
  // TODO: Add check if note is portamento / legato to skip attack phase
//  if (!portamento)
  _sampleStart = 0;
//  else
//    sampleStart = ctrlSample->portaOffset;

  _sampleEnd = ctrlSample->sampleLen;
  _loopStart = _sampleEnd - ctrlSample->loopLen;
  _loopLength = ctrlSample->loopLen;

  if (_loopMode == LoopMode::PingPong) {
    _sampleEnd = ctrlSample->sampleLen + _loopLength + 1;
    _loopStart = _sampleEnd - 2 * ctrlSample->loopLen - 1;
  }

  _index = _sampleStart;
}


void WaveOscillator::get_sample_set(Pitch *pitch, float pitchBend,
                                    std::array<float, 256> &dryBus)
{
  for (int i = 0; i < 256; i++) {
    float output = _interpolate();
    dryBus[i] = output;

    _phase += pitchBend * pitch->get_phase_increment() / 16384.0f;
    while (_phase >= 1.0f) {
      _phase -= 1.0f;

      if (!_firstRunComplete && _index >= _sampleEnd) {
        _firstRunComplete = true;
        if (_firstRunCompleteCallback) _firstRunCompleteCallback();
      }

      _index++;
      if (_index > _sampleEnd)
	_index = _loopStart;
    }
  }
}


float WaveOscillator::_fetch_sample(int index)
{
  index = std::clamp(index, 0, (int) _pcmSamples->size() - 1);
  return _pcmSamples->at(index);
}


// Interpolation algorithm is based on information from the Nuked-SC55 project
// by nukeykt
float WaveOscillator::_interpolate()
{
  int i = _index;
  auto step = [&]() {
    i++;
    if (i > _sampleEnd)
      i = _loopStart;
  };

  float s0 = _fetch_sample(i);  step();
  float s1 = _fetch_sample(i);  step();
  float s2 = _fetch_sample(i);  step();
  float s3 = _fetch_sample(i);

  // Hardware uses only the top 7 bits of the fractional phase.
  int r = static_cast<int>(_phase * 128.0f) & 127;

  constexpr float q = 1.0f / 4096.0f;
  float c0 = _interpolationLUT[0][r] * q;
  float c1 = _interpolationLUT[1][r] * q;
  float c2 = _interpolationLUT[2][r] * q;

  return s0 + c0 * (s1 - s0) + c1 * (s2 - s1) + c2 * (s3 - s2);
}


} // namespace EmuSC
