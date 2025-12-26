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


#include "control_rom.h"
#include "envelope.h"
#include "settings.h"
#include "wave_generator.h"

#include <stdint.h>

#include <array>


namespace EmuSC {


class TVA
{
public:
  TVA(ControlRom::InstPartial &instPartial, uint8_t key, uint8_t velocity,
      ControlRom::Sample *ctrlSample, WaveGenerator *LFO1, WaveGenerator *LFO2,
      ControlRom::LookupTables &LUT, Settings *settings, int8_t partId,
      int instVolAtt);
  ~TVA();

  void update_dynamic_params(bool reset = false);

  void apply(double *sample);
  float get_current_value()
  { if (_envelope) return _envelope->get_current_value(); return 0; }

  void note_off();
  bool finished(void);

private:
  int _dynLevel;

  uint32_t _sampleRate;

  WaveGenerator *_LFO1;
  WaveGenerator *_LFO2;

  ControlRom::LookupTables &_LUT;

  uint8_t _key;
  int _drumSet;

  int _panpot;
  int _panpotL;
  int _panpotR;
  bool _panpotLocked;

  Envelope *_envelope;
  bool _finished;
  
  ControlRom::InstPartial &_instPartial;

  Settings *_settings;
  int8_t _partId;

  TVA();

  void _init_envelope(int levelIndex, uint8_t velocity);

  int _get_bias_level(int biasPoint);
  int _get_velocity_from_vcurve(uint8_t velocity);
};

}

#endif  // __TVA_H__
