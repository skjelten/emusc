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


#ifndef __Pitch_H__
#define __Pitch_H__


#include "control_rom.h"
#include "envelope.h"
#include "settings.h"
#include "wave_generator.h"

#include <stdint.h>

#include <array>


namespace EmuSC {


class Pitch : public Envelope
{
public:
  Pitch(ControlRom &ctrlRom, uint16_t instrumentIndex, int partialId,
        uint8_t key, uint8_t velocity, WaveGenerator *LFO1, WaveGenerator *LFO2,
        Settings *settings, int8_t partId);
  ~Pitch();

  void update(void);

  void note_off();

  inline int get_phase_increment(void) { return _phaseIncrement; }
  inline float get_scaled_phase_increment(void)
  { return (_phaseIncrement / 16384.0) * _sampleRateScale; }

  inline uint16_t get_sample_id(void) { return _sampleIndex; }

  void first_sample_run_complete(void);


private:
  float _sampleRateScale;

  uint8_t _key;                // MIDI key number

  ControlRom &_ctrlRom;
  uint16_t _instrumentIndex;

  ControlRom::InstPartial &_instPartial;
  ControlRom::LookupTables &_LUT;

  WaveGenerator *_LFO1;
  WaveGenerator *_LFO2;

  bool _lfo1FadeComplete;
  bool _lfo2FadeComplete;
  int _lfo1Depth;
  int _lfo2Depth;

  int _envTimeKeyFlwT14;
  int _envTimeKeyFlwT5;
  int _envTimeVelSens;

  int _envPhaseRate[8];
  bool _isAscending;

  int _envVelSens;

  int _keyFollowOffset;

  int _phaseLevel[6];

  int _basePitchC;
  int _basePitchF;
  uint16_t _sampleIndex;

  int _samplePitchOffsetInit;
  int _samplePitchOffsetSust;
  int _samplePitchOffsetActive;
  int _portamentoDelta;
  int _releasePitch;
  int _targetPitch;

  int _cachedPFineTune;
  int _cachedPFineTuneOffset;

  int _phaseIncrement;

  Settings *_settings;
  int8_t _partId;

  // Portamento pitch values are shared among all voices / instrument partials.
  // Portamento base pitch is reused in a round robin fashion for all available
  // voices, while the portamento target pitch is a global target.
  static int _portaTargetPitch;
  static std::array<int, 28> _portaBasePitch;
  static int _pbpIndex;

  Pitch();

  void _init_envelope(uint8_t envelope);
  int _init_portamento(bool portamento, bool legato);

  void _init_base_pitch(void);
  void _apply_key_shifts_bp(void);
  void _apply_key_follow_bp(void);
  void _apply_scale_tuning_bp(void);

  int _calc_phase_inc_from_pitch(int frac);

  int _get_env_key_follow(int p, int kfRom);
  int _get_env_time_velocity_sensitivity(int etvsROM, int velocity);
  int _get_env_phase_rate(int etRom, int phase);

  int _get_pitch_curve_correction(int pitchCurve);

  void _init_new_phase(enum Phase newPhase);
  void _iterate_phase(void);
};

}

#endif  // __Pitch_H__
