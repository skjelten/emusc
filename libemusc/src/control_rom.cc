/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2024  Håkon Skjelten
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

// The SC-55 linueup controls the audio processing with instructions from:
//  * The internal 32k EPROM on the H8/532 main CPU (CPUROM)
//  * External 256kB (SC-55) or 512kB (SC-55mkII) EPROM (PROGROM)

// These two ROMs are very tightly connected and extends each other. They must
// therefore always be of the same ROM set / version.

// This class reads both these ROM files and combines their data for a complete
// set of control data to modify the audio stored in the PCM ROMs.

// The external EPROM (PROGROM) is encrypted. Decoding is based on the
// SC55_Soundfont generator written by Kitrinx and NewRisingSun.
// For more information, see [ https://github.com/Kitrinx/SC55_Soundfont ]


#include "control_rom.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>


namespace EmuSC {


const std::vector<uint32_t> ControlRom::_banksSC55 =
  { 0x10000, 0x1BD00, 0x1DEC0, 0x20000, 0x2BD00, 0x2DEC0, 0x30000, 0x38000 };

// Only a placeholder, SC-88 layout is currently unkown
const std::vector<uint32_t> ControlRom::_banksSC88 =
  { 0x10000, 0x1BD00, 0x1DEC0, 0x20000, 0x2BD00, 0x2DEC0, 0x30000, 0x38000 };

ControlRom::ControlRom(std::string romPath, std::string cpuRomPath)
  : _romPath(romPath)
{
  // External EPROM containing control data
  std::ifstream romFile(romPath, std::ios::binary | std::ios::in);
  if (!romFile.is_open())
    throw(std::string("Unable to open control ROM|: ") + romPath);

  if (_identify_model(romFile))
    throw(std::string("Unknown control ROM file!"));

  // Temporarily block SC-88 ROMs since we don't know how to read them yet
  if (_model == "SC-88")
    throw(std::string("SC-88 ROM files are not supported yet!"));

  // Read internal data structures from ROM file
  _read_instruments(romFile);
  _read_partials(romFile);
  _read_samples(romFile);
  _read_variations(romFile);
  _read_drum_sets(romFile);
  _read_lookup_tables_progrom(romFile);

  romFile.close();

  // CPU EPROM
  romFile.open(cpuRomPath, std::ios::binary | std::ios::in);
  if (!romFile.is_open())
    throw(std::string("Unable to open CPU ROM: ") + romPath);

  // Verify size (always 32kB for all SC-55 variants)
  std::streampos fileSize = romFile.tellg();
  romFile.seekg(0, std::ios::end);
  fileSize = romFile.tellg() - fileSize;
  if (fileSize != 32768)
    throw(std::string("Invalid CPU ROM (size != 32kB): ") + romPath);

  _read_lookup_tables_cpurom(romFile);

  romFile.close();

  if (0)
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
    _synthModel = sm_SC55;
    _synthGeneration = SynthGen::SC55;

    return 0;
  }

  // Search for SC-55mkII control ROM files
  romFile.seekg(0x3d148);
  romFile.read(data, 32);
  if (!strncmp(&data[0], "GS-28 VER=2.00  SC              ", 32)) {
    romFile.seekg(0xfff0);
    romFile.read(data, 10);
    _version.assign(data, 4);
    int year = (uint8_t) data[7];
    int month = (uint8_t) data[8];
    int day = (uint8_t) data[9];
    std::stringstream ss;
    ss << "19" << std::hex << year << "-" << month << "-" << day;
    _date.assign(ss.str());
    _model.assign("SC-55mkII");
    _synthModel = sm_SC55mkII;
    _synthGeneration = SynthGen::SC55mk2;

    return 0;
    
  } else if (!strncmp(&data[0], "GS-28 VER=2.00  LCGS-3 module   ", 32)) {
    _version.assign("?");
    _date.assign("?");
    _model.assign("SCB-55 (SC-55mkII)");
    _synthModel = sm_SC55mkII;
    _synthGeneration = SynthGen::SC55mk2;

    return 0;
  }

