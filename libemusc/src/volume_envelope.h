/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022  Håkon Skjelten
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


#ifndef __VOLUME_ENVELOPE_H__
#define __VOLUME_ENVELOPE_H__


#include "control_rom.h"

#include <stdint.h>

namespace EmuSC {


class VolumeEnvelope
{
private:
  bool _finished;
  
  ControlRom::InstPartial *_instPartial;

  uint32_t _sampleRate;

  uint32_t _phaseSampleIndex;
  uint32_t _phaseSampleNum;
  uint32_t _phaseSampleLen;
  
  double _phaseInitVolume;
  double _currentVolume;

  int _currentPhase;
  int _terminalPhase;

  double _phaseVolume[5];        // Phase volume for phase 1-4
  double _phaseDuration[6];      // Phase duration for phase 1-5
  int    _phaseShape[6];         // Phase shape for phase 1-5

  double _convert_volume(uint8_t volume);
  double _convert_time_to_sec(uint8_t time);

  void _init_next_phase(double volume, double time);

  uint32_t _sampleNum;
  std::ofstream _ofs;

  VolumeEnvelope();

public:
  VolumeEnvelope(ControlRom::InstPartial instPartial, uint32_t sampleRate);
  ~VolumeEnvelope();

  double get_next_value();
  void note_off();

  inline bool finished(void) { return _finished; }

};

}

#endif  // __VOLUME_ENVELOPE_H__
