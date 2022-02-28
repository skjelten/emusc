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
#include "control_rom.h"

#include <string>
#include <vector>

#include <stdint.h>


class PcmRom
{
private:
  struct Samples {
    std::vector<int32_t> samples;    // All samples stored in 24 bit 32kHz mono
    std::vector<float> fsamples;     // 32 bit float, 32kHz, mono
  };
  std::vector<struct Samples> _sampleSets;

  uint32_t _unscramble_pcm_rom_address(uint32_t address);
  int8_t   _unscramble_pcm_rom_data(int8_t byte);

  int _read_samples(std::vector<char> rom, uint32_t address, uint16_t length);

  PcmRom();

public:
  PcmRom(std::vector<std::string> romPath, ControlRom &ctrlRom);
  ~PcmRom();

  inline struct Samples& samples(uint16_t ss) { return _sampleSets[ss]; }

};


#endif  // __PCM_ROM_H__
