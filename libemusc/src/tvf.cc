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

// If TVF filter is enabled, the "TVF Cutoff Frequency" in partial definition
// sets the base cutoff frequency, and envelope values are modifiers to this
// base frequency. The significance of the TVF Envelope on the cutoff frequency
// is controlled by the TVF Envelope Depth parameter.

// Cutoff Freq Key Follow scales filter freq with LUT index = "ROM / 10" keys.
// The Cutoff Freq Key Follow Direction has 4 modes:
//   0 => Adjust all keys with center on key 64
//   1 => Only adjust keys > C4
//   2 => Only adjust keys > C7
//   3 => Only adjust keys < C7

// To calculate the filter resonance there are two calculations needed:
//  * Read resonance value from partial def. in ROM and add 2x SysEx value
//  * Calculate the correct index and read the value from LUT.TVFResonanceFreq
// Whichever of the two values are lowest will be used for calculating the
// resonance from the LUT.Resonance lookup table.


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

  if (_bqFilter == nullptr)                       // TVF disabled
    return;

  _velocity = _get_velocity_from_vcurve(velocity);
  _keyFollow = _get_cof_key_follow(_instPartial.TVFCFKeyFlw - 0x40);
  _coFreqVSens = _read_cutoff_freq_vel_sens(_instPartial.TVFCOFVSens - 0x40);
  _envDepth = _LUT.TVFEnvDepth[_instPartial.TVFEnvDepth];

  _init_envelope();
  _init_freq_and_res();

  _envelope->start();
  update_params();
}


TVF::~TVF()
{
  delete _envelope;
  delete _bqFilter;
}


// TODO: Add suport for Cutoff freq V-sens
void TVF::apply(double *sample)
{
  // Skip filter calculation if filter is disabled for this partial 
  if (_bqFilter == nullptr)
    return;

  // Envelope
  float envKey = 0.0;
  if (_envelope)
    envKey = (_envelope->get_next_value() - 0x40) * _envDepth * 0.0001 * 0.62;

  // LFO modulation
  float lfo1ModFreq = _LFO1->value_float() * _LUT.LFOTVFDepth[_accLFO1Depth] / 256;
  float lfo2ModFreq = _LFO2->value_float() * _LUT.LFOTVFDepth[_accLFO2Depth] / 256;

  float freqIndex = _coFreqIndex + lfo1ModFreq + lfo2ModFreq + envKey +
                    (_keyFollow >> 8); // + _coFreqVSens;
  if (_instPartial.TVFCOFVSens < 0x40)
    freqIndex -= 73.5;
  if (freqIndex < 0)   freqIndex = 0;
  if (freqIndex > 127) freqIndex = 127;

  _filterFreq = _calc_cutoff_frequency(freqIndex);

  _bqFilter->calculate_coefficients(_filterFreq / 4.3, _filterRes);
  *sample = _bqFilter->apply(*sample);

  if (0)
    std::cout << "E=" << envKey
              << " C=" << _coFreqIndex
              << " V=" << _coFreqVSens
              << " K=" << (_keyFollow >> 8)
              << " M=" << lfo1ModFreq + lfo2ModFreq
              << " D=" << _envDepth
              << " I=" << freqIndex << "->[" << freqIndex * 2 << "]"
              << " F=" << _filterFreq << " (" << _filterFreq / 4.3 << "Hz)"
              << " R=" << _filterRes
              << std::endl;
}


void TVF::note_off()
{
  if (_envelope)
    _envelope->release();
}


