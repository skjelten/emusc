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


#include "midi_input.h"

#include <iostream>


MidiInput::MidiInput()
{}


MidiInput::~MidiInput()
{}


void MidiInput::start(EmuSC::Synth *synth, QString device)
{
  _synth = synth;
}


void MidiInput::stop(void)
{}


void MidiInput::send_midi_event(uint8_t status, uint8_t data1, uint8_t data2)
{
  if (0)
    std::cout << "EmuSC: Raw MIDI event -> S=0x" << std::hex << (int) status
	      << " D1=0x" << (int) data1
	      << " D2=0x" << (int) data2 << std::endl;

  _synth->midi_input(status, data1, data2);
  emit new_midi_message(false, 3);
}


void MidiInput::send_midi_event_sysex(uint8_t *data, uint16_t length)
{
  if (0)
    std::cout << "EmuSC: SysEx MIDI event [" << std::dec << length << " bytes]"
	      << std::endl;

  _synth->midi_input_sysex(data, length);
  emit new_midi_message(true, length);
}


bool MidiInput::connect_port(QString portName, bool state)
{
  return 0;
}


QStringList MidiInput::list_subscribers(void)
{
  QStringList list;
  return list;
}
