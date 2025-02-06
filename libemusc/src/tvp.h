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


#ifndef __TVP_H__
#define __TVP_H__


#include "control_rom.h"
#include "envelope.h"
#include "settings.h"
#include "wave_generator.h"

#include <stdint.h>

#include <array>


namespace EmuSC {


class TVP
{
public:
  TVP(ControlRom::InstPartial &instPartial, uint8_t key, uint8_t velocity,
      int keyShift, ControlRom::Sample *ctrlSample, WaveGenerator *LFO1,
      WaveGenerator *LFO2, ControlRom::LookupTables &LUT,
      Settings *settings, int8_t partId);
  ~TVP();

  void update_dynamic_params(void);

  void note_off();
  inline bool finished(void) { if (_envelope) return _envelope->finished(); }

  double get_next_value(void);
  float get_current_value(void)
  { if (_envelope) return _envelope->get_current_value(); return 0; }

private:
  uint32_t _sampleRate;

  uint8_t _key;                 // MIDI key number
  float _keyFreq;               // Frequency of current MIDI key

  float _staticPitchCorr;       // Accumulated static corrections

  WaveGenerator *_LFO1;
  WaveGenerator *_LFO2;

  ControlRom::LookupTables &_LUT;

  int _LFO1Depth;
  int _LFO2Depth;

  int _accLFO1Depth;
  int _accLFO2Depth;

  const float _expFactor;       // log(2) / 12000
  float _pitchOffsetHz;
  float _pitchExp;

  Envelope *_envelope;
  int _multiplier;

  ControlRom::InstPartial &_instPartial;

  Settings *_settings;
  int8_t _partId;

  TVP();

  void _init_envelope(void);
  void _set_static_params(int keyShift, ControlRom::Sample *ctrlSample);
};

}

#endif  // __TVP_H__
