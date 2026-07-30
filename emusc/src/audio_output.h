/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2026  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H


#include "emusc/synth.h"

#include <algorithm>
#include <atomic>
#include <cmath>


class AudioOutput
{
public:
  AudioOutput(EmuSC::Synth *synth);
  virtual ~AudioOutput() = 0;

  virtual void start(void) = 0;
  virtual void stop(void) = 0;

  float volume(void) { return _volume.load(std::memory_order_relaxed); }
  void set_volume(float value) { _volume.store(value,
                                               std::memory_order_relaxed); }

protected:
  bool _quit;

  inline void _get_frame(float &lOut, float &rOut)
  {
    _synth->get_next_frame(lOut, rOut);

    // Apply volume attenuation (volume knob in GUI)
    const float volume = _volume.load(std::memory_order_relaxed);
    lOut  *= volume;
    rOut *= volume;

    // Store accumulated output for statistics (volume meter)
    _accLeft += lOut * lOut;
    _accRight += rOut * rOut;
    _accPeakLeft = std::max(_accPeakLeft, std::fabs(lOut));
    _accPeakRight = std::max(_accPeakRight, std::fabs(rOut));
  }

private:
  EmuSC::Synth *_synth;

  std::atomic<float> _volume;              // [0 - 1] Default 1

  float _accLeft, _accRight;
  float _accPeakLeft, _accPeakRight;
  int _accNum;

  AudioOutput();

};


#endif  // AUDIO_OUTPUT_H