  // Search for SCC-1 control ROM files
  romFile.seekg(0x3D155);
  romFile.read(data, 29);
  if (!strncmp(data, "VER", 3)) {
    _version.assign(&data[3], 4);
    _date.assign(&data[24], 5);
    _model.assign("SCC-1");
    _synthModel = sm_SCC1;
    _synthGeneration = SynthGen::SC55;
  }

  // Search for SC-88 control ROM files
  romFile.seekg(0x7fc0);
  romFile.read(data, 24);
  if (!strncmp(&data[0], "GS-64 VER=3.00  SC-88   ", 24)) {
    _version.assign("?");
    _date.assign("?");
    _model.assign("SC-88");
    _synthModel = sm_SC88;
    _synthGeneration = SynthGen::SC88;
  }

  if (_model.empty())        // No valid ROM file found    TODO: SC88 ??
    return -1;

  return 0;
}


const std::vector<uint32_t> &ControlRom::_banks(void)
{
  switch(_synthModel)
    {
    case sm_SC55:
    case sm_SCC1:
    case sm_SC55mkII:
      return _banksSC55;

    case sm_SC88:                       // No work has been done here yet
      return _banksSC88;
    }

  throw(std::string("No ROM banks defined for this model"));
}


// Note: instrument partials (instPartial) contains 90 unused bytes! ADSR?
int ControlRom::_read_instruments(std::ifstream &romFile)
{
  // ROM is split in 8 banks
  const std::vector<uint32_t> &banks = _banks();

  // Instruments are in bank 0 & 3, each instrument block using 216 bytes
  for (int32_t x = banks[0]; x < banks[4]; x += 216) {

    // Skip area between bank 0 and 3
    if (x == banks[1])
      x = banks[3];

    char data[88];
    romFile.seekg(x);
    struct Instrument i;

    // First 12 bytes are the instrument name
    romFile.read(data, 32);

    // Skip empty slots in the ROM file that have no instrument name
    if (data[0] == '\0')
      continue;

    i.name.assign(data, 12);
    i.name.erase(i.name.find_last_not_of(' ') + 1);

    i.volume       = data[12];
    i.LFO1Waveform = data[14];
    i.LFO1Rate     = data[15];
    i.LFO1Delay    = data[16];
    i.LFO1Fade     = data[17];
    i.partialsUsed = data[18];
    i.pitchCurve   = data[19];

    // We have 2 partial parameters sets; starting in bank position 34 & 126
    for (int p = 0; p < 2; p++) {
      romFile.seekg(x + 34 + (p * 92));
      romFile.read(data, 2);
      i.partials[p].partialIndex = _native_endian_uint16((uint8_t *) data);

      romFile.read(data, 88);
      i.partials[p].LFO2Waveform = data[0];
      i.partials[p].LFO2Rate     = data[1];
      i.partials[p].LFO2Delay    = data[2];
      i.partials[p].LFO2Fade     = data[3];
      i.partials[p].TVFFlags     = data[4];
      i.partials[p].panpot       = data[5];
      i.partials[p].coarsePitch  = data[6];
      i.partials[p].finePitch    = data[7];
      i.partials[p].randPitch    = data[8];
      i.partials[p].pitchKeyFlw  = data[9];
      i.partials[p].TVPLFO1Depth = data[10];
      i.partials[p].TVPLFO2Depth = data[11];
      i.partials[p].pitchEnvDepth= data[12];
      i.partials[p].pitchEnvL0   = data[14];
      i.partials[p].pitchEnvL1   = data[15];
      i.partials[p].pitchEnvL2   = data[16];
      i.partials[p].pitchEnvL3   = data[17];
      i.partials[p].pitchEnvL5   = data[18];
      i.partials[p].pitchEnvT1   = data[19];
      i.partials[p].pitchEnvT2   = data[20];
      i.partials[p].pitchEnvT3   = data[21];
      i.partials[p].pitchEnvT4   = data[22];
      i.partials[p].pitchEnvT5   = data[23];
      i.partials[p].pitchETKeyF14= data[28];
      i.partials[p].pitchETKeyF5 = data[29];
      i.partials[p].pitchEnvVSens= data[30];
      i.partials[p].TVFCOFVelCur = data[32];
      i.partials[p].TVFBaseFlt   = data[33];
      i.partials[p].TVFResonance = data[34];
      i.partials[p].TVFType      = data[35];
      i.partials[p].TVFCFKeyFlwC = data[36];
      i.partials[p].TVFCFKeyFlw  = data[37];
      i.partials[p].TVFLFO1Depth = data[38];
      i.partials[p].TVFLFO2Depth = data[39];
      i.partials[p].TVFEnvDepth  = data[40];
      i.partials[p].TVFEnvL1     = data[41];
      i.partials[p].TVFEnvL2     = data[42];
      i.partials[p].TVFEnvL3     = data[43];
      i.partials[p].TVFEnvL4     = data[44];
      i.partials[p].TVFEnvL5     = data[45];
      i.partials[p].TVFEnvT1     = data[46];
      i.partials[p].TVFEnvT2     = data[47];
      i.partials[p].TVFEnvT3     = data[48];
      i.partials[p].TVFEnvT4     = data[49];
      i.partials[p].TVFEnvT5     = data[50];
      i.partials[p].TVFETKeyFP14 = data[53];
      i.partials[p].TVFETKeyFP5  = data[54];
      i.partials[p].TVFETKeyF14  = data[55];
      i.partials[p].TVFETKeyF5   = data[56];
      i.partials[p].TVFCOFVSens  = data[57];
      i.partials[p].TVFETVSens12 = data[58];
      i.partials[p].TVFETVSens35 = data[59];
      i.partials[p].TVALvlVelCur = data[60];
      i.partials[p].volume       = data[65];
      i.partials[p].TVABiasPoint = data[66];
      i.partials[p].TVABiasLevel = data[67];
      i.partials[p].TVALFO1Depth = data[68];
      i.partials[p].TVALFO2Depth = data[69];
      i.partials[p].TVAEnvL1     = data[70];
      i.partials[p].TVAEnvL2     = data[71];
      i.partials[p].TVAEnvL3     = data[72];
      i.partials[p].TVAEnvL4     = data[73];
      i.partials[p].TVAEnvT1     = data[74];
      i.partials[p].TVAEnvT2     = data[75];
      i.partials[p].TVAEnvT3     = data[76];
      i.partials[p].TVAEnvT4     = data[77];
      i.partials[p].TVAEnvT5     = data[78];
      i.partials[p].TVAETKeyFP14 = data[81];
      i.partials[p].TVAETKeyFP5  = data[82];
      i.partials[p].TVAETKeyF14  = data[83];
      i.partials[p].TVAETKeyF5   = data[84];
      i.partials[p].TVAETVSens12 = data[85];
      i.partials[p].TVAETVSens35 = data[86];
    }

    _instruments.push_back(i);

    if (0)
      std::cout << "  -> Instrument " << _instruments.size() << ": " << i.name
          << " partial0=" << (int) i.partials[0].partialIndex
          << " partial1=" << (int) i.partials[1].partialIndex
          << std::endl;
    }

  return 0;
}


