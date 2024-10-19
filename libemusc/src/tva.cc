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


#include "tva.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string.h>


namespace EmuSC {


TVA::TVA(ControlRom::InstPartial &instPartial, uint8_t key,
	 WaveGenerator *LFO[2], Settings *settings, int8_t partId)
  : _sampleRate(settings->get_param_uint32(SystemParam::SampleRate)),
    _key(key),
    _drumSet(settings->get_param(PatchParam::UseForRhythm, partId)),
    _panpotLocked(false),
    _ahdsr(NULL),
    _finished(false),
    _instPartial(instPartial),
    _settings(settings),
    _partId(partId)
{
  _LFO1 = LFO[0];
  _LFO2 = LFO[1];

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

  _ahdsr = new AHDSR(phaseVolume, phaseDuration, phaseShape, key,
		     settings, partId, AHDSR::Type::TVA);
  _ahdsr->start();

  // Calculate random panpot if part or drum panpot value is 0 (RND)
  if (_settings->get_param(PatchParam::PartPanpot, partId) == 0 ||
      (_drumSet > 0 &&
       _settings->get_param(DrumParam::Panpot, _drumSet - 1, key) == 0)) {
    _panpot = std::rand() % 128;                     // TODO: Add srand
    _panpotLocked = true;
  } else {
    _update_panpot(false);
  }
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


// TODO:
// Supposed to run approx. 100 times a second to give proper transition speed
// Use a common timer in Partial? TVF & TVP has similar updates? Other TVA val?
void TVA::_update_panpot(bool fade)
{
  if (_panpotLocked)
    return;

  int newPanpot = _instPartial.panpot +
    _settings->get_param(PatchParam::PartPanpot, _partId) +
    _settings->get_param(SystemParam::Pan) - 0x80;

  // If partial is in a drum set we also need to add the drum's panpot setting
  if (_drumSet > 0)
    newPanpot +=
      _settings->get_param(DrumParam::Panpot, _drumSet - 1, _key) - 0x40;

  if (newPanpot < 0) newPanpot = 0;
  if (newPanpot > 0x7f) newPanpot = 0x7f;

  if (newPanpot == _panpot)
    return;

  if (fade == false) {
    _panpot = newPanpot;

  } else {
    if (newPanpot > _panpot && _panpot < 0x7f)
      _panpot ++;
    else if(newPanpot < _panpot && _panpot > 0)
      _panpot --;
  }
}


void TVA::apply(double *sample)
{
  // LFO1
  int LFO1DepthParam = _LFO1DepthPartial +
    _settings->get_param(PatchParam::Acc_LFO1TVADepth, _partId);
  float lfo1Depth = LFO1DepthParam * 0.005;         // TODO: Find correct factor
  if (lfo1Depth < 0) lfo1Depth = 0;

  // LFO2
  int LFO2DepthParam = _settings->get_param(PatchParam::Acc_LFO2TVADepth,
					    _partId);
  float lfo2Depth = LFO2DepthParam * 0.005;         // TODO: Find correct factor
  if (lfo2Depth < 0) lfo2Depth = 0;

  // Tremolo
  double tremolo = (_LFO1->value() * lfo1Depth) + (_LFO2->value() * lfo2Depth);

  // Volume envelope
  double volEnvelope = 0;
  if (_ahdsr)
    volEnvelope = _ahdsr->get_next_value();

  sample[0] *= tremolo + volEnvelope;

  // Panpot
  if (!(_updateTimeout++ % 300))
    _update_panpot();

  sample[1] = sample[0];
  if (_panpot > 64)
    sample[0] *= 1.0 - (_panpot - 64) / 63.0;
  else if (_panpot < 64)
    sample[1] *= ((_panpot - 1) / 64.0);
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
