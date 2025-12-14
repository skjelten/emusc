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


#ifndef __WAVE_GENERATOR_H__
#define __WAVE_GENERATOR_H__


#include "control_rom.h"
#include "settings.h"

#include <array>

#include <stdint.h>


namespace EmuSC {

class WaveGenerator
{
public:
  enum class Waveform {
    Sine       = 0,
    Square     = 1,
    Sawtooth   = 2,
    Triangle   = 3,
    SampleHold = 8,
    Random     = 9,
  };

  // LFO1 is defined in the Instrument section
  WaveGenerator(struct ControlRom::Instrument &instrument,
                struct ControlRom::LookupTables &LUT,
                Settings *settings, int partId);

  // LFO2s are defined in the Instrument Partial section
  WaveGenerator(struct ControlRom::InstPartial &instPartial,
                struct ControlRom::LookupTables &LUT,
                Settings *settings, int partId);
  ~WaveGenerator();

  void update(void);
  inline int value(void) { return _currentValueNorm; }
  inline float value_float(void) { return (float) _currentValueNorm / 32767.0; }

private:
  WaveGenerator();

  bool _id;
  enum Waveform _waveform;

  struct ControlRom::LookupTables &_LUT;

  int _instRate;              // LFO Rate from instrument [partial] definition
  int _rateChange;            // Change in rate due to controller input etc.

  int _delay;                 // Current delay status from 0 to 0xffff
  int _delayIncLUT;           // Delay increment @125Hz from LUT

  int _fade;                  // Current fade status from 0 to 0xffff
  int _fadeIncLUT;            // Fade increment @125Hz from LUT

  int _currentValue;          // 16 bit SC-55 scpecific LFO format
  int _currentValueNorm;      // 16 bit normalized LFO value

  uint16_t _accRate;          // Accumulated rate (+ phase shift)
  int _random;
  bool _randomFirstRun;

  Settings *_settings;
  int _partId;

  int _generate_sine(int rate);
  int _generate_square(int rate);
  int _generate_sawtooth(int rate);
  int _generate_triangle(int rate);
  int _generate_sample_hold(int rate);
  int _generate_random(int rate);
};

}

#endif  // __WAVE_GENERATOR_H__
