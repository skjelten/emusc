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


#ifndef __PARTIAL_H__
#define __PARTIAL_H__


#include "control_rom.h"
#include "pcm_rom.h"
#include "settings.h"
#include "tva.h"
#include "tvf.h"
#include "tvp.h"
#include "wave_generator.h"

#include <stdint.h>


namespace EmuSC {


class Partial
{
public:
  Partial(uint8_t key, int partialId, uint16_t instrumentIndex,
	  ControlRom &controlRom, PcmRom &pcmRom, WaveGenerator *LFO[2],
	  Settings *settings, int8_t partId);
  ~Partial();

  void stop(void);
  bool get_next_sample(float *sampleOut);

private:
  uint8_t _key;           // MIDI key number for note on
  float _keyDiff;         // Difference in number of keys from original tone
                          // If pitchKeyFollow is used, keyDiff is adjusted

  struct ControlRom::InstPartial &_instPartial;
  struct ControlRom::Sample *_ctrlSample;

  std::vector<float> *_pcmSamples;

  int _lastPos;           // Last read sample position
  float _index;           // Sample position in number of samples from start
  bool _isLooping;        // Have we entered the loop region? Important for determining previous position

  float _staticPitchTune;

  double _volumeCorrection;
  double _panpot;

  Settings *_settings;
  int8_t _partId;

  bool _isDrum;
  int _drumMap;

  TVP *_tvp;
  TVF *_tvf;
  TVA *_tva;

  double _sample;

  unsigned int _updateTimeout = 0;   // Temporary quickfix
  int _updatePeriod;                 // samples to skip for params update

  bool _next_sample_from_rom(float timeStep);
  double _convert_volume(uint8_t volume);
  void _update_params(void);
};

}

#endif  // __PARTIAL_H__
