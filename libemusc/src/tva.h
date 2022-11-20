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


#include "control_rom.h"
#include "ahdsr.h"

#include <stdint.h>


namespace EmuSC {


class TVA
{
public:
  TVA(ControlRom::InstPartial instPartial, uint32_t sampleRate);
  ~TVA();

  double get_amplification();
  void note_off();

  bool finished(void);

private:
  uint8_t _LFODepth;
  float _LFOFrequency;



  float _readPointer;
  std::vector<uint8_t> _sinus = { 0,0,0,0,1,1,2,3,4,6,7,9,10,12,14,16,18,20,23,25,28,30,33,36,39,42,45,48,51,54,57,60,63,66,69,72,75,78,81,84,87,90,93,96,98,101,103,106,108,110,112,114,116,117,119,120,122,123,124,125,125,126,126,127,127,127,126,126,125,125,124,123,122,120,119,117,116,114,112,110,108,106,103,101,98,96,93,90,87,84,81,78,75,72,69,66,63,60,57,54,51,48,45,42,39,36,33,30,28,25,23,20,18,16,14,12,10,9,7,6,4,3,2,1,1,0,0,0 };


  
  AHDSR *_ahdsr;
  bool _finished;
  
  ControlRom::InstPartial *_instPartial;

  uint32_t _sampleRate;

  TVA();

  double _convert_volume(uint8_t volume);
  double _convert_time_to_sec(uint8_t time);
};

}

#endif  // __TVA_H__
