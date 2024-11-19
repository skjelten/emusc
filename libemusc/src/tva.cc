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

// Make Clang compiler happy
constexpr std::array<float, 128> TVA::_convert_volume_LUT;


TVA::TVA(ControlRom::InstPartial &instPartial, uint8_t key,
         ControlRom::Sample *ctrlSample,WaveGenerator *LFO1,WaveGenerator *LFO2,
         Settings *settings, int8_t partId, int instVolAtt)
  : _sampleRate(settings->get_param_uint32(SystemParam::SampleRate)),
    _LFO1(LFO1),
    _LFO2(LFO2),
    _key(key),
    _drumSet(settings->get_param(PatchParam::UseForRhythm, partId)),
    _panpotLocked(false),
    _ahdsr(NULL),
    _finished(false),
    _instPartial(instPartial),
    _settings(settings),
    _partId(partId),
    _drumVol(1)
{
  _set_static_params(ctrlSample, instVolAtt);
  update_dynamic_params(true);

  // All instruments must have a TVA envelope defined
  _init_envelope();
  if (_ahdsr)
    _ahdsr->start();
}


TVA::~TVA()
{
  delete _ahdsr;
}


void TVA::apply(double *sample)
{
  // Tremolo
  double tremolo = (_LFO1->value() * 0.01 * (float) _accLFO1Depth) +
    (_LFO2->value() * 0.01 * (float) _accLFO2Depth);

  // Volume envelope
  double volEnvelope = 0;
  if (_ahdsr)
    volEnvelope = _ahdsr->get_next_value();

  // Apply all volume corrections
  sample[0] *= _staticVolCorr * _drumVol * _ctrlVol;
  sample[0] *= tremolo + volEnvelope;

  // Panpot
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


// TODO: Figure out how often this update is supposed to happen (new thread?)
void TVA::update_dynamic_params(bool reset)
{
  // Update LFO inputs
  int newLFODepth = (_instPartial.TVALFO1Depth & 0x7f) +
    _settings->get_param(PatchParam::Acc_LFO1TVADepth, _partId);
  if (newLFODepth < 0) newLFODepth = 0;
  if (newLFODepth > 127) newLFODepth = 127;
  _accLFO1Depth = newLFODepth;

  newLFODepth = (_instPartial.TVALFO2Depth & 0x7f) +
    _settings->get_param(PatchParam::Acc_LFO2TVADepth, _partId);
  if (newLFODepth < 0) newLFODepth = 0;
  if (newLFODepth > 127) newLFODepth = 127;
  _accLFO2Depth = newLFODepth;

  // Drum volume is dynamic
  if (_drumSet)
    _drumVol = _convert_volume_LUT[_settings->get_param(DrumParam::Level,
                                                        _drumSet - 1, _key)];

  // Accumulated volume control from controllers
  _ctrlVol =
    _settings->get_param(PatchParam::Acc_AmplitudeControl, _partId) / 64.0;

  // Update panpot if not locked in random mode
  if (_panpotLocked)
    return;

  int newPanpot = _instPartial.panpot +
    _settings->get_param(PatchParam::PartPanpot, _partId) +
    _settings->get_param(SystemParam::Pan) - 0x80;

  // Drum panpot is also dynamic
  if (_drumSet)
    newPanpot +=
      _settings->get_param(DrumParam::Panpot, _drumSet - 1, _key) - 0x40;

  if (newPanpot < 0) newPanpot = 0;
  if (newPanpot > 0x7f) newPanpot = 0x7f;

  if (newPanpot == _panpot)
    return;

  if (reset == true) {
    _panpot = newPanpot;

  } else {
    if (newPanpot > _panpot && _panpot < 0x7f)
      _panpot ++;
    else if(newPanpot < _panpot && _panpot > 0)
      _panpot --;
  }
}


bool TVA::finished(void)
{
  if (_ahdsr)
    return _ahdsr->finished();

  return _finished;
}


void TVA::_set_static_params(ControlRom::Sample *ctrlSample, int instVolAtt)
{
  // Calculate static volume corrections
  double instVol = _convert_volume_LUT[instVolAtt];
  double sampleVol = _convert_volume_LUT[ctrlSample->volume +
                                         (ctrlSample->fineVolume -1024)/1000.0];
  double partialVol = _convert_volume_LUT[_instPartial.volume];

  _staticVolCorr = instVol * sampleVol * partialVol;

  // Calculate random pan if part pan or drum pan value is 0 (RND)
  if (_settings->get_param(PatchParam::PartPanpot, _partId) == 0 ||
      (_drumSet &&
       _settings->get_param(DrumParam::Panpot, _drumSet - 1, _key) == 0)) {
    _panpot = std::rand() % 128;
    _panpotLocked = true;
  }
}


void TVA::_init_envelope(void)
{
  double  phaseVolume[5];        // Phase volume for phase 1-5
  uint8_t phaseDuration[5];      // Phase duration for phase 1-5
  bool    phaseShape[5];         // Phase shape for phase 1-5

  // Set adjusted values for volume (0-127) and time (seconds)
  phaseVolume[0] = _convert_volume_LUT[_instPartial.TVAVolP1];
  phaseVolume[1] = _convert_volume_LUT[_instPartial.TVAVolP2];
  phaseVolume[2] = _convert_volume_LUT[_instPartial.TVAVolP3];
  phaseVolume[3] = _convert_volume_LUT[_instPartial.TVAVolP4];
  phaseVolume[4] = 0;

  phaseDuration[0] = _instPartial.TVALenP1 & 0x7F;
  phaseDuration[1] = _instPartial.TVALenP2 & 0x7F;
  phaseDuration[2] = _instPartial.TVALenP3 & 0x7F;
  phaseDuration[3] = _instPartial.TVALenP4 & 0x7F;
  phaseDuration[4] = _instPartial.TVALenP5 & 0x7F;
  phaseDuration[4] *= (_instPartial.TVALenP5 & 0x80) ? 2 : 1;

  phaseShape[0] = (_instPartial.TVALenP1 & 0x80) ? 0 : 1;
  phaseShape[1] = (_instPartial.TVALenP2 & 0x80) ? 0 : 1;
  phaseShape[2] = (_instPartial.TVALenP3 & 0x80) ? 0 : 1;
  phaseShape[3] = (_instPartial.TVALenP4 & 0x80) ? 0 : 1;
  phaseShape[4] = (_instPartial.TVALenP5 & 0x80) ? 0 : 1;

  _ahdsr = new AHDSR(phaseVolume, phaseDuration, phaseShape, _key,
                    _settings, _partId, AHDSR::Type::TVA);
}

}
