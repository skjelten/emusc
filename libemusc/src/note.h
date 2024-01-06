/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2024  Håkon Skjelten
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
#include "partial.h"
#include "settings.h"

#include <stdint.h>

#include <array>


namespace EmuSC {

class Note
{
public:
  Note(uint8_t key, uint8_t velocity, ControlRom &ctrlRom, PcmRom &pcmRom,
       Settings *settings, int8_t partId);
  ~Note();

  void stop(void);
  void stop(uint8_t key);
  void sustain(bool state);

  bool get_next_sample(float *sampleOut);
  int get_num_partials(void);

private:
  uint8_t _key;
  uint8_t _velocity;

  bool _sustain;
  bool _stopped;

  const double _7bScale;     // Constant: 1 / 127

  struct Partial *_partial[2];
};

}

#endif  // __NOTE_H__
