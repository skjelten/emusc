/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2023-2023  Håkon Skjelten
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


#ifndef __SETTINGS_H__
#define __SETTINGS_H__


#include "params.h"

#include <stdint.h>

#include <array>
#include <string>


namespace EmuSC {

class Settings
{
public:
  Settings();
  ~Settings();

  // Sound Canvas Modes
  enum class Mode {
    GS,                  // Standard GS mode. Default.
    MT32,                // MT32 mode
    SC55                 // SC-55 mode for SC-88     TODO

    // TODO: What about GM mode?
  };
  
  // Retrieve settings from Config paramters
  uint8_t  get_param(enum SystemParam sp);
  uint8_t* get_param_ptr(enum SystemParam sp);
  uint32_t get_param_uint32(enum SystemParam sp);
  uint16_t get_param_32nib(enum SystemParam sp);
  uint8_t  get_param(enum PatchParam pp, int8_t part = -1);
  uint8_t* get_param_ptr(enum PatchParam pp, int8_t part = -1);
  uint16_t get_param_uint16(enum PatchParam pp, int8_t part = -1);
  uint8_t  get_patch_param(uint16_t address, int8_t part = -1);

  uint8_t  get_param(enum DrumParam);
  uint8_t* get_param_ptr(enum DrumParam);

  // Set settings from Config paramters
  void set_param(enum SystemParam sp, uint8_t value);
  void set_param(enum SystemParam sp, uint8_t *data, uint8_t size);
  void set_param_uint32(enum SystemParam sp, uint32_t value);
  void set_param_32nib(enum SystemParam sp, uint16_t value);
  void set_system_param(uint16_t address, uint8_t *data, uint8_t size = 1);
  
  void set_param(enum PatchParam pp, uint8_t value, int8_t part = -1);
  void set_param(enum PatchParam pp, uint8_t *data, uint8_t size = 1,
		       int8_t part = -1);
  void set_patch_param(uint16_t address, uint8_t *data, uint8_t size = 1);
  void set_patch_param(uint16_t address, uint8_t value, int8_t part = -1);
  
  // TODO: Add drum params
  
  // Store settings paramters to file (aka battery backup)
  bool load(std::string filePath);
  bool save(std::string filePath);

  // Reset all settings to specific mode
  void reset(enum Mode);

  // Temporary solution
  // Figure out the need for a common way with all controllers
  void set_pitchBend(float value, int8_t partId) { _PBController[partId]=value;}
  float get_pitchBend(int8_t partId) { return _PBController[partId]; }

  static int8_t convert_to_roland_part_id(int8_t part);
  static int8_t convert_from_roland_part_id(int8_t part);

private:
  std::array<uint8_t, 0x0100> _systemParams;  // Both SysEx and non-SysEx data
  std::array<uint8_t, 0x4000> _patchParams;   // SysEx data
  
  void _initialize_system_params(enum Mode = Mode::GS);
  void _initialize_patch_params(enum Mode = Mode::GS);

  // BE / LE conversion
  inline bool _le_native(void) { uint16_t n = 1; return (*(uint8_t *) & n); }
  uint16_t _to_native_endian_uint16(uint8_t *ptr);
  uint32_t _to_native_endian_uint32(uint8_t *ptr);

  // Temporary storage for pitchbend
  float _PBController[16];
};

}

#endif  // __SETTINGS_H__
