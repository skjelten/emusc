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

// Initial version of reverb system effect using networks of 3 series allpass
// filters and 4 parallel comb filters for reverb mode 1-6.
// Mode 7 and 8 are implemented using only a delay line

// TODO: Verify correct structure for sound canvas reverb filters and adjust
// parameters for correct room size. Fix proper paning for Panning delay mode.


#include "reverb.h"

#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>


namespace EmuSC {


Reverb::Reverb(Settings *settings)
  : _settings(settings),
    _effectMix(0.3),
    _sampleRate(settings->sample_rate()),
    _time(1.0),
    _panning(0),
    _lp1Filter(_sampleRate),
    _preLPF(-1),
    _reverbTime(-1),
    _delayFeedback(-1)
{
  const int maxDelay = 2000;                       // Number of samples

  // FreeVerb inspired delay lengths for 44100 Hz sample rate
  int delayLengths[9] = { 225, 341, 441, 1116, 1356, 1422, 1617, 211, 179 };
  if (_sampleRate != 44100) {
    float scaler = _sampleRate / 44100;
    for (auto &length : delayLengths) {      
      int scaledDelay = (int) std::floor(scaler * length);
      if ( (scaledDelay & 1) == 0) scaledDelay++;
      length = scaledDelay;
    }
  }

  // Initialize all-pass filters
  allPassFilters.emplace_back(maxDelay, delayLengths[0]);
  allPassFilters.emplace_back(maxDelay, delayLengths[1]);
  allPassFilters.emplace_back(maxDelay, delayLengths[2]);

  // Initialize comb filters
  combFilters.emplace_back(maxDelay, delayLengths[3], _sampleRate);
  combFilters.emplace_back(maxDelay, delayLengths[4], _sampleRate);
  combFilters.emplace_back(maxDelay, delayLengths[5], _sampleRate);
  combFilters.emplace_back(maxDelay, delayLengths[6], _sampleRate);

  // Delay testing: Min delay=0, Max delay=0.425s
  _delayFilter = new Delay(0.5 * _sampleRate, 100);
  _delayFilter->set_feedback(0);

  _delayLeft = new Delay(maxDelay, delayLengths[7]);
  _delayRight = new Delay(maxDelay, delayLengths[8]);
}


Reverb::~Reverb()
{
  delete _delayFilter;

  delete _delayLeft;
  delete _delayRight;
}


// Process a single audio sample
void Reverb::process_sample(float *input, float *output)
{
  // TODO: Stereo reverb. For now: mix left and right to MONO
  float sample = (input[0] + input[1]) / 2;

  // Reverb time is guessed to be a linear scale for T60 between 0.0 and 4.0
  if (_reverbTime != _settings->get_param(PatchParam::ReverbTime)) {
    _reverbTime = _settings->get_param(PatchParam::ReverbTime);

    for (auto& cFilter : combFilters)
      cFilter.set_coefficient((float) _reverbTime / 32.0);

    _delayFilter->set_delay((_reverbTime / 127.0) * _sampleRate * 0.430);
  }

  // Run through pre lowpass filter
  if (_preLPF != _settings->get_param(PatchParam::ReverbPreLPF)) {
    _preLPF = _settings->get_param(PatchParam::ReverbPreLPF);
    _lp1Filter.calculate_alpha(_lpCutoffFreq[_preLPF]);
  }
  float inputAP = _lp1Filter.apply(sample);

  if (_settings->get_param(PatchParam::ReverbCharacter) < 6) {

    // Process allpass filters in series
    for (auto &aFilter : allPassFilters)
      inputAP = aFilter.process_sample(inputAP);

    // Process comb filters in parallell
    float combOutput = 0.0f;
    for (auto& cFilter : combFilters)
      combOutput += cFilter.process_sample(inputAP);

    output[0] = output[1] = (1.0 - _effectMix) * (*input);
    output[0] += _effectMix * (_delayLeft->process_sample(combOutput));
    output[1] += _effectMix * (_delayRight->process_sample(combOutput));

  // Delay modes
  } else if (_settings->get_param(PatchParam::ReverbCharacter) >= 6) {
    if (_delayFeedback !=_settings->get_param(PatchParam::ReverbDelayFeedback)){
      _delayFeedback = _settings->get_param(PatchParam::ReverbDelayFeedback);
      _delayFilter->set_feedback(_delayFeedback / 180.0);
    }

    // Delay mode
    if (_settings->get_param(PatchParam::ReverbCharacter) == 6) {
      float outputAP = _delayFilter->process_sample(inputAP);

      // FIXME: Add stereo mixer
      output[0] = output[1] = outputAP;

    // Panning delay mode
    } else {
      if (!_panning)
	output[0] = _delayFilter->process_sample(inputAP);
      else
	output[1] = _delayFilter->process_sample(inputAP);

      if (!(_delayFilter->get_readIndex() % (int) (_reverbTime * _sampleRate * 0.430)))
//      if (_delayFilter->get_readIndex() == 0)
	_panning = !_panning;
    }
  }
}

}
