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


#include "settings.h"
#include "config.h"

#include <cmath>
#include <iostream>

#include <vector>
#include <algorithm>
#include <iterator>

namespace EmuSC {


Settings::Settings(ControlRom &ctrlRom)
  : _ctrlRom(ctrlRom)
{
  // TODO: Add SC-55/88 to master settings
  _initialize_system_params();
  _initialize_patch_params();
  _initialize_drumSet_params();

  // TODO: Find a proper way to handle calculated controller values
  for (int i = 0; i < 16; i ++)
    _PBController[i] = 1;
}


Settings::~Settings()
{}


uint8_t Settings::get_param(enum SystemParam sp)
{
  return (uint8_t) _systemParams[(int) sp];
}

  
uint8_t* Settings::get_param_ptr(enum SystemParam sp)
{
  return (uint8_t *) &_systemParams[(int) sp];
}


uint32_t Settings::get_param_uint32(enum SystemParam sp)
{
  return _to_native_endian_uint32(&_systemParams[(int) sp]);
}


uint16_t Settings::get_param_32nib(enum SystemParam sp)
{
  uint8_t *vPtr = &_systemParams[(int) sp];

  if (_le_native())
    return (uint16_t) vPtr[3] | vPtr[2] << 4 | vPtr[1] << 8 | vPtr[0] << 12;

  return (uint16_t) vPtr[0] | vPtr[1] << 4 | vPtr[2] << 8 | vPtr[3] << 12;
}


uint8_t Settings::get_param(enum PatchParam pp, int8_t part)
{
  int8_t rolandPart = _convert_to_roland_part_id_LUT[part];

  if (rolandPart < 0)
    return (uint8_t) _patchParams[(int) pp];

  return (uint8_t) _patchParams[(((int) pp) | (rolandPart << 8))];
}


uint8_t* Settings::get_param_ptr(enum PatchParam pp, int8_t part)
{
  int8_t rolandPart = _convert_to_roland_part_id_LUT[part];
  int address = (int) pp;

  if (rolandPart < 0)
    return  &_patchParams[address];

  return &_patchParams[(address | (rolandPart << 8))];
}


uint16_t Settings::get_param_uint14(enum PatchParam pp, int8_t part)
{
  int8_t rolandPart = _convert_to_roland_part_id_LUT[part];
  int address = (int) pp;

  if (rolandPart < 0)
    return  _to_native_endian_uint14(&_patchParams[address]);

  return _to_native_endian_uint14(&_patchParams[(address | (rolandPart << 8))]);
}


uint16_t Settings::get_param_uint16(enum PatchParam pp, int8_t part)
{
  int8_t rolandPart = _convert_to_roland_part_id_LUT[part];
  int address = (int) pp;

  if (rolandPart < 0)
    return  _to_native_endian_uint16(&_patchParams[address]);

  return _to_native_endian_uint16(&_patchParams[(address | (rolandPart << 8))]);
}


uint8_t Settings::get_param_nib16(enum PatchParam pp, int8_t part)
{
  int8_t rolandPart = _convert_to_roland_part_id_LUT[part];
  if (rolandPart < 0)
    rolandPart = 0;

  int address = (int) pp;
  return _to_native_endian_nib16(&_patchParams[(address | (rolandPart << 8))]);
}


uint8_t Settings::get_patch_param(uint16_t address, int8_t part)
{
  int8_t rolandPart = _convert_to_roland_part_id_LUT[part];

  if (rolandPart < 0)
    return (uint8_t) _patchParams[address];

  return (uint8_t) _patchParams[(address | (rolandPart << 8))];
}


uint8_t Settings::get_param(enum DrumParam dp, uint8_t map, uint8_t key)
{
  return (uint8_t) _drumParams[(int) dp | (map << 12) | key];
}


int8_t* Settings::get_param_ptr(enum DrumParam dp, uint8_t map)
{
  return (int8_t *) &_drumParams[(int) dp | (map << 12)];
}


void Settings::set_param(enum SystemParam sp, uint8_t value)
{
  _systemParams[(int) sp] = value;
}


void Settings::set_param(enum SystemParam sp, uint8_t *value, uint8_t size)
{
  for (int i = 0; i < size; i++)
    _systemParams[(int) sp + i] = value[i];
}


void Settings::set_param_uint32(enum SystemParam sp, uint32_t value)
{
  if (_le_native()) {
    _systemParams[(int) sp + 0] = (value >> 24) & 0xff;
    _systemParams[(int) sp + 1] = (value >> 16) & 0xff;
    _systemParams[(int) sp + 2] = (value >>  8) & 0xff;
    _systemParams[(int) sp + 3] = (value >>  0) & 0xff;

  } else {
    _systemParams[(int) sp + 0] = (value >>  0) & 0xff;
    _systemParams[(int) sp + 1] = (value >>  8) & 0xff;
    _systemParams[(int) sp + 2] = (value >> 16) & 0xff;
    _systemParams[(int) sp + 3] = (value >> 24) & 0xff;
  }
}


void Settings::set_param_32nib(enum SystemParam sp, uint16_t value)
{
  if (_le_native()) {
    _systemParams[(int) sp + 0] = ((value >> 8) & 0xf0) >> 4;
    _systemParams[(int) sp + 1] = ((value >> 8) & 0x0f) >> 0;
    _systemParams[(int) sp + 2] = ((value >> 0) & 0xf0) >> 4;
    _systemParams[(int) sp + 3] = ((value >> 0) & 0x0f) >> 0;
    
  } else {
    _systemParams[(int) sp + 3] = ((value >> 8) & 0xf0) >> 4;
    _systemParams[(int) sp + 2] = ((value >> 8) & 0x0f) >> 0;
    _systemParams[(int) sp + 1] = ((value >> 0) & 0xf0) >> 4;
    _systemParams[(int) sp + 0] = ((value >> 0) & 0x0f) >> 0;
  }
}


void Settings::set_system_param(uint16_t address, uint8_t *value, uint8_t size)
{
  for (int i = 0; i < size; i++)
    _systemParams[address + i] = value[i];
}


void Settings::set_param(enum PatchParam pp, uint8_t value, int8_t part)
{
  int8_t rolandPart = _convert_to_roland_part_id_LUT[part];

  if (rolandPart < 0)
    _patchParams[(int) pp] = value;
  else
    _patchParams[(((int) pp) | (rolandPart << 8))] = value;

  if (pp == EmuSC::PatchParam::ChorusMacro) {
    _run_macro_chorus(value);
  } else if (pp == EmuSC::PatchParam::ReverbMacro) {
    _run_macro_reverb(value);

  // Changes to one of the 6 defined controller groups triggers an update
  // across all controller paramters for that part
  } else if ((int) pp >= 0x1080 && (int) pp <= 0x1086) {
    _update_controller_input(pp, value, part);
  }
}


void Settings::set_param(enum PatchParam pp, uint8_t *data, uint8_t size,
			 int8_t part)
{
  int8_t rolandPart = _convert_to_roland_part_id_LUT[part];

  for (int i = 0; i < size; i++) {
    if (rolandPart < 0)
      _patchParams[(int) pp + i] = data[i];
    else
      _patchParams[(((int) pp) | (rolandPart << 8)) + i] = data[i];   
  }
}


void Settings::set_param_uint14(enum PatchParam pp, uint16_t value, int8_t part)
{
  int8_t rolandPart = _convert_to_roland_part_id_LUT[part];
  if (rolandPart < 0)
    rolandPart == 0;

  if (_le_native()) {
    _patchParams[(int) pp | (rolandPart << 8) + 0] = (value >> 7) & 0x7f;
    _patchParams[(int) pp | (rolandPart << 8) + 1] = (value >> 0) & 0x7f;

  } else {
    _patchParams[(int) pp | (rolandPart << 8) + 0] = (value >> 0) & 0x7f;
    _patchParams[(int) pp | (rolandPart << 8) + 1] = (value >> 7) & 0x7f;
  }
}


void Settings::set_param_nib16(enum PatchParam pp, uint8_t value, int8_t part)
{
  int8_t rolandPart = _convert_to_roland_part_id_LUT[part];
  if (rolandPart < 0)
    rolandPart == 0;

  int address = (int) pp | (rolandPart << 8);
  if (_le_native()) {
    _patchParams[address + 0] = ((value & 0xf0) >> 4) & 0x0f;
    _patchParams[address + 1] = (value & 0x0f);

  } else { // TODO: Verify!
    _patchParams[address + 0] = (value & 0x0f);
    _patchParams[address + 1] = ((value & 0xf0) >> 4) & 0x0f;
  }
}


void Settings::set_patch_param(uint16_t address, uint8_t *data, uint8_t size)
{
  for (int i = 0; i < size; i++)
    _patchParams[address + i] = data[i];

  if (address == 0x138 && size >= 1) {
    _run_macro_chorus(data[0]);
  } else if (address == 0x130 && size >= 1) {
    _run_macro_reverb(data[0]);
  }
}


void Settings::set_patch_param(uint16_t address, uint8_t value, int8_t part)
{
  int8_t rolandPart = _convert_to_roland_part_id_LUT[part];

  if (rolandPart < 0)
    _patchParams[address] = value;

  _patchParams[(address | (rolandPart << 8))] = value;
}


void Settings::set_param(enum DrumParam dp, uint8_t map, uint8_t key, uint8_t value)
{
  if (map > 1 || key > 127)
    return;

  _drumParams[(int) dp | (map << 12) | key] = value;
}


void Settings::set_param(enum DrumParam dp, uint8_t map, uint8_t *data,
			 uint8_t length)
{
  if (map > 1)
    return;

  length = (length > 12) ? 12 : length;

  for (int i = 0; i < length; i++)
    _drumParams[(int) DrumParam::DrumsMapName + i | (map << 12)] = data[i];
}


void Settings::set_drum_param(uint16_t address, uint8_t *data, uint8_t size)
{
  if (address + size > _drumParams.size())
    return;

  for (int i = 0; i < size; i++)
    _drumParams[address + i] = data[i];
}


int8_t Settings::convert_from_roland_part_id(int8_t part)
{
  if (part < 0 || part > 15)  // TODO: Check for SC-88 and adjust max parts
    return -1;
  
  if (part == 0)
    return 9;
  else if (part <= 9)
    return part - 1;

  return part; 
}


// TODO: Evaluate this solution with other controllers
void Settings::update_pitchBend_factor(int8_t part)
{
  uint8_t pbRng = get_param(PatchParam::PB_PitchControl, part) - 0x40;
  uint16_t pbIn = get_param_uint16(PatchParam::PitchBend, part);
  _PBController[part] = exp(((pbIn - 8192) / 8192.0) * pbRng * (log(2) / 12));
}


void Settings::_initialize_system_params(enum Mode m)
{
  // SysEx messages
  _systemParams[(int) SystemParam::Tune + 0] = 0x00;
  _systemParams[(int) SystemParam::Tune + 1] = 0x04;
  _systemParams[(int) SystemParam::Tune + 2] = 0x00;
  _systemParams[(int) SystemParam::Tune + 3] = 0x00;
  _systemParams[(int) SystemParam::Volume] = 0x7f;
  _systemParams[(int) SystemParam::KeyShift] = 0x40;
  _systemParams[(int) SystemParam::Pan] = 0x40;

  // Non-SysEx configuration settings
  _systemParams[(int) SystemParam::SampleRate] = 0;
  _systemParams[(int) SystemParam::Channels] = 2;

  _systemParams[(int) SystemParam::RxSysEx] = 1;
  _systemParams[(int) SystemParam::RxGMOn] = 1;
  _systemParams[(int) SystemParam::RxGSReset] = 1;
  _systemParams[(int) SystemParam::RxInstrumentChange] = 1;
  _systemParams[(int) SystemParam::RxFunctionControl] = 1;
  _systemParams[(int) SystemParam::DeviceID] = 17;
}


void Settings::_initialize_patch_params(enum Mode m)
{
  _patchParams[(int) PatchParam::PartialReserve +  0] = 0x02;
  _patchParams[(int) PatchParam::PartialReserve +  1] = 0x06;
  _patchParams[(int) PatchParam::PartialReserve +  2] = 0x02;
  _patchParams[(int) PatchParam::PartialReserve +  3] = 0x02;
  _patchParams[(int) PatchParam::PartialReserve +  4] = 0x02;
  _patchParams[(int) PatchParam::PartialReserve +  5] = 0x02;
  _patchParams[(int) PatchParam::PartialReserve +  6] = 0x02;
  _patchParams[(int) PatchParam::PartialReserve +  7] = 0x02;
  _patchParams[(int) PatchParam::PartialReserve +  8] = 0x02;
  _patchParams[(int) PatchParam::PartialReserve +  9] = 0x02;
  _patchParams[(int) PatchParam::PartialReserve + 10] = 0x00;
  _patchParams[(int) PatchParam::PartialReserve + 11] = 0x00;
  _patchParams[(int) PatchParam::PartialReserve + 12] = 0x00;
  _patchParams[(int) PatchParam::PartialReserve + 13] = 0x00;
  _patchParams[(int) PatchParam::PartialReserve + 14] = 0x00;
  _patchParams[(int) PatchParam::PartialReserve + 15] = 0x00;

  _patchParams[(int) PatchParam::ReverbMacro] =         0x04;
  _patchParams[(int) PatchParam::ReverbCharacter] =     0x04;
  _patchParams[(int) PatchParam::ReverbPreLPF] =        0x00;
  _patchParams[(int) PatchParam::ReverbLevel] =         0x40;
  _patchParams[(int) PatchParam::ReverbTime] =          0x40;
  _patchParams[(int) PatchParam::ReverbDelayFeedback] = 0x00;

  _patchParams[(int) PatchParam::ChorusMacro] =         0x02;
  _patchParams[(int) PatchParam::ChorusPreLPF] =        0x00;
  _patchParams[(int) PatchParam::ChorusLevel] =         0x40;
  _patchParams[(int) PatchParam::ChorusFeedback] =      0x08;
  _patchParams[(int) PatchParam::ChorusDelay] =         0x50;
  _patchParams[(int) PatchParam::ChorusRate] =          0x03;
  _patchParams[(int) PatchParam::ChorusDepth] =         0x13;
  _patchParams[(int) PatchParam::ChorusSendToReverb] =  0x00;

  // Remaining parameters are separate for each part
  for (int p = 0; p < 16; p ++) {           // TODO: Support SC-88 with 32 parts
    uint8_t partAddr = _convert_to_roland_part_id_LUT[p];

    _patchParams[(int) PatchParam::ToneNumber      | (partAddr << 8)] = 0;
    _patchParams[(int) PatchParam::ToneNumber + 1  | (partAddr << 8)] = 0;
    _patchParams[(int) PatchParam::RxChannel       | (partAddr << 8)] = p;
    _patchParams[(int) PatchParam::RxPitchBend     | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxChPressure    | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxProgramChange | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxControlChange | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxPolyPressure  | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxNoteMessage   | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxRPN           | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxNRPN          | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxModulation    | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxVolume        | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxPanpot        | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxExpression    | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxHold1         | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxPortamento    | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxSostenuto     | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::RxSoft          | (partAddr << 8)] = 1;
    _patchParams[(int) PatchParam::PolyMode        | (partAddr << 8)] = 1;
    
    // MIDI channel 10 defaults to rhythm mode 1 (Drum1) in GS mode
    if (_patchParams[(int) PatchParam::RxChannel  | (partAddr << 8)] == 9) {
      _patchParams[(int) PatchParam::AssignMode   | (partAddr << 8)] = 0;
      _patchParams[(int) PatchParam::UseForRhythm | (partAddr << 8)] = 1;
    } else {
      _patchParams[(int) PatchParam::AssignMode   | (partAddr << 8)] = 1;
      _patchParams[(int) PatchParam::UseForRhythm | (partAddr << 8)] = 0;
    }

    _patchParams[(int) PatchParam::PitchKeyShift      | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PitchOffsetFine    | (partAddr << 8)] = 0x08;
    _patchParams[(int) PatchParam::PitchOffsetFine + 1| (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PartLevel          | (partAddr << 8)] = 0x64;
    _patchParams[(int) PatchParam::VelocitySenseDepth | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::VelocitySenseOffset| (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PartPanpot         | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::KeyRangeLow        | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::KeyRangeHigh       | (partAddr << 8)] = 0x7f;
    _patchParams[(int) PatchParam::CC1ControllerNumber| (partAddr << 8)] = 0x10;
    _patchParams[(int) PatchParam::CC2ControllerNumber| (partAddr << 8)] = 0x11;
    _patchParams[(int) PatchParam::ChorusSendLevel    | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::ReverbSendLevel    | (partAddr << 8)] = 0x28;
    _patchParams[(int) PatchParam::RxBankSelect       | (partAddr << 8)] = 0x01;

    _patchParams[(int) PatchParam::PitchFineTune      | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PitchFineTune + 1  | (partAddr << 8)] = 0x00;

    // Tone mofify
    _patchParams[(int) PatchParam::VibratoRate    | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::VibratoDepth   | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::TVFCutoffFreq  | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::TVFResonance   | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::TVFAEnvAttack  | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::TVFAEnvDecay   | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::TVFAEnvRelease | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::VibratoDelay   | (partAddr << 8)] = 0x40;

    // Scale tuning
    _patchParams[(int) PatchParam::ScaleTuningC  | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::ScaleTuningC_ | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::ScaleTuningD  | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::ScaleTuningD_ | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::ScaleTuningE  | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::ScaleTuningF  | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::ScaleTuningF_ | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::ScaleTuningG  | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::ScaleTuningG_ | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::ScaleTuningA  | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::ScaleTuningA_ | (partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::ScaleTuningB  | (partAddr << 8)] = 0x40;

    // Controller settings
    _patchParams[(int) PatchParam::MOD_PitchControl    |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::MOD_TVFCutoffControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::MOD_AmplitudeControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::MOD_LFO1RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::MOD_LFO1PitchDepth  |(partAddr << 8)] = 0x0a;
    _patchParams[(int) PatchParam::MOD_LFO1TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::MOD_LFO1TVADepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::MOD_LFO2RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::MOD_LFO2PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::MOD_LFO2TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::MOD_LFO2TVADepth    |(partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::PB_PitchControl     |(partAddr << 8)] = 0x42;
    _patchParams[(int) PatchParam::PB_TVFCutoffControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PB_AmplitudeControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PB_LFO1RateControl  |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PB_LFO1PitchDepth   |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PB_LFO1TVFDepth     |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PB_LFO1TVADepth     |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PB_LFO2RateControl  |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PB_LFO2PitchDepth   |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PB_LFO2TVFDepth     |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PB_LFO2TVADepth     |(partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::CAf_PitchControl    |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CAf_TVFCutoffControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CAf_AmplitudeControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CAf_LFO1RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CAf_LFO1PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CAf_LFO1TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CAf_LFO1TVADepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CAf_LFO2RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CAf_LFO2PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CAf_LFO2TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CAf_LFO2TVADepth    |(partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::PAf_PitchControl    |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PAf_TVFCutoffControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PAf_AmplitudeControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PAf_LFO1RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PAf_LFO1PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PAf_LFO1TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PAf_LFO1TVADepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PAf_LFO2RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::PAf_LFO2PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PAf_LFO2TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PAf_LFO2TVADepth    |(partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::CC1_PitchControl    |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CC1_TVFCutoffControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CC1_AmplitudeControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CC1_LFO1RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CC1_LFO1PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC1_LFO1TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC1_LFO1TVADepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC1_LFO2RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CC1_LFO2PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC1_LFO2TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC1_LFO2TVADepth    |(partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::CC2_PitchControl    |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CC2_TVFCutoffControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CC2_AmplitudeControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CC2_LFO1RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CC2_LFO1PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC2_LFO1TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC2_LFO1TVADepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC2_LFO2RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CC2_LFO2PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC2_LFO2TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC2_LFO2TVADepth    |(partAddr << 8)] = 0x00;

    // Controller input list  --- TO BE DELETED?? AUTOGENERATE AT START?
    _patchParams[(int) PatchParam::CtM_PitchControl    |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtM_TVFCutoffControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtM_AmplitudeControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtM_LFO1RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtM_LFO1PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtM_LFO1TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtM_LFO1TVADepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtM_LFO2RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtM_LFO2PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtM_LFO2TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtM_LFO2TVADepth    |(partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::CtB_PitchControl    |(partAddr << 8)] = 0x42;
    _patchParams[(int) PatchParam::CtB_TVFCutoffControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtB_AmplitudeControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtB_LFO1RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtB_LFO1PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtB_LFO1TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtB_LFO1TVADepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtB_LFO2RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtB_LFO2PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtB_LFO2TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtB_LFO2TVADepth    |(partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::CtC_PitchControl    |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtC_TVFCutoffControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtC_AmplitudeControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtC_LFO1RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtC_LFO1PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtC_LFO1TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtC_LFO1TVADepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtC_LFO2RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtC_LFO2PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtC_LFO2TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtC_LFO2TVADepth    |(partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::CtP_PitchControl    |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtP_TVFCutoffControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtP_AmplitudeControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtP_LFO1RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtP_LFO1PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtP_LFO1TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtP_LFO1TVADepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtP_LFO2RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::CtP_LFO2PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtP_LFO2TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CtP_LFO2TVADepth    |(partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::Ct1_PitchControl    |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::Ct1_TVFCutoffControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::Ct1_AmplitudeControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::Ct1_LFO1RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::Ct1_LFO1PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Ct1_LFO1TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Ct1_LFO1TVADepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Ct1_LFO2RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::Ct1_LFO2PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Ct1_LFO2TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Ct1_LFO2TVADepth    |(partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::Ct2_PitchControl    |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::Ct2_TVFCutoffControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::Ct2_AmplitudeControl|(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::Ct2_LFO1RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::Ct2_LFO1PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Ct2_LFO1TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Ct2_LFO1TVADepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Ct2_LFO2RateControl |(partAddr << 8)] = 0x40;
    _patchParams[(int) PatchParam::Ct2_LFO2PitchDepth  |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Ct2_LFO2TVFDepth    |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Ct2_LFO2TVADepth    |(partAddr << 8)] = 0x00;

    _update_controller_input_acc(PatchParam::PitchBend, p);

    // Controller values
    _patchParams[(int) PatchParam::PitchBend       | (partAddr << 8)] = 0x20;
    _patchParams[(int) PatchParam::PitchBend + 1   | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Modulation      | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC1Controller   | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC2Controller   | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::ChannelPressure | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PolyKeyPressure | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Hold1           | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Sostenuto       | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Soft            | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Expression      | (partAddr << 8)] = 0x7f;
    _patchParams[(int) PatchParam::Portamento      | (partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PortamentoTime  | (partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::RPN_LSB         | (partAddr << 8)] = 0x7f;
    _patchParams[(int) PatchParam::RPN_MSB         | (partAddr << 8)] = 0x7f;
    _patchParams[(int) PatchParam::NRPN_LSB        | (partAddr << 8)] = 0x7f;
    _patchParams[(int) PatchParam::NRPN_MSB        | (partAddr << 8)] = 0x7f;

    _patchParams[(int) PatchParam::PitchCoarseTune | (partAddr << 8)] = 0x40;
  }
}


void Settings::_initialize_drumSet_params(void)
{
  // Both MAP0 & MAP1 to Standard drum set (0)
  update_drum_set(0, 0);
  update_drum_set(1, 0);
}


void Settings::set_gm_mode(void)
{
  for (int p = 0; p < 16; p ++) {           // TODO: Support SC-88 with 32 parts
    uint8_t partAddr = _convert_to_roland_part_id_LUT[p];
    _patchParams[(int) PatchParam::RxNRPN       | (partAddr << 8)] = 0x0;
    _patchParams[(int) PatchParam::RxBankSelect | (partAddr << 8)] = 0x0;
  }
}


void Settings::set_map_mt32(void)
{
  uint8_t partAddr = _convert_to_roland_part_id_LUT[0];
  _patchParams[(int) PatchParam::ToneNumber      | (partAddr << 8)] = 0x7f;
  _patchParams[(int) PatchParam::ToneNumber + 1  | (partAddr << 8)] = 0x00;
  _patchParams[(int) PatchParam::PartPanpot      | (partAddr << 8)] = 0x40;
  _patchParams[(int) PatchParam::ReverbSendLevel | (partAddr << 8)] = 0x40;

  partAddr = _convert_to_roland_part_id_LUT[1];
  _patchParams[(int) PatchParam::ToneNumber      | (partAddr << 8)] = 0x7f;
  _patchParams[(int) PatchParam::ToneNumber + 1  | (partAddr << 8)] = 0x44;
  _patchParams[(int) PatchParam::PartPanpot      | (partAddr << 8)] = 0x36;
  _patchParams[(int) PatchParam::ReverbSendLevel | (partAddr << 8)] = 0x40;

  partAddr = _convert_to_roland_part_id_LUT[2];
  _patchParams[(int) PatchParam::ToneNumber      | (partAddr << 8)] = 0x7f;
  _patchParams[(int) PatchParam::ToneNumber + 1  | (partAddr << 8)] = 0x30;
  _patchParams[(int) PatchParam::PartPanpot      | (partAddr << 8)] = 0x36;
  _patchParams[(int) PatchParam::ReverbSendLevel | (partAddr << 8)] = 0x40;

  partAddr = _convert_to_roland_part_id_LUT[3];
  _patchParams[(int) PatchParam::ToneNumber      | (partAddr << 8)] = 0x7f;
  _patchParams[(int) PatchParam::ToneNumber + 1  | (partAddr << 8)] = 0x5f;
  _patchParams[(int) PatchParam::PartPanpot      | (partAddr << 8)] = 0x36;
  _patchParams[(int) PatchParam::ReverbSendLevel | (partAddr << 8)] = 0x40;

  partAddr = _convert_to_roland_part_id_LUT[4];
  _patchParams[(int) PatchParam::ToneNumber      | (partAddr << 8)] = 0x7f;
  _patchParams[(int) PatchParam::ToneNumber + 1  | (partAddr << 8)] = 0x4e;
  _patchParams[(int) PatchParam::PartPanpot      | (partAddr << 8)] = 0x36;
  _patchParams[(int) PatchParam::ReverbSendLevel | (partAddr << 8)] = 0x40;

  partAddr = _convert_to_roland_part_id_LUT[5];
  _patchParams[(int) PatchParam::ToneNumber      | (partAddr << 8)] = 0x7f;
  _patchParams[(int) PatchParam::ToneNumber + 1  | (partAddr << 8)] = 0x29;
  _patchParams[(int) PatchParam::PartPanpot      | (partAddr << 8)] = 0x12;
  _patchParams[(int) PatchParam::ReverbSendLevel | (partAddr << 8)] = 0x40;

  partAddr = _convert_to_roland_part_id_LUT[6];
  _patchParams[(int) PatchParam::ToneNumber      | (partAddr << 8)] = 0x7f;
  _patchParams[(int) PatchParam::ToneNumber + 1  | (partAddr << 8)] = 0x03;
  _patchParams[(int) PatchParam::PartPanpot      | (partAddr << 8)] = 0x5b;
  _patchParams[(int) PatchParam::ReverbSendLevel | (partAddr << 8)] = 0x40;

  partAddr = _convert_to_roland_part_id_LUT[7];
  _patchParams[(int) PatchParam::ToneNumber      | (partAddr << 8)] = 0x7f;
  _patchParams[(int) PatchParam::ToneNumber + 1  | (partAddr << 8)] = 0x6e;
  _patchParams[(int) PatchParam::PartPanpot      | (partAddr << 8)] = 0x01;
  _patchParams[(int) PatchParam::ReverbSendLevel | (partAddr << 8)] = 0x40;

  partAddr = _convert_to_roland_part_id_LUT[8];
  _patchParams[(int) PatchParam::ToneNumber      | (partAddr << 8)] = 0x7f;
  _patchParams[(int) PatchParam::ToneNumber + 1  | (partAddr << 8)] = 0x7a;
  _patchParams[(int) PatchParam::PartPanpot      | (partAddr << 8)] = 0x7f;
  _patchParams[(int) PatchParam::ReverbSendLevel | (partAddr << 8)] = 0x40;

  int dsIndex = update_drum_set(0, 127);
  partAddr = _convert_to_roland_part_id_LUT[9];
  _patchParams[(int) PatchParam::ToneNumber      | (partAddr << 8)] = dsIndex;
  _patchParams[(int) PatchParam::ToneNumber + 1  | (partAddr << 8)] = 0x7f;
  _patchParams[(int) PatchParam::PartPanpot      | (partAddr << 8)] = 0x40;
  _patchParams[(int) PatchParam::ReverbSendLevel | (partAddr << 8)] = 0x40;
}


int Settings::update_drum_set(uint8_t map, uint8_t bank)
{
  if (map > 1 || bank > 127)
    return -2;

  // Find index for drum set in given bank
  const std::array<uint8_t, 128> &drumSetsLUT = _ctrlRom.get_drum_sets_LUT();

  // Invalid entries in the drum sets lookup table have the value 0xff
  if (drumSetsLUT[bank] == 0xff)
    return -1;

  int index = 0;
  for (int it = 0; it < bank; it ++) {
    if (drumSetsLUT[it] != 0xff)
      index++;
  }

  // On the original hardware both the active drum set configurations are
  // copied from ROM to RAM where they can be modified by the user
  for (unsigned int i = 0; i < 12; i ++) {
    if (i < _ctrlRom.drumSet(index).name.length()) {
      _drumParams[(int) DrumParam::DrumsMapName + i |(map << 12)] =
	_ctrlRom.drumSet(index).name[i];
    } else {
      _drumParams[(int) DrumParam::DrumsMapName + i |(map << 12)] = ' ';
    }
  }
  for (int r = 0; r < 128; r++) {
    _drumParams[(int) DrumParam::PlayKeyNumber     | (map << 12) | r] =
      _ctrlRom.drumSet(index).key[r];
    _drumParams[(int) DrumParam::Level             | (map << 12) | r] =
      _ctrlRom.drumSet(index).volume[r];
    _drumParams[(int) DrumParam::AssignGroupNumber | (map << 12) | r] =
      _ctrlRom.drumSet(index).assignGroup[r];
    _drumParams[(int) DrumParam::Panpot            | (map << 12) | r] =
      _ctrlRom.drumSet(index).panpot[r];
    _drumParams[(int) DrumParam::ReverbDepth       | (map << 12) | r] =
      _ctrlRom.drumSet(index).reverb[r];
    _drumParams[(int) DrumParam::ChorusDepth       | (map << 12) | r] =
      _ctrlRom.drumSet(index).chorus[r];
    _drumParams[(int) DrumParam::RxNoteOff         | (map << 12) | r] =
      _ctrlRom.drumSet(index).flags[r] & 0x01;
    _drumParams[(int) DrumParam::RxNoteOn          | (map << 12) | r] =
      _ctrlRom.drumSet(index).flags[r] & 0x10;
  }

  return index;
}


bool Settings::load(std::string filePath)
{
  return 0;
}


bool Settings::save(std::string filePath)
{
  return 0;
}

void Settings::reset(void)
{
  _initialize_system_params();
  _initialize_patch_params();
  _initialize_drumSet_params();
}


uint8_t Settings::_to_native_endian_nib16(uint8_t *ptr)
{
  if (_le_native())
    return ((ptr[0] << 4) | (ptr[1] & 0x0f));

  return (ptr[1] << 4 | (ptr[0] & 0x0f));
}


uint16_t Settings::_to_native_endian_uint14(uint8_t *ptr)
{
  if (_le_native())
    return ((ptr[0] & 0x7f) << 7 | ptr[1]);

  return ((ptr[1] & 0x7f) << 7 | ptr[0]);
}


uint16_t Settings::_to_native_endian_uint16(uint8_t *ptr)
{
  if (_le_native())
    return (ptr[0] << 8 | ptr[1]);

  return (ptr[1] << 8 | ptr[0]);
}


uint32_t Settings::_to_native_endian_uint32(uint8_t *ptr)
{
  if (_le_native())
    return (ptr[0] << 24 | ptr[1] << 16 | ptr [2] << 8 | ptr[3]);

  return (ptr[3] << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0]);
}


// Macro based on table from SC-8820 owner's manual and verified on SC-55MkII
void Settings::_run_macro_chorus(uint8_t value)
{
  switch(value)
    {
    case 0: // Chorus 1
      _patchParams[(int) PatchParam::ChorusFeedback] = 0x00;
      _patchParams[(int) PatchParam::ChorusDelay] =    0x70;
      _patchParams[(int) PatchParam::ChorusRate] =     0x03;
      _patchParams[(int) PatchParam::ChorusDepth] =    0x06;
      break;

    case 1: // Chorus 2
      _patchParams[(int) PatchParam::ChorusFeedback] = 0x08;
      _patchParams[(int) PatchParam::ChorusDelay] =    0x50;
      _patchParams[(int) PatchParam::ChorusRate] =     0x09;
      _patchParams[(int) PatchParam::ChorusDepth] =    0x13;
      break;

    case 2: // Chorus 3
      _patchParams[(int) PatchParam::ChorusFeedback] = 0x08;
      _patchParams[(int) PatchParam::ChorusDelay] =    0x50;
      _patchParams[(int) PatchParam::ChorusRate] =     0x03;
      _patchParams[(int) PatchParam::ChorusDepth] =    0x13;
      break;

    case 3: // Chorus 4
      _patchParams[(int) PatchParam::ChorusFeedback] = 0x08;
      _patchParams[(int) PatchParam::ChorusDelay] =    0x40;
      _patchParams[(int) PatchParam::ChorusRate] =     0x09;
      _patchParams[(int) PatchParam::ChorusDepth] =    0x10;
      break;

    case 4: // Feedback Chorus
      _patchParams[(int) PatchParam::ChorusFeedback] = 0x40;
      _patchParams[(int) PatchParam::ChorusDelay] =    0x7f;
      _patchParams[(int) PatchParam::ChorusRate] =     0x02;
      _patchParams[(int) PatchParam::ChorusDepth] =    0x18;
      break;

    case 5: // Flanger
      _patchParams[(int) PatchParam::ChorusFeedback] = 0x70;
      _patchParams[(int) PatchParam::ChorusDelay] =    0x7f;
      _patchParams[(int) PatchParam::ChorusRate] =     0x01;
      _patchParams[(int) PatchParam::ChorusDepth] =    0x05;
      break;

    case 6: // Short Delay
      _patchParams[(int) PatchParam::ChorusFeedback] = 0x00;
      _patchParams[(int) PatchParam::ChorusDelay] =    0x7f;
      _patchParams[(int) PatchParam::ChorusRate] =     0x00;
      _patchParams[(int) PatchParam::ChorusDepth] =    0x7f;
      break;

    case 7: // Short Delay (FB)
      _patchParams[(int) PatchParam::ChorusFeedback] = 0x50;
      _patchParams[(int) PatchParam::ChorusDelay] =    0x7f;
      _patchParams[(int) PatchParam::ChorusRate] =     0x00;
      _patchParams[(int) PatchParam::ChorusDepth] =    0x7f;
      break;
    }

  // These params are equal for all macro values
  _patchParams[(int) PatchParam::ChorusLevel] =        0x40;
  _patchParams[(int) PatchParam::ChorusPreLPF] =       0x00;
  _patchParams[(int) PatchParam::ChorusSendToReverb] = 0x00;
}


// Macro based on table from SC-8820 owner's manual and verified on SC-55MkII
void Settings::_run_macro_reverb(uint8_t value)
{
  switch(value)
    {
    case 0: // Room 1
      _patchParams[(int) PatchParam::ReverbCharacter] =     0x00;
      _patchParams[(int) PatchParam::ReverbPreLPF] =        0x03;
      _patchParams[(int) PatchParam::ReverbTime] =          0x50;
      _patchParams[(int) PatchParam::ReverbDelayFeedback] = 0x00;
      break;

    case 1: // Room 2
      _patchParams[(int) PatchParam::ReverbCharacter] =     0x01;
      _patchParams[(int) PatchParam::ReverbPreLPF] =        0x04;
      _patchParams[(int) PatchParam::ReverbTime] =          0x38;
      _patchParams[(int) PatchParam::ReverbDelayFeedback] = 0x00;
      break;

    case 2: // Room 3
      _patchParams[(int) PatchParam::ReverbCharacter] =     0x02;
      _patchParams[(int) PatchParam::ReverbPreLPF] =        0x00;
      _patchParams[(int) PatchParam::ReverbTime] =          0x40;
      _patchParams[(int) PatchParam::ReverbDelayFeedback] = 0x00;
      break;

    case 3: // Hall 1
      _patchParams[(int) PatchParam::ReverbCharacter] =     0x03;
      _patchParams[(int) PatchParam::ReverbPreLPF] =        0x04;
      _patchParams[(int) PatchParam::ReverbTime] =          0x48;
      _patchParams[(int) PatchParam::ReverbDelayFeedback] = 0x00;
      break;

    case 4: // Hall 2
      _patchParams[(int) PatchParam::ReverbCharacter] =     0x04;
      _patchParams[(int) PatchParam::ReverbPreLPF] =        0x00;
      _patchParams[(int) PatchParam::ReverbTime] =          0x40;
      _patchParams[(int) PatchParam::ReverbDelayFeedback] = 0x00;
      break;

    case 5: // Plate
      _patchParams[(int) PatchParam::ReverbCharacter] =     0x05;
      _patchParams[(int) PatchParam::ReverbPreLPF] =        0x00;
      _patchParams[(int) PatchParam::ReverbTime] =          0x58;
      _patchParams[(int) PatchParam::ReverbDelayFeedback] = 0x00;
      break;

    case 6: // Delay
      _patchParams[(int) PatchParam::ReverbCharacter] =     0x06;
      _patchParams[(int) PatchParam::ReverbPreLPF] =        0x00;
      _patchParams[(int) PatchParam::ReverbTime] =          0x20;
      _patchParams[(int) PatchParam::ReverbDelayFeedback] = 0x28;
      break;

    case 7: // Panning Delay
      _patchParams[(int) PatchParam::ReverbCharacter] =     0x07;
      _patchParams[(int) PatchParam::ReverbPreLPF] =        0x00;
      _patchParams[(int) PatchParam::ReverbTime] =          0x40;
      _patchParams[(int) PatchParam::ReverbDelayFeedback] = 0x20;
      break;
    }

  // These params are equal for all macro values
  _patchParams[(int) PatchParam::ReverbLevel] =         0x40;
}


// The SC-55+ have 6 independent controllers that each controls 11 parameters.
// These parameters are accumulated from each of the controllers. Note that
// accumulated values are only updated when a controller changes value - and
// not when a controller parameter is changed.
void Settings::_update_controller_input(enum PatchParam pp, uint8_t value,
					int8_t part)
{
  uint8_t partAddr = _convert_to_roland_part_id_LUT[part];
  int ctrlId;

  switch ((int) pp)
    {
    case ((int) PatchParam::Modulation):
      ctrlId = 0x00;
      break;
    case ((int) PatchParam::PitchBend):
      ctrlId = 0x10;
      break;
    case ((int) PatchParam::ChannelPressure):
      ctrlId = 0x20;
      break;
    case ((int) PatchParam::PolyKeyPressure):
      ctrlId = 0x30;
      break;
    case ((int) PatchParam::CC1Controller):
      ctrlId = 0x40;
      break;
    case ((int) PatchParam::CC2Controller):
      ctrlId = 0x50;
      break;
    default:
      std::cerr << "libEmuSC: Internal error (unkown controller)" << std::endl;
      return;
    }

  // Pitch control is the only controller input that uses 16 bit data to store
  // its value. This is only relevant for the combination of pitchbend
  // controller input controlling pitch bend (?).
  /*
  if (pp == PatchParam::PitchBend) {
    uint16_t pitchBend =
      (_patchParams[((int) PatchParam::PB_PitchControl | (partAddr << 8))] - 0x40) *
      (value / 8192) * (((value & 0x7f) << 7) + (lsb & 0x7f));
    set_param_uint14(PatchParam::CtM_PitchControl, pitchBend, part);
  } else {
    _patchParams[((int) PatchParam::CtM_PitchControl | (partAddr << 8)) + ctrlId] =
      0x40 + (_patchParams[((int) PatchParam::MOD_PitchControl | (partAddr << 8)) + ctrlId] - 0x40) * (value / 127.0);
  }
  */
  _patchParams[((int) PatchParam::CtM_TVFCutoffControl | (partAddr << 8)) + ctrlId] =
    0x40 + (_patchParams[((int) PatchParam::MOD_TVFCutoffControl | (partAddr << 8)) + ctrlId] - 0x40) * (value / 127.0);
  _patchParams[((int) PatchParam::CtM_AmplitudeControl | (partAddr << 8)) + ctrlId] =
    0x40 + (_patchParams[((int) PatchParam::MOD_AmplitudeControl | (partAddr << 8)) + ctrlId] - 0x40) * (value / 127.0);
  _patchParams[((int) PatchParam::CtM_LFO1RateControl | (partAddr << 8)) + ctrlId] =
    + 0x40 + (_patchParams[((int) PatchParam::MOD_LFO1RateControl | (partAddr << 8)) + ctrlId] - 0x40) * (value / 127.0);
  _patchParams[((int) PatchParam::CtM_LFO1PitchDepth | (partAddr << 8)) + ctrlId] =
    _patchParams[((int) PatchParam::MOD_LFO1PitchDepth | (partAddr << 8)) + ctrlId] * (value / 127.0);
  _patchParams[((int) PatchParam::CtM_LFO1TVFDepth | (partAddr << 8)) + ctrlId] =
    _patchParams[((int) PatchParam::MOD_LFO1TVFDepth | (partAddr << 8)) + ctrlId] * (value / 127.0);
  _patchParams[((int) PatchParam::CtM_LFO1TVADepth | (partAddr << 8)) + ctrlId] =
    _patchParams[((int) PatchParam::MOD_LFO1TVADepth | (partAddr << 8)) + ctrlId] * (value / 127.0);
  _patchParams[((int) PatchParam::CtM_LFO2RateControl | (partAddr << 8)) + ctrlId] =
    0x40 + (_patchParams[((int) PatchParam::MOD_LFO2RateControl | (partAddr << 8)) + ctrlId] - 0x40) * (value / 127.0);
  _patchParams[((int) PatchParam::CtM_LFO2PitchDepth | (partAddr << 8)) + ctrlId] =
    _patchParams[((int) PatchParam::MOD_LFO2PitchDepth | (partAddr << 8)) + ctrlId] * (value / 127.0);
  _patchParams[((int) PatchParam::CtM_LFO2TVFDepth | (partAddr << 8)) + ctrlId] =
    _patchParams[((int) PatchParam::MOD_LFO2TVFDepth | (partAddr << 8)) + ctrlId] * (value / 127.0);
  _patchParams[((int) PatchParam::CtM_LFO2TVADepth | (partAddr << 8)) + ctrlId] =
    _patchParams[((int) PatchParam::MOD_LFO2TVADepth | (partAddr << 8)) + ctrlId] * (value / 127.0);

  // Update list of accumulated controller values with new values
  _update_controller_input_acc(pp, part);
}


// TODO: Remove pp if not needed by pitch bend
void Settings::_update_controller_input_acc(enum PatchParam pp, int8_t part)
{
  uint8_t partAddr = _convert_to_roland_part_id_LUT[part];

  // TODO: Add support for pitch bend

  _accumulate_controller_values(PatchParam::CtM_TVFCutoffControl,
				PatchParam::Acc_TVFCutoffControl,
				part, -64, 63, true);
  _accumulate_controller_values(PatchParam::CtM_AmplitudeControl,
				PatchParam::Acc_AmplitudeControl,
				part, 0, 127, true);
  _accumulate_controller_values(PatchParam::CtM_LFO1RateControl,
				PatchParam::Acc_LFO1RateControl,
				part, 0, 127, true);
  _accumulate_controller_values(PatchParam::CtM_LFO1PitchDepth,
				PatchParam::Acc_LFO1PitchDepth,
				part, 0, 127, false);
  _accumulate_controller_values(PatchParam::CtM_LFO1TVFDepth,
				PatchParam::Acc_LFO1TVFDepth,
				part, 0, 127, false);
  _accumulate_controller_values(PatchParam::CtM_LFO1TVADepth,
				PatchParam::Acc_LFO1TVADepth,
				part, 0, 127, false);
  _accumulate_controller_values(PatchParam::CtM_LFO2RateControl,
				PatchParam::Acc_LFO2RateControl,
				part, 0, 127, true);
  _accumulate_controller_values(PatchParam::CtM_LFO2PitchDepth,
				PatchParam::Acc_LFO2PitchDepth,
				part, 0, 127, false);
  _accumulate_controller_values(PatchParam::CtM_LFO2TVFDepth,
				PatchParam::Acc_LFO2TVFDepth,
				part, 0, 127, false);
  _accumulate_controller_values(PatchParam::CtM_LFO2TVADepth,
				PatchParam::Acc_LFO2TVADepth,
				part, 0, 127, false);
}


void Settings::_accumulate_controller_values(enum PatchParam ctm,
					     enum PatchParam acc,
					     int8_t part, int min, int max, bool center)
{
  uint8_t partAddr = _convert_to_roland_part_id_LUT[part];
  int accValue = 0;

  // Assuming we recieve the PatchParam from the first controller, and that each
  // controller is separated by 0x10 in RAM
  for (int i = 0; i < 6; i++) {
    if (center)
      accValue += _patchParams[((int) ctm | (partAddr << 8)) + (i * 0x10)] - 0x40;
    else
      accValue += _patchParams[((int) ctm | (partAddr << 8)) + (i * 0x10)];
  }

  if (center) accValue += 0x40;
  accValue = std::min(accValue, max);
  accValue = std::max(accValue, min);
  _patchParams[(int) acc | (partAddr << 8)] = accValue;
}

}
