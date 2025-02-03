/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2024  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC. If not, see <http://www.gnu.org/licenses/>.
 */


#include "rom_info.h" 

#include <iostream>


ROMInfo::ROMInfo(void)
{
}


ROMInfo::~ROMInfo()
{}


ROMInfo::ROMSetInfo *ROMInfo::get_rom_set_info_from_prog(std::string sha256)
{
  for (auto& romSetInfo : _romSetList) {
    if (romSetInfo.controlROMs.progROMsha256 == sha256)
      return &romSetInfo;
  }

  return nullptr;
}


ROMInfo::ROMSetInfo *ROMInfo::get_rom_set_info_from_cpu(std::string sha256)
{
  for (auto& romSetInfo : _romSetList) {
    if (romSetInfo.controlROMs.cpuROMsha256 == sha256)
      return &romSetInfo;
  }

  return nullptr;
}


ROMInfo::ROMSetInfo *ROMInfo::get_rom_set_info_from_wave(std::string sha256,
							 int *index)
{
  for (auto& romSetInfo : _romSetList) {
    for (int i = 0; i < 3; i ++)
      if (romSetInfo.waveROMs.sha256[i] == sha256) {
	*index = i;
        return &romSetInfo;
      }
  }

  return nullptr;
}
