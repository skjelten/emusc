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

// The SC-55's interpolator is implemented in the external audio chip, but
// this four-point Cubic Hermite interpolator gives a pretty good aproximation.

// To save runtime CPU usage EmuSC pre-decodes all the DPCM samples to sample
// sets stored in vectors. To make this work with ping-pong loops the sample
// sets have been extended with the return path so they basically becomes
// forward loops.


#include "resample.h"

#include <algorithm>
#include <iostream>


namespace EmuSC {


Resample::Resample(ControlRom::Sample *ctrlSample,
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
    std::cout << "Resample: lm=" << (int) ctrlSample->loopMode
              << " ss=" << _sampleStart
              << " sl=" << _sampleEnd
              << " ll=" << ctrlSample->loopLen
              << " ns=" << _pcmSamples->size() << std::endl;

  // Priming the sliding window values
  y0 = _fetch_sample(_index - 1);
  y1 = _fetch_sample(_index);
  y2 = _fetch_sample(_index + 1);
  y3 = _fetch_sample(_index + 2);
}


Resample::~Resample()
{}


float Resample::get_next_sample(float rate)
{
  // Calculate output based on current window & phase
  float output = _interpolate_cubic(_phase);

  _phase += rate;
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

  y0 = _fetch_sample(_index - 1);
  y1 = _fetch_sample(_index);
  y2 = _fetch_sample(_index + 1);
  y3 = _fetch_sample(_index + 2);

  return output;
}


float Resample::_fetch_sample(int index)
{
  index = std::clamp(index, 0, (int) _pcmSamples->size() - 1);

  if ((_loopMode == LoopMode::Forward && index > _sampleEnd) ||
      (_loopMode == LoopMode::PingPong && index > _sampleEnd + _loopLength)) {
    index = _loopStart + (index - _sampleEnd - 1);
    if (index >= (int) _pcmSamples->size()) index = _loopStart;
  }

  return _pcmSamples->at(index);
}


// Cubic Hermite interpolator
// Constant of 0.5f is based on reverse engineering of old Roland software
float Resample::_interpolate_cubic(float t)
{
  float v1 = (y2 - y0) * 0.5f;
  float v2 = (y3 - y1) * 0.5f;

  float a = 2.0f * y1 - 2.0f * y2 + v1 + v2;
  float b = -3.0f * y1 + 3.0f * y2 - 2.0f * v1 - v2;
  float c = v1;
  float d = y1;

  return ((a * t + b) * t + c) * t + d;
}

}
