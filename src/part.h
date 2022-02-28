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


#ifndef __PART_H__
#define __PART_H__


#include "control_rom.h"
#include "pcm_rom.h"
#include "note.h"

#include <list>
#include <vector>

#include <stdint.h>


class Part
{
private:
  const uint8_t _id;          // Part id: [0-15] on SC-55, [0-31] on SC-88
  
  uint16_t _instrument;       // [0-127] -> variation table
  int8_t _drumSet;            // [0-13] drumSet (SC-55)
  uint8_t _volume;            // [0-127] 100 is factory preset
  int8_t _pan;                // [-63-63] 
  uint8_t _reverb;            // [0-127]
  uint8_t _chours;            // [0-127]
  int8_t _keyShift;           // [-24-24]
  uint8_t _midiChannel;       // [0-15] MIDI channel

  uint8_t _mode;              // [0-2] 0=Norm, 1=Drum1, 2=Drum2
  uint8_t _bendRange;         // [0-24] Default 2
  uint8_t _modDepth;          // [0-127] Default 10
  uint8_t _keyRangeL;         // [c1-g9] Default  24 => C1
  uint8_t _keyRangeH;         // [c1-g9] Default 127 => G9
  uint8_t _velSensDepth;      // [0-127] Default 64
  uint8_t _velSensOffset;     // [0-127] Default 64
  uint8_t _partialReserve;    // [0-24] Default 2
  bool _polyMode;             // true = poly, false = mono

  int8_t _vibRate;            // [-50-50] Default 0
  int8_t _vibDepth;           // [-50-50] Default 0
  int8_t _vibDelay;           // [-50-50] Default 0
  int8_t _cutoffFreq;         // [-50-16] Default 0
  int8_t _resonance;          // [-50-50] Default 0
  int8_t _attackTime;         // [-50-50] Default 0
  int8_t _decayTime;          // [-50-50] Default 0
  int8_t _releaseTime;        // [-50-50] Default 0

  bool _mute;                 // Part muted
//    uint8_t lever;          // [0-127]

  float _pitchBend;
  
  enum Mode {
    mode_Norm  = 0,
    mode_Drum1,
    mode_Drum2
  };
  
  enum AdsrState {
    adsr_Attack,
    adsr_Hold,
    adsr_Decay,
    adsr_Sustain,
    adsr_Release,
    adsr_Done
  };

  std::list<int>_drumSetBanks;

  struct std::list<Note*> _notes;

  ControlRom &_ctrlRom;
  PcmRom &_pcmRom;

public:
  Part(uint8_t id, uint8_t mode, uint8_t type, ControlRom &cRom, PcmRom &pRom);
  ~Part();

  enum ControlMsg {
    cmsg_Unknown,
    cmsg_ModWheel,
    cmsg_Pan,
    cmsg_Volume
  };

  int get_next_sample(int32_t *sampleOut);

  inline bool mute() { return(_mute); }
  inline void set_mute(bool mute) { _mute = mute; }

  int add_note(uint8_t midiChannel, uint8_t key, uint8_t velocity);
  int delete_note(uint8_t midiChannel, uint8_t key);
  int clear_all_notes(void);

  int set_program(uint8_t midiChannel, uint8_t index, uint8_t bank);
  int set_control(enum ControlMsg m, uint8_t midiChannel, uint8_t value);
  int set_pitchBend(uint8_t midiChannel, int16_t pitchbend);

};


#endif  // __PART_H__
