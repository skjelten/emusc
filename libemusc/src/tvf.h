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
#include "envelope.h"
#include "lowpass_filter2.h"
#include "settings.h"
#include "wave_generator.h"

#include <stdint.h>

#include <array>


namespace EmuSC {


class TVF
{
public:
  TVF(ControlRom::InstPartial &instPartial, uint8_t key, uint8_t velocity,
      WaveGenerator *LFO1, WaveGenerator *LFO2,
      Settings *settings, int8_t partId);
  ~TVF();

  void apply(double *sample);
  void update_params(void);

  void note_off();
  inline bool finished(void) { if (_envelope) return _envelope->finished(); }

  float get_current_value()
  { if (_envelope) return _envelope->get_current_value(); return 0; }

private:
  uint32_t _sampleRate;

  WaveGenerator *_LFO1;
  WaveGenerator *_LFO2;

  uint8_t _accLFO1Depth;
  uint8_t _accLFO2Depth;

  int8_t _coFreq;
  uint8_t _res;

  uint8_t _key;

  Envelope *_envelope;
  LowPassFilter2 *_lpFilter;

  ControlRom::InstPartial &_instPartial;

  Settings *_settings;
  int8_t _partId;

  TVF();

  void _init_envelope(void);
};

}

#endif  // __TVF_H__
