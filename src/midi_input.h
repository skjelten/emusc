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


#ifndef __MIDI_INPUT_H__
#define __MIDI_INPUT_H__


#include <stdint.h>


class Synth;


class MidiInput
{
private:

protected:
  void send_raw_std_msg(Synth *synth, uint8_t status, char data1, char data2);

public:
  MidiInput();
  virtual ~MidiInput() = 0;

  // Continuously read MIDI input and send to Synth (new thread)
  virtual void run(Synth *synth) = 0;

  bool _quit;
  virtual void stop(void);

  enum SeqEventType {
    se_NoteOff,
    se_NoteOn,
    se_CtrlChange,
    se_PrgChange,
    se_KeyPressure,
    se_ChPressure,
    se_PitchBend,
    se_Sysex,
    se_Qframe,
    se_SongPos,
    se_SongSel,
    se_TuneRequest,
    se_Clock,
    se_Start,
    se_Continue,
    se_Stop,
    se_Sensing,
    se_Reset,
    se_Control14,
    se_NonRegParam,
    se_RegParam,
    se_Cancel             // Cancels all active keys
  };
  
  struct MidiEvent {
    enum SeqEventType type;
    uint8_t channel;          // MIDI channel for channel messages
    uint8_t status;           // Data in 1st MIDI message byte
    uint8_t data1;            // Data in 2nd MIDI message byte
    uint8_t data2;            // Data in 3rd MIDI message byte
    int16_t data;             // Used instead of data1/2 when they are LSB/MSB
    uint8_t *ptr;             // Used for SYSEX data (data1 = length)
  };

};


#endif  // __MIDI_INPUT_H__
