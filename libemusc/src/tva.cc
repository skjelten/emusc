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


#include "tva.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string.h>


namespace EmuSC {

// Make Clang compiler happy
constexpr std::array<float, 128> TVA::_convert_LFO_depth_high_LUT;
constexpr std::array<float, 128> TVA::_convert_LFO_depth_low_LUT;
constexpr std::array<float, 128> TVA::_convert_volume_LUT;


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
    _panpotLocked(false),
    _envelope(NULL),
    _finished(false),
    _instPartial(instPartial),
    _settings(settings),
    _partId(partId),
    _drumVol(1)
{
  _set_static_params(ctrlSample, instVolAtt, velocity);
  update_dynamic_params(true);

  // All instruments must have a TVA envelope defined
  _init_envelope(velocity);
  if (_envelope)
    _envelope->start();
}


TVA::~TVA()
{
  delete _envelope;
}


void TVA::apply(double *sample)
{
  // Apply static volume corrections
  sample[0] *= _staticVolCorr * _drumVol * _ctrlVol;

  // Apply tremolo (LFOs)
  // TVA LFO waveforms needs more work. The sine function is streched? Is it
  // the same "deformation" as seen with triangle?
  float LFO1Mod, LFO2Mod;
  if (1) {
    // Do we have a LUT for this?
    LFO1Mod = 1 + (_LFO1->value() * _accLFO1Depth / (256 * 127 / 3));
    LFO2Mod = 1 + (_LFO2->value() * _accLFO2Depth / (256 * 127 / 3));

    // LFO is limited to max 3 and min 0
    if (LFO1Mod > 3) LFO1Mod = 3;
    if (LFO1Mod < 0) LFO1Mod = 0;
    if (LFO2Mod > 3) LFO2Mod = 3;
    if (LFO2Mod < 0) LFO2Mod = 0;

  } else {
    float high = _convert_LFO_depth_high_LUT[_accLFO1Depth];
    float low =  _convert_LFO_depth_low_LUT[_accLFO1Depth];
    LFO1Mod = 1 + ((_LFO1->value() * _accLFO1Depth / (255*127*2)) *  (high - low) + low );
  }

  sample[0] *= (_instPartial.TVALFO1Depth & 0x80) ? -1 * LFO1Mod : LFO1Mod;
  sample[0] *= (_instPartial.TVALFO1Depth & 0x80) ? -1 * LFO2Mod : LFO2Mod;

  // Apply volume envelope
  if (_envelope)
    sample[0] *= _envelope->get_next_value();

  // Panpot
  sample[1] = sample[0] * (_LUT.TVAPanpot[_panpot] / 127.0);
  sample[0] *= _LUT.TVAPanpot[128 - _panpot] / 127.0;
}


void TVA::note_off()
{
  if (_envelope)
    _envelope->release();
  else
    _finished = true;
}


// TODO: Figure out how often this update is supposed to happen (new thread?)
void TVA::update_dynamic_params(bool reset)
{
  // Update LFO inputs
  _accLFO1Depth = (_instPartial.TVALFO1Depth & 0x7f) +
    _settings->get_param(PatchParam::Acc_LFO1TVADepth, _partId);
  if (_accLFO1Depth < 0) _accLFO1Depth = 0;
  if (_accLFO1Depth > 127) _accLFO1Depth = 127;

  _accLFO2Depth = (_instPartial.TVALFO2Depth & 0x7f) +
    _settings->get_param(PatchParam::Acc_LFO2TVADepth, _partId);
  if (_accLFO2Depth < 0) _accLFO2Depth = 0;
  if (_accLFO2Depth > 127) _accLFO2Depth = 127;

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
  if (_envelope)
    return _envelope->finished();

  return _finished;
}


void TVA::_set_static_params(ControlRom::Sample *ctrlSample, int instVolAtt,
                             uint8_t velocity)
{
  // Calculate static volume corrections
  float instVol = _convert_volume_LUT[instVolAtt];
  float sampleVol = _convert_volume_LUT[ctrlSample->volume +
                                         (ctrlSample->fineVolume -1024)/1000.0];
  float partialVol = _convert_volume_LUT[_instPartial.volume];

  _staticVolCorr = instVol * sampleVol * partialVol;

  // Set level from bias
  _staticVolCorr *= 1.0 - _get_bias_level();

  // Set level from velocity
  _staticVolCorr *= _get_velocity_from_vcurve(velocity) * (1/127.0);

  // Calculate random pan if part pan or drum pan value is 0 (RND)
  if (_settings->get_param(PatchParam::PartPanpot, _partId) == 0 ||
      (_drumSet &&
       _settings->get_param(DrumParam::Panpot, _drumSet - 1, _key) == 0)) {
    _panpot = std::rand() % 128;
    _panpotLocked = true;
  }
}


float TVA::_get_bias_level(void)
{
  // Note: The SC-55 uses a lookup table for finding the index when TVA Bias
  // Point = 1 (biasKey = _LUT.TVABiasPoint1[_key]). Consider to use this LUT.
  int biasKey = 0;
  if (_instPartial.TVABiasPoint == 0)
    biasKey = ((_instPartial.TVABiasLevel - 0x40)/ 10.0) * std::abs(_key - 64);
  else if (_instPartial.TVABiasPoint == 1 && _key >= 85)
    biasKey = ((_instPartial.TVABiasLevel - 0x40) / 10.0) * (_key - 84);
  else if (_instPartial.TVABiasPoint == 2)
    biasKey = std::abs(_instPartial.TVABiasLevel - 0x40) * 6.35;

  int biasLevel = _LUT.TVABiasLevel[std::abs(biasKey)];

  // Cancel out "the wrong side" of bias point
  bool positiveBias = (_instPartial.TVABiasLevel > 0x40) ? true : false;
  if (_instPartial.TVABiasPoint == 0) {
    if (!positiveBias && _key < 64)
      biasLevel = 0;
    else if (positiveBias && _key > 64)
      biasLevel = 0;
  } else if (_instPartial.TVABiasPoint == 1) {
    if (positiveBias || _key < 85)
      biasLevel = 0;
  } else if (_instPartial.TVABiasPoint == 2 && _instPartial.TVABiasLevel >0x40){
    biasLevel = 0;
  }

  return _convert_volume_LUT[(biasLevel/73.0)*127];
}


float TVA::_get_velocity_from_vcurve(uint8_t velocity)
{
  int vLevel;
  switch(_instPartial.TVALvlVelCur)
    {
    case 0:
      vLevel = _LUT.VelocityCurve0[velocity];
      break;
    case 1:
      vLevel = _LUT.VelocityCurve1[velocity];
      break;
    case 2:
      vLevel = _LUT.VelocityCurve2[velocity];
      break;
    case 3:
      vLevel = _LUT.VelocityCurve3[velocity];
      break;
    case 4:
      vLevel = _LUT.VelocityCurve4[velocity];
      break;
    case 5:
      vLevel = _LUT.VelocityCurve5[velocity];
      break;
    case 6:
      vLevel = _LUT.VelocityCurve6[velocity];
      break;
    case 7:
      vLevel = _LUT.VelocityCurve7[velocity];
      break;
    case 8:
      vLevel = _LUT.VelocityCurve8[velocity];
      break;
    case 9:
      vLevel = _LUT.VelocityCurve9[velocity];
      break;
    default:
      vLevel = 0;
      std::cerr << "libEmuSC internal error: Illegal velocity curve used"
                << std::endl;
    }

  return vLevel;
}


void TVA::_init_envelope(uint8_t velocity)
{
  double  phaseVolume[6];
  uint8_t phaseDuration[6];
  bool    phaseShape[6];

  phaseVolume[0] = 0;
  phaseVolume[1] = _convert_volume_LUT[_instPartial.TVAEnvL1];
  phaseVolume[2] = _convert_volume_LUT[_instPartial.TVAEnvL2];
  phaseVolume[3] = _convert_volume_LUT[_instPartial.TVAEnvL3];
  phaseVolume[4] = _convert_volume_LUT[_instPartial.TVAEnvL4];
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
  if (_instPartial.TVAETKeyF14 != 0x40)
    _envelope->set_time_key_follow(0, _instPartial.TVAETKeyF14 - 0x40,
                                   _instPartial.TVAETKeyP14);

  if (_instPartial.TVAETKeyF5 != 0x40)
    _envelope->set_time_key_follow(1, _instPartial.TVAETKeyF5 - 0x40,
                                   _instPartial.TVAETKeyP5);

  // Adjust time for Envelope Time Velocity Sensitivity
  float taVelSens = 1;
  if (_instPartial.TVAETVSens14 < 0x40) {
    taVelSens = -((_instPartial.TVAETVSens14 - 0x40) * velocity) / 1270.0 +
      1 + ((_instPartial.TVAETVSens14 - 0x40) / 10.0);
  } else if (_instPartial.TVAETVSens14 > 0x40) {
    std::cerr << "Positive TVA envelope V-sensitivity not implemented yet"
              << std::endl;
  }

  _envelope->set_time_vel_sens_t1_t4(taVelSens);

  taVelSens = 1;
  if (_instPartial.TVAETVSens5 < 0x40) {
    taVelSens = -((_instPartial.TVAETVSens5 - 0x40) * velocity) / 1270.0 +
      1 + ((_instPartial.TVAETVSens5 - 0x40) / 10.0);
  } else if (_instPartial.TVAETVSens5 > 0x40) {
    std::cerr << "Positive TVA envelope V-sensitivity not implemented yet"
              << std::endl;
  }
  _envelope->set_time_vel_sens_t5(taVelSens);


}

}
