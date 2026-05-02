/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2026  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 2.1 of the License, or
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
// Delay and Equalizer was first introduced to the System Effects in the SC-88
// model and therefore not part of libEmuSC.


#include "system_effects.h"

#include <algorithm>
#include <cmath>
#include <iostream>


namespace EmuSC {


SystemEffects::SystemEffects(Settings *settings)
  : _settings(settings),
    _chorus(NULL),
    _reverb(NULL)
{
  _chorus = new Chorus(settings);
  _reverb = new Reverb(settings);

  update();
}


SystemEffects::~SystemEffects()
{
  delete _chorus;
  delete _reverb;
}


// System Effects always produce 2 channel & 32kHz (native) output.
int SystemEffects::apply(std::array<std::array<float, 256>, 2> &chorusBus,
			 std::array<std::array<float, 256>, 2> &reverbBus,
			 std::array<std::array<float, 256>, 2> &chorusOut,
			 std::array<std::array<float, 256>, 2> &reverbOut)
{
  for (int i = 0; i < 256; i ++) {
    // Apply chorus if active
    if (_chorusLevel) {
      float cSample[2] = { 0, 0 };
      float cInput = ((chorusBus[0][i] + chorusBus[1][i]) / 2);

      _chorus->process_sample(cInput, cSample);

      chorusOut[0][i] = cSample[0] * (_chorusLevel / 128.0);
      chorusOut[1][i] = cSample[1] * (_chorusLevel / 128.0);

    } else {
      chorusOut[0][i] = 0.0;
      chorusOut[1][i] = 0.0;
    }

    // Apply reverb if active
    if (_reverbLevel) {
      float rSample[2] = { 0, 0 };
      float rInput[2];
      rInput[0] = rInput[1] = ((reverbBus[0][i] + reverbBus[1][i]) / 2) +
	((chorusBus[0][i] + chorusBus[1][i]) / 2) *
	(float) _chorusSendLevelToReverb / 128.0;

      _reverb->process_sample(rInput, rSample);

      reverbOut[0][i] = rSample[0] * (_reverbLevel / 128.0);
      reverbOut[1][i] = rSample[1] * (_reverbLevel / 128.0);

    } else {
      reverbOut[0][i] = 0.0;
      reverbOut[1][i] = 0.0;
    }
  }

  return 0;
}


void SystemEffects::update(void)
{
  _chorusLevel = _settings->get_param(PatchParam::ChorusLevel);
  _chorusSendLevelToReverb=_settings->get_param(PatchParam::ChorusSendToReverb);

  _reverbLevel = _settings->get_param(PatchParam::ReverbLevel);
}

}
