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


ControlRom::ControlRom(std::string romPath, int verbose)
  : _romPath(romPath),
    _verbose(verbose)
{
  std::ifstream romFile(romPath, std::ios::binary | std::ios::in);
  if (!romFile.is_open())
    throw(Ex(-1,"Unable to open control ROM: " + romPath));

  if (_identify_model(romFile))
    throw(Ex(-1,"Unknown control ROM file!"));

  // Temporarily block SC-88 ROMs since we don't know how to read them yet
  if (_model == "SC-88")
    throw(Ex(-1,"SC-88 ROM files are not supported yet!"));

  // Read internal data structures from ROM file
  _read_instruments(romFile);
  _read_partials(romFile);
  _read_samples(romFile);
  _read_variations(romFile);
  _read_drum_sets(romFile);

  romFile.close();

  std::cout << "EmuSC: " << _model << " control ROM found [version=" << _version
	    << " date=" << _date << "]" << std::endl;

  if (verbose)
    std::cout << "EmuSC: Found " << _instruments.size() << " instruments, "
	      << _partials.size() << " parts, "
	      << _samples.size() << " samples and "
	      << _drumSets.size() << " drum sets" << std::endl;
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


int ControlRom::_identify_model(std::ifstream &romFile)
{
  char data[32];

  // Search for SC-55 control ROM files
  romFile.seekg(0xf380);
  romFile.read(data, 29);
  if (!strncmp(data, "Ver", 3)) {
    _version.assign(&data[3], 4);
    _date.assign(&data[24], 5);
    _model.assign("SC-55");
    synthModel = sm_SC55;
  }

  // Search for SC-55mkII control ROM files
  romFile.seekg(0x3d148);
  romFile.read(data, 24);
  if (!strncmp(&data[0], "GS-28 VER=2.00  SC", 18)) {
    romFile.seekg(0xfff0);
    romFile.read(data, 4);
    _version.assign(data, 4);
    _date.assign("?");
    _model.assign("SC-55mkII");
    synthModel = sm_SC55mkII;
  } else if (!strncmp(&data[0], "GS-28 VER=2.00  LCGS-3", 22)) {
    _version.assign("?");
    _date.assign("?");
    _model.assign("SCB-55 (SC-55mkII)");
    synthModel = sm_SC55mkII;
  }

  // Search for SC-88 control ROM files
  romFile.seekg(0x7fc0);
  romFile.read(data, 24);
  if (!strncmp(&data[0], "GS-64 VER=3.00  SC-88   ", 24)) {
    _version.assign("?");
    _date.assign("?");
    _model.assign("SC-88");
    synthModel = sm_SC88;
  }

  if (_model.empty())         // No valid ROM file found    TODO: SC88 ??
    return -1;

  return 0;
}


uint32_t *ControlRom::_get_banks(void)
{
  switch(synthModel)
    {
    case sm_SC55:
    case sm_SC55mkII:
      return _banksSC55;
      
    case sm_SC88:                       // No work has been done here yet
      return _banksSC88;
    }

  return 0;
}


// Note: instrument partials (instPartial) contains 90 unused bytes! ADSR?
int ControlRom::_read_instruments(std::ifstream &romFile)
{
  // ROM is split in 8 banks
  uint32_t *banks = _get_banks();

  // Instruments are in bank 0 & 3, each instrument block using 216 bytes
  for (int32_t x = banks[0]; x < banks[4]; x += 216) {

    // Skip area between bank 0 and 3
    if (x == banks[1])
      x = banks[3];

    char data[88];
    romFile.seekg(x);
    struct Instrument i;

    // First 12 bytes are the instrument name
    romFile.read(data, 12);
    i.name.assign(data , 12);
    i.name.erase(std::find_if(i.name.rbegin(), i.name.rend(),
			      std::bind1st(std::not_equal_to<char>(),
					   ' ')).base(), i.name.end());

    // We have 2 partial parameters sets; starting in bank position 34 & 126
    for (int p = 0; p < 2; p++) {
      romFile.seekg(x + 34 + (p * 92));
      romFile.read(data, 2);
      i.partials[p].partialIndex = _native_endian_uint16((uint8_t *) data);

      romFile.read(data, 88);
      i.partials[p].panpot      = data[5];
      i.partials[p].coarsePitch = data[6];
      i.partials[p].finePitch   = data[7];
      i.partials[p].randPitch   = data[8];
      i.partials[p].PitchKeyFlw = data[9];
      i.partials[p].TvpLfoDepth = data[10];
      i.partials[p].pitchMult   = data[12];
      i.partials[p].pitchLvlP0  = data[14];
      i.partials[p].pitchLvlP1  = data[15];
      i.partials[p].pitchLvlP2  = data[16];
      i.partials[p].pitchLvlP3  = data[17];
      i.partials[p].pitchLvlP4  = data[18];
      i.partials[p].pitchDurP1  = data[19];
      i.partials[p].pitchDurP2  = data[20];
      i.partials[p].pitchDurP3  = data[21];
      i.partials[p].pitchDurP4  = data[22];
      i.partials[p].pitchDurRel = data[23];
      i.partials[p].TVFBaseFlt  = data[33];
      i.partials[p].TVFResonance= data[34];
      i.partials[p].LowVelClear = data[35];
      i.partials[p].TVFLvlInit  = data[40];
      i.partials[p].TVFLvlP1    = data[41];
      i.partials[p].TVFLvlP2    = data[42];
      i.partials[p].TVFLvlP3    = data[43];
      i.partials[p].TVFLvlP4    = data[44];
      i.partials[p].TVFDurP1    = data[46];
      i.partials[p].TVFDurP2    = data[47];
      i.partials[p].TVFDurP3    = data[48];
      i.partials[p].TVFDurP4    = data[49];
      i.partials[p].TVFDurRel   = data[50];
      i.partials[p].volume      = data[65];
      i.partials[p].TVFLFODepth = data[68];
      i.partials[p].TVAVolP1    = data[70];
      i.partials[p].TVAVolP2    = data[71];
      i.partials[p].TVAVolP3    = data[72];
      i.partials[p].TVAVolP4    = data[73];
      i.partials[p].TVALenP1    = data[74];
      i.partials[p].TVALenP2    = data[75];
      i.partials[p].TVALenP3    = data[76];
      i.partials[p].TVALenP4    = data[77];
      i.partials[p].TVALenP5    = data[78];
    }

    // Skip empty slots in the ROM file that has no instrument name
    if (i.name[0]) {
      _instruments.push_back(i);

      if (_verbose)
	std::cout << "  -> Instrument " << _instruments.size() << ": " << i.name
		  << " partial0=" << (int) i.partials[0].partialIndex
		  << " partial1=" << (int) i.partials[1].partialIndex
		  << std::endl;
    }
  }

  return 0;
}


int ControlRom::_read_partials(std::ifstream &romFile)
{
  // ROM is split in 8 banks
  uint32_t *banks = _get_banks();

  // Partials are in bank 1 & 4, each partial block using 60 bytes
  for (int32_t x = banks[1]; x < banks[5]; x += 60) {

    // Skip area between bank 1 and 4
    if (x == banks[2])
      x = banks[4];

    char data[32];
    romFile.seekg(x);
    struct Partial p;

    // First 12 bytes are the partial name
    romFile.read(data, 12);
    p.name.assign(data, 12);
    p.name.erase(std::find_if(p.name.rbegin(), p.name.rend(),
			      std::bind1st(std::not_equal_to<char>(),
					   ' ')).base(), p.name.end());

    // 16 byte array of break values for tone pitch
    romFile.read(data, 16);
    for (int i = 0; i < 16; i++)
      p.breaks[i] = data[i];

    // 16 2-byte array with accompanying sample IDs
    romFile.read(data, 32);
    for (int i = 0; i < 16; i++)
      p.samples[i] = _native_endian_uint16((uint8_t *) &data[2 * i]);

    // Skip empty slots in the ROM file that has no partial name
    if (p.name[0]) {
      _partials.push_back(p);

      if (_verbose)
	std::cout << "  -> Partial group " << _partials.size() <<  ": "
		  << p.name << std::endl;
    }
  }

  return 0;
}


int ControlRom::_read_variations(std::ifstream &romFile)
{
  // ROM is split in 8 banks
  uint32_t *banks = _get_banks();

  // Variations are in bank 6, a table of 128 x 128 2 byte values
  for (int32_t x = banks[6]; x < (banks[7] - 128); x += 256) {

    char data[2];
    romFile.seekg(x);
    struct Variation v;

    for (int y = 0; y < 128; y++) {
      romFile.read(data, 2);
      v.variation[y] = _native_endian_uint16((uint8_t *) &data[0]);
    }

    _variations.push_back(v);
  }

  if (_verbose) {
    int i = 0;
    for (struct Variation v : _variations) {
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


int ControlRom::_read_samples(std::ifstream &romFile)
{
  // ROM is split in 8 banks
  uint32_t *banks = _get_banks();

  // Samples are in bank 2 & X, each sample block using 16 bytes
  for (int32_t x = banks[2]; x < banks[6]; x += 16) {

    // Skip area between bank 1 and 4
    if (x == banks[3])
      x = banks[5];

    char data[16];
    romFile.seekg(x);
    struct Sample s;

    romFile.read(data, 16);
    s.volume = data[0];
    s.address = _native_endian_3bytes_uint32((uint8_t *) &data[1]);
    s.attackEnd = _native_endian_uint16((uint8_t *) &data[4]);
    s.sampleLen = _native_endian_uint16((uint8_t *) &data[6]);
    s.loopLen = _native_endian_uint16((uint8_t *) &data[8]);
    s.loopMode = data[10];
    s.rootKey = data[11];
    s.pitch = _native_endian_uint16((uint8_t *) &data[12]);
    s.fineVolume = _native_endian_uint16((uint8_t *) &data[14]);
    
    if (s.sampleLen) {                          // Ignore empty parts
      _samples.push_back(s);
      
      if (_verbose)
	std::cout << "  -> Sample " << std::setw(3) << _samples.size()
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
  
  return 0;
}           


int ControlRom::_read_drum_sets(std::ifstream &romFile)
{
  // ROM is split in 8 banks
  uint32_t *banks = _get_banks();

  // The drum set is in bank 7, a total of 14 drums in 1164 byte blocks 
  for (int32_t x = banks[7]; x < 0x03c028; x += 1164) {

    char data[128];
    romFile.seekg(x);
    struct DrumSet d;

    // First array is 16 bit instrument reference
    for (int i = 0; i < 128; i++) {
      romFile.read(data, 2);
      d.preset[i] = _native_endian_uint16((uint8_t *) &data[0]);
    }

    // Next 7 arrays are 8 bit data
    romFile.read((char *) d.volume, 128);
    romFile.read((char *) d.key, 128);
    romFile.read((char *) d.assignGroup, 128);
    romFile.read((char *) d.panpot, 128);
    romFile.read((char *) d.reverb, 128);
    romFile.read((char *) d.chorus, 128);
    romFile.read((char *) d.flags, 128);
    
    // Last 12 bytes are the drum name
    romFile.read(data, 12);
    d.name.assign(data, 12);
    d.name.erase(std::find_if(d.name.rbegin(), d.name.rend(),
			      std::bind1st(std::not_equal_to<char>(),
					   ' ')).base(), d.name.end());

    // Ignore undocumented drum sets and unused memory slots
    if ((d.name.rfind("AC.", 0) == 0) || data[0] < 0)
      continue;

    _drumSets.push_back(d);

    if (_verbose)
      std::cout << "  -> Drum " << _drumSets.size() << ": " << d.name
		<< std::endl;
  }

  return _drumSets.size();
}


std::list<int> ControlRom::get_drum_set_banks(enum SynthModel model)
{
  if (model == sm_SC55 || model == sm_SC55mkII)
    return std::list<int> ({0, 8, 16, 24, 25, 32, 40, 48, 56, 127});
  else if (model == sm_SC88)
    return std::list<int> ({0, 1, 8, 16, 24, 25, 26, 32, 40, 48, 49, 50,56,57});
  else  // model == sm_SC88Pro
    return std::list<int> ({0, 1, 2, 8, 9, 10, 11, 16, 24, 25, 26, 27, 28, 29,
	30, 31, 32, 40, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,58,59,60,61,62});
}


int ControlRom::dump_demo_songs(std::string path)
{
  int index = 1;
  std::cout << "EmuSC: Searching for MIDI songs in control ROM..." << std::endl;

  std::ifstream romFile(_romPath, std::ios::binary | std::ios::in);
  if (!romFile.is_open()) {
    std::cerr << "Unable to open control ROM: " << _romPath << std::endl;
    return -1;
  }

  // MIDI files are placed at different places in the ROM depending on model
  int romIndex;
  int romSize;
  if (synthModel == sm_SC55) {
    romIndex = 0;
    romSize = _get_banks()[0];
  } else if (synthModel == sm_SC55mkII) {
    romIndex = 0x03fff0;
    romFile.seekg(0, std::ios::end);
    romSize = romFile.tellg();
  } else {          // Unkown structures for SC-88, just read entire ROM
    romIndex = 0;
    romFile.seekg(0, std::ios::end);
    romSize = romFile.tellg();
  }

  std::vector<uint8_t> romData(romSize - romIndex);
  romFile.seekg(romIndex);
  romFile.read((char*) &romData[0], romSize);
  romFile.close();

  for (uint32_t i = 0; i < romData.size() - 6; i++) {
    if (romData[i + 0] == 0x4d &&
	romData[i + 1] == 0x54 &&
	romData[i + 2] == 0x68 &&
	romData[i + 3] == 0x64 &&
	romData[i + 4] == 0x00 &&
	romData[i + 5] == 0x00 &&
	romData[i + 6] == 0x00 &&
	romData[i + 7] == 0x06) {

      uint16_t numTracks = _native_endian_uint16(&romData[i+10]);
      uint32_t fileSize = 14;
      for (int n = 0; n < numTracks; n++) {
	if (romData[i + fileSize] == 0x4d &&
	    romData[i + fileSize + 1] == 0x54 &&
	    romData[i + fileSize + 2] == 0x72 &&
	    romData[i + fileSize + 3] == 0x6b) {
	  fileSize += _native_endian_4bytes_uint32(&romData[i + fileSize + 4]);
	  fileSize += 8;                                    // Add track header
	} else {
	  return -1;
	}
      }

      if (path.back() != '/')
	path.append("/");

      std::string fileName = "sc_song_" + std::to_string(index++) + ".mid";

      std::ofstream midiFile(path + fileName, std::ios::out | std::ios::binary);
      midiFile.write((char*) &romData[i], fileSize);
      if (midiFile.good())
	std::cout << " -> Found demo song at 0x" << std::hex << romIndex + i
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

  if (index == 1)
    std::cout << "EmuSC: Control ROM contained no MIDI files " << std::endl;

  return index;
}
