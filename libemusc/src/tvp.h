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


#ifndef __TVP_H__
#define __TVP_H__


#include "ahdsr.h"
#include "control_rom.h"
#include "settings.h"
#include "wavetable.h"

#include <stdint.h>

#include <array>


namespace EmuSC {


class TVP
{
public:
  TVP(ControlRom::InstPartial &instPartial, Settings *settings, int8_t partId);
  ~TVP();

  double get_pitch(float modWheel);
  void note_off();

  inline bool finished(void) { if (_ahdsr) return _ahdsr->finished(); }

private:
  uint32_t _sampleRate;

  Wavetable _LFO;
  float _LFODepth;

  AHDSR *_ahdsr;
  
  ControlRom::InstPartial *_instPartial;

  uint32_t _delay;
  uint32_t _fadeIn;
  float _fadeInStep;
  float _fade;

  int test, test2;
  
  // Since LFO pitch rate is not found in control ROM (yet) all capital
  // instruments have been measured on an SC-55mkII. Numbers in Hz.
  // Note: Last ~10 instruments where hard to measure precisely
  float lfoRateTable[128] = {
    5.2, 5.2, 5.2, 5.2, 5.2, 5.2, 5.2, 5.2,
    5.2, 5.2, 5.2, 5.2, 5.2, 5.2, 5.2, 5.2,
    7.8, 7.8, 7.8, 6.0, 6.0, 6.0, 5.3, 6.0,
    6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0,
    6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0,
    6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0,
    6.0, 6.0, 6.0, 6.0, 4.3, 4.3, 4.3, 6.0,
    5.2, 5.2, 5.2, 5.2, 5.2, 5.2, 5.2, 5.2,
    5.4, 5.4, 5.4, 5.4, 5.8, 5.8, 5.8, 5.8,
    6.0, 6.0, 6.0, 6.0, 5.6, 5.6, 5.6, 5.6,
    6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0,
    5.6, 5.6, 5.6, 5.6, 5.6, 5.6, 5.6, 5.6,
    5.6, 5.6, 5.6, 5.6, 5.6, 5.6, 5.6, 5.6,
    6.0, 6.0, 6.0, 6.0, 6.0, 5.8, 6.0, 5.8,
    4.2, 4.2, 4.2, 4.2, 4.2, 4.2, 4.2, 4.2,
    5.6, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 4.2};

  /* All instruments in variation bank 8 seems to follow LFO pitch rate of
     capital tone:
v8: 5.2, 5.2, 5.2, 5.2, 7.8, 7.8, 6.0, 6.0,  // 8
    6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0, 6.0,  //16
    6.0, 6.0, 6.0, 5.2, 5.2, 5.2, 6.0, 6.0,  //24
    0.0, 0.0, 0.0, 0.0, 4.2

     MT32 instruments in variation bank 127 seems to follow completely
     differtent LFO pitch rates though.
v127: 5.2 // 0
      2.0 // 8
      4,5 // 16
      4.0 // 24
      4.2 // 32
      4.0 // 40
      4.0 // 48
      4.1 // 56
      4.2 // 64
      5.0 // 72
      4.9 // 80
      4.5 // 88
      4.2 // 96
      5.0 // 104
      4.2 // 112
      0.0 // 120
*/

  TVP();

  double _convert_time_to_sec(uint8_t time);

};

}

#endif  // __TVP_H__
