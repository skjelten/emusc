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

// TVF - Time Variant Filter
// The Sound Canvas is using a 2nd. order low or high pass filter for TVF.
// The "TVF Type" variable in partial definitions specifies the filter type or
// whether the TVF filter is disabled.

// If TVF filter is enabled, the "TVF Cutoff Frequency" sets the base cutoff
// frequency, and envelope values are modifiers to this base frequency. Note
// that the initial envelope value is a common factor for the amount of effect
// for the 5 phases.

// TVF cutoff frequency relative change (NRPN / SysEx) has steps of 100 cents.


#include "tvf.h"
#include "lowpass_filter2.h"
#include "highpass_filter2.h"

#include <cmath>
#include <iostream>
#include <string.h>


namespace EmuSC {


TVF::TVF(ControlRom::InstPartial &instPartial, uint8_t key, uint8_t velocity,
	 WaveGenerator *LFO1, WaveGenerator *LFO2,
	 Settings *settings, int8_t partId)
  : _sampleRate(settings->sample_rate()),
    _LFO1(LFO1),
    _LFO2(LFO2),
    _key(key),
    _envelope(NULL),
    _instPartial(instPartial),
    _settings(settings),
    _partId(partId),
    _logSemitoneRatio(log(2.0) / 12.0)
{
  if (instPartial.TVFType == 0)
    _bqFilter = new LowPassFilter2(_sampleRate);
  else if (instPartial.TVFType == 1)
    _bqFilter = new HighPassFilter2(_sampleRate);
  else
    _bqFilter = nullptr;

  if (_bqFilter != nullptr) {
    _init_envelope();
    _envelope->start();

    update_params();
  }
}


TVF::~TVF()
{
  delete _envelope;
  delete _bqFilter;
}


// TODO: Add suport for the following features:
// * Cutoff freq key follow
// * Cutoff freq V-sens
// * TVF envelope
// * TVF LFO modulation
// * TVF Resonance
void TVF::apply(double *sample)
{
  // Skip filter calculation if filter is disabled for this partial 
  if (_bqFilter == nullptr)
    return;

  float envelopeDiff = 0.0;
  if (_envelope) {
    int envelope = _envelope->get_next_value() - 0x40;
    envelopeDiff = ((float) _instPartial.TVFLvlInit / 64.0) * envelope;
  }

  float midiKey = _instPartial.TVFBaseFlt + _coFreq;// + envelopeDiff;
  if (midiKey < 0)   midiKey = 0;
  if (midiKey > 127) midiKey = 127;

  float filterFreq = 440.0 * exp(_logSemitoneRatio * (midiKey - 69));

  // Key follow: f(p) = p/10 * ((key - 60) / 12) ??
  // Turns out that TVF filter key follow also have a non-linear curve
//float keyFollowST = ((_instPartial.TVFCFKeyFlw - 0x40) / 10.0) * (_key - 60);
//float newFilterFreq = filterFreq + pow(2.0, keyFollowST / 12.0);

  // FIXME: LFO modulation temporarily disabled due to clipping on instruments
  //        with high LFOx TVF depth. Needs to find out how to properly apply
  //        filter effect.
  //				     (_LFO1->value() * _accLFO1Depth * 0.26) +
//				     (_LFO2->value() * _accLFO2Depth * 0.26)) / 12);

  // Resonance. NEEDS FIXING
  // resonance = _lpResonance + sRes * 0.02;
  float filterRes = 1; //(_res / 64.0) * 0.5;            // Logaritmic?
  if (filterRes < 0.01) filterRes = 0.01;

  _bqFilter->calculate_coefficients(filterFreq, filterRes);
  *sample = _bqFilter->apply(*sample);
}


void TVF::note_off()
{
  if (_envelope)
    _envelope->release();
}


void TVF::update_params(void)
{
  // Update LFOs
  int newLFODepth = (_instPartial.TVFLFO1Depth & 0x7f) +
    _settings->get_param(PatchParam::Acc_LFO1TVFDepth, _partId);
  if (newLFODepth < 0) newLFODepth = 0;
  if (newLFODepth > 127) newLFODepth = 127;
  _accLFO1Depth = newLFODepth;

  newLFODepth = (_instPartial.TVFLFO2Depth & 0x7f) +
    _settings->get_param(PatchParam::Acc_LFO2TVFDepth, _partId);
  if (newLFODepth < 0) newLFODepth = 0;
  if (newLFODepth > 127) newLFODepth = 127;
  _accLFO2Depth = newLFODepth;

  _coFreq = _settings->get_param(PatchParam::TVFCutoffFreq, _partId) - 0x40;
  _res = _settings->get_param(PatchParam::TVFResonance, _partId);
}


void TVF::_init_envelope(void)
{
  double phaseLevel[5];
  uint8_t phaseDuration[5];

  // Set adjusted values for frequencies (0-127) and time (seconds)
  phaseLevel[0] = _instPartial.TVFLvlP1;
  phaseLevel[1] = _instPartial.TVFLvlP2;
  phaseLevel[2] = _instPartial.TVFLvlP3;
  phaseLevel[3] = _instPartial.TVFLvlP4;
  phaseLevel[4] = _instPartial.TVFLvlP5;

  phaseDuration[0] = _instPartial.TVFDurP1 & 0x7F;
  phaseDuration[1] = _instPartial.TVFDurP2 & 0x7F;
  phaseDuration[2] = _instPartial.TVFDurP3 & 0x7F;
  phaseDuration[3] = _instPartial.TVFDurP4 & 0x7F;
  phaseDuration[4] = _instPartial.TVFDurP5 & 0x7F;

  bool phaseShape[5] = { 0, 0, 0, 0, 0 };

  _envelope = new Envelope(phaseLevel, phaseDuration, phaseShape, _key,
			   _settings, _partId, Envelope::Type::TVF, 0x40);
}

}
