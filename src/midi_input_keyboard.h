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


#ifndef __MIDI_INPUT_KEYBOARD_H__
#define __MIDI_INPUT_KEYBOARD_H__


#include "midi_input.h"
#include "synth.h"


class MidiInputKeyboard: public MidiInput
{
private:

public:
  MidiInputKeyboard();
  virtual ~MidiInputKeyboard();

  virtual void run(Synth *synth);
  virtual void stop(void);;
};


#endif  // __MIDI_INPUT_KEYBOARD_H__

