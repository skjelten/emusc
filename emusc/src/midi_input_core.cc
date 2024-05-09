/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2024  Håkon Skjelten
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

  const MIDIPacket *packet = const_cast<MIDIPacket *>(packetList->packet);
  for (int i = 0; i < packetList->numPackets; i++) {
    length = packet->length;
    data = packet->data;

    if (data[0] != 0xf0 && _sysExDataLength == 0) {
      // Regular MIDI message
      // A packet can contain multiple messages.
      // See https://stackoverflow.com/a/30657822/264970
      int idx = 0;
      while (idx < length) {
        int statusByte = data[idx] & 0xf0;
        switch(statusByte) {
          // 3-byte messages
          case 0x80: // Note Off
          case 0x90: // Note On
          case 0xa0: // Polyphonic Key Pressure (Aftertouch)
          case 0xb0: // Control Change
          case 0xe0: // Pitch Bend
            send_midi_event(data[idx], data[idx + 1], data[idx + 2]);
            idx += 3;
            break;
          // 2-byte messages
          case 0xc0: // Program Change
          case 0xd0: // Channel Pressure (Aftertouch)
            send_midi_event(data[idx], data[idx + 1], 0);
            idx += 2;
            break;
          // Unexpected status byte, skip rest of packet
          default:
            idx = length;
            break;
        }
      }
    } else if (data[0] == 0xf0) {
      // TODO: combine with switch-case above
      // SysEx MIDI message

      // New and complete SysEx message
      if (_sysExDataLength == 0 && (data[0] & 0xff) == 0xf0 && (data[length - 1] & 0xff) == 0xf7) {
        send_midi_event_sysex(const_cast<uint8_t *>(data), length);

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


void MidiInputCore::start(EmuSC::Synth *synth, QString deviceName)
{
  _synth = synth;
  
  ItemCount n = MIDIGetNumberOfSources();
  for (int i = 0; i < n; ++i) {
    MIDIEndpointRef source = MIDIGetSource(i);

    if (source) {
      QString devName = _get_device_name(source);
      if (devName == deviceName) {
        MIDIPortConnectSource(_inPort, source, nullptr);
        _sourceInUse = source;
        break;
      }
    }
  }
}


void MidiInputCore::stop()
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


QStringList MidiInputCore::get_available_devices()
{
  QStringList deviceList;
  ItemCount n = MIDIGetNumberOfSources();

  for (int i = 0; i < n; ++i) {
    MIDIEndpointRef source = MIDIGetSource(i);

    if (source) {
      QString deviceName = _get_device_name(source);
      deviceList << deviceName;
    }
  }

  return deviceList;
}

QString MidiInputCore::_get_device_name(MIDIEndpointRef ep) {
  MIDIEntityRef entity;
  MIDIDeviceRef device;
  CFStringRef endpointName, deviceName, fullName;
  CFStringEncoding enc = CFStringGetSystemEncoding();

  MIDIEndpointGetEntity(ep, &entity);
  MIDIEntityGetDevice(entity, &device);
  MIDIObjectGetStringProperty(ep, kMIDIPropertyName, &endpointName);
  MIDIObjectGetStringProperty(device, kMIDIPropertyName, &deviceName);
  if (deviceName != nullptr && CFStringCompare(endpointName, deviceName, 0) != kCFCompareEqualTo) {
    fullName = CFStringCreateWithFormat(nullptr, nullptr, CFSTR("%@: %@"), deviceName, endpointName);
  } else {
    fullName = endpointName;
  }

  char buf[128];
  CFStringGetCString(fullName, buf, 128, enc);

  return {buf};
}

#endif  // __CORE_MIDI__