// TODO: Re-add support for controller input
void TVF::update_params(void)
{
  if (_bqFilter == nullptr)                       // TVF disabled
    return;

  _accLFO1Depth = (_instPartial.TVFLFO1Depth & 0x7f);
  // + _settings->get_param(PatchParam::Acc_LFO1TVFDepth, _partId);
  if (_accLFO1Depth < 0) _accLFO1Depth = 0;
  if (_accLFO1Depth > 127) _accLFO1Depth = 127;

  _accLFO2Depth = (_instPartial.TVFLFO2Depth & 0x7f);
  // + _settings->get_param(PatchParam::Acc_LFO2TVFDepth, _partId);
  if (_accLFO2Depth < 0) _accLFO2Depth = 0;
  if (_accLFO2Depth > 127) _accLFO2Depth = 127;

  _coFreqIndex = _instPartial.TVFBaseFlt +
    (_settings->get_param(PatchParam::TVFCutoffFreq, _partId) - 0x40) * 2;
  if (_coFreqIndex < 0) _coFreqIndex = 0;
  if (_coFreqIndex > 127) _coFreqIndex = 127;

  int resIndexUsed = std::min(_resIndexROM, _resIndexFreq);
  resIndexUsed -=
    (_settings->get_param(PatchParam::TVFResonance, _partId) - 0x40) * 2;
  if (resIndexUsed > _resIndexFreq) resIndexUsed = _resIndexFreq;
  if (resIndexUsed < 8) resIndexUsed = 8;

  if (resIndexUsed > 128) {
    std::cerr << "libEmuSC internal error: Invalid TVF Resonance LUT index"
              << std::endl;
    resIndexUsed = 128;
  }

  int resonance = _LUT.TVFResonance[resIndexUsed];
  _filterRes = ((resonance - 106) / (255.0f - 106.0f)) * (10.0f - 0.5f) + 0.5f;

  if (0)
    std::cout << "TVF: fFreq=" << _filterFreq
              << " fRes=" << _filterRes
              << " res[" << resIndexUsed << "]=" << resonance << std::endl;
}


void TVF::_init_envelope(void)
{
  float phaseLevel[6];
  uint8_t phaseDuration[6];

  phaseLevel[0] = 0x40;
  phaseLevel[1] = _instPartial.TVFEnvL1;
  phaseLevel[2] = _instPartial.TVFEnvL2;
  phaseLevel[3] = _instPartial.TVFEnvL3;
  phaseLevel[4] = _instPartial.TVFEnvL4;
  phaseLevel[5] = _instPartial.TVFEnvL5;

  phaseDuration[0] = 0;                                     // Never used
  phaseDuration[1] = _instPartial.TVFEnvT1 & 0x7F;
  phaseDuration[2] = _instPartial.TVFEnvT2 & 0x7F;
  phaseDuration[3] = _instPartial.TVFEnvT3 & 0x7F;
  phaseDuration[4] = _instPartial.TVFEnvT4 & 0x7F;
  phaseDuration[5] = _instPartial.TVFEnvT5 & 0x7F;

  bool phaseShape[6] = { 0, 0, 0, 0, 0, 0 };

  _envelope = new Envelope(phaseLevel, phaseDuration, phaseShape, _key, _LUT,
                           _settings, _partId, Envelope::Type::TVF);

  // Adjust time for Envelope Time Key Follow including Envelope Time Key Preset
  _envelope->set_time_key_follow(0, _instPartial.TVFETKeyF14 - 0x40,
                                 _instPartial.TVFETKeyFP14);
  _envelope->set_time_key_follow(1, _instPartial.TVFETKeyF5 - 0x40,
                                 _instPartial.TVFETKeyFP5);

  // Adjust time for Envelope Time Velocity Sensitivity
  _envelope->set_time_velocity_sensitivity(0, _instPartial.TVFETVSens12 - 0x40,
                                           _velocity);
  _envelope->set_time_velocity_sensitivity(1, _instPartial.TVFETVSens35 - 0x40,
                                           _velocity);
}


int TVF::_get_velocity_from_vcurve(uint8_t velocity)
{
  unsigned int address = _instPartial.TVFCOFVelCur * 128 + velocity;
  if (address > _LUT.VelocityCurves.size()) {
    std::cerr << "libEmuSC internal error: Illegal velocity curve used"
              << std::endl;
    return 0;
  }

  return _LUT.VelocityCurves[address];
}


int TVF::_read_cutoff_freq_vel_sens(int cofvsROM)
{
  int v = 127 - _velocity;
  int res = 0x7fff;
  if (cofvsROM != 0)
    res -= ((v * _LUT.TVFCutoffVSens[std::abs(cofvsROM)]) & 0xffff);

  return res;
}


int TVF::_get_level_init(int level)
{
  int depth = (_envDepth * _coFreqVSens) * 2;
  int scale = _LUT.TVFEnvScale[std::abs(level - 0x40)];
  int tmp = (scale * ((depth & 0xffff0000) >> 16)) * 2;
  int res = ((tmp & 0x0000ff00) >> 8) + ((tmp & 0x00ff0000) >> 8);

  if (level >= 0x40)
    res += _keyFollow;
  else
    res = _keyFollow - res;

  return res;
}