int ControlRom::_read_partials(std::ifstream &romFile)
{
  // ROM is split in 8 banks
  const std::vector<uint32_t> &banks = _banks();

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
    p.name.erase(p.name.find_last_not_of(' ') + 1);

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

      if (0)
	std::cout << "  -> Partial group " << _partials.size() <<  ": "
		  << p.name << std::endl;
    }
  }

  return 0;
}


int ControlRom::_read_variations(std::ifstream &romFile)
{
  // ROM is split in 8 banks
  const std::vector<uint32_t> &banks = _banks();

  // Variations are in bank 6, a table of 128 x 128 2 byte values
  for (int x = 0; x < 128; x++) {
    const size_t offset = banks[6] + x * 128 * sizeof(uint16_t);
    romFile.seekg(offset);

    for (int y = 0; y < 128; y++) {
      char data[2];
      romFile.read(data, 2);
      _variations[x][y] = _native_endian_uint16(reinterpret_cast<uint8_t*>(data));
    }
  }

  if (0) {
    int i = 0;
    for (auto v : _variations) {
      std::cout << "  -> Variations " << i++ << ": ";
      for (int y = 0; y < 128; y++)
	if (v[y] == 0xffff)
	  std::cout << "-,";
	else
	  std::cout << v[y] << ",";
      std::cout << '\b' << " " << std::endl;
    }
  }

  return 0;
}


