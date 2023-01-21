/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2023  Håkon Skjelten
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


#ifndef __PART_H__
#define __PART_H__


#include "control_rom.h"
#include "pcm_rom.h"
#include "note.h"
#include "settings.h"

#include <stdint.h>

#include <array>
#include <list>
#include <vector>


namespace EmuSC {

class Part
{
public:
  Part(uint8_t id, uint8_t mode, uint8_t type, Settings *settings,
       ControlRom &cRom, PcmRom &pRom);
  ~Part();

  enum ControlMsg {
    cmsg_Unknown,
    cmsg_ModWheel,
    cmsg_PortamentoTime,
    cmsg_Volume,
    cmsg_Pan,
    cmsg_Expression,
    cmsg_HoldPedal,
    cmsg_Portamento,
    cmsg_Reverb,
    cmsg_Chorus,
    cmsg_RPN_LSB,
    cmsg_RPN_MSB,
    cmsg_DataEntry_LSB,
    cmsg_DataEntry_MSB
  };

  int get_next_sample(float *sampleOut);
  float get_last_peak_sample(void);
  int get_num_partials(void);

  int add_note(uint8_t key, uint8_t velocity);
  int stop_note(uint8_t key);
  int clear_all_notes(void);

  void reset(void);

  int set_control(enum ControlMsg m, uint8_t value);
  void set_pitchBend(int16_t pitchbend);

  uint8_t id(void) { return _id; }

  bool mute() { return(_mute); }
  void set_mute(bool mute) { _mute = mute; }

  int set_program(uint8_t index, uint8_t bank);

  uint8_t midi_channel(void) { return _settings->get_param(PatchParam::RxChannel, _id); }

private:
  const uint8_t _id;          // Part id: [0-15] on SC-55, [0-31] on SC-88

  Settings *_settings;

  uint16_t _instrument;       // [0-127] -> variation table
  int8_t _drumSet;            // [0-13] drumSet (SC-55)

//  uint8_t _bendRange;         // [0-24] Number of semitones Default 2
//  uint8_t _modDepth;          // [0-127] Default 10
  uint8_t _partialReserve;    // [0-24] Default 2

  bool _mute;                 // Part muted
//  uint8_t _modulation;        // [0-127] MOD Wheel (CM 1) Default 0
//  uint8_t _expression;        // [0-127] temporary volume modifier (CM 11)
 // bool _portamento;           // Portamento pitch slide [on / off] Default off
 // bool _holdPedal;            // Hold all notes [on / off] Default off
//  uint8_t _portamentoTime;    // [0-127] Pitch slide, 0 is slowest

  std::vector<uint8_t> _holdPedalKeys;
  //    uint8_t lever;          // [0-127]

//  uint16_t _masterFineTuning; // [0 - 0x4000] Default 0x2000
//  uint8_t _masterCoarseTuning;// [0 - 0x7f] Default 0x40
//  float _pitchBend;
//  float _modWheel;

  const double _7bScale;      // Constant: 1 / 127

  float _lastPeakSample;

  // All SC-55/88 supports four official RPN messages, but LSB is ignored on
  // pitch bend sensitivity and master coarse tuning. See SC-55 OM page 75.
  enum RPN {
    rpn_PitchBendSensitivity = 0x00,
    rpn_MasterFineTuning     = 0x01,
    rpn_MasterCoarseTuning   = 0x02,
    rpn_None                 = 0x7f
  };
  enum RPN _rpnMSB;
  enum RPN _rpnLSB;

  enum Mode {
    mode_Norm  = 0,
    mode_Drum1 = 1,
    mode_Drum2 = 2
  };

  struct std::list<Note*> _notes;

  ControlRom &_ctrlRom;
  PcmRom &_pcmRom;

  inline bool _le_native(void) { uint16_t n = 1; return (*(uint8_t *) & n); } 
  uint16_t _native_endian_uint16(uint8_t *ptr);

};

}

#endif  // __PART_H__
