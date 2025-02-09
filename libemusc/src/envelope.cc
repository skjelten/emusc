/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2024  Håkon Skjelten
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

// Envelopes in the Sound Canvas have 5 phases:
//  - Phase 1 and 2 are the "Attack" phases
//  - Phase 3 and 4 are the "Decay" phases
//  - If the level in phase 4 (L4) is non-zero the value is sustained
//  - Phase 5 is the "Release" phase triggered by a note off event
//
//  |  | Attack  |Decay|  Sustain   |Release
//  |  | T1 | T2 |T3|T4|            | T5 |
//  |  |    |    |  |  |            |    |
//  |  |  L1|____|L2|  |            |    |
//  |  |    /    \  |  |____________|    |
//  |  |   /      \ | /L4           |\   |
//  |  |  /        \|/              | \  |
//  |  | /          L3              |  \ |
//  |__|/___________________________|___\|___
//     ^L0                          ^    L5
//  Note on                      Note off
//
// Some notes:
// * Sound Canvas line has 3 similar envelopes available: Pitch, TVF and TVA
// * TVA have both linear and expoential curves, pitch and TVF only linear
// * In TVA envelopes L0 and L5 are not specified as they are always 0

// TODO: Phase duration adjustments are dynamically updated


#include "envelope.h"

#include <cmath>
#include <iostream>
#include <string.h>


