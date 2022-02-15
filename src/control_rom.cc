/*  
 *  EmuSC - Sound Canvas emulator
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

// Control ROM decoding is based on the SC55_Soundfont generator written by
// Kitrinx and NewRisingSun [ https://github.com/Kitrinx/SC55_Soundfont ]


#include "control_rom.h"
#include "ex.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>


ControlRom::ControlRom(Config &config)
  : _config(config)
{
  std::string romPath = config.get("control_rom");
  std::ifstream romFile(romPath, std::ios::binary | std::ios::in);
  if (!romFile.is_open()) {
    throw(Ex(-1,"Unable to open control ROM: " + romPath));
  }

  _romData.assign((std::istreambuf_iterator<char>(romFile)),
		   std::istreambuf_iterator<char>());

  romFile.close();

  if (_identify_model()) {
    throw(Ex(-1,"Unknown control ROM file! Terminating."));
  }
}


ControlRom::~ControlRom()
{}


uint16_t ControlRom::_native_endian_uint16(uint8_t *ptr)
{
  if (_le_native())
    return (ptr[0] << 8 | ptr[1]);

  return (ptr[1] << 8 | ptr[0]);
}


uint32_t ControlRom::_native_endian_3bytes_uint32(uint8_t *ptr)
{
  uint32_t result = 0;
  uint8_t *result_ptr = (uint8_t *) &result;

  if (!_le_native()) {
    for (uint32_t x = 0; x < 3; x++) {
      result_ptr[x] = ptr[x];
    }
  } else {
    for (int x = 0; x < 3; x++) {
      result_ptr[x] = ptr[2 - x];
    }
  }

  return result;
}


uint32_t ControlRom::_native_endian_4bytes_uint32(uint8_t *ptr)
{
  uint32_t result = 0;
  uint8_t *result_ptr = (uint8_t *) &result;

  if (!_le_native()) {
    for (uint32_t x = 0; x < 4; x++) {
      result_ptr[x] = ptr[x];
    }
  } else {
    for (int x = 0; x < 4; x++) {
      result_ptr[x] = ptr[3 - x];
    }
  }

  return result;
}


int ControlRom::_identify_model(void)
{
  std::string model;
  std::string version;
  std::string date;

  // Search for SC-55 control ROM files
  if (!strncmp((char *) &_romData[0xf380], "Ver", 3)) {
    _synthModel = sm_SC55;
    model.assign("SC-55");
    version.assign((char *) &_romData[0xf383], 4); 
    date.assign((char *) &_romData[0xf398], 14); 
  }

  // Search for SC-55mkII control ROM files
  if (!strncmp((char *)&_romData[0x3d148], "GS-28 VER=2.00  SC", 18)) {
    _synthModel = sm_SC55mkII;
    model.assign("SC-55mkII");
    version.assign("2.00");
    date.assign("?");
  }

  if (model.empty())         // No valid ROM file found    TODO: SC88 ??
    return -1;
  
  std::cout << "EmuSC: " << model << " control ROM found [version=" << version
	    << " date=" << date << "]" << std::endl;

  return 0;
}


uint32_t *ControlRom::_get_banks(void)
{
  switch(_synthModel)
    {
    case sm_VSC:
      return _banksVSC;
        
    case sm_SC55:
    case sm_SC55mkII:                   // Are these banks actually the same?
      return _banksSC55;
      
      case sm_SC88:                     // No work has been done here yet
	return 0;
    }

  return 0;
}


// Note: instrument partials (instPartial) contains 90 unused bytes! ADSR?
int ControlRom::get_instruments(std::vector<Instrument> &instruments)
{
  // ROM is split in 8 banks
  uint32_t *banks = _get_banks();

  // Instruments are in bank 0 & 3, each instrument block using 216 bytes
  for (int32_t x = banks[0]; x < banks[4]; x += 216) {

    // Skip area between bank 0 and 3
    if (x == banks[1])
      x = banks[3];

    struct Instrument i;

    // First 12 bytes are the instrument name
    i.name.assign((char *)&_romData[x], 12); 
    i.name.erase(std::find_if(i.name.rbegin(), i.name.rend(),
			      std::bind1st(std::not_equal_to<char>(),
					   ' ')).base(), i.name.end());

    // Partial indexes has bank position 34 & 126
    i.partials[0].partialIndex = _native_endian_uint16(&_romData[x + 34]);
    i.partials[1].partialIndex = _native_endian_uint16(&_romData[x + 126]);

    // Skip empty slots in the ROM file that has no instrument name
    if (i.name[0]) {
      instruments.push_back(i);

      if (_config.verbose())
	std::cout << "  -> Instrument " << instruments.size() << ": " << i.name
		  << std::endl;
    }
  }

  return 0;
}


int ControlRom::get_partials(std::vector<Partial> &partials)
{
  // ROM is split in 8 banks
  uint32_t *banks = _get_banks();

  // Partials are in bank 1 & 4, each partial block using 60 bytes
  for (int32_t x = banks[1]; x < banks[5]; x += 60) {

    // Skip area between bank 1 and 4
    if (x == banks[2])
      x = banks[4];

    struct Partial p;

    p.name.assign((char *)&_romData[x], 12);
    p.name.erase(std::find_if(p.name.rbegin(), p.name.rend(),
			      std::bind1st(std::not_equal_to<char>(),
					   ' ')).base(), p.name.end());
    
    // 16 byte array of break values for tone pitch
    for (int i = 0; i < 16; i++)
      p.breaks[i] = _romData[x + 12 + i];

    // 16 2-byte array with accompanying sample IDs
    for (int i = 0; i < 16; i++)
      p.samples[i] = _native_endian_uint16(&_romData[x + 28 + (2 * i)]);


    // Skip empty slots in the ROM file that has no partial name
    if (p.name[0]) {
      partials.push_back(p);

      if (_config.verbose())
	std::cout << "  -> Partial group " << partials.size() <<  ": "
		  << p.name << std::endl;
    }
  }

  return 0;
}


int ControlRom::get_samples(std::vector<Sample> &samples)
{
  // ROM is split in 8 banks
  uint32_t *banks = _get_banks();

  // Samples are in bank 2 & X, each sample block using 16 bytes
  for (int32_t x = banks[2]; x < banks[6]; x += 16) {

    // Skip area between bank 1 and 4
    if (x == banks[3])
      x = banks[5];

    struct Sample s;

    s.volume = _romData[x];
    s.address = _native_endian_3bytes_uint32(&_romData[x + 1]);
    s.attackEnd = _native_endian_uint16(&_romData[x + 4]);
    s.sampleLen = _native_endian_uint16(&_romData[x + 6]);
    s.loopLen = _native_endian_uint16(&_romData[x + 8]);
    s.loopMode = _romData[x + 10];
    s.rootKey = _romData[x + 11];
    s.pitch = _native_endian_uint16(&_romData[x + 12]);
    s.fineVolume = _native_endian_uint16(&_romData[x + 14]);
    
    if (s.sampleLen) {                          // Ignore empty parts
      samples.push_back(s);
      
      if (_config.verbose()) {
	std::cout << "  -> Sample " << std::setw(3) << samples.size()
		  << ": V=" << std::setw(3) << +s.volume
		  << " AE=" << std::setw(5) << +s.attackEnd
		  << " SL=" << std::setw(5) << +s.sampleLen
		  << " LL=" << std::setw(5) << +s.loopLen
		  << " LM=" << std::setw(3) << +s.loopMode
		  << " RK=" << std::setw(3) << +s.rootKey
		  << " P="  << std::setw(5) << +s.pitch - 1024
		  << " FV=" << std::setw(4) << +s.fineVolume - 1024
		  << std::endl;
      }
    }
  }
  
  return 0;
}           


int ControlRom::get_variations(std::vector<Variation> &variations)
{
  // ROM is split in 8 banks
  uint32_t *banks = _get_banks();

  // Variations are in bank 6, a table of 128 x 128 2 byte values
  for (int32_t x = banks[6]; x < (banks[7] - 128); x += 256) {

    struct Variation v;
    for (int y = 0; y < 128; y++) {
      v.variation[y] = _native_endian_uint16(&_romData[x + (2 * y)]);
    }

    variations.push_back(v);
  }

  if (0) { //_config.verbose()) {
    int i = 0;
    for (struct Variation v : variations) {
      std::cout << "  -> Variations " << i++ << ": ";
      for (int y = 0; y < 128; y++)
	if (v.variation[y] == 0xffff)
	  std::cout << "-,";
	else
	  std::cout << v.variation[y] << ",";
      std::cout << '\b' << " " << std::endl;
    }
  }
  
  return 0;
}


int ControlRom::get_drumSet(std::vector<DrumSet> &drums)
{
  // ROM is split in 8 banks
  uint32_t *banks = _get_banks();

  // The drum set is in bank 7, a total of 14 drums in 1164 byte blocks 
  for (int32_t x = banks[7]; x < 0x03c028; x += 1164) {

    struct DrumSet d;

    for (int i = 0; i < 128; i++)
      d.preset[i] = _native_endian_uint16(&_romData[x + (i * 2)]);

    std::memcpy(d.volume,      &_romData[x +  256], 128);
    std::memcpy(d.key,         &_romData[x +  384], 128);
    std::memcpy(d.assignGroup, &_romData[x +  512], 128);
    std::memcpy(d.panpot,      &_romData[x +  640], 128);
    std::memcpy(d.reverb,      &_romData[x +  768], 128);
    std::memcpy(d.chorus,      &_romData[x +  896], 128);
    std::memcpy(d.flags,       &_romData[x + 1024], 128);
    
    // Last 12 bytes are the drum name
    d.name.assign((char *) &_romData[x + 1152], 12); 
    d.name.erase(std::find_if(d.name.rbegin(), d.name.rend(),
			      std::bind1st(std::not_equal_to<char>(),
					   ' ')).base(), d.name.end());
    drums.push_back(d);

    if (_config.verbose())
      std::cout << "  -> Drum " << drums.size() << ": " << d.name
		<< std::endl;
  }

  return 0;
}


int ControlRom::dump_demo_songs(std::string path)
{
  int index = 1;
  std::cout << "EmuSC: Searching for MIDI songs in control ROM" << std::endl;
  
  for (uint32_t i = 0; i < _get_banks()[0] - 6; i++) {
    if (_romData[i + 0] == 0x4d &&
	_romData[i + 1] == 0x54 &&
	_romData[i + 2] == 0x68 &&
	_romData[i + 3] == 0x64 &&
	_romData[i + 4] == 0x00 &&
	_romData[i + 5] == 0x00 &&
	_romData[i + 6] == 0x00 &&
	_romData[i + 7] == 0x06) {

      uint16_t numTracks = _native_endian_uint16(&_romData[i+10]);
      uint32_t fileSize = 14;
      for (int n = 0; n < numTracks; n++) {
	if (_romData[i + fileSize] == 0x4d &&
	    _romData[i + fileSize + 1] == 0x54 &&
	    _romData[i + fileSize + 2] == 0x72 &&
	    _romData[i + fileSize + 3] == 0x6b) {
	  fileSize += _native_endian_4bytes_uint32(&_romData[i + fileSize + 4]);
	  fileSize += 8;                                    // Add track header
	} else {
	  return -1;
	}
      }

      if (path.back() != '/')
	path.append("/");

      std::string fileName = "sc_song_" + std::to_string(index++) + ".mid";

      std::ofstream midiFile(path + fileName, std::ios::out | std::ios::binary);
      midiFile.write((char*) &_romData[i], fileSize);
      if (midiFile.good())
	std::cout << " -> Found demo song at 0x" << std::hex << i
		  << " (" << std::dec << (int) fileSize << " bytes)"
		  << std::endl
		  << "  -> File written to " << path + fileName << std::endl;
      else
	std::cout << " -> Error writing demo song to disk: " << path
		  << " Check write permissions and available space."
		  << std::endl;
      midiFile.close();
    }
  }

  return 0;
}
