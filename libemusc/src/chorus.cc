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


#include "chorus.h"

#include <iostream>
#include <vector>
#include <cmath>

#include <iomanip>


namespace EmuSC {

Chorus::Chorus(Settings *settings)
  : _settings(settings),
    _sampleRate(settings->get_param_uint32(SystemParam::SampleRate)),
    _stereoWidth(0.5),
    _lfoPhase(0),
    _lpFilter(_sampleRate),
    _numVoices(1),
    _writeIndex(0)
{
  _delayLineSize = _sampleRate * 0.2;             // Max 100ms delay

  _delayLinesLeft.reserve(_numVoices);
  _delayLinesRight.reserve(_numVoices);
  
  for (int i = 0; i < _numVoices; ++i) {
    _delayLinesLeft.push_back(std::vector<float>(_delayLineSize, 0.0f));
    _delayLinesRight.push_back(std::vector<float>(_delayLineSize, 0.0f));
  }
}


Chorus::~Chorus()
{}


// Process a single audio sample
void Chorus::process_sample(float input, float *output)
{
  // TODO: Figure out proper depth levels. Linear increase?
  _depth = 1.4 *_settings->get_param(PatchParam::ChorusDepth);

  // TODO: Figure out proper values for feedback. Linear increase?
  _feedback = _settings->get_param(PatchParam::ChorusFeedback) / 165.0;

  // TODO: How is the f(x) for Chorus delay? Linear function 0 - 100 ms?
  _delay = (_sampleRate / 8192.0) * _settings->get_param(PatchParam::ChorusDelay);

  // Chorus rate measured on an SC-55 MkII.
  //  - Linear in the range 0 > rate > 105: y = rate / 8
  //  - Flat when rate > 105: y = 105 / 8
  int chorusRate = _settings->get_param(PatchParam::ChorusRate);
  _rate = chorusRate <= 105 ? chorusRate / 8.0 : 105 / 8.0;

  // Run through pre lowpass filter
  // TODO: This seems to be a single order lowpass filter?
  _preLPF = _settings->get_param(PatchParam::ChorusPreLPF);
  _lpFilter.calculate_coefficients(_lpCutoffFreq[_preLPF], 0.707);
  float filteredInput = _lpFilter.apply(input);

  // Create delayed and detuned voices for left channel
  float outputL = 0.0f;

  // Calculate modulation for left channel
  double modDepthL = _depth * (4.0 * std::fabs(_lfoPhase - 0.5));  // - 1.0);  
  float modDelayTimeL = (_delay + modDepthL) * 0.0001;
  int modDelaySamplesL = std::round(modDelayTimeL * _sampleRate);

  for (int i = 0; i < _numVoices; ++i) {
    int _readIndex = fmod(_writeIndex + _delayLineSize - modDelaySamplesL,
			  _delayLineSize);
    if (0) {
      static int b=0;
      if (++b % 10000 == 0)
	std::cout << std::fixed << std::setprecision(1)
		  << " ri=" << std::setw(4) << _readIndex
		  << " wi=" << std::setw(4) << _writeIndex
		  << " md=" << std::setw(5) << modDepthL
		  << " mdt=" << std::setw(7) << std::setprecision(4) << modDelayTimeL
		  << " mds=" << std::setw(4) << modDelaySamplesL
		  << " dls=" << std::setw(4) << _delayLineSize
		  << " dly=" << std::setw(5) << std::setprecision(1) << _delay
		  << " dpt=" << std::setw(5) << _depth
		  << std::endl;
    }

    float delayedSample = _delayLinesLeft[i][_readIndex];
    _delayLinesLeft[i][_writeIndex] = filteredInput + delayedSample * _feedback;

    outputL += delayedSample;
  }

  // Create delayed and detuned voices for right channel
  float outputR = 0.0f;

  // Create LFO signal 90 degree phase adjusted for right channel. TODO: Verify
  float lfoPhase90 = _lfoPhase + 0.25;
  if (lfoPhase90 >= 1.0)
    lfoPhase90 -= 1.0;

  // Calculate modulation for right channel
  float modDepthR = _depth * (4.0 * std::fabs(lfoPhase90 - 0.5));  // - 1.0);  
  float modDelayTimeR = (_delay + modDepthR) * 0.0001;
  float modDelaySamplesR = std::round(modDelayTimeR * _sampleRate);

  for (int i = 0; i < _numVoices; ++i) {
    int _readIndex = fmod(_writeIndex + _delayLineSize - modDelaySamplesR,
			  _delayLineSize);

    float delayedSample = _delayLinesRight[i][_readIndex];
    _delayLinesRight[i][_writeIndex] = filteredInput + delayedSample * _feedback;
    outputR += delayedSample;
  }

  // Normalize output
  output[0] = outputL / _numVoices;
  output[1] = outputR / _numVoices;

  // Apply stereo width
  apply_stereo_width(output);

  // Update LFO
  _lfoPhase += _rate / _sampleRate;
  if (_lfoPhase >= 1.0)
    _lfoPhase -= 1.0;

  _writeIndex = (_writeIndex + 1) % _delayLineSize;
}


// Apply stereo width using a simple panning function
void Chorus::apply_stereo_width(float *output)
{
  float panL = std::cos(0.5 * M_PI * _stereoWidth);
  float panR = std::sin(0.5 * M_PI * _stereoWidth);

  float mid = 0.5f * (output[0] + output[1]);
  float side = 0.5f * (output[0] - output[1]);

  output[0] = panL * mid + panR * side;
  output[1] = panR * mid - panL * side;
}

}
