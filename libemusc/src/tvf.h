/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2026  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 2.1 of the License, or
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


#include "svf.h"
#include "control_rom.h"
#include "envelope.h"
#include "settings.h"
#include "wave_generator.h"

#include <array>
#include <cstdint>


namespace EmuSC {


class TVF : public Envelope
{
public:
  TVF(ControlRom::InstPartial &instPartial, uint8_t key, uint8_t velocity,
      WaveGenerator *LFO1, WaveGenerator *LFO2, ControlRom::LookupTables &LUT,
      Settings *settings, int8_t partId);
  ~TVF();

  void apply(float *sample);
  void apply_sample_set(std::array<float, 256> &dryBus);
  void update(void);

  void note_off();

private:
  uint32_t _sampleRate;

  WaveGenerator *_LFO1;
  WaveGenerator *_LFO2;

  bool _lfo1FadeComplete;
  bool _lfo2FadeComplete;
  int _lfo1Depth;
  int _lfo2Depth;

  ControlRom::LookupTables &_LUT;
  ControlRom::InstPartial &_instPartial;

  int _L1Init;
  int _L2Init;
  int _L3Init;
  int _L4Init;
  int _L5Init;

  int _ipLevelInit;

  int _currentEnvTime;
  int _currentLevelInit;
  int _prevLevelInit;

  int _coFreqIndex;

  int _resIndexFreq;
  int _resIndexUsed;
  int _resonance;

  int _envDepth;

  int _envLevel;
  int _envLevelMode;
  int _prevEnvLevel;

  uint8_t _key;
  int _velocity;

  int _coFreqVSens;

  int _keyFollow;

  SVF *_svf;

  Settings *_settings;
  int8_t _partId;

  TVF();

  int _get_velocity_from_vcurve(uint8_t velocity);

  void _init_envelope(void);
  void _init_freq_and_res(void);

  void _update_lfo_depth(int lfo);

  int _get_cof_key_follow(int cofkfROM);
  int _get_level_init(int level);

  int _read_cutoff_freq_vel_sens(int cofvsROM);

  inline bool _le_native(void) { uint16_t n = 1; return (*(uint8_t *) & n); }
  uint16_t _native_endian_uint16(uint8_t *ptr);

  void _init_new_phase(enum Phase newPhase);
  void _iterate_phase(void);
};

}

#endif  // __TVF_H__
