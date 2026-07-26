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


#include "reverb.h"

#include <algorithm>
#include <cmath>


namespace EmuSC {


Reverb::Reverb(Settings *settings)
  : _settings(settings),
    _sweepIndex(0),
    _preLpfState(0.0f),
    _preLpfA(0.0f),
    _preLpfB(1.0f),
    _dampA(0.0f),
    _dampB(0.0f),
    _gLoop(0.0f),
    _outGain(0.0f),
    _character(-1),
    _preLPF(-1),
    _reverbTime(-1),
    _delayFeedback(-1)
{
  _rBuffer.fill(0.0f);
}


void Reverb::update(void)
{
  int character = _settings->get_param(PatchParam::ReverbCharacter);
  if (character != _character) {
    _set_character(character);
    _reverbTime = -1;
    _delayFeedback = -1;
  }

  int preLPF = _settings->get_param(PatchParam::ReverbPreLPF);
  if (preLPF != _preLPF)
    _set_pre_lpf(preLPF);

  int reverbTime = _settings->get_param(PatchParam::ReverbTime);
  if (reverbTime != _reverbTime)
    _set_reverb_time(reverbTime);

  int delayFeedback = _settings->get_param(PatchParam::ReverbDelayFeedback);
  if (delayFeedback != _delayFeedback)
    _set_delay_feedback(delayFeedback);

  _set_level(_settings->get_param(PatchParam::ReverbLevel));
}


// Reverb algorithm based on information from the Nuked-SC55 project by nukeykt
void Reverb::process_sample(float input, float output[2])
{
  if (_character < 0 || _character > 7) {
    output[0] = output[1] = 0;
    return;
  }

  auto read = [&](uint16_t base) -> float {
    return _rBuffer[(base + _sweepIndex) & rBufferMask];
  };

  auto write = [&](uint16_t base, float v) {
    _rBuffer[(base + _sweepIndex) & rBufferMask] = v;
  };

  _preLpfState = _preLpfA * _preLpfState + _preLpfB * input;
  float x = _preLpfState * uByte(_activeCharRegs.c4, true);

  const float dLo   = uByte(_activeCharRegs.c4, false);
  const float d4Lo  = uByte(_activeCharRegs.c5, false);
  const bool  dEn   = (_activeCharRegs.c4 & 0x30) != 0;
  const bool  d4En  = (_activeCharRegs.c5 & 0x30) != 0;
  const float aTank = sByte(_activeCharRegs.c6, true);
  const float bTank = uByte(_activeCharRegs.c6, false);

  float D1 = read(_activeCharRegs.p28[1]);
  float n1 = dEn ? (x - 0.5f * D1) : x;
  float o1 = dLo * n1 + D1;

  float D2 = read(_activeCharRegs.p28[2]);
  float n2 = dEn ? (o1 - 0.5f * D2) : o1;
  float o2 = dLo * n2 + D2;

  float D3 = read(_activeCharRegs.p28[3]);
  float n3 = dEn ? (o2 - 0.5f * D3) : o2;
  float o3 = dLo * n3 + D3;

  float dA1 = read(_activeCharRegs.p28[5]);

  float D4 = read(_activeCharRegs.p28[4]);
  float n4 = d4En ? (o3 - 0.5f * D4) : o3;
  float o4 = d4Lo * n4 + D4;

  float dB1 = read(_activeCharRegs.p29[1]);
  float fbA = read(_activeCharRegs.p29[0]);
  write(_activeCharRegs.p28[0], n1);
  float fbB = read(_activeCharRegs.p29[8]);
  write(_activeCharRegs.p28[1], n2);
  write(_activeCharRegs.p28[2], n3);
  write(_activeCharRegs.p28[3], n4);

  _dampA = uByte(_activeCharRegs.c7, true) * _dampA +
           sByte(_activeCharRegs.c7, false) * fbA;
  _dampB = uByte(_activeCharRegs.c8, true) * _dampB +
           sByte(_activeCharRegs.c8, false) * fbB;

  float inA = o4 + _gLoop * _dampA;
  float vA1 = inA + aTank * dA1;
  float mA1 = dA1 + bTank * vA1;

  float dA2  = read(_activeCharRegs.p28[9]);
  float dB2  = read(_activeCharRegs.p29[5]);
  float inA2 = read(_activeCharRegs.p28[8]);
  write(_activeCharRegs.p28[4], vA1);
  float inB2 = read(_activeCharRegs.p29[4]);
  write(_activeCharRegs.p28[5], mA1);

  float inB = o4 + _gLoop * _dampB;
  float vB1 = inB + aTank * dB1;
  float mB1 = dB1 + bTank * vB1;
  write(_activeCharRegs.p29[0], vB1);

  float wetL = read(_activeCharRegs.p28[6]) + read(_activeCharRegs.p28[10]) +
               read(_activeCharRegs.p29[2]) + read(_activeCharRegs.p29[6]);
  float wetR = read(_activeCharRegs.p28[7]) + read(_activeCharRegs.p28[11]) +
               read(_activeCharRegs.p29[3]) + read(_activeCharRegs.p29[7]);

  float vA2 = inA2 + aTank * dA2;
  float mA2 = dA2 + bTank * vA2;
  float vB2 = inB2 + aTank * dB2;
  float mB2 = dB2 + bTank * vB2;
  write(_activeCharRegs.p29[1], mB1);
  write(_activeCharRegs.p28[8], vA2);
  write(_activeCharRegs.p28[9], mA2);
  write(_activeCharRegs.p29[4], vB2);
  write(_activeCharRegs.p29[5], mB2);

  _sweepIndex = (_sweepIndex - 1) & rBufferMask;

  output[0] = wetL * _outGain;
  output[1] = wetR * _outGain;
}


void Reverb::_set_character(int character)
{
  _character = character;

  // TODO: Firmware fades out before starting with the new character.
  //       We simply reset the ring buffer and switch character instantly, but
  //       must add the following:
  //        - Fade-out (measured to ~165 ms)
  //        - Silent reset time (measured to ~280 ms)

  // Room1-3, Hall1-2, Plate
  if (character >= 0 && character < 6) {
    _activeCharRegs = *_charRegs[character];

    std::fill(_rBuffer.begin(), _rBuffer.end(), 0.0f);
    _dampA = _dampB = 0.0f;
    _preLpfState = 0.0f;

  // Delay, Panning Delay
  } else if (character == 6 || character == 7) {
    _activeCharRegs = _crDelayBase;
    std::fill(_rBuffer.begin(), _rBuffer.end(), 0.0f);
    _dampA = _dampB = 0.0f;
    _preLpfState = 0.0f;
  }
}


void Reverb::_set_reverb_time(int reverbTime)
{
  _reverbTime = reverbTime;

  int rt = std::clamp(reverbTime, 0, 127);
  if (_character >= 0 && _character <= 5) {
    _gLoop = (_timeTargetLUT[rt] >> 1) / 64.0f;         // Measured & verified

  } else if (_character == 6 || _character == 7) {      // Measured & verified
    uint16_t tapR = (uint16_t)(0x16 + 112 * rt);
    uint16_t tapL = (_character == 7) ? (uint16_t)(0x16 + 56 * rt) : tapR;
    _activeCharRegs.p28[6] = tapL;  _activeCharRegs.p28[10] = tapL;   // wet L
    _activeCharRegs.p29[2] = tapL;  _activeCharRegs.p29[6]  = tapL;
    _activeCharRegs.p28[7] = tapR;  _activeCharRegs.p28[11] = tapR;   // wet R
    _activeCharRegs.p29[3] = tapR;  _activeCharRegs.p29[7]  = tapR;
    _activeCharRegs.p29[8] = tapR;                 // Feedback tap (damp B)
  }
}


void Reverb::_set_pre_lpf(int preLPF)
{
  _preLPF = preLPF;

  int k = std::clamp(preLPF, 0, 4);        // PreLPF level 0-7, but capped at 4

  _preLpfA = (8 * k) / 64.0f;
  _preLpfB = (0x3f - 8 * k) / 64.0f;
}


void Reverb::_set_delay_feedback(int delayFeedback)
{
  _delayFeedback = delayFeedback;

  if (_character == 6 || _character == 7)
    _gLoop = fbToTarget(delayFeedback) / 128.0f;
}


void Reverb::_set_level(int level)
{
  _outGain = std::clamp(level, 0, 127) / 64.0f;
}


}  // namespace EmuSC
