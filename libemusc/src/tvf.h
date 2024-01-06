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


#ifndef __TVF_H__
#define __TVF_H__


#include "control_rom.h"
#include "lowpass_filter.h"
#include "ahdsr.h"
#include "settings.h"
#include "wavetable.h"

#include <stdint.h>

#include <array>


namespace EmuSC {


class TVF
{
public:
  TVF(ControlRom::InstPartial &instPartial, uint8_t key, Settings *settings,
      int8_t partId);
  ~TVF();

  double apply(double input);
  void note_off();

  inline bool finished(void) { if (_ahdsr) return _ahdsr->finished(); }

private:
  uint32_t _sampleRate;

  Wavetable _LFO1;
  Wavetable _LFO2;
  int _LFO1DepthPartial;

  Settings *_settings;
  int8_t _partId;

  AHDSR *_ahdsr;
  LowPassFilter *_lpFilter;

  float _filterBaseLFOFreq;

  uint32_t _lpBaseFrequency;
  float _lpResonance;

  ControlRom::InstPartial _instPartial;
  int _tempi;
  float _tempf;

  // Since LFO pitch rate is not found in control ROM (yet) all capital
  // instruments have been measured on an SC-55mkII. Numbers in Hz.
  // All variations, except 127 (MT32 assignement) follows catital tone.
  float lfo1RateTable[128] = {
    5.2, 5.2, 5.2, 5.2, 5.2, 5.2, 5.2, 5.2,
    5.2, 5.2, 5.2, 5.2, 5.2, 5.2, 5.2, 5.2,
    7.8, 7.8, 7.8, 6.0, 6.0, 6.0, 5.4, 6.0,
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
    4.2, 6.0, 4.2, 4.2, 4.2, 6.0, 4.2, 4.2};

  TVF();

};

}

#endif  // __TVF_H__
