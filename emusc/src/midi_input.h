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


#ifndef MIDI_INPUT_H
#define MIDI_INPUT_H

#include "emusc/synth.h"

#include <stdint.h>

#include <QString>


// MIDI input base class. All MIDI systems must implement a callback (or
// polling in separate thread) setup for receiving  MIDI event from the OS.
class MidiInput
{
protected:
  //  void run(void(EmuSC::Synth::*midi_input)(EmuSC::Synth::MidiEvent))
  EmuSC::Synth *_synth;

public:
  MidiInput();
  virtual ~MidiInput() = 0;

  virtual void start(EmuSC::Synth *synth, QString device);
  virtual void stop(void);
  
  void send_midi_event(uint8_t status, uint8_t data1, uint8_t data2);
  void send_midi_event_sysex(uint8_t *data, uint16_t length);
//  virtual static QStringList get_available_devices(void);
};


#endif  // MIDI_INPUT_H
