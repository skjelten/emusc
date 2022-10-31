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

// The volume envelope is based on information from the SC55_Soundfont generator
// written by Kitrinx and NewRisingSun.


#include "volume_envelope.h"

#include <cmath>
#include <iostream>
#include <string.h>

namespace EmuSC {

VolumeEnvelope::VolumeEnvelope(ControlRom::InstPartial instPartial,
			       uint32_t sampleRate)
  : _sampleRate(sampleRate),
    _currentPhase(1),
    _currentVolume(0),
    _finished(false)
{
  // Set adjusted values for volume (0-127) and time (seconds)
  _phaseVolume[1] = _convert_volume(instPartial.TVAVolP1);
  _phaseVolume[2] = _convert_volume(instPartial.TVAVolP2);
  _phaseVolume[3] = _convert_volume(instPartial.TVAVolP3);
  _phaseVolume[4] = _convert_volume(instPartial.TVAVolP4);
  _phaseVolume[5] = 0;

  _phaseDuration[1] = _convert_time_to_sec(instPartial.TVALenP1 & 0x7F);
  _phaseDuration[2] = _convert_time_to_sec(instPartial.TVALenP2 & 0x7F);
  _phaseDuration[3] = _convert_time_to_sec(instPartial.TVALenP3 & 0x7F);
  _phaseDuration[4] = _convert_time_to_sec(instPartial.TVALenP4 & 0x7F);

  _phaseDuration[5] = _convert_time_to_sec(instPartial.TVALenP5 & 0x7F); //Fixme
  _phaseDuration[5] *= (instPartial.TVALenP5 & 0x80) ? 2 : 1;

  _phaseShape[1] = (instPartial.TVALenP1 & 0x80) ? 0 : 1;
  _phaseShape[2] = (instPartial.TVALenP2 & 0x80) ? 0 : 1;
  _phaseShape[3] = (instPartial.TVALenP3 & 0x80) ? 0 : 1;
  _phaseShape[4] = (instPartial.TVALenP4 & 0x80) ? 0 : 1;
  _phaseShape[5] = (instPartial.TVALenP5 & 0x80) ? 0 : 1;

  // Identify terminal phase
  if (instPartial.TVAVolP2 == 0) {
    _terminalPhase = 2;
  } else if (instPartial.TVAVolP3 == 0) {
    _terminalPhase = 3;
  } else if (instPartial.TVAVolP2 == instPartial.TVAVolP3 &&
	     instPartial.TVAVolP3 == instPartial.TVAVolP4) {
    _terminalPhase = 2;
  } else if(instPartial.TVAVolP3 == instPartial.TVAVolP4) {
    _terminalPhase = 3;
  } else {
    _terminalPhase = 4;
  }

  if (0)
    std::cout << "Volume envelope:" << std::endl << std::dec
	      << " P1: Vol=" << (int) instPartial.TVAVolP1 << "("
	      << (double) _phaseVolume[1]
	      << ") Time=" << (instPartial.TVALenP1 & 0x7F)
	      << "(" << (float) _phaseDuration[1] << ")" << std::endl
	      << " P2: Vol=" << (int) instPartial.TVAVolP2 << "("
	      << (double) _phaseVolume[2]
	      << ") Time=" << (instPartial.TVALenP2 & 0x7F)
	      << "(" << (float) _phaseDuration[2] << ")" << std::endl
	      << " P3: Vol=" << (int) instPartial.TVAVolP3 << "("
	      << (double) _phaseVolume[3]
	      << ") Time=" << (instPartial.TVALenP3 & 0x7F)
	      << "(" << (float) _phaseDuration[3] << ")" << std::endl
	      << " P4: Vol=" << (int) instPartial.TVAVolP4 << "("
	      << (double) _phaseVolume[4]
	      << ") Time=" << (instPartial.TVALenP4 & 0x7F)
	      << "(" << (float) _phaseDuration[4] << ")" << std::endl
	      << " P5: Vol=0 Time=" << (instPartial.TVALenP5 & 0x7F)
	      << "(" << (float) _phaseDuration[5] << ")" << std::endl
	      <<  "Terminal phase=" << _terminalPhase << std::endl;

  _init_next_phase(_phaseVolume[1], _phaseDuration[1]);

//  _sampleNum = 0;
//  _ofs.open("/tmp/TVA.txt", std::ios_base::trunc);
}


VolumeEnvelope::~VolumeEnvelope()
{
//  _ofs.close();
}


double VolumeEnvelope::_convert_volume(uint8_t volume)
{
  return (0.1 * pow(2.0, (double)(volume) / 36.7111) - 0.1);
}


double VolumeEnvelope::_convert_time_to_sec(uint8_t time)
{
  if (time == 0)
    return 0;

  return (pow(2.0, (double)(time) / 18.0) / 5.45 - 0.183);
}


void VolumeEnvelope::_init_next_phase(double phaseVolume, double phaseTime)
{
  _phaseInitVolume = _currentVolume;
  _phaseSampleLen = round(phaseTime * _sampleRate);
  _phaseSampleIndex = 0;

  if (_phaseSampleLen == 0)
    _currentVolume = phaseVolume;

  if (0)
    std::cout << "New TVA phase [" << _currentPhase
	      << "]: numSamples=" << (int) _phaseSampleLen
	      << " initVol=" << _phaseInitVolume
	      << " destVol=" << phaseVolume << std::endl;
}


double VolumeEnvelope::get_next_value()
{
  // 1. Check if we are looping for ever in phase 4
  if (_currentPhase == 4 && _phaseVolume[4] != 0)
    return _currentVolume;

  // 2. Check whether we have finished one phase and update to next
  if (_phaseSampleIndex >= _phaseSampleLen) {

    // Phase finished
    _currentPhase ++;
    if (_currentPhase >= 6) {
      _finished = true;
      return 0;

    } else if (_currentPhase > _terminalPhase) {
      _currentPhase = 5;
      _init_next_phase(0, _phaseDuration[5]);
    } else if (_currentPhase == 2) {
      _init_next_phase(_phaseVolume[2], _phaseDuration[2]);
    } else if (_currentPhase == 3) {
      _init_next_phase(_phaseVolume[3], _phaseDuration[3]);
    } else if (_currentPhase == 4) {
      _init_next_phase(_phaseVolume[4], _phaseDuration[4]);
    }
  }

  // 3. Update and return current volume
  if (_phaseSampleLen <= 0)
    _currentVolume = _phaseInitVolume;
  else if (_phaseShape[_currentPhase] == 0)            // Linear
    _currentVolume = _phaseInitVolume +
      (_phaseVolume[_currentPhase] - _phaseInitVolume) *
      ((double) _phaseSampleIndex / (double) _phaseSampleLen);
  else
    _currentVolume = _phaseInitVolume +                // Concave / convex
      (_phaseVolume[_currentPhase] - _phaseInitVolume) *
      (log(10.0 * _phaseSampleIndex / _phaseSampleLen + 1) / log(10.0 + 1));

  /*  // Write volume envelope to file for plotting / debuging in Octave
  char number[24];
  snprintf(number, 24, "%f ", (float) _sampleNum/_sampleRate);
  _ofs.write(&number[0], strlen(number));
  snprintf(number, 24, "%f", (float) (_currentVolume));
  _ofs.write(&number[0], strlen(number));
  _ofs.write("\n", 1);
  _sampleNum ++;
  */

  _phaseSampleIndex ++;

  return _currentVolume;
}


void VolumeEnvelope::note_off()
{
  if (_currentPhase == 5)
    return;

  _currentPhase = 5;                  // Jump immediately to phase 5 (release)
  _init_next_phase(0, _phaseDuration[5]);
}

}
