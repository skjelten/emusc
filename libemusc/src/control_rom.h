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

// Control ROM decoding is based on the SC55_Soundfont generator written by
// Kitrinx and NewRisingSun [ https://github.com/Kitrinx/SC55_Soundfont ]


#ifndef __CONTROL_ROM_H__
#define __CONTROL_ROM_H__


#include <fstream>
#include <list>
#include <string>
#include <vector>

#include <stdint.h>


namespace EmuSC {

class ControlRom
{
public:
  ControlRom(std::string romPath);
  ~ControlRom();

  enum SynthModel {
    sm_SC55,
    sm_SC55mkII,
    sm_SC88,
    sm_SC88Pro,
    sm_SCC1
  };
  enum SynthModel synthModel;

  // Internal data structures extracted from the control ROM file

  struct Sample {         // 16 bytes
    uint8_t  volume;      // Volume attenuation 7F to 0
    uint32_t address;     // Offset on vsc, bank + scrambled address on SC55.
                          // Bits above 20 are wave bank.
    uint16_t attackEnd;   // boundry between attack and decay? Unconfirmed.
    uint16_t sampleLen;   // Sample Size
    uint16_t loopLen;     // Loop point, used as sample_len - loop_len - 1
    uint8_t  loopMode;    // 2 if not a looping sound, 1 forward then back,
                          // 0 forward only.
    uint8_t  rootKey;     // Base pitch of the sample
    uint16_t pitch;       // Fine pitch adjustment, 2048 to 0. Pos. incr. pitch.
    uint16_t fineVolume;  // Always 0x400 on VSC, appears to be 1000ths of a
                          // decibel. Positive is higher volume.
  };

  struct Partial {        // 48 bytes in total
    std::string name;
    uint8_t breaks[16];   // Note breakpoints corresponding to sample addresses
    uint16_t samples[16]; // Set of addresses to the sample table. 0 is default
  };                      // and above corresponds to breakpoints

  struct InstPartial {    // 92 bytes in total
    uint16_t partialIndex;// Partial table index, 0xFFFF for unused
    int8_t panpot;        // [-64, 64]. Default 0x40 (0-127)
    int8_t coarsePitch;   // Default 0x40
    int8_t finePitch;     // Default 0x40
    int8_t randPitch;
    int8_t volume;        // Volume attenuation (7f - 0)
    int8_t PitchKeyFlw;
    int8_t TvpLfoDepth;
    int8_t pitchMult;
    int8_t pitchLvlP0;
    int8_t pitchLvlP1;
    int8_t pitchLvlP2;
    int8_t pitchLvlP3;
    int8_t pitchLvlP4;
    int8_t pitchDurP1;
    int8_t pitchDurP2;
    int8_t pitchDurP3;
    int8_t pitchDurP4;
    int8_t pitchDurRel;
    int8_t TVFBaseFlt;
    int8_t TVFResonance;
    int8_t LowVelClear;

    int8_t TVFLvlInit;
    int8_t TVFLvlP1;
    int8_t TVFLvlP2;
    int8_t TVFLvlP3;
    int8_t TVFLvlP4;
    int8_t TVFDurP1;
    int8_t TVFDurP2;
    int8_t TVFDurP3;
    int8_t TVFDurP4;
    int8_t TVFDurRel;

    int8_t TVFLFODepth;    // TVF LFO Depth
    int8_t TVALFODepth;    // TVA LFO Depth

    uint8_t TVAVolP1;      // TVA level phase 1 (Attack)     Default 0x7f
    uint8_t TVAVolP2;      // TVA level phase 2 (Hold)       Default 0x7f
    uint8_t TVAVolP3;      // TVA level phase 3 (Decay)      Default 0x7f
    uint8_t TVAVolP4;      // TVA level phase 4 (Sustain)    Default 0x7f
    uint8_t TVALenP1;      // TVA duration phase 1 (Attack)  Default 0x80
    uint8_t TVALenP2;      // TVA duration phase 2 (Hold)    Default 0x80
    uint8_t TVALenP3;      // TVA duration phase 3 (Decay)   Default 0
    uint8_t TVALenP4;      // TVA duration phase 4 (Sustain) Default 0
    uint8_t TVALenP5;      // TVA duration phase 5 (Release) Default 0x09
    
  };

  struct Instrument {     // 204 bytes in total
    std::string name;
    struct InstPartial partials[2];
  };                      // Contains 20 unused bytes (header, unknown purpose)

  struct DrumSet {        //1164 bytes
    uint16_t preset[128];
    uint8_t volume[128];
    uint8_t key[128];
    uint8_t assignGroup[128];// AKA exclusive class
    uint8_t panpot[128];
    uint8_t reverb[128];
    uint8_t chorus[128];
    uint8_t flags[128];   // 0x10 -> accept note on,  0x01 -> accept note off
    std::string name;     // 12 chars
  };

  struct Variation {
    uint16_t variation[128];  // All models have 128 x 128 variation table
  };

  std::list<int> get_drum_set_banks(enum SynthModel mode);
  int dump_demo_songs(std::string path);
  std::vector<uint8_t> get_intro_anim(void);

  std::string get_info_model(void) { return _model; }
  std::string get_info_version(void) { return _version; }
  std::string get_info_date(void) { return _date; }

  std::vector<std::vector<std::string>> get_instruments_list(void);
  std::vector<std::vector<std::string>> get_partials_list(void);
  std::vector<std::vector<std::string>> get_samples_list(void);
  std::vector<std::vector<std::string>> get_variations_list(void);
  std::vector<std::string> get_drum_sets_list(void);
  
  inline struct Instrument& instrument(int i) { return _instruments[i]; }
  inline struct Partial& partial(int p) { return _partials[p]; }
  inline struct Sample& sample(int s) { return _samples[s]; }
  inline struct Variation& variation(int v) { return _variations[v]; }
  inline struct DrumSet& drumSet(int ds) { return _drumSets[ds]; }

  inline int numSampleSets(void) { return _samples.size(); }
  inline int numInstruments(void) { return _instruments.size(); }

private:
  std::string _romPath;

  std::string _model;
  std::string _version;
  std::string _date;

  uint32_t _banksSC55[8] = { 0x10000, 0x1BD00, 0x1DEC0, 0x20000,
                             0x2BD00, 0x2DEC0, 0x30000, 0x38080 };

  // Only a placeholder, SC-88 layout is currently unkown
  uint32_t _banksSC88[8] = { 0x10000, 0x1BD00, 0x1DEC0, 0x20000,
                             0x2BD00, 0x2DEC0, 0x30000, 0x38080 };

  int _identify_model(std::ifstream &romFile);
  uint32_t *_get_banks(void);

  // To be replaced with std::endian::native from C++20
  inline bool _le_native(void) { uint16_t n = 1; return (*(uint8_t *) & n); } 

  uint16_t _native_endian_uint16(uint8_t *ptr);
  uint32_t _native_endian_3bytes_uint32(uint8_t *ptr);
  uint32_t _native_endian_4bytes_uint32(uint8_t *ptr);

  int _read_instruments(std::ifstream &romFile);
  int _read_partials(std::ifstream &romFile);
  int _read_variations(std::ifstream &romFile);
  int _read_samples(std::ifstream &romFile);
  int _read_drum_sets(std::ifstream &romFile);

  std::vector<Instrument> _instruments;
  std::vector<Partial> _partials;
  std::vector<Sample> _samples;
  std::vector<Variation> _variations;
  std::vector<DrumSet> _drumSets;

  ControlRom();

};

}

#endif  // __CONTROL_ROM_H__
