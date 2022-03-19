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


#ifndef __NOTE_PARTIAL_H__
#define __NOTE_PARTIAL_H__


#include "control_rom.h"
#include "pcm_rom.h"

#include <stdint.h>


class NotePartial
{
private:
  int8_t _keyDiff;        // Difference in number of keys from original tone
  bool _drum;

  uint16_t _sampleIndex;

  float _samplePos;       // Sample position in number of samples from start
  bool _sampleDir;        // 0 = backward & 1 = foreward 
  
  ControlRom &_ctrlRom;
  PcmRom &_pcmRom;
  
  enum AhdsrState {
    ahdsr_Attack,
    ahdsr_Hold,
    ahdsr_Decay,
    ahdsr_Sustain,
    ahdsr_Release,
    ahdsr_Inactive
  };
  enum AhdsrState _ahdsrState;

public:
  NotePartial(int8_t keyDiff, uint16_t sampleIndex, bool drum,
	      ControlRom &ctrlRom, PcmRom &pcmRom);
  ~NotePartial();

  void stop(void);
  bool get_next_sample(float *sampleOut, float pitchBend);

};


#endif  // __NOTE_PARTIAL_H__
