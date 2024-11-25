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
#include "resample.h"
#include "settings.h"
#include "tva.h"
#include "tvf.h"
#include "tvp.h"
#include "wave_generator.h"

#include <cmath>
#include <stdint.h>


namespace EmuSC {


class Partial
{
public:
  Partial(int partialId, uint8_t key, uint8_t velocity,
	  uint16_t instrumentIndex, ControlRom &controlRom, PcmRom &pcmRom,
	  WaveGenerator *LFO1, Settings *settings, int8_t partId);
  ~Partial();

  void stop(void);
  bool get_next_sample(float *sampleOut);

  inline float get_current_lfo(void)
  { if (_LFO2) return (float) _LFO2->value(); return std::nanf("");}
  float get_current_tvp(void)
  { if (_tvp) return _tvp->get_current_value(); return 0;}
  float get_current_tvf(void)
  { if (_tvf) return _tvf->get_current_value(); return 0;}
  float get_current_tva(void)
  { if (_tva) return _tva->get_current_value(); return 0;}

private:
  struct ControlRom::InstPartial &_instPartial;
  struct ControlRom::Sample *_ctrlSample;

  std::vector<float> *_pcmSamples;

  int _lastPos;           // Last read sample position
  float _index;           // Sample position in number of samples from start
  bool _isLooping;        // Have we entered the loop region? Important for determining previous position

  Settings *_settings;
  int8_t _partId;

  int _drumSet;           // 0 = Not a drumset, 1 & 2 is drumset 0 & 1
  bool _drumRxNoteOff;    // Static parameter (cannot change during a note)

  WaveGenerator *_LFO2;

  TVP *_tvp;
  TVF *_tvf;
  TVA *_tva;

  enum Settings::InterpMode _interpMode;
  double _sample;

  unsigned int _updateTimeout = 0;   // Temporary quickfix
  int _updatePeriod;                 // samples to skip for params update

  bool _next_sample_from_rom(float timeStep);

  void _update_params(void);
};

}

#endif  // __PARTIAL_H__
