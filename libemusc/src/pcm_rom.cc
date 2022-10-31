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

// PCM ROM decoding is based on the SC55_Soundfont generator written by
// Kitrinx and NewRisingSun [ https://github.com/Kitrinx/SC55_Soundfont ]


#include "pcm_rom.h"
#include "riaa_filter.h"

#include <cmath>
#include <fstream>
#include <iostream>


namespace EmuSC {


PcmRom::PcmRom(std::vector<std::string> romPath, ControlRom &ctrlRom)
{
  std::vector<char> romData;

  if (romPath.empty())
    throw (std::string("No PCM ROM file specified"));

  for (auto rp : romPath) {
    std::ifstream romFile(rp, std::ios::binary | std::ios::in);
    if (!romFile.is_open()) {
      throw(std::string("Unable to open PCM ROM file: ") + rp);
    }

    std::vector<char> encBuf((std::istreambuf_iterator<char>(romFile)),
			     std::istreambuf_iterator<char>());

    if (!(encBuf.size() == 0x100000 || encBuf.size() == 0x200000)) {
      throw (std::string("Incorrect file size of PCM ROM file ") + rp +
	     std::string(". PCM ROM files are always either 1MB or 2MB"));
    }

    uint32_t offset = romData.size();
    romData.resize(romData.size() + encBuf.size());

    // TODO: Add support for decoding SC-88 ROMs
    for (uint32_t i = 0; i < 0x100000; i++) {
      romData[_unscramble_pcm_rom_address(i) + offset] =
	i >= 0x20 ? _unscramble_pcm_rom_data(encBuf[i]) : encBuf[i];
    }

    if (encBuf.size() == 0x200000) {
      offset += 0x100000;
      for (uint32_t i = 0; i < 0x100000; i++) {
	romData[_unscramble_pcm_rom_address(i) + offset] =
	  i >= 0x20 ? _unscramble_pcm_rom_data(encBuf[i + offset]) : encBuf[i + offset];
      }
    }

    romFile.close();
  }

  if (0) {  // Dump complete ROM to file
    std::ofstream dump("/tmp/pcm_rom_mk2.bin", std::ios::binary);
    dump.write(&romData[0], romData.size());
    dump.close();
  }

  // Read through the entire memory and extract sample sets
  for (int i = 0; i < ctrlRom.numSampleSets(); i ++)
    _read_samples(romData, ctrlRom.sample(i));

  _version = std::string(&romData[0x1c], 4);
  _date = std::string(&romData[0x30], 10);
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


uint32_t PcmRom::_find_samples_rom_address(uint32_t address)
{
  uint32_t bank = 0;
  switch ((address & 0x700000) >> 20)
    {
    case 0: bank = 0x000000; break;
    case 1: bank = 0x100000; break;               // Used in SC-55mkII / SCB
    case 2: bank = 0x100000; break;
    case 4: bank = 0x200000; break;
    default:
      throw(std::string("Unknown bank ID in PcmRom::get_sample(): ") +
	    std::to_string(address & 0x700000));
    }

  return (address & 0xFFFFF) | bank;
}


int PcmRom::_read_samples(std::vector<char> romData, struct ControlRom::Sample &ctrlSample)
{
  RiaaFilter rf1(32000, 15); // Gain 28 seems right, but becomes too much later
  RiaaFilter rf2(32000, 15);

  uint32_t romAddress = _find_samples_rom_address(ctrlSample.address);

  struct Samples s;

  // Read PCM samples from ROM
  for (int i = 0; i < ctrlSample.sampleLen; i++) {
    uint32_t sAddress = romAddress + i;
    int8_t data = romData[sAddress];
    uint8_t sByte = romData[((sAddress & 0xFFFFF) >> 5)|(sAddress & 0xF00000)];
    uint8_t sNibble = (sAddress & 0x10) ? (sByte >> 4 ) : (sByte & 0x0F);
    int32_t final = ((data << sNibble) << 14);

    // Move to float and apply 2x RIAA deemphasis filters
    float ffinal = (float) final / (1 << 31);
    ffinal = rf1.apply(ffinal);
    ffinal = rf2.apply(ffinal);

    s.samplesF.push_back(ffinal);
  }

  _sampleSets.push_back(s);

  return s.samplesF.size();
}
  
}