int ControlRom::_read_samples(std::ifstream &romFile)
{
  // ROM is split in 8 banks
  const std::vector<uint32_t> &banks = _banks();

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
      
      if (0)
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
  const std::vector<uint32_t> &banks = _banks();

  // The drum sets are defined in bank 7, starting with a 128 byte lookup table
  int32_t x = banks[7];
  romFile.seekg(x);
  romFile.read(reinterpret_cast<char*>(_drumSetsLUT.data()), 128);

  // After the map array there are 14 drum set definitions in 1164 byte blocks 
  char data[128];
  for (x = banks[7] + 128; x < 0x03c028; x += 1164) {
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
    d.name.erase(d.name.find_last_not_of(' ') + 1);

    // Ignore undocumented drum sets and unused memory slots
    if ((d.name.rfind("AC.", 0) == 0) || data[0] < 0)
      continue;

    _drumSets.push_back(d);

    if (0)
      std::cout << "  -> Drum " << _drumSets.size() << ": " << d.name
		<< std::endl;
  }

  return _drumSets.size();
}


int ControlRom::_read_lookup_tables_progrom(std::ifstream &romFile)
{
  int numVCurves = 10;
  const struct _ProgMemoryMapLUT *PROGmmLUT;
  switch(_synthModel)
    {
    case SynthModel::sm_SC55:
      PROGmmLUT = &SC55_1_21_Prog_LUT;
      numVCurves = 10;
      break;
    case SynthModel::sm_SC55mkII:
      PROGmmLUT = &SC55mkII_1_01_Prog_LUT;
      numVCurves = 12;
      break;
    default:
      std::cerr << "libEmuSC: Unsupported ROM file!" << std::endl;
      exit(0);
    }

  lookupTables.VelocityCurves.resize(128 * numVCurves);
  romFile.seekg(PROGmmLUT->VelocityCurves);
  romFile.read(reinterpret_cast<char*> (lookupTables.VelocityCurves.data()),
               lookupTables.VelocityCurves.size());

  _read_lut_16bit(romFile, PROGmmLUT->KeyMapperIndex,
                  lookupTables.KeyMapperIndex);

  int kmSize = 128 +
    lookupTables.KeyMapperIndex.back() - lookupTables.KeyMapperIndex.front();
  lookupTables.KeyMapper.resize(kmSize);

  romFile.seekg(PROGmmLUT->KeyMapper);
  romFile.read(reinterpret_cast<char*> (lookupTables.KeyMapper.data()), kmSize);
  lookupTables.KeyMapperOffset = PROGmmLUT->KeyMapper - 0x30000;

  return 0;
}


