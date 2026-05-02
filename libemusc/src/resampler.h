/*
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2026  Håkon Skjelten
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


#ifndef __RESAMPLER_H__
#define __RESAMPLER_H__


#include "control_rom.h"

#include <functional>
#include <array>
#include <vector>


namespace EmuSC {


class Resampler
{
public:
  Resampler();
  ~Resampler();

  void set_sample_rate(int sampeRate);
  void push(float left, float right);
  bool get_next_sample(float &outL, float &outR);

  // Tunable parameters
  static constexpr int   TAPS       = 16;    // Half-length of sinc kernel
  static constexpr int   PHASES     = 32;    // Polyphase table resolution
  static constexpr float CUTOFF     = 0.47f; // Normalised to source Nyquist
                                             // 0.47 * 16000 = 7520 Hz rolloff
private:
  static constexpr int KERNEL_LEN  = TAPS * 2;
  static constexpr int HISTORY_LEN = KERNEL_LEN + 1;

  float _phase;
  float _increment;
  int   _writePos;

  std::array<float, HISTORY_LEN> _history;
  std::array<float, HISTORY_LEN> _historyR;
  std::vector<float>             _kernel;

  void _buildKernel(void);
  static float _kaiser(float x, float beta);
  static float _I0(float x);
};

}

#endif // __RESAMPLER_H__
