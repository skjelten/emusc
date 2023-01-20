/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2023  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libEmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libEmuSC. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __NOTE_H__
#define __NOTE_H__


#include "control_rom.h"
#include "pcm_rom.h"
#include "note_partial.h"
#include "settings.h"

#include <stdint.h>

#include <array>


namespace EmuSC {

class Note
{
private:
  uint8_t _key;
  uint8_t _velocity;

  const double _7bScale;     // Constant: 1 / 127

  struct NotePartial *_notePartial[2];
  
public:
  Note(uint8_t key, int8_t keyShift, uint8_t velocity, uint16_t instrument,
       int drum, ControlRom &ctrlRom, PcmRom &pcmRom, Settings *settings,
       int8_t partId);
  ~Note();

  bool stop(uint8_t key);
  bool get_next_sample(float *sampleOut, float pitchBend, float modWheel);
  int get_num_partials(void);

};

}

#endif  // __NOTE_H__