int ControlRom::_read_lookup_tables_cpurom(std::ifstream &romFile)
{
  const struct _CPUMemoryMapLUT *CPUmmLUT;
  switch(_synthModel)
    {
    case SynthModel::sm_SC55:
      CPUmmLUT = &SC55_1_21_CPU_LUT;
      break;
    case SynthModel::sm_SC55mkII:
      CPUmmLUT = &SC55mkII_1_01_CPU_LUT;
      break;
    default:
      std::cerr << "libEmuSC: Unsupported ROM file!" << std::endl;
      exit(0);
    }

  // 8-bit values
  romFile.seekg(CPUmmLUT->TimeKeyFollowDiv);
  romFile.read(reinterpret_cast<char*> (&lookupTables.TimeKeyFollowDiv), 21);
  romFile.seekg(CPUmmLUT->TVFResonanceFreq);
  romFile.read(reinterpret_cast<char*> (&lookupTables.TVFResonanceFreq), 256);
  romFile.seekg(CPUmmLUT->TVFResonance);
  romFile.read(reinterpret_cast<char*> (&lookupTables.TVFResonance), 128);
  romFile.seekg(CPUmmLUT->TVFEnvScale);
  romFile.read(reinterpret_cast<char*> (&lookupTables.TVFEnvScale), 64);
  romFile.seekg(CPUmmLUT->LFOSine);
  romFile.read(reinterpret_cast<char*> (&lookupTables.LFOSine), 130);
  romFile.seekg(CPUmmLUT->TVABiasLevel);
  romFile.read(reinterpret_cast<char*> (&lookupTables.TVABiasLevel), 130);
  romFile.seekg(CPUmmLUT->TVAPanpot);
  romFile.read(reinterpret_cast<char*> (&lookupTables.TVAPanpot), 128);
  romFile.seekg(CPUmmLUT->TVALevelIndex);
  romFile.read(reinterpret_cast<char*> (&lookupTables.TVALevelIndex), 128);
  romFile.seekg(CPUmmLUT->TVALevel);
  romFile.read(reinterpret_cast<char*> (&lookupTables.TVALevel), 256);

  // 16-bit values
  _read_lut_16bit(romFile, CPUmmLUT->TimeKeyFollow, lookupTables.TimeKeyFollow);
  _read_lut_16bit(romFile, CPUmmLUT->TVFEnvDepth, lookupTables.TVFEnvDepth);
  _read_lut_16bit(romFile, CPUmmLUT->TVFCutoffFreq, lookupTables.TVFCutoffFreq);
  _read_lut_16bit(romFile, CPUmmLUT->EnvelopeTime, lookupTables.envelopeTime);
  _read_lut_16bit(romFile, CPUmmLUT->LFORate, lookupTables.LFORate);
  _read_lut_16bit(romFile, CPUmmLUT->LFODelayTime, lookupTables.LFODelayTime);
  _read_lut_16bit(romFile, CPUmmLUT->LFOTVFDepth, lookupTables.LFOTVFDepth);
  _read_lut_16bit(romFile, CPUmmLUT->LFOTVPDepth, lookupTables.LFOTVPDepth);
  _read_lut_16bit(romFile, CPUmmLUT->PitchEnvDepth, lookupTables.PitchEnvDepth);
  _read_lut_16bit(romFile, CPUmmLUT->TVAEnvExpChange, lookupTables.TVAEnvExpChange);
  _read_lut_16bit(romFile, CPUmmLUT->TVFCutoffVSens, lookupTables.TVFCutoffVSens);
  _read_lut_16bit(romFile, CPUmmLUT->TVFCutoffFreqKF,
		  lookupTables.TVFCutoffFreqKF);

  return 0;
}


int ControlRom::_read_lut_16bit(std::ifstream &ifs, int pos,
                                std::array<int, 11> &lut)
{
  ifs.seekg(pos);

  for (int i = 0; i < 11; i ++) {
    uint16_t value;
    if (!ifs.read(reinterpret_cast<char*>(&value), sizeof(value))) {
      std::cerr << "libEmuSC: Error reading LUT from ROM" << std::endl;
      return i;
    }

    lut[i] = static_cast<int>(_native_endian_uint16((uint8_t *) &value));
  }

  return 11;
}


int ControlRom::_read_lut_16bit(std::ifstream &ifs, int pos,
                                std::array<int, 21> &lut)
{
  ifs.seekg(pos);

  for (int i = 0; i < 21; i ++) {
    uint16_t value;
    if (!ifs.read(reinterpret_cast<char*>(&value), sizeof(value))) {
      std::cerr << "libEmuSC: Error reading LUT from ROM" << std::endl;
      return i;
    }

    lut[i] = static_cast<int>(_native_endian_uint16((uint8_t *) &value));
  }

  return 21;
}


