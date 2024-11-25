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

// NOTES:
// The SC-55 & SC-88 are using 2nd. order lowpass filter for TVF.

// There are three possible scenarios for TVF in partials:
// 1: No filter defined (base filter = 0 & no envelope). TVF output = input.
// 2: Static filter with fixed cutoff frequency and resonance (no envelope)
// 3: TVF based on base filter and modified by filter envelope

// TVF cutoff frequency relative change (NRPN / SysEx) has steps of 100 cents.
// TVF filter frequency in ROM, both for static base filter and envelope, seems
// to be specified as note numbers.

// TODO:
// Figure out correct values and scale for resonance
// Figure out correct frequency scaling for init value and envelope
// Figure out correct absolute values for cutoff frequencies
// Find & implement TVF key follow


#include "tvf.h"

#include <cmath>
#include <iostream>
#include <string.h>


namespace EmuSC {


TVF::TVF(ControlRom::InstPartial &instPartial, uint8_t key, WaveGenerator *LFO1,
	 WaveGenerator *LFO2, Settings *settings, int8_t partId)
  : _sampleRate(settings->sample_rate()),
    _LFO1(LFO1),
    _LFO2(LFO2),
    _key(key),
    _envelope(NULL),
    _lpFilter(NULL),
    _instPartial(instPartial),
    _settings(settings),
    _partId(partId)
{
  // If any of the TVF envelope phase duraions != 0 we have an envelope to run
  if (!(!instPartial.TVFDurP1 && !instPartial.TVFDurP2 && !instPartial.TVFDurP3
	&& !instPartial.TVFDurP4 && !instPartial.TVFDurP5)) {
    _init_envelope();
    if (_envelope)
      _envelope->start();

  } else if (instPartial.TVFBaseFlt == 0) {
    // If no envelope nor base filter is specified the entire TVF is disabled
    return;
  }

  update_params();

  _lpFilter = new LowPassFilter2(_sampleRate);
}


TVF::~TVF()
{
  if (_envelope)
    delete _envelope;

  if (_lpFilter)
    delete _lpFilter;
}


void TVF::apply(double *sample)
{
  // Skip filter calculation if filter is disabled for this partial 
  if (_instPartial.TVFBaseFlt == 0)
    return;

  int noteFreq;
  float filterFreq;
  float filterRes;

  float envelopeDiff = 0.0;
  if (_envelope) {
    int envelope = _envelope->get_next_value() - 0x40;
    envelopeDiff = ((float) _instPartial.TVFLvlInit / 64.0) * envelope;
  }

  float note = _instPartial.TVFBaseFlt + _coFreq + envelopeDiff;
  if (note > 127)
    note = 127;
  else if (note < 0)
    note = 0;

  noteFreq = 25 + note * 0.66;  // FIXME: Figure out scaling + TVF key follow
  filterFreq = 440.0 * (double) exp((((float) noteFreq - 69))/12);// +

  // FIXME: LFO modulation temporarily disabled due to clipping on instruments
  //        with high LFOx TVF depth. Needs to find out how to properly apply
  //        filter effect.
  //				     (_LFO1->value() * _accLFO1Depth * 0.26) +
//				     (_LFO2->value() * _accLFO2Depth * 0.26)) / 12);

  // Resonance. NEEDS FIXING
  // resonance = _lpResonance + sRes * 0.02;
  filterRes = (_res / 64.0) * 0.5;            // Logaritmic?
  if (filterRes < 0.01) filterRes = 0.01;

  _lpFilter->calculate_coefficients(filterFreq, filterRes);

  *sample = _lpFilter->apply(*sample);
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
