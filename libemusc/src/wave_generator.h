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
    Sine        = 0,
    Square      = 1,
    Sawtooth    = 2,
    Triangle    = 3,
    RandomLevel = 8,
    RandomSlope = 9,
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
  inline double value() { return _currentValue; }

private:
  WaveGenerator();

  bool _id;
  enum Waveform _waveForm;
  int _sampleRate;

  struct ControlRom::LookupTables &_LUT;

  int _rate;
  int _rateChange;            // Change in rate due to controller input etc.
  int _delay;                 // Delay time in number of samples
  int _fade;                  // Fade time in number of samples
  int _fadeMax;

  float _currentValue;
  float _random;
  float _randomStart;

  Settings *_settings;
  int _partId;

  bool _useLUT;
  bool _interpolate;          // Linear interpolation

  int _index;                 // Sample index
  int _period;                // Number of samples in one period (2*pi)


  void _update_params(void);
  float _phase_shift_to_index(int phaseShift);

};

}

#endif  // __WAVE_GENERATOR_H__