int ControlRom::_read_lut_16bit(std::ifstream &ifs, int pos,
                                std::array<int, 128> &lut)
{
  ifs.seekg(pos);

  for (int i = 0; i < 128; i ++) {
    uint16_t value;
    if (!ifs.read(reinterpret_cast<char*>(&value), sizeof(value))) {
      std::cerr << "libEmuSC: Error reading LUT from ROM" << std::endl;
      return i;
    }

    lut[i] = static_cast<int>(_native_endian_uint16((uint8_t *) &value));
  }

  return 128;
}


int ControlRom::_read_lut_16bit(std::ifstream &ifs, int pos,
                                std::array<int, 130> &lut)
{
  ifs.seekg(pos);

  for (int i = 0; i < 130; i ++) {
    uint16_t value;
    if (!ifs.read(reinterpret_cast<char*>(&value), sizeof(value))) {
      std::cerr << "libEmuSC: Error reading LUT from ROM" << std::endl;
      return i;
    }

    lut[i] = static_cast<int>(_native_endian_uint16((uint8_t *) &value));
  }

  return 130;
}


int ControlRom::_read_lut_16bit(std::ifstream &ifs, int pos,
                                std::array<int, 136> &lut)
{
  ifs.seekg(pos);

  for (int i = 0; i < 136; i ++) {
    uint16_t value;
    if (!ifs.read(reinterpret_cast<char*>(&value), sizeof(value))) {
      std::cerr << "libEmuSC: Error reading LUT from ROM" << std::endl;
      return i;
    }

    lut[i] = static_cast<int>(_native_endian_uint16((uint8_t *) &value));
  }

  return 136;
}


int ControlRom::_read_lut_16bit(std::ifstream &ifs, int pos,
                                std::array<int, 256> &lut)
{
  ifs.seekg(pos);

  for (int i = 0; i < 256; i ++) {
    uint16_t value;
    if (!ifs.read(reinterpret_cast<char*>(&value), sizeof(value))) {
      std::cerr << "libEmuSC: Error reading LUT from ROM" << std::endl;
      return i;
    }

    lut[i] = static_cast<int>(_native_endian_uint16((uint8_t *) &value));
  }

  return 256;
}


