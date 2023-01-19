/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022  Håkon Skjelten
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


#ifndef __TVA_H__
#define __TVA_H__


#include "ahdsr.h"
#include "control_rom.h"
#include "settings.h"
#include "wavetable.h"

#include <stdint.h>

#include <array>


namespace EmuSC {


class TVA
{
public:
  TVA(ControlRom::InstPartial &instPartial, uint8_t key, Settings *settings,
      int8_t partId);
  ~TVA();

  double get_amplification();
  void note_off();

  bool finished(void);

private:
  uint32_t _sampleRate;

  Wavetable _LFO;
  float _LFODepth;

  AHDSR *_ahdsr;
  bool _finished;
  
  ControlRom::InstPartial *_instPartial;

  TVA();

  double _convert_volume(uint8_t volume);
  double _convert_time_to_sec(uint8_t time, uint8_t key = 0);
};

}

#endif  // __TVA_H__
