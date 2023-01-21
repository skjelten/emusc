/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2023  Håkon Skjelten
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

#include <iostream>


namespace EmuSC {


Settings::Settings()
{
  // TODO: Add SC-55/88 to master settings
  _initialize_system_params();
  _initialize_patch_params();
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
  int8_t rolandPart = convert_to_roland_part_id(part);

  if (rolandPart < 0)
    return (uint8_t) _patchParams[(int) pp];

  return (uint8_t) _patchParams[(((int) pp) | (rolandPart << 8))];
}


uint8_t* Settings::get_param_ptr(enum PatchParam pp, int8_t part)
{
  int8_t rolandPart = convert_to_roland_part_id(part);
  int address = (int) pp;

  if (rolandPart < 0)
    return  &_patchParams[address];

  return &_patchParams[(address | (rolandPart << 8))];
}


uint16_t Settings::get_param_uint16(enum PatchParam pp, int8_t part)
{
  int8_t rolandPart = convert_to_roland_part_id(part);
  int address = (int) pp;

  if (rolandPart < 0)
    return  _to_native_endian_uint16(&_patchParams[address]);

  return _to_native_endian_uint16(&_patchParams[(address | (rolandPart << 8))]);
}


uint8_t Settings::get_patch_param(uint16_t address, int8_t part)
{
  int8_t rolandPart = convert_to_roland_part_id(part);

  if (rolandPart < 0)
    return (uint8_t) _patchParams[address];

  return (uint8_t) _patchParams[(address | (rolandPart << 8))];
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
    
  } else { // TODO: FIXME!!
    _systemParams[(int) sp + 3] = (value >> 8) & 0xf0 >> 4;
    _systemParams[(int) sp + 2] = (value >> 8) & 0x0f >> 0;
    _systemParams[(int) sp + 1] = (value >> 0) & 0xf0 >> 4;
    _systemParams[(int) sp + 0] = (value >> 0) & 0x0f >> 0;
  }
}


void Settings::set_system_param(uint16_t address, uint8_t *value, uint8_t size)
{
  for (int i = 0; i < size; i++)
    _systemParams[address + i] = value[i];
}


void Settings::set_param(enum PatchParam pp, uint8_t value, int8_t part)
{
  int8_t rolandPart = convert_to_roland_part_id(part);

  if (rolandPart < 0)
    _patchParams[(int) pp] = value;
  else
    _patchParams[(((int) pp) | (rolandPart << 8))] = value;
}


void Settings::set_param(enum PatchParam pp, uint8_t *data, uint8_t size,
			 int8_t part)
{
  int8_t rolandPart = convert_to_roland_part_id(part);

  for (int i = 0; i < size; i++) {
    if (rolandPart < 0)
      _patchParams[(int) pp + i] = data[i];
    else
      _patchParams[(((int) pp) | (rolandPart << 8)) + i] = data[i];   
  }
}


void Settings::set_patch_param(uint16_t address, uint8_t *data, uint8_t size)
{
  for (int i = 0; i < size; i++)
    _patchParams[address + i] = data[i];
}


void Settings::set_patch_param(uint16_t address, uint8_t value, int8_t part)
{
  int8_t rolandPart = convert_to_roland_part_id(part);

  if (rolandPart < 0)
    _patchParams[address] = value;

  _patchParams[(address | (rolandPart << 8))] = value;
}


// What on earth did Roland employees smoke to come up with this?
int8_t Settings::convert_to_roland_part_id(int8_t part)
{
  if (part < 0 || part > 15)  // TODO: Check for SC-88 and adjust max parts
    return -1;

  if (part == 9)
    return 0;
  else if (part < 9)
    return part + 1;

  return part; 
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
  _patchParams[(int) PatchParam::ReverbSendToChorus] =  0x00;

  _patchParams[(int) PatchParam::ChorusMacro] =         0x02;
  _patchParams[(int) PatchParam::ChorusPreLPF] =        0x00;
  _patchParams[(int) PatchParam::ChorusLevel] =         0x40;
  _patchParams[(int) PatchParam::ChorusFeedback] =      0x08;
  _patchParams[(int) PatchParam::ChorusDelay] =         0x50;
  _patchParams[(int) PatchParam::ChorusRate] =          0x03;
  _patchParams[(int) PatchParam::ChorusDepth] =         0x13;
  _patchParams[(int) PatchParam::ChorusSendToReverb] =  0x00;

  // Remaining paramters are separate for each part
  for (int p = 0; p < 16; p ++) {           // TODO: Support SC-88 with 32 parts
    uint8_t partAddr = convert_to_roland_part_id(p);

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
    _patchParams[(int) PatchParam::ReverbSendLevel    | (partAddr << 8)] = 0x28;
    _patchParams[(int) PatchParam::ChorusSendLevel    | (partAddr << 8)] = 0x00;

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
    _patchParams[(int) PatchParam::PB_LFO2TVADepth     |(partAddr << 8)] = 0x40;

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

    // Controller values
    _patchParams[(int) PatchParam::PitchBend       |(partAddr << 8)] = 0x20;
    _patchParams[(int) PatchParam::PitchBend + 1   |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Modulation      |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC1Controller   |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::CC2Controller   |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::ChannelPressure |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PolyKeyPressure |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Hold1           |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Sostenuto       |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Soft            |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::Expression      |(partAddr << 8)] = 0x7f;
    _patchParams[(int) PatchParam::Portamento      |(partAddr << 8)] = 0x00;
    _patchParams[(int) PatchParam::PortamentoTime  |(partAddr << 8)] = 0x00;

    _patchParams[(int) PatchParam::RPN_LSB         |(partAddr << 8)] = 0x7f;
    _patchParams[(int) PatchParam::RPN_MSB         |(partAddr << 8)] = 0x7f;
    _patchParams[(int) PatchParam::NRPN_LSB        |(partAddr << 8)] = 0x7f;
    _patchParams[(int) PatchParam::NRPN_MSB        |(partAddr << 8)] = 0x7f;
  }
}


bool Settings::load(std::string filePath)
{
  return 0;
}


bool Settings::save(std::string filePath)
{
  return 0;
}

void Settings::reset(enum Mode m)
{
  _initialize_system_params();
  _initialize_patch_params();
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

}
