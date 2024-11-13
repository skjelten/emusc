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


#include "ahdsr.h"

#include <cmath>
#include <iostream>
#include <string.h>


namespace EmuSC {

// Make Clang compiler happy
constexpr std::array<float, 128> AHDSR::_convert_time_to_sec_LUT;


AHDSR::AHDSR(double value[5], uint8_t duration[5], bool shape[5], int key,
	     Settings *settings, int8_t partId, enum Type type, int initValue)
  : _finished(false),
    _sampleRate(settings->get_param_uint32(SystemParam::SampleRate)),
    _currentValue(initValue),
    _phase(ahdsr_Off),
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

    std::cout << "\nNew AHDSR envelope: " << envType << std::endl << std::dec
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
  // Debug output for plotting in Octave
  // _sampleNum = 0; _ofs.open("/tmp/TVA.txt", std::ios_base::trunc);
}


AHDSR::~AHDSR()
{
//  _ofs.close();
}


void AHDSR::start(void)
{
  _init_new_phase(ahdsr_Attack);
}

  
void AHDSR::_init_new_phase(enum Phase newPhase)
{
  if (newPhase == ahdsr_Off) {
    std::cerr << "libEmuSC: Internal error, envelope in illegal state"
	      << std::endl;
    return;
  }

  _phaseInitValue = _currentValue;

  int durationTotal = _phaseDuration[newPhase];
  if (newPhase == ahdsr_Attack || newPhase == ahdsr_Hold) {
    durationTotal += _settings->get_param(PatchParam::TVFAEnvAttack, _partId)
                     - 0x40;

  } else if (newPhase == ahdsr_Decay) {
    durationTotal += _settings->get_param(PatchParam::TVFAEnvDecay, _partId)
                     - 0x40;

  } else if (newPhase == ahdsr_Release) {
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

    std::cout << "New " << envType << " envelope phase: -> " << _phase
	      << " (" << _phaseName[_phase] << ")"
	      << ": len = " << phaseDurationSec << "s"
	      << " (" << _phaseSampleLen << " samples)"
	      << " val = " << _phaseInitValue << " -> " << _phaseValue[_phase]
	      << std::endl;
  }
}


double AHDSR::get_next_value(void)
{
  if (_phase == ahdsr_Off) {
    std::cerr << "libEmuSC: Internal error, envelope used in Off phase"
	      << std::endl; 
    return 0;

  } else if (_phase == ahdsr_Attack) {
    if (_phaseSampleIndex > _phaseSampleLen)
      _init_new_phase(ahdsr_Hold);

  } else if (_phase == ahdsr_Hold) {
    if (_phaseSampleIndex > _phaseSampleLen)
      _init_new_phase(ahdsr_Decay);

  } else if (_phase == ahdsr_Decay) {
    if (_phaseSampleIndex > _phaseSampleLen)
      _init_new_phase(ahdsr_Sustain);

  } else if (_phase == ahdsr_Sustain) {
    if (_phaseSampleIndex > _phaseSampleLen) {
      if (_phaseValue[ahdsr_Sustain] == 0)
	_init_new_phase(ahdsr_Release);
      else
	return _currentValue;                  // Sustain can last forever
    }

  } else if (_phase == ahdsr_Release) {
    if (_phaseSampleIndex > _phaseSampleLen) {
      _finished = true;
      return 0;
    }
  }

  if (_phaseSampleLen <= 0)
    _currentValue = _phaseValue[_phase];
  else if (_phaseShape[_phase - 1] == 0)            // Linear
    _currentValue = _phaseInitValue +
      (_phaseValue[_phase] - _phaseInitValue) *
      ((double) _phaseSampleIndex / (double) _phaseSampleLen);
  else
    _currentValue = _phaseInitValue +                // Concave / convex
      (_phaseValue[_phase] - _phaseInitValue) *
      (log(10.0 * _phaseSampleIndex / _phaseSampleLen + 1) / log(10.0 + 1));

  /*
  // Debug output to file for plotting in Octave
  char number[24];
  setlocale(LC_ALL, "POSIX");
  snprintf(number, 24, "%f ", (float) _sampleNum/_sampleRate);
  _ofs.write(&number[0], strlen(number));
  snprintf(number, 24, "%f", (float) (_currentValue));
  _ofs.write(&number[0], strlen(number));
  _ofs.write("\n", 1);
  _sampleNum ++;
  */

  _phaseSampleIndex ++;

  return _currentValue;
}


void AHDSR::release()
{
  if (_phase == ahdsr_Release)
    return;

  _init_new_phase(ahdsr_Release);
}

}
