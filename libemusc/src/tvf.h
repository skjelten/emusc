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


#include "biquad_filter.h"
#include "control_rom.h"
#include "envelope.h"
#include "settings.h"
#include "wave_generator.h"

#include <stdint.h>

#include <array>


namespace EmuSC {


class TVF
{
public:
  TVF(ControlRom::InstPartial &instPartial, uint8_t key, uint8_t velocity,
      WaveGenerator *LFO1, WaveGenerator *LFO2, ControlRom::LookupTables &LUT,
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

  ControlRom::LookupTables &_LUT;

  int _L1Init;
  int _L2Init;
  int _L3Init;
  int _L4Init;
  int _L5Init;

  int _accLFO1Depth;
  int _accLFO2Depth;

  int _coFreqIndex;

  int _resIndexROM;
  int _resIndexFreq;

  float _filterFreq;
  float _filterRes;

  uint8_t _key;
  int _velocity;

  int _coFreqVSens;

  int _keyFollow;

  int _envDepth;
  Envelope *_envelope;

  BiquadFilter *_bqFilter;

  ControlRom::InstPartial &_instPartial;

  Settings *_settings;
  int8_t _partId;

  TVF();

  int _get_velocity_from_vcurve(uint8_t velocity);

  void _init_envelope(void);
  void _init_freq_and_res(void);

  float _calc_cutoff_frequency(float index);

  int _get_cof_key_follow(int cofkfROM);

  int _read_cutoff_freq_vel_sens(int cofvsROM);

  int _get_level_init(int level);

  inline bool _le_native(void) { uint16_t n = 1; return (*(uint8_t *) & n); }
  uint16_t _native_endian_uint16(uint8_t *ptr);
};

}

#endif  // __TVF_H__
