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


#include "midi_input_keyboard.h"
#include "ex.h"

#include <iostream>
#include <string>


MidiInputKeyboard::MidiInputKeyboard()
{
  std::cout << "EmuSC: Using keyboard as MIDI sequencer" << std::endl;
}


MidiInputKeyboard::~MidiInputKeyboard()
{
}


// Used to manually enter spesific MIDI messages for testing / debuging purposes
// Currently only accepts a subset of channel voice messages
// 1. char   = channel (0-f in ASCII)
// 2. char   = message type (v = channel voice message)
// 3-5. char = Key number / MIDI Data Byte 1 (0-256 in ASCII)
// 6-8. char = Velocity / MIDI Data Byte 2 (0-256 in ASCII)
// TODO: Make a more generic interface, eg. 3 ints -> status + data1 + data2
void MidiInputKeyboard::run(Synth *synth)
{ 
  std::cout << "Example message: 1vn064100" << std::endl;

//  MidiInputKeyboard *midiInputKeyboard = 0; //(MidiInputKeyboard *) userData;
  MidiInput::MidiEvent midiEvent;
  
  while(!_quit)
    {
      static std::string lastInput = "";
      std::string input;
      getline(std::cin, input);

      // Repeat last message with flipped note on/off
      if (input.empty() && (lastInput[2] == 'n' || lastInput[2] == 'f')) {
	input = lastInput;
	input[2] = input[2] == 'n' ? 'f' : 'n';
      }

      // Send MIDI message
      if (input[1] != 'v' || input.size() != 9) {
	std::cerr << "EmuSC: Ignored illegal keyboard input" << std::endl;
      } else {
	if (input[2] != 'n' || input[2] != 'f') {
	  try {
	    int channel = std::stoi(input.substr(0, 1), NULL, 16);
	    int db1 = stoi(input.substr(3, 3));
	    int db2 = stoi(input.substr(6, 3));
	    lastInput = input;

	    // Build midi struct
	    switch (input[2])
	      {
	      case 'n':
		midiEvent.type = se_NoteOn;
		midiEvent.channel = channel;
		midiEvent.data1 = db1;
		midiEvent.data2 = db2;
		break;
	      case 'f':
		midiEvent.type = se_NoteOff;		
		midiEvent.channel = channel;
		midiEvent.data1 = db1;
		midiEvent.data2 = db2;
		break;
	      }

	    synth->midi_input(&midiEvent);
	    
	  } catch(const std::exception& e) {
	    std::cerr << "EmuSC: Ignored illegal keyboard input" << std::endl;
	  }
	} else {
	  std::cerr << "EmuSC: Ignored illegal keyboard input" << std::endl;
	}
      }
    }
 
  return;
}


void MidiInputKeyboard::stop(void)
{
  MidiInput::stop();

  // TODO: Any way to un-block getline?
}