void TVF::_init_freq_and_res(void)
{
  _L1Init = _get_level_init(_instPartial.TVFEnvL1);
  _L2Init = _get_level_init(_instPartial.TVFEnvL2);
  _L3Init = _get_level_init(_instPartial.TVFEnvL3);
  _L4Init = _get_level_init(_instPartial.TVFEnvL4);
  _L5Init = _get_level_init(_instPartial.TVFEnvL5);

  int envLevelMax = _keyFollow;
  envLevelMax = std::max(envLevelMax, _L1Init);
  envLevelMax = std::max(envLevelMax, _L2Init);
  envLevelMax = std::max(envLevelMax, _L3Init);
  envLevelMax = std::max(envLevelMax, _L4Init);
  envLevelMax = std::max(envLevelMax, _L5Init);

  int baseFilter = _instPartial.TVFBaseFlt;

  // TODO: Fix the proper calculation of TVFCutoffFreq from settings
  // It is -not- a linear function, but depends on several unknown factors
  int var = (_keyFollow & 0xff00) +
    _settings->get_param(PatchParam::TVFCutoffFreq, _partId) - 0x40;

  if (var < 0) {
    baseFilter -= (var & 0x00ff);

    if (_instPartial.TVFBaseFlt < var)
      baseFilter = 0; //&= 0xff00;

  } else {
    if (!(_instPartial.TVFFlags & (1 << 2))) {
      if ((var & 0x00ff) < 0x10) {
	var = (var & 0xff00) | 0x10;

	baseFilter += var;
	if (baseFilter < 0)
	  baseFilter = 0x7f;
      }
    }
  }

  int16_t cofIndex = envLevelMax + (baseFilter << 8);
  if (envLevelMax < 0 && cofIndex < 0)
    cofIndex = 0;
  else if (envLevelMax >= 0 && cofIndex < 0)
    cofIndex = 0x7fff;

  cofIndex += 0xff;
  if (cofIndex < 0)
    cofIndex = 0x7fff;

  cofIndex = (cofIndex >> 8);
  _filterFreq = _LUT.TVFCutoffFreq[cofIndex];

  int rfIndex = (2 * _filterFreq) + 0xff;
  if (rfIndex > 0xffff)
    rfIndex = 0xff00;

  rfIndex &= 0xff00;
  // Then find the two filter resonance index alternatives
  _resIndexFreq = _LUT.TVFResonanceFreq[(rfIndex >> 8)];
  _resIndexROM = (int) _instPartial.TVFResonance;

  if (0)
    std::cout << "Init TVF: COFFreq[" << cofIndex * 2 << "]=" << _filterFreq
              << " ResIndexFreq[" << (rfIndex >> 8) << "]=" << _resIndexFreq
              << " ResIndexROM=" << _resIndexROM << std::endl;
}


// Implementation based on disassembly of the CPUROM binary code. There must be
// some function later in the processing to extract the decimal part (LSB).
int TVF::_get_cof_key_follow(int cofkfROM)
{
  int kmIndex = _LUT.KeyMapperIndex[48 + _instPartial.TVFCFKeyFlwC] - _LUT.KeyMapperOffset;
  int km = static_cast<int>(_native_endian_uint16((uint8_t *) &_LUT.KeyMapper[kmIndex + _key * 2]));
  int cofkf = _LUT.TVFCutoffFreqKF[std::abs(cofkfROM)];
  int res = ((km - 0x4000) * cofkf) >> 8;

  if (0)
    std::cout << "km=0x" << std::hex << km
	      << " cofkfROM=" << std::dec << cofkfROM
	      << " mulxu.w=0x" << std::hex << ((km - 0x4000) * cofkf)
	      << " res=" << res << std::dec
	      << std::endl;

  if (cofkfROM < 0)
    return  -res;

  return res;
}


float TVF::_calc_cutoff_frequency(float index)
{
  if (index < 0)
    return std::nanf("");
  else if (index >= 127)
    return (float) _LUT.TVFCutoffFreq[127];

  int p0 = _LUT.TVFCutoffFreq[(int) index];
  int p1 = _LUT.TVFCutoffFreq[(int) index + 1];

  float fractionP1 = index - (int) index;
  float fractionP0 = 1.0 - fractionP1;

  return fractionP0 * p0 + fractionP1 * p1;
}


uint16_t TVF::_native_endian_uint16(uint8_t *ptr)
{
  if (_le_native())
    return (ptr[0] << 8 | ptr[1]);

  return (ptr[1] << 8 | ptr[0]);
}

}
