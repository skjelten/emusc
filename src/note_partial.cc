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


NotePartial::NotePartial(uint8_t key, int8_t keyDiff, uint16_t sampleIndex,
			 int drumSet, ControlRom &ctrlRom, PcmRom &pcmRom,
			 uint16_t instrument, bool partialIndex,
			 uint32_t sampleRate)
  : _key(key),
    _keyDiff(keyDiff),
    _sampleIndex(sampleIndex),
    _drumSet(drumSet),
    _instrument(instrument),
    _partialIndex(partialIndex),
    _samplePos(0),
    _sampleDir(1),
    _ctrlRom(ctrlRom),
    _pcmRom(pcmRom)
{
  _sampleFactor = 32000.0 / sampleRate;

  // Calculate coarse & fine pitch correction from instrument partial definition
  _coarsePitch = exp((_ctrlRom.instrument(_instrument).partials[0].coarsePitch
		      - 0x40) * log(2)/12);
  _finePitch = exp((_ctrlRom.instrument(_instrument).partials[0].finePitch
		    - 0x40) * log(2)/1200);

  // Calculate fine pitch from instument definition
  _instFinePitch = exp((_ctrlRom.sample(_sampleIndex).pitch - 1024) / 16
		       * log(2)/1200);

  // Volume (TVA) envelope
  _volumeEnvelope =
    new VolumeEnvelope(ctrlRom.instrument(instrument).partials[partialIndex],
		       sampleRate);
}


NotePartial::~NotePartial()
{
  delete _volumeEnvelope;
}


double NotePartial::_convert_volume(uint8_t volume)
{
  return (0.1 * pow(2.0, (double)(volume) / 36.7111) - 0.1);
}


void NotePartial::stop(void)
{
  // Ignore note off for uninterruptible drums (set by drum set flag)
  if (!(_drumSet >= 0 && !(_ctrlRom.drumSet(_drumSet).flags[_key] & 0x01)))
    _volumeEnvelope->note_off();
}


// Pitch is the only variable input for a note's get_next_sample
// Pitch < 0 => fixed pitch (e.g. for drums)
bool NotePartial::get_next_sample(float *noteSample, float pitchBend)
{
  // Terminate this partial if its volume envelope is finished
  if  (_volumeEnvelope->finished())
    return 1;

  // Update sample position going in forward direction
  if (_sampleDir == 1) {
    if (0)
      std::cout << "-> FW " << std::dec << "pos=" << (int)_samplePos
		<< " sIndex=" << (int) _sampleIndex
		<< " sLength=" << (int) _ctrlRom.sample(_sampleIndex).sampleLen
		<< " lpMode=" << (int) _ctrlRom.sample(_sampleIndex).loopMode
		<< " lpLength=" << _ctrlRom.sample(_sampleIndex).loopLen
		<< std::endl;

    // Update partial sample position based on pitch input
    if (_drumSet >= 0)
      _samplePos += _coarsePitch * _finePitch * _instFinePitch * _sampleFactor;
    else
      _samplePos += exp(_keyDiff * (log(2)/12)) * (pitchBend + 1) * _coarsePitch
	* _finePitch * _instFinePitch * _sampleFactor;

    // Check for sample position passing sample boundary
    if (_samplePos >= _ctrlRom.sample(_sampleIndex).sampleLen) {

      // Keep track of correct samplePos switching samplePos
      float remaining = _ctrlRom.sample(_sampleIndex).sampleLen - _samplePos;

      // loopMode == 0 => Forward only w/loop jump back "loopLen + 1"
      if (_ctrlRom.sample(_sampleIndex).loopMode == 0) {
	_samplePos = _ctrlRom.sample(_sampleIndex).sampleLen -
	  _ctrlRom.sample(_sampleIndex).loopLen - 1 + remaining;

	// loopMode == 1 => Forward-backward. Start moving backwards
      } else if (_ctrlRom.sample(_sampleIndex).loopMode == 1) {
	_samplePos = _ctrlRom.sample(_sampleIndex).sampleLen - remaining - 1;
	_sampleDir = 0;                                // Turn backward

	// loopMode == 2 => Forward-stop. End playback
      } else if (_ctrlRom.sample(_sampleIndex).loopMode == 2) {
	if (_samplePos >=_ctrlRom.sample(_sampleIndex).sampleLen)
	  return 1;                 // Terminate this partial
      }
    }

  // Update sample position going in backward direction
  } else {   // => _sampleDir == 0
    if (0)
      std::cout << "<- BW " << std::dec << "pos=" << (int) _samplePos
		<< " sIndex=" << (int) _sampleIndex
		<< " length=" << (int) _ctrlRom.sample(_sampleIndex).sampleLen 
		<< std::endl;

    // Update partial sample position based on pitch input
    if (_drumSet >= 0) {
      _samplePos -= _coarsePitch * _finePitch * _instFinePitch * _sampleFactor;
    } else {
      _samplePos -= exp(_keyDiff * (log(2)/12)) * (pitchBend + 1) * _coarsePitch
	* _finePitch * _instFinePitch * _sampleFactor;
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

  // Temporary samples for LEFT and RIGHT channel
  float sample[2] = {0, 0};
  sample[0] = _pcmRom.samples(_sampleIndex).samplesF[(int) _samplePos];

  // Calculate volume correction from sample definition (7f - 0)
  double sampleVol = _convert_volume(_ctrlRom.sample(_sampleIndex).volume +
				     ((_ctrlRom.sample(_sampleIndex).fineVolume
				       - 1024) / 1000.0));

  // Calculate volume correction from partial definition (7f - 0)
  double partialVol = _convert_volume(_ctrlRom.instrument(_instrument).partials[_partialIndex].volume);

  // Calculate volume correction from drum set definition
  double drumVol = 1;
  if (_drumSet >= 0)
    drumVol = _convert_volume(_ctrlRom.drumSet(_drumSet).volume[_key]);
  
  // Apply volume changes
  sample[0] *= sampleVol * partialVol * drumVol;

  // Apply volume envelope
  sample[0] *= _volumeEnvelope->get_next_value();

  // Make both channels equal before adding pan (stereo position)
  sample[1] = sample[0];

  // Add panpot (stereo positioning of sounds)
  double panpot;
  if (_drumSet < 0)
    panpot = (_ctrlRom.instrument(_instrument).partials[_partialIndex].panpot
	      - 0x40) / 64.0;
  else
    panpot = (_ctrlRom.drumSet(_drumSet).panpot[_key] - 0x40) / 64.0;

  if (panpot < 0)
    sample[1] *= (1 + panpot);
  else if (panpot > 0)
    sample[0] *= (1 - panpot);

  // Finally add samples to the sample pointers (always 2 channels / stereo)
  noteSample[0] += sample[0];
  noteSample[1] += sample[1];

  return 0;
}
