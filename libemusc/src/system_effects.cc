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

// All Sound Canvas models (SC-55+) have System Effects consisting of:
//  - Chorus (8 types)
//  - Reverb (8 types)
//
// SC-88+ introduced the additional effects:
//  - Delay (10 types)
//  - Equalizer (2-band)
//
// The SC-88+ also introduced the Insertion Effects, a group of audio effects
// separate from the System Effects.
//
// Only the Chorus and Reverb effects are currently implemented in libemusc.


#include "system_effects.h"

#include <algorithm>
#include <cmath>
#include <iostream>


namespace EmuSC {

SystemEffects::SystemEffects(Settings *settings, uint8_t partId)
  : _settings(settings),
    _partId(partId),
    _chorus(NULL),
    _reverb(NULL)
{
  _chorus = new Chorus(settings);
  _reverb = new Reverb(settings);

  update_params();
}


SystemEffects::~SystemEffects()
{
  delete _chorus;
  delete _reverb;
}


// System Effects always produce 2 channel & 32kHz (native) output. Other
// channel numbers and sample rates are handled by the calling Synth class.
int SystemEffects::apply(float *sample)
{
  float cSample[2] = { 0, 0 };
  float sysEffect[2] = { 0, 0 };

  // Apply chorus system effect if both global & part chorus levels > 0
  if (_chorusLevel && _chorusSendLevel) {
    float cInput = ((sample[0] + sample[1]) / 2) * _cLevel;
    _chorus->process_sample(cInput, cSample);

    // TODO: Need an audio compressor to compensate for additive audio signals
    sysEffect[0] += cSample[0] * (_chorusLevel / 127.0);
    sysEffect[1] += cSample[1] * (_chorusLevel / 127.0);
  }

  // Apply reverb system effect if both global & part reverb levels > 0
  if (_reverbLevel && _reverbSendLevel) {
    float rSample[2] = { 0, 0 };

    // TODO: Figure out if the reverb has mono or stereo input
    // Current guess is mono input and variable delay to create stereo output
    float rInput[2];
    rInput[0] = rInput[1] = ((sample[0] + sample[1]) / 2) * _rLevel +
      ((cSample[0] + cSample[1]) / 2) * (float) _chorusSendLevelToReverb /127.0;

    _reverb->process_sample(rInput, rSample);

    // TODO: Need an audio compressor to compensate for additive audio signals
    sysEffect[0] += rSample[0] * (_reverbLevel / 127.0);
    sysEffect[1] += rSample[1] * (_reverbLevel / 127.0);
  }

  sample[0] += sysEffect[0];
  sample[1] += sysEffect[1];

  return 0;
}


void SystemEffects::update_params(void)
{
  _chorusLevel = _settings->get_param(PatchParam::ChorusLevel, _partId);
  _chorusSendLevel= _settings->get_param(PatchParam::ChorusSendLevel, _partId);
  _chorusSendLevelToReverb= _settings->get_param(PatchParam::ChorusSendToReverb,
                                                 _partId);

  _reverbLevel= _settings->get_param(PatchParam::ReverbLevel, _partId);
  _reverbSendLevel= _settings->get_param(PatchParam::ReverbSendLevel, _partId);

  _cLevel = _chorusSendLevel / 127.0;
  _rLevel = _reverbSendLevel / 127.0;
}

}
