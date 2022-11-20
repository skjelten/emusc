/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022  Håkon Skjelten
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


TVA::TVA(ControlRom::InstPartial instPartial, uint32_t sampleRate)
  : _sampleRate(sampleRate),
    _finished(false),
    _ahdsr(NULL)
{
  // Tremolo
  _LFODepth = instPartial.TVALFODepth;
  _LFOFrequency = 10;                        // FIXME!!
  
  // TVA (volume) envelope
  double phaseVolume[5];        // Phase volume for phase 1-5
  double phaseDuration[5];      // Phase duration for phase 1-5
  bool   phaseShape[5];         // Phase shape for phase 1-5

  // Set adjusted values for volume (0-127) and time (seconds)
  phaseVolume[0] = _convert_volume(instPartial.TVAVolP1);
  phaseVolume[1] = _convert_volume(instPartial.TVAVolP2);
  phaseVolume[2] = _convert_volume(instPartial.TVAVolP3);
  phaseVolume[3] = _convert_volume(instPartial.TVAVolP4);
  phaseVolume[4] = 0;

  phaseDuration[0] = _convert_time_to_sec(instPartial.TVALenP1 & 0x7F);
  phaseDuration[1] = _convert_time_to_sec(instPartial.TVALenP2 & 0x7F);
  phaseDuration[2] = _convert_time_to_sec(instPartial.TVALenP3 & 0x7F);
  phaseDuration[3] = _convert_time_to_sec(instPartial.TVALenP4 & 0x7F);
  phaseDuration[4] = _convert_time_to_sec(instPartial.TVALenP5 & 0x7F); //Fixme
  phaseDuration[4] *= (instPartial.TVALenP5 & 0x80) ? 2 : 1;

  phaseShape[0] = (instPartial.TVALenP1 & 0x80) ? 0 : 1;
  phaseShape[1] = (instPartial.TVALenP2 & 0x80) ? 0 : 1;
  phaseShape[2] = (instPartial.TVALenP3 & 0x80) ? 0 : 1;
  phaseShape[3] = (instPartial.TVALenP4 & 0x80) ? 0 : 1;
  phaseShape[4] = (instPartial.TVALenP5 & 0x80) ? 0 : 1;

  _ahdsr = new AHDSR(phaseVolume, phaseDuration, phaseShape, sampleRate);
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


double TVA::_convert_time_to_sec(uint8_t time)
{
  if (time == 0)
    return 0;

  return (pow(2.0, (double)(time) / 18.0) / 5.45 - 0.183);
}


double TVA::get_amplification(void)
{
  // LFO hack -Tremolo
  float x = 1.0 / (float) _sampleRate;
  _readPointer += _sinus.size() * _LFOFrequency * x;
  while(_readPointer >= _sinus.size())
    _readPointer -= _sinus.size();

  double tremolo = (double) _sinus[(int) _readPointer] / 128.0 * (_LFODepth / 32.0);

// if (tremolo)
//  std::cout << "TREMOLO:" << tremolo << " og depth:" << (int) _LFODepth << std::endl;
      
  // Volume envelope
  double volEnvelope = 0;
  if (_ahdsr)
    volEnvelope = _ahdsr->get_next_value();

  tremolo=0; // Disable tremolo for now
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
