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

  _index = _sampleStart;

  if (0)
    std::cout << "WaveOscillator: lm=" << (int) ctrlSample->loopMode
              << " ss=" << _sampleStart
              << " sl=" << _sampleEnd
              << " ll=" << ctrlSample->loopLen
              << " ns=" << _pcmSamples->size() << std::endl;

  // Priming the sliding window values
  y0 = _fetch_sample(_index);
  y1 = _fetch_sample(_index + 1);
}


void WaveOscillator::get_sample_set(Pitch *pitch, float pitchBend,
                                    std::array<std::array<float,256>,2> &dryBus)
{
  for (int i = 0; i < 256; i ++) {
    // Linear interpolation
    float output = y0 + (y1 - y0) * _phase;

    _phase += pitchBend * pitch->get_phase_increment() / 16384.0f;
    while (_phase >= 1.0f) {
      _phase -= 1.0f;
      _index++;

      // Check if we need to send first-run-of-sample-set-complete callback
      // This triggers a kill on no-loop sets and pitch change on looping sets
      if (!_firstRunComplete && _index > _sampleEnd) {
        _firstRunComplete = true;
        if (_firstRunCompleteCallback) _firstRunCompleteCallback();
      }

      // Note: Ping-pong loops have been extended to forward-loops in sample set
      if ((_loopMode == LoopMode::Forward && _index > _sampleEnd) ||
          (_loopMode == LoopMode::PingPong && _index > _sampleEnd + _loopLength))
        _index = _loopStart;
    }

    y0 = _fetch_sample(_index);
    y1 = _fetch_sample(_index + 1);

    dryBus[0][i] = dryBus[1][i] = output;
  }
}


float WaveOscillator::_fetch_sample(int index)
{
  if (index > _sampleEnd &&
      (_loopMode == LoopMode::Forward || _loopMode == LoopMode::PingPong)) {
    int over = index - _sampleEnd - 1;
    if (_loopLength > 0)
      index = _loopStart + (over % _loopLength);
    else
      index = _loopStart;
  }

  index = std::clamp(index, 0, (int) _pcmSamples->size() - 1);
  return _pcmSamples->at(index);
}

}
