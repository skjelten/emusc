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

// TVF and TVA envelopes in the Sound Canvas have 5 phases:
//  - Phase 1 and 2 are the "Attack" phases
//  - Phase 3 and 4 are the "Decay" phases
//  - End of phase 4 is sustained at L4 (must be non-zero for sustained TVA)
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
//  |__|/___________________________|___\|___ Time
//     ^L0                          ^    L5
//  Note on                      Note off
//          Example TVA envelope
//
// Some notes on envelopes:
// - TVA have both linear and expoential curves, pitch and TVF only linear
// - In Pitch envelopes L4 is always 0 (0x40 in ROM)
// - In TVF envelopes L0 is always 0 (0x40 in ROM)
// - In TVA envelopes L0 and L5 are not specified as they are always 0
// - Both pitch and TVF envelopes has a depth parameter to control the total
//   effect of the envelope
// - SysEx changes to phase durations does not affect the pitch envelope


#include "envelope.h"

#include <cmath>
#include <iostream>
#include <string.h>


namespace EmuSC {


Envelope::Envelope(double value[6], uint8_t duration[6], bool shape[6], int key,
                   ControlRom::LookupTables &LUT, Settings *settings,
                   int8_t partId, enum Type type)
  : _LUT(LUT),
    _finished(false),
    _sampleRate(settings->sample_rate()),
    _currentValue(value[0]),
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
  for (int i = 0; i < 6; i++) {
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
	      << " Attack 1: -> L=" << (double) _phaseValue[1]
	      << " T=" << (float) _phaseDuration[1]
	      << " S=" << shape[_phaseShape[1]] << std::endl
	      << " Attack 2: -> L=" << (double) _phaseValue[2]
	      << " T=" << (float) _phaseDuration[2]
	      << " S=" << shape[_phaseShape[2]] << std::endl
	      << " Decay 1:  -> L=" << (double) _phaseValue[3]
	      << " T=" << (float) _phaseDuration[3]
	      << " S=" << shape[_phaseShape[3]] << std::endl
	      << " Decay 2:  -> L=" << (double) _phaseValue[4]
	      << " T=" << (float) _phaseDuration[4]
	      << " S=" << shape[_phaseShape[4]] << std::endl;

    if (!(type == Type::TVA && _phaseValue[4] == 0))
      std::cout << "   > Sustain -> L=" << (double) _phaseValue[4] << std::endl;
    else
      std::cout << "   > No sustain" << std::endl;

    std::cout << " Release:  -> L=" << (double) _phaseValue[5]
	      << " T=" << (float) _phaseDuration[5]
	      << " S=" << shape[_phaseShape[5]] << std::endl
	      << " Key=" << key << std::endl;
  }
}


Envelope::~Envelope()
{}


void Envelope::start(void)
{
  _init_new_phase(Phase::Attack1);
}


// etkpROM != 0 is only possible for the TVA envelope
void Envelope::set_time_key_follow(bool phase, int etkfROM, int etkpROM)
{
  if (!etkfROM)
    return;


  int tkfDiv = _LUT.TimeKeyFollowDiv[std::abs(etkfROM)];
  if (etkfROM < 0)
    tkfDiv *= -1;

  int tkfIndex;
  if (etkpROM == 0) {
    tkfIndex = ((tkfDiv * (_key - 64)) / 64) + 128;

  } else if (etkpROM == 1 && phase == 0) {
    int p1 = _LUT.TVAEnvTKFP1T14Index[_key];
    tkfIndex = p1 + (128 - std::abs(tkfDiv)) * (128 - p1) / 128.0;
    if (etkfROM < 0)
      tkfIndex = 255 - tkfIndex;

  } else if (etkpROM == 1 && phase == 1) {
    int p1 = _LUT.TVAEnvTKFP1T5Index[_key];
    tkfIndex = p1 + (128 - std::abs(tkfDiv)) * (128 - p1) / 128.0;
    if (etkfROM < 0)
      tkfIndex = 255 - tkfIndex;

  } else {
    tkfIndex = ((tkfDiv * (127 - 64)) / 64) + 128;
  }

  if (phase == 0)
    _timeKeyFlwT1T4 = _LUT.TimeKeyFollow[tkfIndex];
  else
    _timeKeyFlwT5 =  _LUT.TimeKeyFollow[tkfIndex];

  if (0)
    std::cout << "ETKF: phase=" << phase << std::dec
              << " key=" << (int) _key << " etkpROM=" << etkpROM
              << " LUT1[" << std::abs(etkfROM) << "]=" << tkfDiv
              << " LUT2[" << tkfIndex << "]=" << _LUT.TimeKeyFollow[tkfIndex]
              << " => time change=" << _LUT.TimeKeyFollow[tkfIndex] / 256.0
              << std::endl;
}


// phase=0 -> T1-T4 (Attacks and Decays), phase=1 -> T5 (Release)
void Envelope::set_time_velocity_sensitvity(bool phase, int etvsROM)
{
  // TODO
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
  if (_type != Type::Pitch) {
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
  }

  // Make sure synth settings does not go outside valid values
  if (durationTotal < 0) durationTotal = 0;
  if (durationTotal > 127) durationTotal = 127;

  float phaseDurationSec = (_LUT.envelopeTime[durationTotal] + 1) / 1000.0;

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

  if (_phaseShape[static_cast<int>(_phase)] == 0)
    _linearChange = (_phaseValue[static_cast<int>(_phase)] - _phaseInitValue) /
      (double) _phaseSampleLen;

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
      if (_phaseValue[static_cast<int>(Phase::Decay2)] == 0 &&
          _type == Type::TVA)
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

  if (_phaseShape[static_cast<int>(_phase)] == 0) {  // Linear rise / decay
    _currentValue += _linearChange;
  } else {                                           // Exponential rise / decay
    _currentValue = _phaseValue[static_cast<int>(_phase)] + 
      (_phaseInitValue - _phaseValue[static_cast<int>(_phase)]) *
      _calc_exp_change(255 - 255 * (_phaseSampleIndex / (float)_phaseSampleLen))
      / 65535.0;
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



float Envelope::_calc_exp_change(float index)
{
  if (index < 0)
    return std::nanf("");
  else if (index >= 255)
    return (float) _LUT.TVAEnvExpChange[255];

  int p0 = _LUT.TVAEnvExpChange[(int) index];
  int p1 = _LUT.TVAEnvExpChange[(int) index + 1];

  float fractionP1 = index - (int) index;
  float fractionP0 = 1.0 - fractionP1;

  return fractionP0 * p0 + fractionP1 * p1;
}

}
