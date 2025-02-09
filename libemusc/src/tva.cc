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
  _set_static_params(ctrlSample, instVolAtt);
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
  sample[1] = sample[0];
  if (_panpot > 64)
    sample[0] *= 1.0 - (_panpot - 64) / 63.0;
  else if (_panpot < 64)
    sample[1] *= ((_panpot - 1) / 64.0);
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


void TVA::_init_envelope(uint8_t velocity)
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

  phaseShape[0] = (_instPartial.TVALenP1 & 0x80) ? 0 : 1;
  phaseShape[1] = (_instPartial.TVALenP2 & 0x80) ? 0 : 1;
  phaseShape[2] = (_instPartial.TVALenP3 & 0x80) ? 0 : 1;
  phaseShape[3] = (_instPartial.TVALenP4 & 0x80) ? 0 : 1;
  phaseShape[4] = (_instPartial.TVALenP5 & 0x80) ? 0 : 1;

  _envelope = new Envelope(phaseVolume, phaseDuration, phaseShape, _key,
                           _LUT, _settings, _partId, Envelope::Type::TVA);

  // Adjust envelope phase durations.
  // The Sound Canvas has three parameters for tuning the TVA durations:
  // - TVA Envelope Time Key Presets
  // - TVA Envelope Time Key Follow
  // - TVA Envelope Time Velocity Sensitivity

  // Adjust time for T1 - T4 (Attacks & Decays)

  // TODO: Fix TVA Envelope Time Key Presets
  int key;
  if (_instPartial.TVAETKeyP14 == 1)
    key = 127 - _key;
  else if (_instPartial.TVAETKeyP14 == 2)
    key = 127;
  else
    key = _key;

  if (_instPartial.TVAETKeyF14 != 0x40) {
    int tkfDiv = _LUT.TimeKeyFollowDiv[std::abs(_instPartial.TVAETKeyF14-0x40)];
    if (_instPartial.TVAETKeyF14 < 0x40)
      tkfDiv *= -1;
    int tkfIndex = ((tkfDiv * (_key - 64)) / 64) + 128;
    _envelope->set_time_key_follow_t1_t4(_LUT.TimeKeyFollow[tkfIndex]);

    if (0)   // Debug output TVA Envelope Time Key Follow T1-T4
      std::cout << "TVA ETKF T1-T4: key=" << (int) _key
                << " LUT1[" << (int) _instPartial.TVAETKeyF14 - 0x40
                << "]=" << tkfDiv << " LUT2[" << tkfIndex
                << "]=" << _LUT.TimeKeyFollow[tkfIndex]
                << " => time change=" << _LUT.TimeKeyFollow[tkfIndex] / 256.0
                << "x" << std::endl;
  }

  float taVelSens = 1;
  if (_instPartial.TVAETVSens14 < 0x40) {
    taVelSens = -((_instPartial.TVAETVSens14 - 0x40) * velocity) / 1270.0 +
      1 + ((_instPartial.TVAETVSens14 - 0x40) / 10.0);
  } else if (_instPartial.TVAETVSens14 > 0x40) {
    std::cerr << "Positive TVA envelope V-sensitivity not implemented yet"
              << std::endl;
  }

  _envelope->set_time_vel_sens_t1_t4(taVelSens);

  // Adjust time for T5 (Release)
  if (_instPartial.TVAETKeyP5 == 1)
    key = 127 - _key;
  else if (_instPartial.TVAETKeyP5 == 2)
    key = 127;
  else
    key = _key;

  if (_instPartial.TVAETKeyF5 != 0x40) {
    int tkfDiv = _LUT.TimeKeyFollowDiv[std::abs(_instPartial.TVAETKeyF5 -0x40)];
    if (_instPartial.TVAETKeyF5 < 0x40)
      tkfDiv *= -1;
    int tkfIndex = ((tkfDiv * (_key - 64)) / 64) + 128;
    _envelope->set_time_key_follow_t5(_LUT.TimeKeyFollow[tkfIndex]);
  }

  taVelSens = 1;
  if (_instPartial.TVAETVSens5 < 0x40) {
    taVelSens = -((_instPartial.TVAETVSens5 - 0x40) * velocity) / 1270.0 +
      1 + ((_instPartial.TVAETVSens5 - 0x40) / 10.0);
  } else if (_instPartial.TVAETVSens5 > 0x40) {
    std::cerr << "Positive TVA envelope V-sensitivity not implemented yet"
              << std::endl;
  }
  _envelope->set_time_vel_sens_t5(taVelSens);

  // Adjust level
  // TBD
}

}