namespace EmuSC {

// Make Clang compiler happy
constexpr std::array<float, 128> Envelope::_convert_time_to_sec_LUT;


Envelope::Envelope(double value[5], uint8_t duration[5], bool shape[5], int key,
                   std::array<int, 128> &timeLUT, Settings *settings,
                   int8_t partId, enum Type type, int initValue)
  : _timeLUT(timeLUT),
    _finished(false),
    _sampleRate(settings->sample_rate()),
    _currentValue(initValue),
    _phase(Phase::Off),
    _key(key),
    _settings(settings),
    _partId(partId),
    _timeKeyFlwT1T4(-1),
    _timeKeyFlwT5(-1),
    _timeVelSensT1T4(1.0),
    _timeVelSensT5(1.0),
    _type(type)
{
  for (int i = 0; i < 5; i++) {
    _phaseValue[i] = value[i];
    _phaseDuration[i] = duration[i];
    _phaseShape[i] = shape[i];
  }

  if (0) {
    std::string envType;
    if (type == Type::TVA) envType = "TVA";
    else if (type == Type::TVF) envType = "TVF";
    else envType = "Pitch";
    std::array<std::string, 2> shape = { "Linear", "Exponential" };

    std::cout << "\nNew " << envType << " envelope" << std::endl << std::dec
	      << " Attack 1: -> L=" << (double) _phaseValue[0]
	      << " T=" << (float) _phaseDuration[0]
	      << " S=" << shape[_phaseShape[0]] << std::endl
	      << " Attack 2: -> L=" << (double) _phaseValue[1]
	      << " T=" << (float) _phaseDuration[1]
	      << " S=" << shape[_phaseShape[1]] << std::endl
	      << " Decay 1:  -> L=" << (double) _phaseValue[2]
	      << " T=" << (float) _phaseDuration[2]
	      << " S=" << shape[_phaseShape[2]] << std::endl
	      << " Decay 2:  -> L=" << (double) _phaseValue[3]
	      << " T=" << (float) _phaseDuration[3]
	      << " S=" << shape[_phaseShape[3]] << std::endl;

    if (_phaseValue[3] != 0)
      std::cout << "   > Sustain -> L=" << (double) _phaseValue[3] << std::endl;
    else
      std::cout << "   > No sustain" << std::endl;

    std::cout << " Release:  -> L=" << (double) _phaseValue[4]
	      << " T=" << (float) _phaseDuration[4]
	      << " S=" << shape[_phaseShape[4]] << std::endl
	      << " Key=" << key << std::endl;
  }
}


Envelope::~Envelope()
{}


void Envelope::start(void)
{
  _init_new_phase(Phase::Attack1);
}


void Envelope::_init_new_phase(enum Phase newPhase)
{
  if (newPhase == Phase::Off) {
    std::cerr << "libEmuSC: Internal error, envelope in illegal state"
	      << std::endl;
    return;
  }

  _phaseInitValue = _currentValue;

  int durationTotal = _phaseDuration[static_cast<int>(newPhase)];
  if (newPhase == Phase::Attack1 || newPhase == Phase::Attack2) {
    durationTotal +=
      (_settings->get_param(PatchParam::TVFAEnvAttack, _partId) - 0x40) * 2;

  } else if (newPhase == Phase::Decay1 || newPhase == Phase::Decay2) {
    durationTotal +=
      (_settings->get_param(PatchParam::TVFAEnvDecay, _partId) - 0x40) * 2;

  } else if (newPhase == Phase::Release) {
    durationTotal +=
      (_settings->get_param(PatchParam::TVFAEnvRelease, _partId) - 0x40) * 2;
  }

  // Make sure synth settings does not go outside valid values
  if (durationTotal < 0) durationTotal = 0;
  if (durationTotal > 127) durationTotal = 127;

  double phaseDurationSec = _convert_time_to_sec_LUT[durationTotal];

  // FIXME: Find out when this should be used and re-enable it
  //(_key < 0) ?
  //    _convert_time_to_sec_LUT[durationTotal] :
  //    _convert_time_to_sec_LUT[durationTotal] * (1 - (_key / 128.0));

  if (newPhase != Phase::Release) {
    if (_timeKeyFlwT1T4 >= 0)
      phaseDurationSec *= (float) _timeKeyFlwT1T4 / 256.0;

    phaseDurationSec *= _timeVelSensT1T4;
  } else {
    if (_timeKeyFlwT5 >= 0)
      phaseDurationSec *= (float) _timeKeyFlwT5 / 256.0;

    phaseDurationSec *= _timeVelSensT5;
  }

  _phaseSampleLen = round(phaseDurationSec * _sampleRate);

  _phaseSampleIndex = 0;
  _phase = newPhase;

  // Exponential decay: y(x) = y0 * exp(-k*x)
  // k (_expDecayRate) = 0.00025 * (1 + a * (60 – key) / 60.0) / t
  // 0.00025 is pretty good estimation for 1 second of key C4
  // "a" is the scaling factor for key value compensation. Should be approx.
  // 0.45, but this creates instability in volume level for high pitches.
  // TODO: Investigate how "a" can be decreased without affecting volume
  if (_phaseShape[static_cast<int>(_phase)] == 1)
    _expDecayRate = 0.00025 * (1 + 0.48 * (60 - _key) / 60.0) /phaseDurationSec;

  if (0) {
    std::string envType;
    if (_type == Type::TVA) envType = "TVA";
    else if (_type == Type::TVF) envType = "TVF";
    else envType = "Pitch";

    std::cout << "New " << envType << " envelope phase: -> " << static_cast<int>(_phase)
	      << " (" << _phaseName[static_cast<int>(_phase)] << "): Level = "
              << _phaseInitValue << " -> " << _phaseValue[static_cast<int>(_phase)]
	      << " Time = " << phaseDurationSec << "s"
	      << " (" << _phaseSampleLen << " samples)" << std::endl;
  }
}


double Envelope::get_next_value(void)
{
  if (_phase == Phase::Off) {
    std::cerr << "libEmuSC: Internal error, envelope used in Off phase"
	      << std::endl; 
    return 0;

  } else if (_phase == Phase::Attack1) {
    if (_phaseSampleIndex > _phaseSampleLen)
      _init_new_phase(Phase::Attack2);

  } else if (_phase == Phase::Attack2) {
    if (_phaseSampleIndex > _phaseSampleLen)
      _init_new_phase(Phase::Decay1);

  } else if (_phase == Phase::Decay1) {
    if (_phaseSampleIndex > _phaseSampleLen)
      _init_new_phase(Phase::Decay2);

  } else if (_phase == Phase::Decay2) {
    if (_phaseSampleIndex > _phaseSampleLen) {
      if (_phaseValue[static_cast<int>(Phase::Decay2)] == 0)
	_init_new_phase(Phase::Release);
      else
	return _currentValue;                  // Sustain can last forever
    }

  } else if (_phase == Phase::Release) {
    if (_phaseSampleIndex > _phaseSampleLen) {
      _finished = true;
      return 0;
    }
  }

  if (_phaseShape[static_cast<int>(_phase)] == 0) {       // Linear decay
    _currentValue = _phaseInitValue +
      (_phaseValue[static_cast<int>(_phase)] - _phaseInitValue) *
      ((double) _phaseSampleIndex / (double) _phaseSampleLen);

  } else {                                                // Exponential decay
    _currentValue = _phaseValue[static_cast<int>(_phase)] +
      (_phaseInitValue - _phaseValue[static_cast<int>(_phase)]) *
      exp(-_expDecayRate * _phaseSampleIndex);
  }

  _phaseSampleIndex ++;

  return _currentValue;
}


void Envelope::release()
{
  if (_phase == Phase::Release)
    return;

  _init_new_phase(Phase::Release);
}

}
