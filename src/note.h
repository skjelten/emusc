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


#ifndef __NOTE_H__
#define __NOTE_H__


#include "control_rom.h"
#include "pcm_rom.h"
#include "note_partial.h"

#include <stdint.h>


class Note
{
private:
  uint8_t _key;
  uint8_t _velocity;
  bool _drum;

  struct NotePartial *_notePartial[2];
  
public:
  Note(uint8_t key, uint8_t velocity, uint16_t instrument, bool drum,
       ControlRom &ctrlRom, PcmRom &pcmRom);
  ~Note();

  bool stop(uint8_t key);
  bool get_next_sample(int32_t *sampleOut, float pitchBend);

};


#endif  // __NOTE_H__
