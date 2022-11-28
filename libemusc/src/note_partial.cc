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


#include "note_partial.h"

#include <iostream>
#include <cmath>

namespace EmuSC {


NotePartial::NotePartial(uint8_t key, int8_t keyDiff, int drumSet,
			 struct ControlRom::InstPartial &instPartial,
			 struct ControlRom::Sample &ctrlSample,
			 std::vector<float> &pcmSamples,
			 ControlRom &ctrlRom, uint32_t sampleRate)
  : _key(key),
    _keyDiff(keyDiff),
    _ctrlSample(ctrlSample),
    _pcmSamples(pcmSamples),
    _drumSet(drumSet),
    _samplePos(0),
    _sampleDir(1),
    _instPartial(instPartial),
    _ctrlRom(ctrlRom)
{
  _sampleFactor = 32000.0 / sampleRate;

  // Calculate coarse & fine pitch offset from instrument partial definition
  double instPartPitchTune =
    exp(((instPartial.coarsePitch - 0x40)* 100 + (instPartial.finePitch - 0x40))
	* log(2) / 1200);

  // Calculate fine pitch offset from sample definition
  double samplePitchTune = exp((ctrlSample.pitch - 1024) / 16 * log(2) / 1200);

  _instPitchTune = instPartPitchTune * samplePitchTune;

  // TODO: Find correct formula for pitch key follow
  if (instPartial.pitchKeyFlw - 0x40 != 10) {
    _keyDiff *= ((float) instPartial.pitchKeyFlw - 0x40) / 10.0;

    if (0)
      std::cout << "Pitch key: " << (int) _instPartial.pitchKeyFlw - 0x40
		<< " old keyDiff=" << (int) keyDiff
		<< " => new keyDiff=" << _keyDiff << std::endl;
  }

  // 1. Pitch: Vibrato & TVP envelope
  _tvp = new TVP(instPartial, sampleRate);

  // 2. Filter: ?wah? & TVF envelope
  _tvf = new TVF(instPartial, sampleRate);

  // 3. Volume: Tremolo & TVA envelope
  _tva = new TVA(instPartial, sampleRate);
}


NotePartial::~NotePartial()
{
  delete _tvp;
  delete _tvf;
  delete _tva;
}


double NotePartial::_convert_volume(uint8_t volume)
{
  return (0.1 * pow(2.0, (double)(volume) / 36.7111) - 0.1);
}


void NotePartial::stop(void)
{
  // Ignore note off for uninterruptible drums (set by drum set flag)
  if (!(_drumSet >= 0 && !(_ctrlRom.drumSet(_drumSet).flags[_key] & 0x01)))
    _tva->note_off();
}


// Pitch is the only variable input for a note's get_next_sample
// Pitch < 0 => fixed pitch (e.g. for drums)
bool NotePartial::get_next_sample(float *noteSample, float pitchBend)
{
  // Terminate this partial if its TVA envelope is finished
  if  (_tva->finished())
    return 1;

  // Update sample position going in forward direction
  if (_sampleDir == 1) {
    if (0)
      std::cout << "-> FW " << std::dec << "pos=" << (int)_samplePos
		<< " sLength=" << (int) _ctrlSample.sampleLen
		<< " lpMode=" << (int) _ctrlSample.loopMode
		<< " lpLength=" << _ctrlSample.loopLen
		<< std::endl;

    // Update partial sample position based on pitch input
    // FIXME: Should drumsets be modified by pitch bend messages?
    _samplePos += exp(_keyDiff * (log(2)/12)) * pitchBend * _instPitchTune *
      _sampleFactor * _tvp->get_pitch();

    // Check for sample position passing sample boundary
    if (_samplePos >= _ctrlSample.sampleLen) {

      // Keep track of correct samplePos switching samplePos
      float remaining = _ctrlSample.sampleLen - _samplePos;

      // loopMode == 0 => Forward only w/loop jump back "loopLen + 1"
      if (_ctrlSample.loopMode == 0) {
	_samplePos = _ctrlSample.sampleLen - _ctrlSample.loopLen - 1 +remaining;

	// loopMode == 1 => Forward-backward. Start moving backwards
      } else if (_ctrlSample.loopMode == 1) {
	_samplePos = _ctrlSample.sampleLen - remaining - 1;
	_sampleDir = 0;                                // Turn backward

	// loopMode == 2 => Forward-stop. End playback
      } else if (_ctrlSample.loopMode == 2) {
	if (_samplePos >=_ctrlSample.sampleLen)
	  return 1;                 // Terminate this partial
      }
    }

  // Update sample position going in backward direction
  } else {   // => _sampleDir == 0
    if (0)
      std::cout << "<- BW " << std::dec << "pos=" << (int) _samplePos
		<< " length=" << (int) _ctrlSample.sampleLen 
		<< std::endl;

    // Update partial sample position based on pitch input
    // FIXME: Should drumsets be modified by pitch bend messages?
    _samplePos -= exp(_keyDiff * (log(2)/12)) * pitchBend * _instPitchTune *
      _sampleFactor * _tvp->get_pitch();

    // Check for sample position passing sample boundary
    if (_samplePos <= _ctrlSample.sampleLen - _ctrlSample.loopLen - 1) {

      // Keep track of correct position switching position
      int remaining =
	_ctrlSample.sampleLen - _ctrlSample.loopLen - 1 - _samplePos;

      // Start moving forward
      _samplePos = _ctrlSample.sampleLen - _ctrlSample.loopLen - 1 + remaining;
      _sampleDir = 1;
    }
  }

  // Temporary samples for LEFT and RIGHT channel
  float sample[2] = {0, 0};
  sample[0] = _pcmSamples[(int) _samplePos];

  // Calculate volume correction from sample definition (7f - 0)
  double sampleVol = _convert_volume(_ctrlSample.volume +
				     ((_ctrlSample.fineVolume- 1024) / 1000.0));

  // Calculate volume correction from partial definition (7f - 0)
  double partialVol = _convert_volume(_instPartial.volume);

  // Calculate volume correction from drum set definition
  double drumVol = 1;
  if (_drumSet >= 0)
    drumVol = _convert_volume(_ctrlRom.drumSet(_drumSet).volume[_key]);

  // Apply volume changes
  sample[0] *= sampleVol * partialVol * drumVol;

  // Apply TVF
  sample[0] = _tvf->apply(sample[0]);

  // Apply TVA
  sample[0] *= _tva->get_amplification();

  // Make both channels equal before adding pan (stereo position)
  sample[1] = sample[0];

  // Add panpot (stereo positioning of sounds)
  double panpot;
  if (_drumSet < 0)
    panpot = (_instPartial.panpot - 0x40) / 64.0;
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

}
