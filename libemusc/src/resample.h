
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


#ifndef __RESAMPLE_H__
#define __RESAMPLE_H__


#include "control_rom.h"

#include <functional>
#include <vector>


namespace EmuSC {


class Resample
{
public:
  Resample(ControlRom::Sample *ctrlSample, std::vector<float> *pcmSamples,
           std::function<void(void)> cb);
  ~Resample();

  float get_next_sample(float rate);

private:
  int _sampleStart;           // 0 or sample attack end if portamento is active
  int _sampleEnd;             // _sampleStart + sample set length
  int _loopStart;             // _sampleEnd - sample set loop length
  int _loopLength;            // Sample set loop length

  std::vector<float> *_pcmSamples;

  float _phase;               // Phase fraction 0.0 - 1.0
  int _index;                 // Integer index

  // 4x sample points (sliding window)
  float y0, y1, y2, y3;

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
  float _interpolate_cubic(float t);
};

}

#endif // __RESAMPLE_H__
