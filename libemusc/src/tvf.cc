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
// frequency, and envelope values are modifiers to this base frequency. The
// significance of the TVF Envelope on the cutoff frequency is controlled by
// the TVF Envelope Depth parameter.

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

  _envDepth = _LUT.TVFEnvDepth[_instPartial.TVFEnvDepth];

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
void TVF::apply(double *sample)
{
  // Skip filter calculation if filter is disabled for this partial 
  if (_bqFilter == nullptr)
    return;

  float envFreq = 0.0;
  if (_envelope) {
    envFreq = (_envelope->get_next_value() - 0x40) * _envDepth * 0.01;
  }

  float midiKey = _coFreqIndex;// + envelopeKey;

  float keyFollow = 0;
  // TODO: These are only approximations, find and use non-linear curves (S?)
  if (_instPartial.TVFCFKeyFlwC == 0 ||
      _instPartial.TVFCFKeyFlwC == 3 ||
      (_instPartial.TVFCFKeyFlwC == 1 && _key > 60))
    keyFollow = ((_instPartial.TVFCFKeyFlw - 0x40) / 10.0) * (_key - 60);
  else if (_instPartial.TVFCFKeyFlwC == 2 && _key > 60)
    keyFollow = ((_instPartial.TVFCFKeyFlw - 0x40) / 100.0) * (_key - 60);

  // The LFO modulation does not make a smooth curve if used with LUT
   float lfo1ModFreq =_LFO1->value()*_LUT.LFOTVFDepth[_accLFO1Depth]/100000.0;
   float lfo2ModFreq =_LFO2->value()*_LUT.LFOTVFDepth[_accLFO2Depth]/100000.0;
   midiKey += lfo1ModFreq + lfo2ModFreq;

   if (midiKey < 0)   midiKey = 0;
   if (midiKey > 127) midiKey = 127;

  // TVF cutoff frequency
  float filterFreq = _interpolate_lut(_LUT.TVFCutoffFreq, midiKey + keyFollow) / 4.3;

  // TESTING ENVELOPE VALUES: Hypothesis: Envelope is calculated in diff Hz
  filterFreq += envFreq;


  // Limit biquad filter frequencies
  if (filterFreq > 12500)
    filterFreq = 12500;
  else if (filterFreq < 35)
    filterFreq = 35;


  /* TO BE DELETED: LFO calculated with separate exp-function
  // Calculate TVF LFO depth separately to get smooth curve and modify freq
  float lfo1ModFreq = _LFO1->value() * _LUT.LFOTVFDepth[_accLFO1Depth] / 1024.0;
  float lfo2ModFreq = _LFO2->value() * _LUT.LFOTVFDepth[_accLFO2Depth] / 1024.0;
  filterFreq *= exp((lfo1ModFreq + lfo2ModFreq) / 1200.0);
  */


  // Filter resonance
  // Output of LUT is in the range of 255 - 106
  // Assuming filter range (Q) of 0.5 to 10
  int resonance = _LUT.TVFResonance[_resIndex];
  float filterRes = 10.0 - (static_cast<float>(resonance - 106) * (9.6)) / 149;

  _bqFilter->calculate_coefficients(filterFreq, filterRes);
  *sample = _bqFilter->apply(*sample);

  if (0) {  // Debug
    static int a = 0;
    if (a++ % 10000 == 0)
      std::cout << "E=" << envFreq
                << " C=" << _coFreqIndex
                << " D=" << (int) _instPartial.TVFEnvDepth
                << " R=" << resonance << "=" << filterRes
                << " k=" << midiKey + keyFollow
                << " cF=" << filterFreq
                << " FD=" << _LUT.TVFEnvDepth[_instPartial.TVFEnvDepth]
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
  _accLFO1Depth = (_instPartial.TVFLFO1Depth & 0x7f) +
    _settings->get_param(PatchParam::Acc_LFO1TVFDepth, _partId);
  if (_accLFO1Depth < 0) _accLFO1Depth = 0;
  if (_accLFO1Depth > 127) _accLFO1Depth = 127;

  _accLFO2Depth = (_instPartial.TVFLFO2Depth & 0x7f) +
    _settings->get_param(PatchParam::Acc_LFO2TVFDepth, _partId);
  if (_accLFO2Depth < 0) _accLFO2Depth = 0;
  if (_accLFO2Depth > 127) _accLFO2Depth = 127;

  _coFreqIndex = _instPartial.TVFBaseFlt +
    (_settings->get_param(PatchParam::TVFCutoffFreq, _partId) - 0x40) * 2;
  if (_coFreqIndex < 0) _coFreqIndex = 0;
  if (_coFreqIndex > 127) _coFreqIndex = 127;

  _resIndex = _instPartial.TVFResonance +
    (_settings->get_param(PatchParam::TVFResonance, _partId) - 0x40) * 2;
  if (_resIndex < 0) _resIndex = 0;
  if (_resIndex > 127) _resIndex = 127;
}


float TVF::_interpolate_lut(std::array<int, 128> &lut, float pos)
{
  if (pos < 0 || pos >= 128)
    return std::nanf("");

  int p0 = lut[(int) pos];
  int p1 = lut[(int) (pos + 1) % 128];

  float fractionP1 = pos - (int) pos;
  float fractionP0 = 1.0 - fractionP1;

  return fractionP0 * p0 + fractionP1 * p1;
}


void TVF::_init_envelope(void)
{
  double phaseLevel[5];
  uint8_t phaseDuration[5];

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

  _envelope = new Envelope(phaseLevel, phaseDuration, phaseShape, _key, _LUT,
                           _settings, _partId, Envelope::Type::TVF, 0x40);

  // Adjust time for Envelope Time Key Follow including Envelope Time Key Preset
  if (_instPartial.TVAETKeyF14 != 0x40)
    _envelope->set_time_key_follow(0, _instPartial.TVFETKeyF14 - 0x40);

  if (_instPartial.TVAETKeyF5 != 0x40)
    _envelope->set_time_key_follow(1, _instPartial.TVFETKeyF5 - 0x40);
}

}
