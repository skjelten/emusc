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

#include "chorus.h"

#include <algorithm>
#include <cmath>


namespace EmuSC {


Chorus::Chorus(Settings *settings)
  : _settings(settings),
    _sweepIndex(0),
    _pIn(0x3800),
    _pTap1(0x3800 + 0x1e1 + 200), _pTap2(0x3800 + 0x1e1),
    _phase(0), _v9(0), _v10(0),
    _preA(0.0f), _preB(1.0f),
    _g2L(1.0f), _g2S(0.0f), _g3R(0.0f), _g3F(0.0f),
    _g4L(0.0f), _g4S(0.0f), _g5R(1.0f), _g5F(0.0f),
    _preLpfState(0.0f), _fbSample(0.0f),
    _phaseInc(0x47),
    _sAddress(0x1e1),
    _loopOfs(0x1e1), _span(200),
    _chorusMacroSeen(-1)
{
  _rBuffer.fill(0.0f);
}


void Chorus::update(void)
{
  // Pre-LPF: 1-pole lowpass filter 0-7, but capped at 4 (same as reverb)
  int k = std::clamp((int) _settings->get_param(PatchParam::ChorusPreLPF), 0, 4);

  _preA = (8 * k) / 64.0f;
  _preB = (0x3f - 8 * k) / 64.0f;

  // Rate: pitch = round(23.75*n + 23.33) sub_phase units/tick
  int rp = std::clamp((int) _settings->get_param(PatchParam::ChorusRate), 0, 127);
  int pitchAt200 = (int) lroundf(23.75f * rp + 23.33f);

  // Depth: loop span = 10*n samples (minimum 2)
  int depth = std::clamp((int) _settings->get_param(PatchParam::ChorusDepth), 0, 127);
  _span = std::max(2, 10 * depth);

  // Delay: loop start = input + 1 + 6*n samples
  int delay = std::clamp((int) _settings->get_param(PatchParam::ChorusDelay), 0, 127);
  _loopOfs = 1 + 6 * delay;

  _phaseInc = (uint16_t) std::max(0L,
                                  lroundf((float) pitchAt200 * _span / 200.0f));

  int feedback = std::clamp((int) _settings->get_param(PatchParam::ChorusFeedback), 0, 127);
  _g3F = (feedback >> 1) / 64.0f;
  _g5F = 0.0f;

  int level = std::clamp((int) _settings->get_param(PatchParam::ChorusLevel), 0, 127);
  _g2L = level / 64.0f;  _g3R = 0.0f;
  _g4L = 0.0f;         _g5R = level / 64.0f;

  int send = std::clamp((int) _settings->get_param(PatchParam::ChorusSendToReverb), 0, 127);
  _g2S = (send >> 1) / 64.0f;
  _g4S = 0.0f;
}


// Chorus algorithm based on information from the Nuked-SC55 project by nukeykt
void Chorus::process_sample(float input, float output[2], float *reverbSend)
{
  auto read = [&](int delay, int ofs) -> float {
    return _rBuffer[(_pIn + _sweepIndex + delay + ofs) & rBufferMask];
  };

  auto write = [&](uint16_t base, float v) {
    _rBuffer[(base + _sweepIndex) & rBufferMask] = v;
  };

  // loop and end are offsets from the write head, so _sAaddress is an offset
  // too, and the taps come out as delays in [0..span]
  {
    int loop = _loopOfs, end = _loopOfs + _span;
    int sp = (_subPhase & 0x3fff) + _phaseInc;
    int of = (sp >> 14) & 7;
    _subPhase = sp & 0x3fff;
    for (int k = 0; k < of; k++) {
      bool atEdge = _dir ? (_sAddress == loop) : (_sAddress == end);
      if (atEdge) _dir = !_dir;
      else        _sAddress += _dir ? -1 : 1;
    }
    if (_sAddress < loop) _sAddress = loop;
    if (_sAddress > end)  _sAddress = end;

    _pTap2 = (uint16_t) (_sAddress);                // Delay of tap 2: 0..span
    _pTap1 = (uint16_t) (loop + end - _sAddress);   // Delay of tap 1: span..0

    uint16_t P = (uint16_t) (_subPhase | (_dir ? 0x8000 : 0));
    if (P & 0x8000) _v9  = P & 0x7fff; else _v10 = P & 0x7fff;
    uint16_t d = (uint16_t) (0x4000 - P);
    if (d & 0x8000) _v10 = d & 0x7fff; else _v9  = d & 0x7fff;
  }

  // Pre-LPF on (bus input + one-tick feedback).
  float bus = input + _fbSample;
  _preLpfState = _preA * _preLpfState + _preB * bus;

  write(_pIn, _preLpfState);

  // Tap 1: Interpolate current(+0) toward older(+1) by fraction f1
  float f1 = (float) (_v9 >> 8) / 64.0f;
  float tap1 = read(_pTap1, 0) * (1.0f - f1) + read(_pTap1, 1) * f1;

  // Tap 2 with the complementary fraction
  float f2 = (float) (_v10 >> 8) / 64.0f;
  float tap2 = read(_pTap2, 0) * (1.0f - f2) + read(_pTap2, 1) * f2;

  // Output matrix + bus sends.
  output[0]   = tap1 * _g2L + tap2 * _g4L;
  output[1]   = tap1 * _g3R + tap2 * _g5R;
  *reverbSend = tap1 * _g2S + tap2 * _g4S;
  _fbSample   = tap1 * _g3F + tap2 * _g5F;

  _sweepIndex = (_sweepIndex - 1) & rBufferMask;
}

}  // namespace EmuSC
