/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC. If not, see <http://www.gnu.org/licenses/>.
 */


#ifdef __CORE_MIDI__


#include "midi_input_core.h"

#include <algorithm>
#include <iostream>


MidiInputCore::MidiInputCore()
  : _sourceInUse(0),
    _sysExDataLength(0)
{
  // Create client
  OSStatus ret;
  ret = MIDIClientCreate(CFSTR("EmuSC"), NULL, NULL, &_client);
  if (ret != noErr)
    throw (QString("Error creating CORE MIDI client"));

  // Create port
  ret = MIDIInputPortCreate(_client, CFSTR("Input port"),
                            MidiInputCore::midi_callback, this, &_inPort);
  if (ret != noErr)
    throw (QString("Error creating CORE MIDI input port"));
}


MidiInputCore::~MidiInputCore()
{
  stop();
//  MIDIClientDispose(&_client);
}


void MidiInputCore::_midi_callback(const MIDIPacketList* packetList,
                                   void* srcConnRefCon)
{
  uint16_t length;
  const uint8_t *data;

  const MIDIPacket* packet = const_cast<MIDIPacket*>(packetList->packet);
  for (int i = 0; i < packetList->numPackets; i++) {
    length = packet->length;
    data = packet->data;

    // Regular MIDI message
    if (data[0] != 0xf0 && _sysExDataLength == 0) {
      send_midi_event(data[0], data[1], data[2]);

    // SysEx MIDI message
    } else if (data[0] == 0xf0) {

      // New and complete SysEx message
      if (_sysExDataLength == 0 && (data[0] & 0xff) == 0xf0 &&
	  (data[length - 1] & 0xff) == 0xf7) {
	send_midi_event_sysex(const_cast<uint8_t*>(data), length);

      // Starting or ongoing SysEx message divided in multiple parts
      } else {
	// SysEx too large
	if (_sysExDataLength + length > _sysExData.size()) {
	  _sysExDataLength = 0;
	  break;
	}

	std::copy(data, data + length, &_sysExData[_sysExDataLength]);
	_sysExDataLength += length;

	// SysEx message is complete
	if (_sysExData[_sysExDataLength - 1] == 0xf7) {
	  send_midi_event_sysex(_sysExData.data(), _sysExDataLength);
	  _sysExDataLength = 0;
	}
      }
    }

    packet = MIDIPacketNext(packet);
  }
}


void MidiInputCore::start(EmuSC::Synth *synth, QString device)
{
  _synth = synth;
  
  int n = MIDIGetNumberOfSources();  
  for (int i = 0; i < n; ++i) {
    MIDIEndpointRef source = MIDIGetSource(i);

    if (source) {
      CFStringRef name = NULL;
      MIDIObjectGetStringProperty(source, kMIDIPropertyName, &name);
      CFStringEncoding enc = CFStringGetSystemEncoding();
      if (!device.compare(CFStringGetCStringPtr(name, enc))) {
	MIDIPortConnectSource(_inPort, source, NULL);
	_sourceInUse = source;
	break;
      }
    }
  }
}


void MidiInputCore::stop(void)
{
  if (_sourceInUse) {
    MIDIPortDisconnectSource(_inPort, _sourceInUse);
    _sourceInUse = 0;
  }
}


void MidiInputCore::midi_callback(const MIDIPacketList* packetList,
				  void* midiInput,
				  void* srcConnRefCon)
{
  MidiInputCore *mic = (MidiInputCore *) midiInput;
  mic->_midi_callback(packetList, srcConnRefCon);
}


QStringList MidiInputCore::get_available_devices(void)
{
  QStringList deviceList;
  int n = MIDIGetNumberOfSources();

  for (int i = 0; i < n; ++i) {
    MIDIEndpointRef src = MIDIGetSource(i);

    if (src) {
      CFStringRef name = NULL;
      MIDIObjectGetStringProperty(src, kMIDIPropertyName, &name);
      CFStringEncoding enc = CFStringGetSystemEncoding();
      deviceList << CFStringGetCStringPtr(name, enc);
    }
  }

  return deviceList;
}


#endif  // __CORE_MIDI__
