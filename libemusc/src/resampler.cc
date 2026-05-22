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


#include "resampler.h"

#include <algorithm>
#include <cmath>


namespace EmuSC {


Resampler::Resampler()
  : _ratio(1.0),
    _readPos(static_cast<double>(HALF)),
    _writeCount(0)
{
  _ringL.fill(0.0f);
  _ringR.fill(0.0f);

  // Default to a unity-ratio table; set_sample_rate() rebuilds as needed.
  _buildTable(0.5f);
}


void Resampler::set_sample_rate(int sampleRate)
{
  _ratio = 32000.0 / sampleRate;

  float cutoff = 0.5f * std::min(1.0f, static_cast<float>(sampleRate) / 32000.0f);
  _buildTable(cutoff);

  // Reset stream state
  _readPos    = static_cast<double>(HALF);
  _writeCount = 0;
  _ringL.fill(0.0f);
  _ringR.fill(0.0f);
}


void Resampler::push(float left, float right)
{
  _ringL[_writeCount & RING_MASK] = left;
  _ringR[_writeCount & RING_MASK] = right;
  _writeCount++;
}


bool Resampler::get_next_sample(float &outL, float &outR)
{
  // We need HALF input samples of lookahead beyond the read position before
  // an output sample can be produced
  if (_readPos + HALF >= static_cast<double>(_writeCount))
    return false;

  int i = static_cast<int>(std::floor(_readPos));
  double frac = _readPos - i;

  // Polyphase row selection with linear interpolation between rows
  double phasePos = frac * NPHASE;
  int pidx = static_cast<int>(phasePos);
  float pf = static_cast<float>(phasePos - pidx);

  const float *row0 = &_table[pidx       * TAPS];
  const float *row1 = &_table[(pidx + 1) * TAPS];

  float accL = 0.0f;
  float accR = 0.0f;

  for (int k = 0; k < TAPS; ++k) {
    int n = k - HALF + 1;
    int idx = (i + n) & RING_MASK;
    float c = row0[k] + pf * (row1[k] - row0[k]);

    accL += _ringL[idx] * c;
    accR += _ringR[idx] * c;
  }

  outL = accL;
  outR = accR;

  _readPos += _ratio;
  return true;
}


void Resampler::_buildTable(float cutoff)
{
  _table.resize((NPHASE + 1) * TAPS);

  const double i0beta = _i0(BETA);

  for (int p = 0; p <= NPHASE; ++p) {
    double frac = static_cast<double>(p) / NPHASE;

    double sum = 0.0;
    for (int k = 0; k < TAPS; ++k) {
      int n = k - HALF + 1;             // Tap offset
      double x = n - frac;              // Distance from interpolation point

      // Windowed sinc, scaled so the prototype has the right cutoff
      double arg = 2.0 * cutoff * x;
      double sinc;
      if (std::abs(arg) < 1e-9)
        sinc = 2.0 * cutoff;
      else
        sinc = 2.0 * cutoff * std::sin(M_PI * arg) / (M_PI * arg);

      // Kaiser window over the full support [-HALF, HALF]
      double wn  = x / HALF;
      double win = 0.0;
      if (std::abs(wn) <= 1.0)
        win = _i0(BETA * std::sqrt(std::max(0.0, 1.0 - wn * wn))) / i0beta;

      double coeff = sinc * win;
      _table[p * TAPS + k] = static_cast<float>(coeff);
      sum += coeff;
    }

    // Normalise each row to unity DC gain
    if (std::abs(sum) > 1e-12) {
      float inv = static_cast<float>(1.0 / sum);
      for (int k = 0; k < TAPS; ++k)
        _table[p * TAPS + k] *= inv;
    }
  }
}


// Modified Bessel function I0 via polynomial series
double Resampler::_i0(double x)
{
  double sum = 1.0;
  double term = 1.0;
  double xh = x * 0.5;

  for (int k = 1; k <= 25; ++k) {
    term *= xh / k;
    sum  += term * term;
  }

  return sum;
}

}
