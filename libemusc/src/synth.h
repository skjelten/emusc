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


#ifndef __SYNTH_H__
#define __SYNTH_H__


#include "control_rom.h"
#include "pcm_rom.h"

#include <functional>
#include <mutex>
#include <string>
#include <vector>


/* Public API for libEmuSC
 *
 * This Synth class is responsible for receiving MIDI events from client,
 * process the event based on information in Control and PCM ROMS, and
 * finally responding to audio buffer requests.
 *
 * Synth class constructor depends on a valid Control Rom and PCM ROM.
 *
 * MIDI events is sent to the emulator via the midi_input() method using the
 * three bytes from raw MIDI events.
 * 
 * Audio samples are extracted by calling the get_next_sample() method. This
 * is typically done from a callback function triggered by the OS audio
 * driver when the audio buffer is running low.
 */


namespace EmuSC {

class Part;
  
class Synth
{
public:

  enum Mode {
    scm_GS,                   // Default GM / GS mode as is default on all hw
    scm_MT32,                 // MT32 mode found on most SC-55/88 models
    scm_55                    // Sound Canvas 55 mode (only available on SC-88)
  };

  Synth(ControlRom &cRom, PcmRom &pRom, Mode mode = scm_GS);
  ~Synth();

  void midi_input(uint8_t status, uint8_t data1, uint8_t data2);

  int get_next_sample(int16_t *sample);
  std::vector<float> get_parts_last_peak_sample(void);

  void set_audio_format(uint32_t sampleRate, uint8_t channels);

  // Mute all parts. Similar to push MUTE-button on real hardware
  void mute(void);

  // Unmute all parts. Similar to push MUTE-button on real hardware
  void unmute(void);

  // Mute 1-n parts
  void mute_parts(std::vector<uint8_t> parts);

  // Unute all parts
  void unmute_parts(std::vector<uint8_t> parts);

  // Set volume in percents [0-100]
  void volume(uint8_t volume);

  // Set pan
  void pan(uint8_t pan);

  // Returns libEmuSC version as a string
  static std::string version(void);

  // Get part information; needed for emulating the LCD display
  bool get_part_mute(uint8_t partId);
  uint16_t get_part_instrument(uint8_t partId);
  uint8_t get_part_level(uint8_t partId);
  int8_t get_part_pan(uint8_t partId);
  uint8_t get_part_reverb(uint8_t partId);
  uint8_t get_part_chorus(uint8_t partId);
  int8_t get_part_key_shift(uint8_t partId);
  uint8_t get_part_midi_channel(uint8_t partId);

  // Update part state; needed for adapting to button inputs
  void set_part_mute(uint8_t partId, bool mute);
  void set_part_instrument(uint8_t partId, uint16_t instrument);
  void set_part_level(uint8_t partId, uint8_t level);
  void set_part_pan(uint8_t partId, uint8_t pan);
  void set_part_reverb(uint8_t partId, uint8_t reverb);
  void set_part_chorus(uint8_t partId, uint8_t chorus);
  void set_part_key_shift(uint8_t partId, int8_t keyShift);
  void set_part_midi_channel(uint8_t partId, uint8_t midiChannel);

  void add_part_midi_mod_callback(std::function<void(const int)> callback);
  void clear_part_midi_mod_callback(void);

  /* End of public API. Below are internal data structures only */

  
private:
  enum Mode _mode;

  uint8_t _volume;            // [0-127] Default 127
  uint8_t _pan;               // [0-127] Default 64 (64 = center)
  uint8_t _reverb;            // [0-127] Default 64
  uint8_t _chours;            // [0-127] Default 64
  int8_t _keyShift;           // [-24-24] Default 0
  float _masterTune;          // [415,3-466,2] Default 440,0
  uint8_t _reverbType;        // [1-8] Default 5
  uint8_t _choursType;        // [1-8] Default 3

  uint8_t _bank;              // MIDI CC0 message (always ch 0)
  
  uint32_t _sampleRate;
  uint8_t _channels;

  std::mutex midiMutex;

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
  std::vector<std::function<void(const int)>> _partMidiModCallbacks;

  struct std::vector<ControlRom::Instrument> _instruments;
  struct std::vector<ControlRom::Partial> _partials;
  struct std::vector<ControlRom::Sample> _samples;
  struct std::vector<ControlRom::DrumSet> _drumSets;
  struct std::vector<ControlRom::Variation> _variations;

  ControlRom &_ctrlRom;

  // MIDI message types
  static const uint8_t midi_NoteOff     = 0x80;
  static const uint8_t midi_NoteOn      = 0x90;
  static const uint8_t midi_KeyPressure = 0xa0;
  static const uint8_t midi_CtrlChange  = 0xb0;
  static const uint8_t midi_PrgChange   = 0xc0;
  static const uint8_t midi_ChPressure  = 0xd0;
  static const uint8_t midi_PitchBend   = 0xe0;

// int _export_sample_24(std::vector<int32_t> &sampleSet, std::string filename);
  void _add_note(uint8_t midiChannel, uint8_t key, uint8_t velocity);

  Synth();
};

}

#endif  // __SYNTH_H__
