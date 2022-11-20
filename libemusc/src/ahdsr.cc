/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022  Håkon Skjelten
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


AHDSR::AHDSR(double value[5], double duration[5], bool shape[5], uint32_t sampleRate)
  : _sampleRate(sampleRate),
    _phase(ahdsr_Off),
    _terminalPhase(ahdsr_Release),
    _currentValue(0),
    _finished(false)
{
  for (int i = 0; i < 5; i++) {
    _phaseValue[i] = value[i];
    _phaseDuration[i] = duration[i];
    _phaseShape[i] = shape[i];
  }

  if (0)
    std::cout << "\nNew AHDSR envelope:" << std::endl << std::dec
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
	      << std::endl;

//  _sampleNum = 0;
//  _ofs.open("/tmp/TVA.txt", std::ios_base::trunc);
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
  _phaseSampleLen = round(_phaseDuration[newPhase] * _sampleRate);
  _phaseSampleIndex = 0;
  _phase = newPhase;
  
  if (0)
    std::cout << "New envelope phase [" << _phase
	      << "]: numSamples=" << (int) _phaseSampleLen
	      << " value " << _phaseInitValue
	      << " -> " << _phaseValue[_phase] << std::endl;
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

  } else if (_phase == ahdsr_Sustain) {           // Sustain can last forever
    if (_phaseSampleIndex > _phaseSampleLen && _phaseValue[ahdsr_Sustain] == 0)
      _init_new_phase(ahdsr_Release);

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

  /*  // Write volume envelope to file for plotting / debuging in Octave
  char number[24];
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
