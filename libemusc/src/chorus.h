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


#ifndef __CHORUS_H__
#define __CHORUS_H__


#include "lowpass_filter1.h"
#include "settings.h"

#include <vector>


namespace EmuSC {

class Chorus
{
 public:
  Chorus(Settings *settings);
  ~Chorus();

  void process_sample(float input, float *output);
  void apply_stereo_width(float *output);


 private:
  Chorus();

  Settings *_settings;

  int _numVoices;     // = 3 Number of chorus voices
  int _delayLineSize; // = 8192;  // Size of each delay line

  uint32_t _sampleRate;
  uint8_t _preLPF;             // Lowpass filter before chorus, 250Hz - 8kHz?
  float _rate;                 // Modulation rate (LFO), 0.05 - 10Hz?
  float _depth;                // Modulation depth (LFO), 1 - 20 cents?
  float _delay;                // Time delay in num of samples, 0, 1.0 ,100 ms?
  float _feedback;             // Feedback, 0 - 0.96?

  float _stereoWidth;
  std::vector<std::vector<float>> _delayLinesLeft;
  std::vector<std::vector<float>> _delayLinesRight;

  int _writeIndex;

  float _lfoPhase;

  LowPassFilter1 _lp1Filter;
  std::array<int, 8> _lpCutoffFreq = { 8000, 5000, 3150, 2000,
				       1250, 800, 400, 250 };
};

}

#endif  // __CHORUS_H__
