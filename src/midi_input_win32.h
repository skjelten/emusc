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


#ifdef __WIN32__

#ifndef __MIDI_INPUT_WIN32_H__
#define __MIDI_INPUT_WIN32_H__


#include "config.h"
#include "midi_input.h"
#include "synth.h"

#include <windows.h>
#include <mmsystem.h>


class MidiInputWin32: public MidiInput
{
private:
  Synth *_synth;

  HMIDIIN _handle;

  MIDIHDR _header;
  char _data[256];            // Data buffer for MIDI messages

  void _midi_callback(HMIDIIN handle, UINT uMsg,
		      DWORD_PTR dwParam1, DWORD_PTR dwParam2);

public:
  MidiInputWin32(Config *config, Synth *synth);
  virtual ~MidiInputWin32();

  virtual void run(Synth *synth);
  virtual void stop(void);

  static void CALLBACK midi_callback(HMIDIIN handle, UINT uMsg, DWORD_PTR dwInstance,
				     DWORD_PTR dwParam1, DWORD_PTR dwParam2);

};


#endif  // __MIDI_INPUT_WIN32_H__

#endif  // __WIN32__
