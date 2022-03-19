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


#include "midi_input.h"
#include "synth.h"

#include <iostream>


MidiInput::MidiInput()
  : _quit(false)
{}


MidiInput::~MidiInput()
{}


void MidiInput::stop(void)
{
  _quit = true;
}


void MidiInput::send_raw_std_msg(Synth *synth, uint8_t status, char data1, char data2)
{
  MidiEvent midiEvent;

  if ((status >> 4) == 0x08) {                           // Note Off
    midiEvent.type = se_NoteOff;
    midiEvent.channel = status & 0x0f;
    midiEvent.data1 = data1;
    midiEvent.data2 = data2;
    midiEvent.status = 0xff;
    synth->midi_input(&midiEvent);

  } else if ((status >> 4) == 0x09) {                    // Note On
    midiEvent.type = se_NoteOn;
    midiEvent.channel = status & 0x0f;
    midiEvent.data1 = data1;
    midiEvent.data2 = data2;
    midiEvent.status = 0xff;
    synth->midi_input(&midiEvent);

  } else if ((status >> 4) == 0x0a) {                    // Key pressure
    midiEvent.type = se_KeyPressure;
    midiEvent.channel = status & 0x0f;
    midiEvent.data1 = data1;
    midiEvent.data2 = data2;
    midiEvent.status = 0xff;
    synth->midi_input(&midiEvent);

  } else if ((status >> 4) == 0x0b) {                    // Control change
   midiEvent.type = se_CtrlChange;
    midiEvent.channel = status & 0x0f;
    midiEvent.data1 = data1;
    midiEvent.data2 = data2;
    midiEvent.status = 0xff;
    synth->midi_input(&midiEvent);

  } else if ((status >> 4) == 0x0c) {                    // Program change
    midiEvent.type = se_PrgChange;
    midiEvent.channel = status & 0x0f;
    midiEvent.data1 = data1;
    midiEvent.data2 = data2;
    midiEvent.status = 0xff;
    synth->midi_input(&midiEvent);

  } else if ((status >> 4) == 0x0d) {                    // Channel pressure
    midiEvent.type = se_ChPressure;
    midiEvent.channel = status & 0x0f;
    midiEvent.data1 = data1;
    midiEvent.data2 = data2;
    midiEvent.status = 0xff;
    synth->midi_input(&midiEvent);

  } else if ((status >> 4) == 0x0e) {                    // Pitch bend
    midiEvent.type = se_PitchBend;
    midiEvent.channel = status & 0x0f;
    midiEvent.data = ((data2 << 7) | data1) - 0x2000;
    midiEvent.status = 0xff;
    synth->midi_input(&midiEvent);
  }

  if (0)
    std::cout << "EmuSC: Raw MIDI event -> S=0x" << std::hex << (int) status
	      << " D1=0x" << (int) data1
	      << " D2=0x" << (int) data2 << std::endl;
}
