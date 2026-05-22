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

// EmuSC uses 32000 Hz internal frequency for all internal audio calculations
// to stay as close to the original hardware as possible. This polyphase
// windowed sinc resampler converts that audio stream to the host system's
// sample rate.


#ifndef __RESAMPLER_H__
#define __RESAMPLER_H__


#include <array>
#include <vector>


namespace EmuSC {


class Resampler
{
public:
  Resampler();

  void set_sample_rate(int sampleRate);
  void push(float left, float right);
  bool get_next_sample(float &outL, float &outR);

  // Filter design parameters
  static constexpr int   HALF   = 16;    // Taps each side
  static constexpr int   NPHASE = 512;   // Polyphase table resolution
  static constexpr float BETA   = 9.0f;  // Kaiser beta (~ -70 dB stopband)

private:
  static constexpr int TAPS = 2 * HALF;
  static constexpr int RING = 64;        // Input ring buffer (> 2*HALF, pow2)
  static constexpr int RING_MASK = RING - 1;

  double _ratio;        // Input samples advanced per output sample (32000/host)
  double _readPos;      // Continuous read position (abs. input sample index)
  long   _writeCount;   // Total input frames pushed

  std::array<float, RING> _ringL;
  std::array<float, RING> _ringR;

  // Polyphase coefficient table: (NPHASE + 1) rows of TAPS coefficients
  std::vector<float> _table;

  void  _buildTable(float cutoff);
  static double _i0(double x);     // Modified Bessel I0 for Kaiser window
};

}

#endif // __RESAMPLER_H__
