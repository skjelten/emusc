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

// EmuSC uses 32000 Hz internal frequency for all internal calculations to
// stay as close to the original as possible. This resampler uses windowed
// sinc interpolator to convert audio to host system's sample rate.


#include "resampler.h"

#include <algorithm>
#include <cmath>
#include <iostream>


namespace EmuSC {


Resampler::Resampler(void)
  :  _phase(0.0f),
     _increment(1.0),
     _writePos(0)
{
  _history.fill(0.0f);
  _historyR.fill(0.0f);
  _buildKernel();
}


Resampler::~Resampler()
{}


void Resampler::set_sample_rate(int sampleRate)
{
  _increment = 32000.0 / sampleRate;
}


// Feed one new stereo sample pair from the 32000 Hz SC-55 pipeline.
// Call this whenever _phase >= 1.0 (i.e. the resampler needs a new input sample).
void Resampler::push(float left, float right)
{
  _history [_writePos] = left;
  _historyR[_writePos] = right;
  _writePos = (_writePos + 1) % HISTORY_LEN;

  _phase -= 1.0f;
  if (_phase < 0.0f) _phase = 0.0f;
}


bool Resampler::get_next_sample(float &outL, float &outR)
{
  if (_phase >= 1.0f)
    return false;

  // Which polyphase kernel to use
  int   phaseIdx  = static_cast<int>(_phase * PHASES);
  float phaseFrac = _phase * PHASES - phaseIdx;

  outL = 0.0f;
  outR = 0.0f;

  for (int tap = 0; tap < KERNEL_LEN; ++tap) {
    // Read history in reverse (most recent first)
    int hi = (_writePos - 1 - tap + HISTORY_LEN) % HISTORY_LEN;

    // Linear interpolation between adjacent kernel phases
    float k0 = _kernel[phaseIdx       * KERNEL_LEN + tap];
    float k1 = _kernel[(phaseIdx + 1) * KERNEL_LEN + tap];
    float k  = k0 + phaseFrac * (k1 - k0);

    outL += _history [hi] * k;
    outR += _historyR[hi] * k;
  }

  _phase += _increment;
  return true;
}


void Resampler::_buildKernel(void)
{
  _kernel.resize((PHASES + 1) * KERNEL_LEN);

  for (int p = 0; p <= PHASES; ++p) {
    // Fractional offset for this phase (0.0 to 1.0)
    float phaseOffset = static_cast<float>(p) / PHASES;

    for (int tap = 0; tap < KERNEL_LEN; ++tap) {
      // Sample position relative to interpolation point
      // tap 0 = most recent sample, tap KERNEL_LEN-1 = oldest
      float t = (TAPS - 1 - tap) + phaseOffset;

      // Normalised sinc: sinc(2*fc*t)
      float sinc;
      float x = 2.0f * CUTOFF * t;
      if (std::abs(x) < 1e-6f)
        sinc = 2.0f * CUTOFF;
      else
        sinc = std::sin(float(M_PI) * x) / (float(M_PI) * t);

      // Kaiser window (beta=6 — good stopband, moderate transition width)
      float window = _kaiser(2.0f * tap / (KERNEL_LEN - 1) - 1.0f, 6.0f);

      _kernel[p * KERNEL_LEN + tap] = sinc * window;
    }
  }
}


// Kaiser window function
float Resampler::_kaiser(float x, float beta)
{
  float arg = beta * std::sqrt(std::max(0.0f, 1.0f - x * x));
  return _I0(arg) / _I0(beta);
}


// Modified Bessel function I0 via polynomial series
float Resampler::_I0(float x)
{
  float sum  = 1.0f;
  float term = 1.0f;
  float xh   = x * 0.5f;

  for (int k = 1; k <= 20; ++k) {
    term *= xh / k;
    sum  += term * term;
  }

  return sum;
}

}
