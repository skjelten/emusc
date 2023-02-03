/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2023  Håkon Skjelten
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


#ifndef __PARTIAL_H__
#define __PARTIAL_H__


#include "control_rom.h"
#include "pcm_rom.h"
#include "settings.h"
#include "tva.h"
#include "tvf.h"
#include "tvp.h"

#include <stdint.h>


namespace EmuSC {


class Partial
{
private:
  uint8_t _key;           // MIDI key number for note on
  float _keyFreq;         // Frequency of current MIDI key
  float _keyDiff;         // Difference in number of keys from original tone
                          // If pitchKeyFollow is used, keyDiff is adjusted

  struct ControlRom::InstPartial &_instPartial;
  struct ControlRom::Sample *_ctrlSample;

  std::vector<float> *_pcmSamples;

  float _index;           // Sample position in number of samples from start
  bool _direction;        // Sample read direction: 0 = backward & 1 = foreward

  float _expFactor;       // log(2) / 12000

  float  _staticPitchTune;

  Settings *_settings;
  int8_t _partId;

  bool _isDrum;
  int _drumMap;

  TVP *_tvp;
  TVF *_tvf;
  TVA *_tva;

  double _convert_volume(uint8_t volume);

public:
  Partial(uint8_t key, int partialId, uint16_t instrumentIndex,
	  ControlRom &controlRom, PcmRom &pcmRom, Settings *settings,
	  int8_t partId);
  ~Partial();

  void stop(void);
  bool get_next_sample(float *sampleOut);

};

}

#endif  // __PARTIAL_H__
