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


#include "envelope.h"

#include <cmath>
#include <iostream>
#include <string.h>


namespace EmuSC {

// Make Clang compiler happy
constexpr std::array<float, 128> Envelope::_convert_time_to_sec_LUT;


Envelope::Envelope(double value[5], uint8_t duration[5], bool shape[5], int key,
	       Settings *settings, int8_t partId, enum Type type, int initValue)
  : _finished(false),
    _sampleRate(settings->sample_rate()),
    _currentValue(initValue),
    _phase(Phase::Off),
    _key(key),
    _settings(settings),
    _partId(partId),
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

    std::cout << "\nNew " << envType << " envelope" << std::endl << std::dec
	      << " Attack:  -> V=" << (double) _phaseValue[0]
	      << " T=" << (float) _phaseDuration[0]
	      << " S=" << _phaseShape[0]
	      << std::endl
	      << " Hold:    -> V=" << (double) _phaseValue[1]
	      << " T=" << (float) _phaseDuration[1]
	      << " S=" << _phaseShape[1]
	      << std::endl
	      << " Decay:   -> V=" << (double) _phaseValue[2]
	      << " T=" << (float) _phaseDuration[2]
	      << " S=" << _phaseShape[2]
	      << std::endl
	      << " Sustain: -> V=" << (double) _phaseValue[3]
	      << " T=" << (float) _phaseDuration[3]
	      << " S=" << _phaseShape[3]
	      << std::endl
	      << " Release: -> V=" << (double) _phaseValue[4]
	      << " T=" << (float) _phaseDuration[4]
	      << " S=" << _phaseShape[4]
	      << std::endl
	      << " Key=" << key << std::endl;
  }
}


Envelope::~Envelope()
{}


void Envelope::start(void)
{
  _init_new_phase(Phase::Attack);
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
  if (newPhase == Phase::Attack || newPhase == Phase::Hold) {
    durationTotal += _settings->get_param(PatchParam::TVFAEnvAttack, _partId)
                     - 0x40;

  } else if (newPhase == Phase::Decay) {
    durationTotal += _settings->get_param(PatchParam::TVFAEnvDecay, _partId)
                     - 0x40;

  } else if (newPhase == Phase::Release) {
    durationTotal += _settings->get_param(PatchParam::TVFAEnvRelease, _partId)
                     - 0x40;
  }

  // Make sure synth settings does not go outside valid values
  if (durationTotal < 0) durationTotal = 0;
  if (durationTotal > 127) durationTotal = 127;

  double phaseDurationSec = (_key < 0) ?
    _convert_time_to_sec_LUT[durationTotal] :
    _convert_time_to_sec_LUT[durationTotal] * (1 - (_key / 128.0));

  _phaseSampleLen = round(phaseDurationSec * _sampleRate);

  _phaseSampleIndex = 0;
  _phase = newPhase;

  if (0) {
    std::string envType;
    if (_type == Type::TVA) envType = "TVA";
    else if (_type == Type::TVF) envType = "TVF";
    else envType = "Pitch";

    std::cout << "New " << envType << " envelope phase: -> " << static_cast<int>(_phase)
	      << " (" << _phaseName[static_cast<int>(_phase)] << ")"
	      << ": len = " << phaseDurationSec << "s"
	      << " (" << _phaseSampleLen << " samples)"
	      << " val = " << _phaseInitValue << " -> " << _phaseValue[static_cast<int>(_phase)]
	      << std::endl;
  }
}


double Envelope::get_next_value(void)
{
  if (_phase == Phase::Off) {
    std::cerr << "libEmuSC: Internal error, envelope used in Off phase"
	      << std::endl; 
    return 0;

  } else if (_phase == Phase::Attack) {
    if (_phaseSampleIndex > _phaseSampleLen)
      _init_new_phase(Phase::Hold);

  } else if (_phase == Phase::Hold) {
    if (_phaseSampleIndex > _phaseSampleLen)
      _init_new_phase(Phase::Decay);

  } else if (_phase == Phase::Decay) {
    if (_phaseSampleIndex > _phaseSampleLen)
      _init_new_phase(Phase::Sustain);

  } else if (_phase == Phase::Sustain) {
    if (_phaseSampleIndex > _phaseSampleLen) {
      if (_phaseValue[static_cast<int>(Phase::Sustain)] == 0)
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

  if (_phaseSampleLen <= 0)
    _currentValue = _phaseValue[static_cast<int>(_phase)];
  else if (_phaseShape[static_cast<int>(_phase) - 1] == 0)            // Linear
    _currentValue = _phaseInitValue +
      (_phaseValue[static_cast<int>(_phase)] - _phaseInitValue) *
      ((double) _phaseSampleIndex / (double) _phaseSampleLen);
  else
    _currentValue = _phaseInitValue +                // Concave / convex
      (_phaseValue[static_cast<int>(_phase)] - _phaseInitValue) *
      (log(10.0 * _phaseSampleIndex / _phaseSampleLen + 1) / log(10.0 + 1));

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
