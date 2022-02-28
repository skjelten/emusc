/* 
 *  EmuSC - Sound Canvas emulator
 *  Copyright (C) 2022  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "note_partial.h"

#include <iostream>
#include <cmath>


NotePartial::NotePartial(int8_t keyDiff, uint16_t sampleIndex,
			 bool drum, ControlRom &ctrlRom, PcmRom &pcmRom)
  : _keyDiff(keyDiff),
    _sampleIndex(sampleIndex),
    _drum(drum),
    _ahdsrState(ahdsr_Attack),
    _samplePos(0),
    _sampleDir(1),
    _ctrlRom(ctrlRom),
    _pcmRom(pcmRom)
{}


NotePartial::~NotePartial()
{}


void NotePartial::stop(void)
{
  // TODO: Add proper logic for stopping samples -> ahdsr_Release
  _ahdsrState = ahdsr_Inactive;
}


// Pitch is the only variable input for a note's get_next_sample
// Pitch < 0 => fixed pitch (e.g. for drums)
bool NotePartial::get_next_sample(int32_t *noteSample, float pitchBend)
{
  // 1. Skip this partial if it's not in use
  if  (_ahdsrState == ahdsr_Inactive)
    return 1;

  // Update sample position going in forward direction
  if (_sampleDir == 1) {
    if (0)
      std::cout << "-> FW " << std::dec << "pos=" << (int)_samplePos
		<< " sLength=" << (int) _ctrlRom.sample(_sampleIndex).sampleLen
		<< " lpMode=" << (int) _ctrlRom.sample(_sampleIndex).loopMode
		<< " lpLength=" << _ctrlRom.sample(_sampleIndex).loopLen
		<< std::endl;
    float a = _samplePos;
    // Update partial sample position based on pitch input
    if (_drum)
      _samplePos += 1;
    else
      _samplePos += exp(_keyDiff * (log(2)/12)) * (pitchBend + 1);

    // Check for sample position passing sample boundary
    if (_samplePos >= _ctrlRom.sample(_sampleIndex).sampleLen) {

      // Keep track of correct samplePos switching samplePos
      int remaining = _ctrlRom.sample(_sampleIndex).sampleLen - _samplePos;

      // loopMode == 0 => Forward only w/loop jump back "loopLen + 1"
      if (_ctrlRom.sample(_sampleIndex).loopMode == 0) {
	_samplePos = _ctrlRom.sample(_sampleIndex).sampleLen -
	  _ctrlRom.sample(_sampleIndex).loopLen - 1 + remaining;

	// loopMode == 1 => Forward-backward. Start moving backwards
      } else if (_ctrlRom.sample(_sampleIndex).loopMode == 1) {
	_samplePos = _ctrlRom.sample(_sampleIndex).sampleLen - remaining;
	_sampleDir = 0;                                // Turn backward

	// loopMode == 2 => Forward-stop. End playback
      } else if (_ctrlRom.sample(_sampleIndex).loopMode == 2) {
	if (_samplePos >=_ctrlRom.sample(_sampleIndex).sampleLen)
	  _ahdsrState = ahdsr_Inactive;                // Set partial inactive
      }
    }

  // Update sample position going in backward direction
  } else {   // => _sampleDir == 0
    if (0)
      std::cout << "<- BW " << std::dec << "pos=" << (int) _samplePos
		<< " length=" << (int) _ctrlRom.sample(_sampleIndex).sampleLen 
		<< std::endl;

    // Update partial sample position based on pitch input
    if (_drum) {
      _samplePos -= 1;
    } else {
      _samplePos -= exp(_keyDiff * (log(2)/12));
      if (pitchBend > 0)
	_samplePos -= exp((pitchBend/8192.0) * (log(2)/12));
      else if (pitchBend < 0)
	_samplePos -= 1.0 + exp((pitchBend/8192.0) * (log(2)/12));
    }

    // Check for sample position passing sample boundary
    if (_samplePos <= _ctrlRom.sample(_sampleIndex).sampleLen -
	_ctrlRom.sample(_sampleIndex).loopLen - 1) {

      // Keep track of correct position switching position
      int remaining = _ctrlRom.sample(_sampleIndex).sampleLen -
	_ctrlRom.sample(_sampleIndex).loopLen - 1 - _samplePos;

      // Start moving forward
      _samplePos = _ctrlRom.sample(_sampleIndex).sampleLen -
	_ctrlRom.sample(_sampleIndex).loopLen - 1 + remaining;
      _sampleDir = 1;
    }
  }

  // Final sample for the partial to be sent to audio out

  // Temporary samples for LEFT and RIGHT channel
  int32_t sample[2] = {0, 0};
  sample[0] = _pcmRom.samples(_sampleIndex).samples[(int)_samplePos];

  // Add volume correction for each sample
  // TODO: ADSR volume
  //       Reduce divisions / optimize calculations

  // WIP
  // Sample volum: 7f - 0
  float scale = 1.0 / 127.;
  float fsample[2];
  fsample[0] = (float) sample[0];
  // Fine Volum correction (?):  10^((fineVolume - 0x0400) / 10000)
  float fineVol = powf(10, ((float) _ctrlRom.sample(_sampleIndex).fineVolume -
			    0x0400) / 10000.);

  fsample[0] *= scale * _ctrlRom.sample(_sampleIndex).volume * fineVol;

  // Both channelse are set identical before applying pan data
  fsample[1] = fsample[0];

  // panpot? TODO

  // Finally add samples to the sample pointers (always 2 channels / stereo)
  noteSample[0] += (int32_t) fsample[0];
  noteSample[1] += (int32_t) fsample[1];

  return 0;
}
