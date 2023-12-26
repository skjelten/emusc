/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2023  Håkon Skjelten
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
// Based on Kitrinx's research it seems like the static base filter is replaced
// by the TVF envelope when present, and not combined as is quite common.
// Base filter is reportedly ranging from ~500Hz at 1, to 'fully open' at 127.
// It is also noted that the SC-55 seemingly suffer from hardware malfunction
// if both the base filer and TVF envelope is set to 0 for a partial.

// Based on the owner's manuals and online reviews is seems like the SC-55 and
// SC-88 are based on low-pass filter only in their TVF.

// There seems to be three possible scenarios for TVF in partials:
// 1: No filter at all (TVF output = input)
// 2: Low-pass filter with fixed cutoff frequency and resonance
// 3: Time variant low-pass filter (TVF) following a dedicated filter envelope

// TVF Cutoff Frequency relative change (NRPN / SysEx) has steps of 100 cents.
// TVF filter frequency in ROM, both for static base filter and envelope, seems
// to be specified as note numbers.

// TODO:
// Add support for LFO input
// Fix resonance calculation
// A lot of fine tuning++


#include "tvf.h"

#include <cmath>
#include <iostream>
#include <string.h>

namespace EmuSC {


TVF::TVF(ControlRom::InstPartial &instPartial, uint8_t key, Settings *settings,
	 int8_t partId)
  : _sampleRate(settings->get_param_uint32(SystemParam::SampleRate)),
    _settings(settings),
    _partId(partId),
    _lpFilter(NULL),
    _ahdsr(NULL),
    _instPartial(instPartial)
{
  // If TVF base filter frequency is set to 0 in ROM, TVF is completely disabled
  if (instPartial.TVFBaseFlt == 0)
    return;

  _lpFilter = new LowPassFilter(_sampleRate);

  // If TVF envelope phase durations are all 0 we only have a static filter
  // TODO: Verify that this is correct - what to do when P1-5 value != 0?
  if ((instPartial.TVFDurP1 + instPartial.TVFDurP2 + instPartial.TVFDurP3 +
       instPartial.TVFDurP4 + instPartial.TVFDurP5) == 0)
    return;

  // Create filter envelope
  double phaseLevelInit;     // Initial level (frequency) before phase 1
  double phaseLevel[5];      // Phase level for phase 1-5
  uint8_t phaseDuration[5];   // Phase duration for phase 1-5

  // Set adjusted values for frequencies (0-127) and time (seconds)
  phaseLevelInit =instPartial.TVFLvlInit;
  phaseLevel[0] = instPartial.TVFLvlP1;
  phaseLevel[1] = instPartial.TVFLvlP2;
  phaseLevel[2] = instPartial.TVFLvlP3;
  phaseLevel[3] = instPartial.TVFLvlP4;
  phaseLevel[4] = instPartial.TVFLvlP5;

  phaseDuration[0] = instPartial.TVFDurP1 & 0x7F;
  phaseDuration[1] = instPartial.TVFDurP2 & 0x7F;
  phaseDuration[2] = instPartial.TVFDurP3 & 0x7F;
  phaseDuration[3] = instPartial.TVFDurP4 & 0x7F;
  phaseDuration[4] = instPartial.TVFDurP5 & 0x7F;

  std::string id = "TVF (" + std::to_string(instPartial.partialIndex) + ")";

  _ahdsr = new AHDSR(phaseLevelInit, phaseLevel, phaseDuration, settings, partId, id);
  _ahdsr->start();

  if (0)
    std::cout << "TVF (" << (int) key << "): freq=" << _lpBaseFrequency
	      << " res=" << _lpResonance << std::endl;
}


TVF::~TVF()
{
  if (_ahdsr)
    delete _ahdsr;

  if (_lpFilter)
    delete _lpFilter;
}


double TVF::apply(double input)
{
  // Skip filter calculation if filter is disabled for this partial 
  if (_instPartial.TVFBaseFlt == 0)
    return input;

  int coFreq = _settings->get_param(PatchParam::TVFCutoffFreq, _partId) - 0x40;
  int tvfRes = _settings->get_param(PatchParam::TVFResonance, _partId) - 0x40;

  int noteFreq;
  float filterFreq;
  float filterRes;

  if (_ahdsr)
    noteFreq = _ahdsr->get_next_value() + coFreq;
  else
    noteFreq = _instPartial.TVFBaseFlt + coFreq;

  filterFreq = 440.0 * (double) exp(((float) noteFreq - 69) / 12);

  // Resonance. NEEDS FIXING
  // resonance = _lpResonance + sRes * 0.02;
  filterRes = 0.5 + tvfRes * 0.1;            // Logaritmic?
  if (filterRes < 0.5) filterRes = 0.5;

  if (0) {
    static int skip = 10;
    if (!skip--) {
      std::cout << '\r'
		<< "TVF: coFreq=" << coFreq
		<< " note=" << noteFreq
		<< " tvfRes=" << tvfRes
		<< " filterFreq=" << filterFreq
		<< " filterRes(Q)=" << filterRes
		<< std::flush;
      skip = 10;
    }
  }

  _lpFilter->calculate_coefficients(filterFreq, filterRes);

  return _lpFilter->apply(input);
}


void TVF::note_off()
{
  if (_ahdsr)
    _ahdsr->release();
}

}
