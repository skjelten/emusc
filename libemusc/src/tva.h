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


#ifndef __TVA_H__
#define __TVA_H__


#include "control_rom.h"
#include "envelope.h"
#include "settings.h"
#include "wave_generator.h"

#include <array>
#include <cstdint>


namespace EmuSC {


class TVA : public Envelope
{
public:
  TVA(ControlRom &ctrlRom, uint8_t key, uint8_t velocity, int sampleIndex,
      WaveGenerator *LFO1, WaveGenerator *LFO2, Settings *settings,
      int8_t partId, uint16_t instrumentIndex, int partialId);

  void update(bool reset = false);
  void apply(double *sample);
  void apply_sample_set(std::array<std::array<float, 256>, 2> &dryBus);

  void note_off();

private:
  int _dynLevel;
  int _dynLevelMode;
  int _prevDynLevel;

  int _envLevel;
  int _envLevelMode;
  int _prevEnvLevel;

  WaveGenerator *_LFO1;
  WaveGenerator *_LFO2;

  bool _lfo1FadeComplete;
  bool _lfo2FadeComplete;
  int _lfo1Depth;
  int _lfo2Depth;

  ControlRom::LookupTables &_LUT;
  ControlRom::InstPartial &_instPartial;

  uint8_t _key;
  int _drumSet;

  int _panpot;
  int _panpotL;
  int _panpotR;
  bool _panpotLocked;

  Settings *_settings;
  int8_t _partId;

  TVA();

  void _init_envelope(ControlRom &ctrlRom, int sampleIndex, int instrumentIndex,
                      uint8_t cVelocity);

  void _update_dynamic_level(void);
  void _update_panpot_level(bool reset);
  void _update_lfo_depth(int lfo);

  int _get_bias_level(int km, int biasPoint);
  int _get_velocity_from_vcurve(uint8_t velocity);

  void _init_new_phase(enum Phase newPhase);
  void _iterate_phase(void);

  static void _smooth(int mode, float start, float target,
                      std::array<float, 256> &gain);
  };

}

#endif  // __TVA_H__
