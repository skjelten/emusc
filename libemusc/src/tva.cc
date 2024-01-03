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


#include "tva.h"

#include <cmath>
#include <iostream>
#include <string.h>

namespace EmuSC {


TVA::TVA(ControlRom::InstPartial &instPartial, uint8_t key, Settings *settings,
	 int8_t partId)
  : _settings(settings),
    _partId(partId),
    _sampleRate(settings->get_param_uint32(SystemParam::SampleRate)),
    _LFO1(_sampleRate),
    _LFO2(_sampleRate),
    _finished(false),
    _ahdsr(NULL)
{
  // TODO: Figure out how the sine wave for amplitude modulation is found on the
  //       Sound Canvas. In the meantime utilize a simple wavetable with 5Hz.
  _tremoloBaseFreq = 5;

  // TODO: Find LUT or formula for using amplitude LFO Depth. For now just
  //       using a static approximation.
  _LFO1DepthPartial = (instPartial.TVALFODepth & 0x7f) / 128.0;

  // TVA (volume) envelope
  double  phaseVolume[5];        // Phase volume for phase 1-5
  uint8_t phaseDuration[5];      // Phase duration for phase 1-5
  bool    phaseShape[5];         // Phase shape for phase 1-5

  // Set adjusted values for volume (0-127) and time (seconds)
  phaseVolume[0] = _convert_volume(instPartial.TVAVolP1);
  phaseVolume[1] = _convert_volume(instPartial.TVAVolP2);
  phaseVolume[2] = _convert_volume(instPartial.TVAVolP3);
  phaseVolume[3] = _convert_volume(instPartial.TVAVolP4);
  phaseVolume[4] = 0;

  phaseDuration[0] = instPartial.TVALenP1 & 0x7F;
  phaseDuration[1] = instPartial.TVALenP2 & 0x7F;
  phaseDuration[2] = instPartial.TVALenP3 & 0x7F;
  phaseDuration[3] = instPartial.TVALenP4 & 0x7F;
  phaseDuration[4] = instPartial.TVALenP5 & 0x7F;
  phaseDuration[4] *= (instPartial.TVALenP5 & 0x80) ? 2 : 1;

  phaseShape[0] = (instPartial.TVALenP1 & 0x80) ? 0 : 1;
  phaseShape[1] = (instPartial.TVALenP2 & 0x80) ? 0 : 1;
  phaseShape[2] = (instPartial.TVALenP3 & 0x80) ? 0 : 1;
  phaseShape[3] = (instPartial.TVALenP4 & 0x80) ? 0 : 1;
  phaseShape[4] = (instPartial.TVALenP5 & 0x80) ? 0 : 1;

  std::string id = "TVA (" + std::to_string(instPartial.partialIndex) + ")";

  _ahdsr = new AHDSR(phaseVolume, phaseDuration, phaseShape, key, settings, partId, id);
  _ahdsr->start();
}


TVA::~TVA()
{
  if (_ahdsr)
    delete _ahdsr;
}


double TVA::_convert_volume(uint8_t volume)
{
  double res = (0.1 * pow(2.0, (double)(volume) / 36.7111) - 0.1);
  if (res > 1)
    res = 1;
  else if (res < 0)
    res = 0;

  return res;
}


double TVA::get_amplification(void)
{
  // LFO1
  float freq = _tremoloBaseFreq +
    (_settings->get_param(PatchParam::Acc_LFO1RateControl, _partId)-0x40) * 0.1;
  if (freq > 0)
    _LFO1.set_frequency(freq);
  else
    _LFO1.set_frequency(0);
  int LFO1DepthParam = _LFO1DepthPartial +
    _settings->get_param(PatchParam::Acc_LFO1TVADepth, _partId);
  float lfo1Depth = LFO1DepthParam * 0.005;         // TODO: Find correct factor
  if (lfo1Depth < 0) lfo1Depth = 0;

  // LFO2
  freq =
    (_settings->get_param(PatchParam::Acc_LFO2RateControl, _partId)-0x40) * 0.1;
  if (freq > 0)
    _LFO2.set_frequency(freq);
  else
    _LFO2.set_frequency(0);
  int LFO2DepthParam = _settings->get_param(PatchParam::Acc_LFO2TVADepth,
					    _partId);
  float lfo2Depth = LFO2DepthParam * 0.005;         // TODO: Find correct factor
  if (lfo2Depth < 0) lfo2Depth = 0;

  // Tremolo
  double tremolo = (_LFO1.next_sample() * lfo1Depth) +
    (_LFO2.next_sample() * lfo2Depth);

  // Volume envelope
  double volEnvelope = 0;
  if (_ahdsr)
    volEnvelope = _ahdsr->get_next_value();

  return tremolo + volEnvelope;
}


void TVA::note_off()
{
  if (_ahdsr)
    _ahdsr->release();
  else
    _finished = true;
}


bool TVA::finished(void)
{
  if (_ahdsr)
    return _ahdsr->finished();

  return _finished;
}

}
