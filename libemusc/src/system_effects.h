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


#ifndef __SYSTEM_EFFECTS_H__
#define __SYSTEM_EFFECTS_H__


#include "chorus.h"
#include "reverb.h"
#include "settings.h"

#include <stdint.h>


namespace EmuSC {

class SystemEffects
{
public:
  SystemEffects(Settings *settings, uint8_t id);
  ~SystemEffects();

  int apply(float *sample);
  void update_params(void);

private:
  Settings *_settings;
  const uint8_t _partId;
  
  Chorus *_chorus;
  Reverb *_reverb;

  uint8_t _chorusLevel;
  uint8_t _chorusSendLevel;
  uint8_t _chorusSendLevelToReverb;

  uint8_t _reverbLevel;
  uint8_t _reverbSendLevel;

  float _cLevel;
  float _rLevel;
};

}

#endif  // __SYSTEM_EFFECTS_H__