const uint8_t ControlRom::max_polyphony(void)
{
  switch (_synthModel)
    {
    case sm_SC55:
    case sm_SCC1:
      return _maxPolyphonySC55;

    case sm_SC55mkII:
      return _maxPolyphonySC55mkII;

    case sm_SC88:
    case sm_SC88Pro:
      return _maxPolyphonySC88;
    }

  throw(std::string("No maximum polyphony defined for this model"));
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
  if (_synthModel == sm_SC55) {
    romIndex = 0;
    romSize = _banks()[0];
  } else if (_synthModel == sm_SC55mkII) {
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

  return index - 1;
}


std::vector<std::vector<std::string>> ControlRom::get_instruments_list(void)
{
  std::vector<std::vector<std::string>> instListVector;

  // First row is header
  std::vector<std::string> headerVector;
  headerVector.push_back("Name");
  headerVector.push_back("Partial 0");
  headerVector.push_back("Partial 1");
  instListVector.push_back(headerVector);

  for (struct Instrument inst: _instruments) {
    std::vector<std::string> instVector;
    instVector.push_back(inst.name);
    instVector.push_back(std::to_string(inst.partials[0].partialIndex));
    instVector.push_back(std::to_string(inst.partials[1].partialIndex));
    instListVector.push_back(instVector);
  }

  return instListVector;
}


std::vector<std::vector<std::string>> ControlRom::get_partials_list(void)
{
  std::vector<std::vector<std::string>> partListVector;

  // First row is header
  std::vector<std::string> headerVector;
  headerVector.push_back("Name");
  for (int i = 0; i < 16; i++)
    headerVector.push_back("Break " + std::to_string(i));
  for (int i = 0; i < 16; i++)
    headerVector.push_back("Sample " + std::to_string(i));
  partListVector.push_back(headerVector);

  for (struct Partial partial: _partials) {
    std::vector<std::string> partVector;
    partVector.push_back(partial.name);
    for (int i = 0; i < 16; i++)
      partVector.push_back(std::to_string(partial.breaks[i]));
    for (int i = 0; i < 16; i++)
      partVector.push_back(std::to_string(partial.samples[i]));
    partListVector.push_back(partVector);
  }

  return partListVector;
}


std::vector<std::vector<std::string>> ControlRom::get_samples_list(void)
{
  std::vector<std::vector<std::string>> samplesListVector;

  // First row is header
  std::vector<std::string> headerVector;
  headerVector.push_back("Volume");
  headerVector.push_back("Attack End");
  headerVector.push_back("Sample Length");
  headerVector.push_back("Loop Length");
  headerVector.push_back("Loop Mode");
  headerVector.push_back("Root Key");
  headerVector.push_back("Pitch");
  headerVector.push_back("Fine Volume");
  samplesListVector.push_back(headerVector);
  
  for (struct Sample sample: _samples) {
    std::vector<std::string> sampleVector;
    sampleVector.push_back(std::to_string(sample.volume));
    sampleVector.push_back(std::to_string(sample.attackEnd));
    sampleVector.push_back(std::to_string(sample.sampleLen));
    sampleVector.push_back(std::to_string(sample.loopLen));
    sampleVector.push_back(std::to_string(sample.loopMode));
    sampleVector.push_back(std::to_string(sample.rootKey));
    sampleVector.push_back(std::to_string(sample.pitch));
    sampleVector.push_back(std::to_string(sample.fineVolume));

    samplesListVector.push_back(sampleVector);
  }

  return samplesListVector;
}


std::vector<std::vector<std::string>> ControlRom::get_variations_list(void)
{
  std::vector<std::vector<std::string>> varListVector;

  // First row is header
  std::vector<std::string> headerVector;
  for (int i = 0; i < 128; i++)
    headerVector.push_back(std::to_string(i));
  varListVector.push_back(headerVector);

  for (auto v : _variations) {
    std::vector<std::string> varVector;
    for (int i = 0; i < 128; i++)
      varVector.push_back(std::to_string(v[i]));
    varListVector.push_back(varVector);
  }

  return varListVector;
}


std::vector<std::string> ControlRom::get_drum_sets_list(void)
{
  std::vector<std::string> drumSetsVector;

  // First row is header
  drumSetsVector.push_back("Name");

  for (struct DrumSet drumSet: _drumSets) {
    drumSetsVector.push_back(drumSet.name);
  }

  return drumSetsVector;
}


bool ControlRom::intro_anim_available(void)
{
  // TODO: Use SHA256 and proper ROM list to identify ROMs with intro animations
  if (_synthModel == sm_SC55mkII)
    return true;

  return false;
}


std::vector<uint8_t> ControlRom::get_intro_anim(int animIndex)
{
  int romIndex;
  int length;

  if (_synthModel == sm_SC55mkII) {
    if (animIndex == 0)
      romIndex = 0x70000;               // SC-55mkII
    else if (animIndex == 1)
      romIndex = 0x71280;               // SC-155mkII
    else
      return std::vector<uint8_t> {};

    length = 0x1280;

  } else {
    return std::vector<uint8_t> {};
  }

  std::ifstream romFile(_romPath, std::ios::binary | std::ios::in);
  if (!romFile.is_open()) {
    std::cerr << "Unable to open control ROM: " << _romPath << std::endl;
  }

  std::vector<uint8_t> romData(length);
  romFile.seekg(romIndex);
  romFile.read((char*) &romData[0], length);
  romFile.close();

  return romData;
}

}
