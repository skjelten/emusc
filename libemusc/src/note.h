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
#include "wave_generator.h"

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

  float get_current_tvp(bool partial);
  float get_current_tvf(bool partial);
  float get_current_tva(bool partial);
  float get_current_lfo(int lfo);

private:
  uint8_t _key;

  bool _sustain;
  bool _stopped;

  const double _7bScale;     // Constant: 1 / 127

  WaveGenerator *_LFO1;

  struct Partial *_partial[2];

  Settings *_settings;
  int8_t _partId;
};

}

#endif  // __NOTE_H__
