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


#ifndef __SYNTH_H__
#define __SYNTH_H__


#include "control_rom.h"
#include "part.h"
#include "pcm_rom.h"
#include "config.h"
#include "midi_input.h"

#include <mutex>
#include <string>
#include <vector>


class Synth
{
private:
  uint8_t _volume;            // [0-127] Default 127
  uint8_t _pan;               // [0-127] Default 64 (64 = center)
  uint8_t _reverb;            // [0-127] Default 64
  uint8_t _chours;            // [0-127] Default 64
  int8_t _keyShift;           // [-24-24] Default 0
  float _masterTune;          // [415,3-466,2] Default 440,0
  uint8_t _reverbType;        // [1-8] Default 5
  uint8_t _choursType;        // [1-8] Default 3

  uint8_t _bank;              // MIDI CC0 message (always ch 0)
  
  Config &_config;

  uint32_t _sampleRate;
  uint8_t _channels;

  std::mutex midiMutex;

  enum Mode {
    mode_GS   = 0,
    mode_MT32
  };
  enum Mode _mode;

  // System function settings
  struct System {
    bool rxInstChg;           // Off / On. Default On
    bool sysEx;               // Off / On. Default On
    bool rxGSReset;           // Off / On. Default On
    uint8_t display;          // [1-8] Default 1
    uint8_t peakHold;         // [0-3] = is Off. Default 1
    uint8_t lcdContrast;      // [1-16] Default 8
    bool backUp;              // Off / On. Default On
    bool rxRemote;            // Off / On. Default On
    uint8_t deviceID;         // [1-32] Default 17
  };
  struct System _system;

  struct std::vector<Part> _parts;

  struct std::vector<ControlRom::Instrument> _instruments;
  struct std::vector<ControlRom::Partial> _partials;
  struct std::vector<ControlRom::Sample> _samples;
  struct std::vector<ControlRom::DrumSet> _drumSets;
  struct std::vector<ControlRom::Variation> _variations;

  ControlRom &_ctrlRom;

// int _export_sample_24(std::vector<int32_t> &sampleSet, std::string filename);
  void _add_note(uint8_t midiChannel, uint8_t key, uint8_t velocity);

  Synth();
  
 public:
  Synth(Config &config, ControlRom &cRom, PcmRom &pRom);
  ~Synth();

  void midi_input(struct MidiInput::MidiEvent *midiEvent);
  int get_next_sample(int16_t *sample);
  void set_audio_format(uint32_t sampleRate, uint8_t channels);

};


#endif  // __SYNTH_H__
