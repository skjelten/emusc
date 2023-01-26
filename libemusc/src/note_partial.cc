/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2023  Håkon Skjelten
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
			 ControlRom &ctrlRom, Settings *settings, int8_t partId)
  : _key(key),
    _keyDiff(keyDiff),
    _ctrlSample(ctrlSample),
    _pcmSamples(pcmSamples),
    _drumSet(drumSet),
    _index(0),
    _direction(1),
    _instPartial(instPartial),
    _ctrlRom(ctrlRom),
    _settings(settings),
    _partId(partId)
{
  // Calculate key frequency for pitch offset in Hz
  _keyFreq = 440 * exp(log(2) * (key - 69) / 12);

  // Calculate static pitch correction for the entire life of the partial:
  //  - Coarse and fine pitch offset from instrument partial definition
  //  - Pitch correction from sample definition
  //  - Sample rate conversion
  _staticPitchTune =
    (exp(((instPartial.coarsePitch - 0x40) * 100 +
	  instPartial.finePitch - 0x40 +
	  (ctrlSample.pitch - 1024) / 16) * log(2) / 1200))
    *  32000.0 / settings->get_param_uint32(SystemParam::SampleRate);

  _expFactor = log(2) / 12000;

  // TODO: Find correct formula for pitch key follow
  if (instPartial.pitchKeyFlw - 0x40 != 10) {
    _keyDiff *= ((float) instPartial.pitchKeyFlw - 0x40) / 10.0;

    if (0)
      std::cout << "Pitch key: " << (int) _instPartial.pitchKeyFlw - 0x40
		<< " old keyDiff=" << (int) keyDiff
		<< " => new keyDiff=" << _keyDiff << std::endl;
  }

  // 1. Pitch: Vibrato & TVP envelope
  _tvp = new TVP(instPartial, settings, partId);

  // 2. Filter: ?wah? & TVF envelope
  _tvf = new TVF(instPartial, key, settings, partId);

  // 3. Volume: Tremolo & TVA envelope
  _tva = new TVA(instPartial, key, settings, partId);
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
bool NotePartial::get_next_sample(float *noteSample)
{
  int nextIndex;

  // Terminate this partial if its TVA envelope is finished
  if  (_tva->finished())
    return 1;

  float pitchBend = _settings->get_pitchBend_factor(_partId);

  float freqKeyTuned = _keyFreq +
    (_settings->get_param_nib16(PatchParam::PitchOffsetFine,_partId) -0x080)/10;
  float pitchOffsetHz = freqKeyTuned / _keyFreq;

  float pitchExp = _keyDiff * 1000
    + _settings->get_param_32nib(SystemParam::Tune) - 0x400
    + (_settings->get_patch_param((int) PatchParam::ScaleTuningC +
				  (_key % 12), _partId) - 0x40) * 10
    + ((_settings->get_param_uint16(PatchParam::PitchFineTune, _partId) - 8192)
	/ 8.192);

  float pitchAdj = exp(pitchExp * _expFactor) * pitchOffsetHz * pitchBend *
    _staticPitchTune * _tvp->get_pitch();

  // Update sample position going in forward direction
  if (_direction == 1) {
    if (0)
      std::cout << "-> FW " << std::dec << "pos=" << (int)_index
		<< " sLength=" << (int) _ctrlSample.sampleLen
		<< " lpMode=" << (int) _ctrlSample.loopMode
		<< " lpLength=" << _ctrlSample.loopLen
		<< std::endl;

    // Update partial sample position based on pitch input
    // FIXME: Should drumsets be modified by pitch bend messages?
    _index += pitchAdj;

    // Check for sample position passing sample boundary
    if (_index >= _ctrlSample.sampleLen) {

      // Keep track of correct sample index when switching sample direction
      float remaining = _ctrlSample.sampleLen - _index;

      // loopMode == 0 => Forward only w/loop jump back "loopLen + 1"
      if (_ctrlSample.loopMode == 0) {
	_index = _ctrlSample.sampleLen - _ctrlSample.loopLen - 1 + remaining;

      // loopMode == 1 => Forward-backward. Start moving backwards
      } else if (_ctrlSample.loopMode == 1) {
	_index = _ctrlSample.sampleLen - remaining - 1;
	_direction = 0;                                // Turn backward

      // loopMode == 2 => Forward-stop. End playback
      } else if (_ctrlSample.loopMode == 2) {
	if (_index >=_ctrlSample.sampleLen)
	  return 1;                 // Terminate this partial
      }
    }

    // Find next index for linear interpolation and adjust if at end of sample
    nextIndex = (int) _index + 1;
    if (nextIndex >= _ctrlSample.sampleLen) {
      if (_ctrlSample.loopMode == 0)
	nextIndex = _ctrlSample.sampleLen - _ctrlSample.loopLen - 1;
      else if (_ctrlSample.loopMode == 1)
	nextIndex = _ctrlSample.sampleLen - 1;
      else if (_ctrlSample.loopMode == 2)
	nextIndex = _ctrlSample.sampleLen;
    }

  // Update sample position going in backward direction
  } else {   // => _direction == 0
    if (0)
      std::cout << "<- BW " << std::dec << "pos=" << (int) _index
		<< " length=" << (int) _ctrlSample.sampleLen 
		<< std::endl;

    // Update partial sample position based on pitch input
    // FIXME: Should drumsets be modified by pitch bend messages?
    _index -= pitchAdj;

    // Check for sample position passing sample boundary
    if (_index < _ctrlSample.sampleLen - _ctrlSample.loopLen - 1) {

      // Keep track of correct position switching position
      float remaining = _ctrlSample.sampleLen - _ctrlSample.loopLen - 1 -_index;

      // Start moving forward
      _index = _ctrlSample.sampleLen - _ctrlSample.loopLen - 1 + remaining;
      _direction = 1;
    }

    // Find next index for linear interpolation and adjust if at start of sample
    nextIndex = (int) _index - 1;
    if (nextIndex < _ctrlSample.sampleLen - _ctrlSample.loopLen - 1)
      nextIndex = _ctrlSample.sampleLen - _ctrlSample.loopLen - 1;
  }

  // Temporary samples for LEFT, RIGHT channel
  double sample[2] = {0, 0};

  // Calculate linear interpolation of PCM sample
  double fractionNext;
  double fractionPrev;
  if (_direction) {                                 // Moving forward in sample
    fractionNext = _index - ((int) _index);
    fractionPrev = 1.0 - fractionNext;
  } else {                                          // Moving backwards
    fractionPrev = _index - ((int) _index);
    fractionNext = 1.0 - fractionPrev;
  }
  sample[0] = fractionPrev * _pcmSamples[(int) _index] +
              fractionNext * _pcmSamples[nextIndex];

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
