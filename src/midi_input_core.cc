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


#ifdef __CORE__


#include "ex.h"
#include "midi_input_core.h"

#include <iostream>


MidiInputCore::MidiInputCore(Synth *synth)
  : _synth(synth)
{
  OSStatus ret;
  // create client and ports
  MIDIClientRef client;
  ret = MIDIClientCreate(CFSTR("EmuSC"), NULL, NULL, &client);
  if (ret != noErr)
    throw (Ex(-1, "Error creating CORE MIDI client"));
  
  MIDIPortRef inPort;
  ret = MIDIInputPortCreate(client, CFSTR("Input port"),
			    MidiInputCore::midi_callback, this, &inPort);
  if (ret != noErr)
    throw (Ex(-1, "Error creating CORE MIDI input port"));
    
  // Open connections from all sources. TODO: How to specify source?
  int n = MIDIGetNumberOfSources();
//  std::cout << n << " sources found" << std::endl;
  for (int i = 0; i < n; ++i) {
    MIDIEndpointRef src = MIDIGetSource(i);
    MIDIPortConnectSource(inPort, src, NULL);
  }



  /*
for(std::vector<unsigned int>::iterator it = sources.begin(); it != sources.end(); it++)
  {
    MIDIEndpointRef src = MIDIGetSource(*it);
    std::cout << "Using source " << *it << " (" << sourceName(src) << ", " << sourceModel(src) << ", " << sourceManufacturer(src) << ")" << std::endl;
    MIDIPortConnectSource(_midi_in, src, 0);
  }
  */

  std::cout << "EmuSC: Core MIDI input initialized" << std::endl;
}


MidiInputCore::~MidiInputCore()
{}


// If this generic interface is also used on windows => base class
void MidiInputCore::_midi_callback(const MIDIPacketList* packetList,
				   void* srcConnRefCon)
{
  MidiEvent midiEvent;
  uint16_t length;
  const uint8_t *data;

  const MIDIPacket* packet = const_cast<MIDIPacket*>(packetList->packet);
  for (int i = 0; i < packetList->numPackets; i++) {
    length = packet->length;
    data = packet->data;

    if ((data[0] >> 4) == 0x08) {                           // Note Off
      midiEvent.type = se_NoteOff;
      midiEvent.channel = data[0] & 0x0f;
      midiEvent.data1 = data[1];
      midiEvent.data2 = data[2];
      midiEvent.status = 0xff;
      _synth->midi_input(&midiEvent);

    } else if ((data[0] >> 4) == 0x09) {                    // Note On
      midiEvent.type = se_NoteOn;
      midiEvent.channel = data[0] & 0x0f;
      midiEvent.data1 = data[1];
      midiEvent.data2 = data[2];
      midiEvent.status = 0xff;
      _synth->midi_input(&midiEvent);

    } else if ((data[0] >> 4) == 0x0a) {                    // Key pressure
      midiEvent.type = se_KeyPressure;
      midiEvent.channel = data[0] & 0x0f;
      midiEvent.data1 = data[1];
      midiEvent.data2 = data[2];
      midiEvent.status = 0xff;
      _synth->midi_input(&midiEvent);

    } else if ((data[0] >> 4) == 0x0b) {                    // Control change
      midiEvent.type = se_CtrlChange;
      midiEvent.channel = data[0] & 0x0f;
      midiEvent.data1 = data[1];
      midiEvent.data2 = data[2];
      midiEvent.status = 0xff;
      _synth->midi_input(&midiEvent);

    } else if ((data[0] >> 4) == 0x0c) {                    // Program change
      midiEvent.type = se_PrgChange;
      midiEvent.channel = data[0] & 0x0f;
      midiEvent.data1 = data[1];
      midiEvent.data2 = data[2];
      midiEvent.status = 0xff;
      _synth->midi_input(&midiEvent);

    } else if ((data[0] >> 4) == 0x0d) {                    // Channel pressure
      midiEvent.type = se_ChPressure;
      midiEvent.channel = data[0] & 0x0f;
      midiEvent.data1 = data[1];
      midiEvent.data2 = data[2];
      midiEvent.status = 0xff;
      _synth->midi_input(&midiEvent);

    } else if ((data[0] >> 4) == 0x0e) {                    // Pitch bend
      midiEvent.type = se_PitchBend;
      midiEvent.channel = data[0] & 0x0f;
      midiEvent.data1 = data[1];
      midiEvent.data2 = data[2];
      midiEvent.status = 0xff;
      _synth->midi_input(&midiEvent);
    }

    if (0)
      std::cout << "D0=0x" << std::hex << (int) data[0]
		<< " D1=0x" << (int) data[1]
		<< " D2=0x" << (int) data[2] << std::endl;
    packet = MIDIPacketNext(packet);
  }
}




void MidiInputCore::stop(void)
{}
  
void MidiInputCore::run(Synth *synth)
{}
/*
void MidiInput::receivePacket(const MIDIPacket* packet)
{
	std::vector<unsigned char> data;
	for(unsigned int i = 0; i < packet->length; i++)
		data.push_back(static_cast<unsigned char>(packet->data[i]));
	std::copy(data.begin(), data.end(), std::ostream_iterator<int>(std::cout, " "));
	std::cout << std::endl;
}

}
*/
void MidiInputCore::midi_callback(const MIDIPacketList* packetList,
				  void* midiInput,
				  void* srcConnRefCon)
{
  MidiInputCore *mic = (MidiInputCore *) midiInput;
  mic->_midi_callback(packetList, srcConnRefCon);
}


#endif  // __CORE__
