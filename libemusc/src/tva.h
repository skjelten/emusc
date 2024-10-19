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


#ifndef __TVA_H__
#define __TVA_H__


#include "ahdsr.h"
#include "control_rom.h"
#include "settings.h"
#include "wave_generator.h"

#include <stdint.h>

#include <array>


namespace EmuSC {


class TVA
{
public:
  TVA(ControlRom::InstPartial &instPartial, uint8_t key, WaveGenerator *LFO[2],
      Settings *settings, int8_t partId);
  ~TVA();

  void apply(double *sample);
  void update_params(bool reset = false);

  void note_off();
  bool finished(void);

private:
  uint32_t _sampleRate;

  WaveGenerator *_LFO1;
  WaveGenerator *_LFO2;
  float _LFO1DepthPartial;

  uint8_t _accLFO1Depth;
  uint8_t _accLFO2Depth;

  uint8_t _key;
  int _drumSet;

  uint8_t _panpot;
  bool _panpotLocked;

  AHDSR *_ahdsr;
  bool _finished;
  
  ControlRom::InstPartial &_instPartial;

  Settings *_settings;
  int8_t _partId;

  TVA();

  double _convert_volume(uint8_t volume);
  void _init_envelope(void);

};

}

#endif  // __TVA_H__
