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


#include "pcm_rom.h"
#include "ex.h"

#include <fstream>
#include <iostream>


PcmRom::PcmRom(Config &config)
{
  for (int i = 1; i <= 3; i++ ) {
    std::string romName = "pcm_rom_" + std::to_string(i);
    std::string romPath = config.get(romName);
    std::ifstream romFile(romPath, std::ios::binary | std::ios::in);
    if (!romFile.is_open()) {
      throw(Ex(-1,"Unable to open PCM ROM file: " + romPath));
    }
    
    if (config.verbose()) {
      std::cout << "EmuSC: Sucsessfully opened " << std::to_string(i)
		<< ". PCM ROM file " << romPath << std::endl;
    }
    std::vector<char> encBuf((std::istreambuf_iterator<char>(romFile)),
			     std::istreambuf_iterator<char>());
    int offset = _romData.size();
    _romData.resize(_romData.size() + encBuf.size());

    int j = 0;
    for (auto it = std::begin(encBuf); it != std::end(encBuf); ++it) {
      _romData[_unscramble_pcm_rom_address(j) + offset] =
	j >= 0x20 ? _unscramble_pcm_rom_data(encBuf[j]) : encBuf[j];
      j++;
    }

    romFile.close();
  }

  std::cout << "EmuSC: PCM ROM(s) found and decrypted [version="
	    << std::string(&_romData[0x1c], 4) << " date="
	    << std::string(&_romData[0x30], 10) << "]" << std::endl;
}


PcmRom::~PcmRom()
{}


// Discovered and written by NewRisingSun
uint32_t PcmRom::_unscramble_pcm_rom_address(uint32_t address)
{
  uint32_t newAddress = 0;
  if (address >= 0x20) {	// The first 32 bytes are not encrypted
    static const int addressOrder [20] =
      { 0x02, 0x00, 0x03, 0x04,0x01, 0x09, 0x0D, 0x0A, 0x12,
        0x11, 0x06, 0x0F, 0x0B, 0x10, 0x08, 0x05, 0x0C, 0x07, 0x0E, 0x13 };
    for (uint32_t bit = 0; bit < 20; bit++) {
      newAddress |= ((address >> addressOrder[bit]) & 1) << bit;
    }
  } else {
    newAddress = address;
  }

  return newAddress;
}


int8_t PcmRom::_unscramble_pcm_rom_data(int8_t byte)
{
  uint8_t byteOrder[8] = {2, 0, 4, 5, 7, 6, 3, 1};
  uint32_t newByte = 0;

  for (uint32_t bit = 0; bit < 8; bit++) {
    newByte |= ((byte >> byteOrder[bit]) & 1) << bit;
  }

  return newByte;
}


int PcmRom::dump_rom(std::string path)
{
  std::ofstream ofs(path, std::ios::out | std::ios::binary);

  if (_romData.empty())
    return -1;
  
  ofs.write(reinterpret_cast<char*>(&_romData[0]),
	    _romData.size() * sizeof(_romData[0]));
  ofs.close();

  std::cout << "EmuSC: Decoded PCM ROM written to " << path << std::endl;

  return 0;
}


int PcmRom::get_samples(std::vector<int32_t> *samples, uint32_t address,int len)
{
  uint32_t bank = 0;
  switch ((address & 0x700000) >> 20)
    {
    case 0: bank = 0x000000; break;
    case 1: bank = 0x100000; break;               // Used in SC-55mkII / SCB
    case 2: bank = 0x100000; break;
    case 4: bank = 0x200000; break;
    default: throw(Ex(-1, "Unknown bank ID in PcmRom::get_sample(): " +
		      std::to_string(address & 0x700000)));
    }

  // Finally the correct address inside the ROM
  uint32_t romAddress = (address & 0xFFFFF) | bank;
  
  for (int i = 0; i < len; i++) {
    uint32_t sAddress = romAddress + i;
    int8_t data = _romData[sAddress];
    uint8_t sByte = _romData[((sAddress & 0xFFFFF) >> 5)|(sAddress & 0xF00000)];
    uint8_t sNibble = (sAddress & 0x10) ? (sByte >> 4 ) : (sByte & 0x0F);
    int32_t final = ((data << sNibble) << 14); // Shift nibbles th}
    final = final >> 1;       // FIXME: works most of the time, need compressor?

    samples->push_back(final);
  }

  return samples->size();
}
