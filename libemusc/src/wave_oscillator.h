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

// The wavetable oscillator reads the sample sets and uses a simple linear
// interpolation.
// To save runtime CPU usage EmuSC pre-decodes all the DPCM samples to sample
// sets stored in vectors. To make this work with ping-pong loops the sample
// sets have been extended to include the return path so they basically
// becomes forward loops.


#ifndef __WAVE_OSCILLATOR_H__
#define __WAVE_OSCILLATOR_H__


#include "control_rom.h"
#include "pitch.h"

#include <functional>
#include <vector>


namespace EmuSC {


class WaveOscillator
{
public:
  WaveOscillator(ControlRom::Sample *ctrlSample, std::vector<float> *pcmSamples,
                 std::function<void(void)> cb);

  void get_sample_set(Pitch *pitch, float pitchBend,
                      std::array<std::array<float, 256>,2> &dryBus);

private:
  int _sampleStart;           // 0 or sample attack end if portamento is active
  int _sampleEnd;             // _sampleStart + sample set length
  int _loopStart;             // _sampleEnd - sample set loop length
  int _loopLength;            // Sample set loop length

  std::vector<float> *_pcmSamples;

  float _phase;               // Phase fraction 0.0 - 1.0
  int _index;                 // Integer index

  float y0, y1;               // 2 sample points for linear interpolation

  enum class LoopMode {
    Forward  = 0,
    PingPong = 1,
    NoLoop   = 2
  };
  enum LoopMode _loopMode;

  // External function call for signaling end of first run of sample set
  std::function<void(void)> _firstRunCompleteCallback = NULL;
  bool _firstRunComplete;

  float _fetch_sample(int index);
};

}

#endif // __WAVE_OSCILLATOR_H__
