/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2026  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 2.1 of the License, or
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
#include "pitch.h"
#include "wave_oscillator.h"
#include "settings.h"
#include "tva.h"
#include "tvf.h"
#include "wave_generator.h"

#include <array>
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

  bool get_sample_set(std::array<std::array<float, 256>, 2> &dryBus);

  void stop(void);
  void update(void);

  void first_run_cb(void);

  inline int get_current_lfo(void)
  { if (_LFO2) return _LFO2->value(); return 0; }
  int get_current_pitch(void)
  { if (_pitch) return _pitch->get_envelope_value(); return 0;}
  int get_current_tvf(void)
  { if (_tvf) return _tvf->get_envelope_value(); return 0;}
  int get_current_tva(void)
  { if (_tva) return _tva->get_envelope_value(); return 0;}

private:
  struct ControlRom::InstPartial &_instPartial;
  struct ControlRom::Sample *_ctrlSample;

  std::vector<float> *_pcmSamples;

  Settings *_settings;
  int8_t _partId;

  int _drumSet;           // 0 = Not a drumset, 1 & 2 is drumset 0 & 1
  bool _drumRxNoteOff;    // Static parameter (cannot change during a note)

  WaveGenerator *_LFO2;

  WaveOscillator *_waveOscillator;

  Pitch *_pitch;
  TVF *_tvf;
  TVA *_tva;

  float _pitchAdj;
};

}

#endif  // __PARTIAL_H__
