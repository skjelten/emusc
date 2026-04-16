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


#include "envelope.h"

#include <iostream>


namespace EmuSC {


Envelope::Envelope(ControlRom::LookupTables &LUT)
  : _finished(false),
    _envelopeOut(0),
    _timeKeyFlwT1T4(256),
    _timeKeyFlwT5(256),
    _timeVelSensT1T2(256),
    _timeVelSensT3T5(256),
    _phase(Phase::Init),
    _LUT(LUT)
{}


Envelope::~Envelope()
{}


void Envelope::set_phase(enum Envelope::Phase newPhase)
{
  if (newPhase != _phase)
    _init_new_phase(newPhase);
}


// etkpROM != 0 is only possible for the TVA envelope
void Envelope::set_time_key_follow(enum Type type, bool phase, int key,
                                   int etkfROM, int etkpROM)
{
  // Skip unnecessary calculations if etkfROM value is 0 => use default values
  if (etkfROM == 0) {
    return;
  }

  int offset;
  if (type == Type::TVA)
    offset = (phase == 0) ? 16 : 32;
  else  // TVF
    offset = (phase == 0) ? 64 : 80;

  int kmIndex = _LUT.KeyMapperIndex[offset + etkpROM] - _LUT.KeyMapperOffset;
  int km = _LUT.KeyMapper[kmIndex + key];
  int tkfSens = _LUT.EnvTimeKeyFollowSens[std::abs(etkfROM)];

  if (etkfROM < 0)
    km = (km == 0) ? 255 : (256 - km) & 0xff;

  int tkfIndex = 128 + ((km - 128) * tkfSens * 2 / 256);

  if (phase == 0)
    _timeKeyFlwT1T4 = _LUT.EnvTimeScale[tkfIndex];
  else
    _timeKeyFlwT5 = _LUT.EnvTimeScale[tkfIndex];

  if (0)
    std::cout << "ETKF: TVA phase=" << std::dec << phase
              << " key=" << (int) key << " offset=" << offset
              << " etkpROM=" << etkpROM
              << " km[" << kmIndex + key << "]=" << km
              << " tkf[" << tkfIndex << "]=" << _LUT.EnvTimeScale[tkfIndex]
              << " => time change=" << _LUT.EnvTimeScale[tkfIndex] / 256.0
              << std::endl;
}


void Envelope::set_time_velocity_sensitivity(enum Type type, bool phase,
                                             int etvsROM, int velocity)
{
  int timeVelSens;
  int tvsDiv = _LUT.EnvTimeKeyFollowSens[std::abs(etvsROM)];
  int divmuliv = tvsDiv * (127 - velocity);

  if (etvsROM < 0) {
    if (divmuliv < 8001)
      timeVelSens = ((8128 - divmuliv) * 2064) >> 16;
    else
      timeVelSens = 4;

  } else if (etvsROM > 0) {
    if ((uint16_t) 0x1fc0 - divmuliv < 0x20)
        timeVelSens = 0xffff;
      else
        timeVelSens = 0x1fc000 / (uint16_t) (0x1fc0 - divmuliv);

  } else {  // etvsROM == 0
    timeVelSens = 256;
  }

  if (phase == 0)
    _timeVelSensT1T2 = timeVelSens;
  else
    _timeVelSensT3T5 = timeVelSens;

  if (0)
    std::cout << "ETVS: phase (0:T1-2 1:T3-5)=" << std::dec << phase
              << " etvsROM=" << etvsROM << " velocity=" << velocity
              << " sensitivity=" << timeVelSens << std::endl;
}

}
