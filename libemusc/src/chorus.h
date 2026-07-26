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

// All SC-55+ variations handle system effects, such as chorus, using an
// external audio chip. This chip's ERAM operates as a single circular
// 16384-word buffer that is shared with the reverb effect. The chorus bus
// passes a 1-pole low-pass filter and is written to the ring buffer; two taps
// are read, each at two adjacent positions and linearly interpolated. The LFO
// output (which actually is a full voice running in ping-ping loop mode) is
// one of the taps, while the other tap is its mirror about the loop midpoint.

// While the first version of chorus in libEmuSC was a generic implementation
// based on general principles and adapted by listening tests, the current
// version is a direct implementation of this SC-55 hardware behavior. This
// implementation is based on the reverse-engineering work done by nukeykt as
// part of the Nuked-SC55 project (https://github.com/nukeykt/Nuked-SC55).


#ifndef __CHORUS_H__
#define __CHORUS_H__

#include "settings.h"

#include <array>
#include <cstdint>


namespace EmuSC {

class Chorus
{
 public:
  Chorus(Settings *settings);

  void process_sample(float input, float output[2], float *reverbSend);
  void update(void);   // call at control rate (every 256 samples)

 private:
  Chorus();

  Settings *_settings;

  static constexpr int rBufferSize = 16384;
  static constexpr int rBufferMask = rBufferSize - 1;
  std::array<float, rBufferSize> _rBuffer;
  int _sweepIndex;

  uint16_t _pIn;              // [29][9] Input write pointer
  uint16_t _pTap1, _pTap2;    // [29][10], [29][11] (firmware-swept)
  uint16_t _phase;            // [31][8] LFO phase (firmware-stepped)
  uint16_t _v9, _v10;         // [31][9], [31][10] triangle values
  float _preA, _preB;         // [31][1] Pre-LPF coefficients
  float _g2L, _g2S;           // [31][2] Tap1 -> L gain, -> reverb send
  float _g3R, _g3F;           // [31][3] Tap1 -> R gain, -> feedback
  float _g4L, _g4S;           // [31][4] Tap2 -> L gain, -> reverb send
  float _g5R, _g5F;           // [31][5] Tap2 -> R gain, -> feedback

  float _preLpfState;
  float _fbSample;            // Feedback re-injected into the bus (1-tick)

  uint16_t _phaseInc;         // Pitch: sub_phase increment per tick
  int  _subPhase = 0;         // 14-bit fraction (ram2[31][8] low bits)
  int  _sAddress;             // Sweeping address (ram1[31][4])

  bool _dir = false;          // Ping-pong direction (bit 15)
  int  _loopOfs, _span;       // loop geometry (Delay / Depth)

  int _chorusMacroSeen;

};

}  // namespace EmuSC

#endif  // __CHORUS_H__
