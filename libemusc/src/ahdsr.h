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


#ifndef __AHDSR_H__
#define __AHDSR_H__


#include "settings.h"

#include <stdint.h>


namespace EmuSC {


class AHDSR
{
public:
  enum class Type {
    TVA,
    TVF,
    TVP
  };

  AHDSR(double value[5], uint8_t duration[5], bool shape[5], int key,
	Settings *settings, int8_t partId, enum Type type, int initValue = 0);
  ~AHDSR();

  void start(void);
  double get_next_value(void);
  double get_current_value(void) { return _currentValue; }

  void release(void);

  inline bool finished(void) { return _finished; }

private:
  double  _phaseValue[5];
  uint8_t _phaseDuration[5];
  bool    _phaseShape[5];        // Phase 0 = linear, 1 = logarithmic

  bool _finished;               // Flag indicating whether enveolope is finished

  uint32_t _sampleRate;

  uint32_t _phaseSampleIndex;
  uint32_t _phaseSampleNum;
  uint32_t _phaseSampleLen;
  
  double _phaseInitValue;
  double _currentValue;

  enum Phase {
    ahdsr_Off     = -1,
    ahdsr_Attack  =  0,
    ahdsr_Hold    =  1,
    ahdsr_Decay   =  2,
    ahdsr_Sustain =  3,
    ahdsr_Release =  4
  };
  enum Phase _phase;

  int _key;

  Settings *_settings;
  int8_t _partId;

  enum Type _type;
  const char *_phaseName[5] = { "Attack", "Hold", "Decay",
				"Sustain", "Release" };

  AHDSR();

  void _init_new_phase(enum Phase newPhase);

//  uint32_t _sampleNum;
//  std::ofstream _ofs;

  // Create a calculated LUT since we are still unable to read this from ROM
  static constexpr std::array<float, 128> _convert_time_to_sec_LUT = {
    0.000486, 0.007689, 0.015176, 0.022956, 0.031042, 0.039445, 0.048178,
    0.057254, 0.066686, 0.076488, 0.086676, 0.097263, 0.108266, 0.119701,
    0.131584, 0.143935, 0.156770, 0.170109, 0.183972, 0.198379, 0.213352,
    0.228912, 0.245084, 0.261890, 0.279356, 0.297508, 0.316372, 0.335977,
    0.356352, 0.377526, 0.399532, 0.422402, 0.446169, 0.470870, 0.496541,
    0.523219, 0.550944, 0.579759, 0.609704, 0.640825, 0.673168, 0.706780,
    0.741712, 0.778016, 0.815744, 0.854954, 0.895704, 0.938053, 0.982064,
    1.027804, 1.075339, 1.124741, 1.176082, 1.229438, 1.284889, 1.342518,
    1.402408, 1.464650, 1.529336, 1.596561, 1.666425, 1.739032, 1.814489,
    1.892909, 1.974408, 2.059106, 2.147129, 2.238609, 2.333679, 2.432482,
    2.535164, 2.641877, 2.752779, 2.868036, 2.987817, 3.112301, 3.241672,
    3.376122, 3.515850, 3.661064, 3.811979, 3.968819, 4.131816, 4.301213,
    4.477259, 4.660218, 4.850359, 5.047964, 5.253328, 5.466754, 5.688559,
    5.919072, 6.158635, 6.407602, 6.666344, 6.935244, 7.214701, 7.505129,
    7.806959, 8.120639, 8.446633, 8.785426, 9.137519, 9.503436, 9.883718,
    10.27892, 10.68965, 11.11650, 11.56011, 12.02114, 12.50027, 12.99820,
    13.51568, 14.05348, 14.61240, 15.19325, 15.79691, 16.42427, 17.07626,
    17.75385, 18.45803, 19.18987, 19.95043, 20.74085, 21.56231, 22.41601,
    23.30323, 24.22529
  }; // pow(2.0, (double)(time) / 18.0) / 5.45 - 0.183

};

}

#endif  // __AHDSR_H__
