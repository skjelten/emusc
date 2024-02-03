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


ROMInfo::CtrlROMInfo *ROMInfo::get_control_rom_info(std::string sha256)
{
  for (auto& romInfo : _ctrlROMList) {
    if (romInfo.sha256 == sha256)
      return &romInfo;
  }

  return nullptr;
}


ROMInfo::PcmROMInfo *ROMInfo::get_pcm_rom_info(std::string sha256)
{
  for (auto& romInfo : _pcmROMList) {
    for (int i = 0; i < 4; i ++)
      if (romInfo.sha256[i] == sha256)
	return &romInfo;
  }

  return nullptr;
}


int ROMInfo::get_number_of_pcm_rom_files(PcmROMInfo *romInfo)
{
  for (int i = 1; i < 4; i ++)
    if (romInfo->sha256[i] == "")
      return i;

  return 4;
}


int ROMInfo::get_pcm_rom_index(PcmROMInfo *romInfo, std::string sha256)
{
  for (int i = 0; i < 4; i ++)
    if (romInfo->sha256[i] == sha256)
      return i;

  return -1;
}
