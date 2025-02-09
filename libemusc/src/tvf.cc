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
         WaveGenerator *LFO1, WaveGenerator *LFO2,ControlRom::LookupTables &LUT,
         Settings *settings, int8_t partId)
  : _sampleRate(settings->sample_rate()),
    _LFO1(LFO1),
    _LFO2(LFO2),
    _LUT(LUT),
    _key(key),
    _envelope(NULL),
    _instPartial(instPartial),
    _settings(settings),
    _partId(partId)
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
// * Cutoff freq V-sens
// * TVF envelope
// * TVF Resonance
void TVF::apply(double *sample)
{
  // Skip filter calculation if filter is disabled for this partial 
  if (_bqFilter == nullptr)
    return;

  float envelopeKey = 0.0;
  float diff;
  if (_envelope) {
    diff = _envelope->get_next_value() - _instPartial.TVFBaseFlt;

    // Asuming the static rate of increase is 1 / 100
    envelopeKey = diff * (float) _instPartial.TVFLvlInit * 0.0125;
  }

  float midiKey = _instPartial.TVFBaseFlt + _coFreq + envelopeKey;
  if (midiKey < 0)   midiKey = 0;
  if (midiKey > 127) midiKey = 127;

  float keyFollow = 0;
  // TODO: These are only approximations, find and use non-linear curves (S?)
  if (_instPartial.TVFCFKeyFlwC == 0 ||
      _instPartial.TVFCFKeyFlwC == 3 ||
      (_instPartial.TVFCFKeyFlwC == 1 && _key > 60))
    keyFollow = ((_instPartial.TVFCFKeyFlw - 0x40) / 10.0) * (_key - 60);
  else if (_instPartial.TVFCFKeyFlwC == 2 && _key > 60)
    keyFollow = ((_instPartial.TVFCFKeyFlw - 0x40) / 100.0) * (_key - 60);

  // The LFO modulation does not make a smooth curve if used with LUT
  // float lfo1ModFreq =_LFO1->value()*_LUT.LFOTVFDepth[_accLFO1Depth]/100000.0;
  // float lfo2ModFreq =_LFO2->value()*_LUT.LFOTVFDepth[_accLFO2Depth]/100000.0;
  // midiKeyCutoffFreq += lfo1ModFreq + lfo2ModFreq;

  // TVF cutoff frequency
  float filterFreq = _LUT.TVFCutoffFreq[(int) (midiKey + keyFollow)] / 4.3;

  // Calculate TVF LFO depth separately to get smooth curve and modify freq
  float lfo1ModFreq = _LFO1->value() * _LUT.LFOTVFDepth[_accLFO1Depth] / 1024.0;
  float lfo2ModFreq = _LFO2->value() * _LUT.LFOTVFDepth[_accLFO2Depth] / 1024.0;
  filterFreq *= exp((lfo1ModFreq + lfo2ModFreq) / 1200.0);

  // Filter resonance
  // TODO: Figure out the exact range and function for the resonance
  // resonance = _lpResonance + sRes * 0.02;
  int resonance = _instPartial.TVFResonance + _res - 0x40;
  float filterRes = exp(2.3 * ((resonance - 64) / 64.0));
  if (filterRes < 0.01) filterRes = 0.01;

  _bqFilter->calculate_coefficients(filterFreq, filterRes);
  *sample = _bqFilter->apply(*sample);


  if (0) {  // Debug
    static int a = 0;
    if (a++ % 10000 == 0)
      std::cout << "E=" << _envelope->get_current_value()
                << " C=" << (int) _instPartial.TVFBaseFlt
                << " D=" << diff
                << " I=" << (int) _instPartial.TVFLvlInit
                << " K=" << envelopeKey
                << " R=" << resonance << "=" << filterRes
                << " k=" << midiKey + keyFollow
                << " cF=" << filterFreq
                << " LF=" << filterFreq / 4.3
                << std::endl;
  }
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
			   _LUT.envelopeTime, _settings, _partId,
                           Envelope::Type::TVF, 0x40);


  // Adjust envelope phase durations based on TVF Envelope Time Key Follow
  // and TVF Envelope Time Velocity Sensitivity.

  // Adjust time for T1 - T4 (Attacks and Decays)
  if (_instPartial.TVFETKeyF14 != 0x40) {
    int tkfDiv = _LUT.TimeKeyFollowDiv[std::abs(_instPartial.TVFETKeyF14-0x40)];
    if (_instPartial.TVFETKeyF14 < 0x40)
      tkfDiv *= -1;
    int tkfIndex = ((tkfDiv * (_key - 64)) / 64) + 128;
    _envelope->set_time_key_follow_t1_t4(_LUT.TimeKeyFollow[tkfIndex]);

    if (0)   // Debug output TVF Envelope Time Key Follow T1-T4
      std::cout << "TVF ETKF T1-T4: key=" << (int) _key
                << " LUT1[" << (int) _instPartial.TVFETKeyF14 - 0x40
                << "]=" << tkfDiv << " LUT2[" << tkfIndex
                << "]=" << _LUT.TimeKeyFollow[tkfIndex]
                << " => time change=" << _LUT.TimeKeyFollow[tkfIndex] / 256.0
                << "x" << std::endl;
  }

  // Adjust time for T5 (Release)
  if (_instPartial.TVFETKeyF5 != 0x40) {
    int tkfDiv = _LUT.TimeKeyFollowDiv[std::abs(_instPartial.TVFETKeyF5 -0x40)];
    if (_instPartial.TVFETKeyF5 < 0x40)
      tkfDiv *= -1;
    int tkfIndex = ((tkfDiv * (_key - 64)) / 64) + 128;
    _envelope->set_time_key_follow_t5(_LUT.TimeKeyFollow[tkfIndex]);
  }
}

}
