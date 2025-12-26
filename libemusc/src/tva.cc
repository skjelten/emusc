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

// TVA: Controlling volume changes and stereo positioning over time
//
// To find the TVA envelope levels (volume), the following parameters are used:
//  - Partial level
//  - Bias level
//  - Velocity
//  - Sample level
//  - Instrument level
//  - Envelope levels in ROM (L1 - L4)
// These parameters are calculated by using lookup tables, and the sum is used
// in another lookup table to find the correct envelope output levels.
//
// Pan (stereo position) is also using a lookup table to find the correct level
// for each channel (left and right).


#include "tva.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string.h>


namespace EmuSC {


TVA::TVA(ControlRom::InstPartial &instPartial, uint8_t key, uint8_t velocity,
         ControlRom::Sample *ctrlSample,WaveGenerator *LFO1,WaveGenerator *LFO2,
         ControlRom::LookupTables &LUT, Settings *settings, int8_t partId,
         int instVolAtt)
  : _sampleRate(settings->sample_rate()),
    _LFO1(LFO1),
    _LFO2(LFO2),
    _LUT(LUT),
    _key(key),
    _drumSet(settings->get_param(PatchParam::UseForRhythm, partId)),
    _panpot(-1),
    _panpotLocked(false),
    _envelope(NULL),
    _finished(false),
    _instPartial(instPartial),
    _settings(settings),
    _partId(partId)
{
  int cVelocity = _get_velocity_from_vcurve(velocity);

  // Calculate accumulated level (volume) indexes
  int levelIndex =
    _get_bias_level(_instPartial.TVABiasPoint) +
    _LUT.TVALevelIndex[_instPartial.volume] +
    _LUT.TVALevelIndex[cVelocity] +
    _LUT.TVALevelIndex[ctrlSample->volume] +
    _LUT.TVALevelIndex[instVolAtt];

  // All instruments must have a TVA envelope defined
  _init_envelope(levelIndex, cVelocity);
  if (_envelope)
    _envelope->start();

  // Calculate random pan if part pan or drum pan value is 0 (RND)
  // A note is locked to RND if it is started with that setting
  if (settings->get_param(PatchParam::PartPanpot, _partId) == 0 ||
      (_drumSet &&
       settings->get_param(DrumParam::Panpot, _drumSet - 1, _key) == 0)) {
    _panpot = std::rand() % 128;
    _panpotL = _LUT.TVAPanpot[_panpot];
    _panpotR = _LUT.TVAPanpot[0x80 - _panpot];
    _panpotLocked = true;
  }

  update_dynamic_params(true);
}


TVA::~TVA()
{
  delete _envelope;
}


void TVA::apply(double *sample)
{
  // Apply volume correction from dynamic parameters including LFOs
  sample[0] *= _dynLevel / 65535.0;

  // Apply volume envelope (includes also static corrections)
  if (_envelope)
    sample[0] *= _envelope->get_next_value();

  // Apply pan
  sample[1] = sample[0] * (_panpotL / 127.0);
  sample[0] *= (_panpotR / 127.0);
}


void TVA::note_off()
{
  if (_envelope)
    _envelope->release();
  else
    _finished = true;
}


// Run regularly for every 256 samples @32k sample rate => 125Hz
void TVA::update_dynamic_params(bool reset)
{
  // 1: Read expression, Part level and System level
  _dynLevel = _settings->get_param(PatchParam::Expression, _partId) *
    _settings->get_param(PatchParam::PartLevel, _partId) *
    _settings->get_param(SystemParam::Volume) * 4;
  _dynLevel = ((_dynLevel >> 8) & 0xffff);

  // 2: Add corrections, different between normal instruments and drums
  if (_drumSet) {
    _dynLevel *= _settings->get_param(DrumParam::Level, _drumSet - 1, _key);
    _dynLevel = (((_dynLevel * 2) >> 8) & 0xffff);
    _dynLevel = ((_dynLevel * 0x830e * 2) >> 16);
  } else {
    _dynLevel = ((_dynLevel * 0x8208 * 2) >> 16);
  }

  // If volume level is 0 at this point we want to force no sound
  if (_dynLevel == 0)
    return;

  // 3: Add accumulated controller values for Amplitude (they are already
  //    adjusted for direct addition)
  _dynLevel += _settings->get_acc_control_param(Settings::ControllerParam::Amplitude, _partId);
  if (_dynLevel < 0) _dynLevel = 0;

  // 4: Add LFO1 and LFO2, including accumulated controller value for TVA Depth
  int result = std::abs((_instPartial.TVALFO1Depth << 8)) +
    std::abs(_settings->get_acc_control_param(Settings::ControllerParam::LFO1TVADepth, _partId));
  if (result > 0x7f00) result = 0x7f00;

  int modulation = _LFO1->value() >= 0 ?
    (static_cast<int>(_LFO1->value()) * result + 0x7fff) >> 15 :
    (static_cast<int>(_LFO1->value()) * result - 0x7fff) >> 15;
  _dynLevel += modulation;
  if (_dynLevel < 0) _dynLevel = 0;

  result = std::abs((_instPartial.TVALFO2Depth << 8)) +
    std::abs(_settings->get_acc_control_param(Settings::ControllerParam::LFO2TVADepth, _partId));
  if (result > 0x7f00) result = 0x7f00;

  modulation = _LFO2->value() >= 0 ?
    (static_cast<int>(_LFO2->value()) * result + 0x7fff) >> 15 :
    (static_cast<int>(_LFO2->value()) * result - 0x7fff) >> 15;
  _dynLevel += modulation;
  if (_dynLevel < 0) _dynLevel = 0;

  // 5. Add corrections
  _dynLevel = ((_dynLevel * _dynLevel) >> 16) * 0x208;
  if ((_dynLevel & 0xffff0000) > 0xff0000)
    _dynLevel = 0xffff;
  else
    _dynLevel >>= 8;

  // 6. Fade-in, up to max 8 samples
  // Have not been able to trigger this effect. Disabled? Ignored for now.

  // 7. Pan calculations
  if (_panpotLocked)               // Do not update panpot if in random mode
    return;

  int newPanpot = _instPartial.panpot +
    _settings->get_param(PatchParam::PartPanpot, _partId) +
    _settings->get_param(SystemParam::Pan) - 0x80;

  if (_drumSet)
    newPanpot +=
      _settings->get_param(DrumParam::Panpot, _drumSet - 1, _key) - 0x40;

  newPanpot = std::clamp(newPanpot, 0, 0x7f);

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

  _panpotL = _LUT.TVAPanpot[_panpot];
  _panpotR = _LUT.TVAPanpot[0x80 - _panpot];
}


bool TVA::finished(void)
{
  if (_envelope)
    return _envelope->finished();

  return _finished;
}


// Exact calculation found by disassembling the CPUROM
int TVA::_get_bias_level(int biasPoint)
{
  int kmIndex = _LUT.KeyMapperIndex[0 + biasPoint] - _LUT.KeyMapperOffset;
  int km = _LUT.KeyMapper[kmIndex + _key];
  int kfDiv = _LUT.TimeKeyFollowDiv[std::abs(_instPartial.TVABiasLevel - 0x40)];
  int biasLevelIndex = ((std::abs(km - 128) * kfDiv) * 2) >> 8;

  return _LUT.TVABiasLevel[biasLevelIndex];
}


int TVA::_get_velocity_from_vcurve(uint8_t velocity)
{
  unsigned int address = _instPartial.TVALvlVelCur * 128 + velocity;
  if (address > _LUT.VelocityCurves.size()) {
    std::cerr << "libEmuSC internal error: Illegal velocity curve used"
              << std::endl;
    return 0;
  }

  return _LUT.VelocityCurves[address];
}


void TVA::_init_envelope(int levelIndex, uint8_t velocity)
{
  float   phaseVolume[6];
  uint8_t phaseDuration[6];
  bool    phaseShape[6];

  int envL1Index = levelIndex + _LUT.TVALevelIndex[_instPartial.TVAEnvL1];
  int envL2Index = levelIndex + _LUT.TVALevelIndex[_instPartial.TVAEnvL2];
  int envL3Index = levelIndex + _LUT.TVALevelIndex[_instPartial.TVAEnvL3];
  int envL4Index = levelIndex + _LUT.TVALevelIndex[_instPartial.TVAEnvL4];

  phaseVolume[0] = 0;
  phaseVolume[1] = _LUT.TVALevel[std::max(255 - envL1Index, 0)] / 255.0;
  phaseVolume[2] = _LUT.TVALevel[std::max(255 - envL2Index, 0)] / 255.0;
  phaseVolume[3] = _LUT.TVALevel[std::max(255 - envL3Index, 0)] / 255.0;
  phaseVolume[4] = _LUT.TVALevel[std::max(255 - envL4Index, 0)] / 255.0;
  phaseVolume[5] = 0;

  phaseDuration[0] = 0;                                     // Never used
  phaseDuration[1] = _instPartial.TVAEnvT1 & 0x7F;
  phaseDuration[2] = _instPartial.TVAEnvT2 & 0x7F;
  phaseDuration[3] = _instPartial.TVAEnvT3 & 0x7F;
  phaseDuration[4] = _instPartial.TVAEnvT4 & 0x7F;
  phaseDuration[5] = _instPartial.TVAEnvT5 & 0x7F;

  phaseShape[0] = 0;                                        // Never used
  phaseShape[1] = (_instPartial.TVAEnvT1 & 0x80) ? 0 : 1;
  phaseShape[2] = (_instPartial.TVAEnvT2 & 0x80) ? 0 : 1;
  phaseShape[3] = (_instPartial.TVAEnvT3 & 0x80) ? 0 : 1;
  phaseShape[4] = (_instPartial.TVAEnvT4 & 0x80) ? 0 : 1;
  phaseShape[5] = (_instPartial.TVAEnvT5 & 0x80) ? 0 : 1;

  _envelope = new Envelope(phaseVolume, phaseDuration, phaseShape, _key,
                           _LUT, _settings, _partId, Envelope::Type::TVA);

  // Adjust time for Envelope Time Key Follow including Envelope Time Key Preset
  _envelope->set_time_key_follow(0, _instPartial.TVAETKeyF14 - 0x40,
                                 _instPartial.TVAETKeyFP14);

  _envelope->set_time_key_follow(1, _instPartial.TVAETKeyF5 - 0x40,
                                 _instPartial.TVAETKeyFP5);

  // Adjust time for Envelope Time Velocity Sensitivity
  _envelope->set_time_velocity_sensitivity(0, _instPartial.TVAETVSens12 - 0x40,
                                           velocity);
  _envelope->set_time_velocity_sensitivity(1, _instPartial.TVAETVSens35 - 0x40,
                                           velocity);
}

}
