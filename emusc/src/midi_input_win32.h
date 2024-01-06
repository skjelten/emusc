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


#ifdef __WIN32_MIDI__

#ifndef MIDI_INPUT_WIN32_H
#define MIDI_INPUT_WIN32_H


#include "midi_input.h"

#include <array>

#include <QString>
#include <QStringList>

#include <windows.h>
#include <mmsystem.h>


class MidiInputWin32: public MidiInput
{
private:
  HMIDIIN _handle;

  // SC-55 Owner's Manual page 73 states that SC-55 limits SysEx DT1 messages
  // to 256 bytes, while the SC-88 Owner's Manual on page 7-25 states that DT1
  // data > 128 bytes must be split i separate messages.
  // => 256 byte static buffer for part messages, and 1024 bytes total buffer
  MIDIHDR _header;
  char _data[256];            // Data buffer for MIDI messages

  std::array<uint8_t, 1024> _sysExData;
  uint16_t _sysExDataLength;

  void _midi_callback(HMIDIIN handle, UINT uMsg,
		      DWORD_PTR dwParam1, DWORD_PTR dwParam2);

public:
  MidiInputWin32();
  virtual ~MidiInputWin32();

  virtual void start(EmuSC::Synth *synth, QString device);
  virtual void stop(void);

  static void CALLBACK midi_callback(HMIDIIN handle, UINT uMsg, DWORD_PTR dwInstance,
				     DWORD_PTR dwParam1, DWORD_PTR dwParam2);

  static QStringList get_available_devices(void);
};


#endif  // MIDI_INPUT_WIN32_H

#endif  // __WIN32_MIDI__
