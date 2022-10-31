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


#ifndef __NOTE_PARTIAL_H__
#define __NOTE_PARTIAL_H__


#include "control_rom.h"
#include "pcm_rom.h"
#include "volume_envelope.h"

#include <stdint.h>


namespace EmuSC {

class NotePartial
{
private:
  uint8_t _key;
  int8_t _keyDiff;        // Difference in number of keys from original tone
  int _drumSet;           // < 0 => not drums (normal instrument)

  uint16_t _instrument;
  uint16_t _sampleIndex;
  bool _partialIndex;

  float _samplePos;       // Sample position in number of samples from start
  bool _sampleDir;        // 0 = backward & 1 = foreward

  ControlRom &_ctrlRom;
  PcmRom &_pcmRom;

  float _sampleFactor;

  double _instPitchTune;  // Instrument pitch offset factor
  
  VolumeEnvelope *_volumeEnvelope;

  double _convert_volume(uint8_t volume);

public:
  NotePartial(uint8_t key, int8_t keyDiff, uint16_t sampleIndex, int drumSet,
	      ControlRom &ctrlRom, PcmRom &pcmRom, uint16_t instrument,
	      bool partialIndex, uint32_t sampleRate);
  ~NotePartial();

  void stop(void);
  bool get_next_sample(float *sampleOut, float pitchBend);

};

}

#endif  // __NOTE_PARTIAL_H__
