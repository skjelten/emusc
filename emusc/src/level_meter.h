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

// Level accumulator shared between the audio thread and the GUI thread. The
// audio thread publishes the loudest block seen since the last GUI read,
// the GUI thread consumes and resets.


#ifndef LEVEL_METER_H
#define LEVEL_METER_H


#include <atomic>
#include <cstdint>


class LevelMeter
{
public:

  struct Levels {
    float msLeft;             // Mean square (power, 10 * log10)
    float msRight;
    float peakLeft;           // Peak amplitude (amplitude, 20 * log10)
    float peakRight;
    bool  clip;               // Clipping occurred since last read
    bool  valid;              // False => no new audio, GUI should decay only
  };

  static inline void update_max(std::atomic<float> &dest, float value) {
    float prev = dest.load(std::memory_order_relaxed);
    while (prev < value &&
           !dest.compare_exchange_weak(prev, value, std::memory_order_relaxed))
      ;
  }


  inline void publish(float msLeft, float msRight, float peakLeft,
                      float peakRight, bool clipped, uint32_t numFrames) {
    update_max(_msLeft, msLeft);
    update_max(_msRight, msRight);
    update_max(_peakLeft, peakLeft);
    update_max(_peakRight, peakRight);

    if (clipped)
      _clip.store(true, std::memory_order_relaxed);

    // Release: everything stored above is visible to whoever acquires _frames
    _frames.fetch_add(numFrames, std::memory_order_release);
  }


  // Called from the GUI thread: read everything and reset
  Levels get_and_reset(void) {
    Levels lv;
    lv.msLeft    = _msLeft.exchange(0.0f, std::memory_order_relaxed);
    lv.msRight   = _msRight.exchange(0.0f, std::memory_order_relaxed);
    lv.peakLeft  = _peakLeft.exchange(0.0f, std::memory_order_relaxed);
    lv.peakRight = _peakRight.exchange(0.0f, std::memory_order_relaxed);
    lv.clip      = _clip.exchange(false, std::memory_order_relaxed);
    lv.valid     = _frames.exchange(0, std::memory_order_acquire) > 0;

    return lv;
  }


  // Call when starting the emulator, with the audio thread stopped
  void reset(void) {
    _msLeft = 0.0f;    _msRight = 0.0f;
    _peakLeft = 0.0f;  _peakRight = 0.0f;
    _clip = false;     _frames = 0;
  }


private:
  std::atomic<float> _msLeft    {0.0f};
  std::atomic<float> _msRight   {0.0f};
  std::atomic<float> _peakLeft  {0.0f};
  std::atomic<float> _peakRight {0.0f};

  std::atomic<uint32_t> _frames {0};
  std::atomic<bool>     _clip   {false};

  static_assert(std::atomic<float>::is_always_lock_free,
                "LevelMeter requires lock-free atomic<float>");
};


#endif  // LEVEL_METER_H
