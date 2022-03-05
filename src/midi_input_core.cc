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
#include <vector>


MidiInputCore::MidiInputCore(Synth *synth)
  : _synth(synth)
{
  OSStatus ret;
  // Create client and ports
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

  std::vector<const char *> names;
  for (int i = 0; i < n; ++i) {
    MIDIEndpointRef src = MIDIGetSource(i);

    if (src) {
      CFStringRef name = NULL;
      MIDIObjectGetStringProperty(src, kMIDIPropertyName, &name);
      CFStringEncoding enc = CFStringGetSystemEncoding();
      names.push_back(CFStringGetCStringPtr(name, enc));

      MIDIPortConnectSource(inPort, src, NULL);
    }
  }

  if (names.empty())
    throw(Ex(-1, "No valid MIDI source endpoints to connect to"));

  std::cout << "EmuSC: Core MIDI input initialized" << std::endl;
  for (auto n : names)
    std::cout << " -> " << n << std::endl;
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

    send_raw_std_msg(_synth, data[0], data[1], data[2]);

    packet = MIDIPacketNext(packet);
  }
}




void MidiInputCore::stop(void)
{}


void MidiInputCore::run(Synth *synth)
{}


void MidiInputCore::midi_callback(const MIDIPacketList* packetList,
				  void* midiInput,
				  void* srcConnRefCon)
{
  MidiInputCore *mic = (MidiInputCore *) midiInput;
  mic->_midi_callback(packetList, srcConnRefCon);
}


#endif  // __CORE__
