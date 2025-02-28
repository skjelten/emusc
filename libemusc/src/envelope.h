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


#ifndef __ENVELOPE_H__
#define __ENVELOPE_H__


#include "control_rom.h"
#include "settings.h"

#include <array>

#include <stdint.h>


namespace EmuSC {


class Envelope
{
public:
  enum class Type {
    TVA,
    TVF,
    TVP
  };

  Envelope(double value[5], uint8_t duration[5], bool shape[5], int key,
           ControlRom::LookupTables &LUT, Settings *settings, int8_t partId,
           enum Type type, int initValue = 0);
  ~Envelope();

  void start(void);
  double get_next_value(void);
  double get_current_value(void) { return _currentValue; }

  void release(void);

  inline bool finished(void) { return _finished; }

  void set_time_key_follow(bool phase, int etkfROM, int etkpROM = 0);
  void set_time_velocity_sensitvity(bool phase, int etvsROM);

  void set_time_vel_sens_t1_t4(float time) { _timeVelSensT1T4 = time; }
  void set_time_vel_sens_t5(float time) { _timeVelSensT5 = time; }

private:
  double  _phaseValue[5];
  uint8_t _phaseDuration[5];
  bool    _phaseShape[5];       // 0 => linear, 1 => exponential

  ControlRom::LookupTables &_LUT;

  bool _finished;               // Flag indicating whether enveolope is finished

  uint32_t _sampleRate;

  uint32_t _phaseSampleIndex;
  uint32_t _phaseSampleNum;
  uint32_t _phaseSampleLen;

  double _phaseInitValue;
  double _currentValue;

  float _linearChange;

  enum class Phase {
    Off     = -1,
    Attack1 =  0,
    Attack2 =  1,
    Decay1  =  2,
    Decay2  =  3,
    Release =  4
  };
  enum Phase _phase;

  int _key;

  Settings *_settings;
  int8_t _partId;

  int _timeKeyFlwT1T4;
  int _timeKeyFlwT5;

  float _timeVelSensT1T4;
  float _timeVelSensT5;

  enum Type _type;
  const char *_phaseName[5] = { "Attack 1", "Attack 2", "Decay 1",
				"Decay 2 (S)", "Release" };

  Envelope();

  void _init_new_phase(enum Phase newPhase);
  float _calc_exp_change(float index);

};

}

#endif  // __ENVELOPE_H__
