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

#ifndef MIDI_INPUT_CORE_H
#define MIDI_INPUT_CORE_H


#include "midi_input.h"

#include <QStringList>

#include <CoreMIDI/MIDIServices.h>


class MidiInputCore: public MidiInput
{
private:
  MIDIClientRef _client;
  MIDIPortRef _inPort;
  MIDIEndpointRef _sourceInUse;
  
  void _midi_callback(const MIDIPacketList* packetList, void *srcConnRefCon);

public:
  MidiInputCore();
  virtual ~MidiInputCore();

  virtual void start(EmuSC::Synth *synth, QString device);
  virtual void stop(void);

  static QStringList get_available_devices(void);

  static void midi_callback(const MIDIPacketList* packetList, void* midiInput,
			    void* srcConnRefCon);
};


#endif  // MIDI_INPUT_CORE_H

#endif  // __CORE_MIDI__
