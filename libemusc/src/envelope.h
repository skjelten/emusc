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
  Envelope(ControlRom::LookupTables &LUT);
  virtual ~Envelope() = 0;

  enum class Phase {
    Init       = 0,
    Attack1    = 1,
    Attack2    = 2,
    Decay1     = 3,
    Decay2     = 4,
    Release    = 5,
    Sustain    = 6,
    TermTVADyn = 7,
    TermTVAEnv = 8,
    Terminated = 9
  };

  virtual void set_phase(enum Phase newPhase);

  inline bool finished(void) { return _finished; }

  int get_envelope_value(void) { return _envOutput; }

protected:
  virtual void _init_new_phase(enum Phase newPhase) = 0;
  virtual void _iterate_phase(void) = 0;


  enum class Type {
    TVA,
    TVF,
    Pitch
  };

  void set_time_key_follow(enum Type, bool phase, int key, int etkfROM,
                           int etkpROM = 0);
  void set_time_velocity_sensitivity(enum Type, bool phase, int etvsROM,
                                     int velocity);

  int _phaseLevel[6];
  int _phaseTime[6];

  int  _phaseValueInit[6];         // Initial phase value from ROM
  int  _phaseDurationInit[6];      // Initial phase duration from ROM
  bool _phaseShape[6];             // 0 => linear, 1 => exponential

  int _phaseDuration;              // Calculated complete phase duration
  int _phaseStepSize;              // Increase phase duration
  int _phasePosition;              // 0x08 Normalized phase accumulator
  int _phaseRemainder;             // 0x12 / 0x14 Fractional reminder TVA / TVF

  bool _finished;                  // Flag indicating if enveolope is finished

  int _phaseSampleIndex;
  int _phaseSampleNum;
  int _phaseSampleLen;

  int _phaseStartValue;
  int _phaseEndValue;

  int _envOutput;                  // Envelope value to GUI callback

  int _timeKeyFlwT1T4;
  int _timeKeyFlwT5;
  int _timeVelSensT1T2;
  int _timeVelSensT3T5;

  enum Phase _phase;
  const char *_phaseName[10] = { "Init", "Attack 1", "Attack 2", "Decay 1",
				 "Decay 2", "Release", "Sustain",
				 "Term TVA dynamic", "Term TVA envelope",
				 "Terminated" };

private:
  ControlRom::LookupTables &_LUT;

};

}

#endif  // __ENVELOPE_H__
