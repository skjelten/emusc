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
  : _pingPongReverse(false),
    _pcmSamples(pcmSamples),
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

  // Priming the sliding window values
  y0 = _fetch_sample(_index);
  y1 = _fetch_sample(_index + 1);
}


void WaveOscillator::get_sample_set(Pitch *pitch, float pitchBend,
                                    std::array<float, 256> &dryBus)
{
  for (int i = 0; i < 256; i++) {
    // Linear interpolation
    float output = y0 + (y1 - y0) * _phase;

    // Ping-pong backward pass uses point-reflection (mirror + invert)
    if (_pingPongReverse)
      output = -output;

    dryBus[i] = output;

    _phase += pitchBend * pitch->get_phase_increment() / 16384.0f;
    while (_phase >= 1.0f) {
      _phase -= 1.0f;

      // First-run-complete callback (pitch init->sustain switch on looping
      // samples, termination on one-shots). Fires once when we first reach
      // the end of the sample.
      if (!_firstRunComplete && _index >= _sampleEnd) {
        _firstRunComplete = true;
        if (_firstRunCompleteCallback) _firstRunCompleteCallback();
      }

      if (_loopMode == LoopMode::PingPong && _firstRunComplete) {
        // Reflective ping-pong over [_loopStart .. _sampleEnd].
        if (!_pingPongReverse) {
          _index++;
          if (_index >= _sampleEnd) {             // Reached top -> turn
            _index = _sampleEnd;
            _pingPongReverse = true;
          }
        } else {
          _index--;
          if (_index <= _loopStart - 2) {         // Reached bottom -> turn
            _index = _loopStart;
            _pingPongReverse = false;
          }
        }
      } else {
        // Forward (and the initial forward pass before the first loop).
        _index++;

        if (_loopMode == LoopMode::Forward && _index > _sampleEnd)
          _index = _loopStart;
      }
    }

    y0 = _fetch_sample(_index);
    y1 = _fetch_sample(_pingPongReverse ? _index - 1 : _index + 1);
  }
}


float WaveOscillator::_fetch_sample(int index)
{
  index = std::clamp(index, 0, (int) _pcmSamples->size() - 1);
  return _pcmSamples->at(index);
}

}
