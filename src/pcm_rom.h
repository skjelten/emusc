/*
 *  EmuSC - Emulating the Sound Canvas
 *  Copyright (C) 2022  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC.  If not, see <http://www.gnu.org/licenses/>.
 */

// PCM ROM decoding is based on the SC55_Soundfont generator written by
// Kitrinx and NewRisingSun [ https://github.com/Kitrinx/SC55_Soundfont ]


#ifndef __PCM_ROM_H__
#define __PCM_ROM_H__


#include "config.h"

#include <string>
#include <vector>


class PcmRom
{
private:
  std::vector<char> _romData;
  
  uint32_t _unscramble_pcm_rom_address(uint32_t address);
  int8_t   _unscramble_pcm_rom_data(int8_t byte);

  PcmRom();
  
public:
  PcmRom(Config &config);
  ~PcmRom();

  int dump_rom(std::string path);
  int get_samples(std::vector<int32_t> *samples, uint32_t address, int len);

};


#endif  // __PCM_ROM_H__
