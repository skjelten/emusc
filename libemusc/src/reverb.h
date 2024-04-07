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


#ifndef __REVERB_H__
#define __REVERB_H__


#include "allpass_filter.h"
#include "comb_filter.h"
#include "delay.h"
#include "lowpass_filter1.h"
#include "settings.h"

#include <array>
#include <vector>


namespace EmuSC {

  
class Reverb
{
 public:
  Reverb(Settings *settings);
  ~Reverb();
 
  void process_sample(float *input, float *output);

 private:
  Reverb();

  Settings *_settings;

  std::vector<CombFilter> combFilters;
  std::vector<AllPassFilter> allPassFilters;
  Delay *_delayLeft;
  Delay *_delayRight;
  float _effectMix;

  Delay *_delayFilter;  // Used for reverb character = Delay

  int _reverbTime;
  int _delayFeedback;

  uint32_t _sampleRate;
  uint8_t _preLPF;             // Lowpass filter before chorus, 250Hz - 8kHz?
  float _time;                 // Reverb time in num of samples, 0, 1.0 ,100 ms?
  float _feedback;             // Feedback, 0 - 0.96?
  bool _panning;               // 0 = left, 1 = right

  LowPassFilter1 _lp1Filter;
  std::array<int, 8> _lpCutoffFreq = { 8000, 5000, 3150, 2000,
				       1250, 800, 400, 250 };

  void _process_sample_delay(float *input, float *output);

};

}

#endif  // __REVERB_H__
