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

// Pitch corrections that must be calculated for each partial
// Static corrections:
//  - Key difference between rootkey and actual key (drum is similar) [semitone]
//  - Sample pitch correction as stored with sample control data [?]
//  - Scale tuning (seems to affect drums also in some unkown manner) [cent]
//  - Master key shift (not for drums) [semitone]
//  - Part key shift (on drums only for SC-55mk2+) [semitone]
//  - Master Coarse Tuning (RPN #2) [semitone]
//  - PitchKeyFollow from partial definition
// Dynamic corrections:
//  - Master tune (SysEx) [cent]
//  - Master fine tuning (RPN #1) [cent]
//  - Fine tune offset [Hz]
//  - Pitch bend

// Some radnom notes:
// - All coarse tune variables are in semitones. They are all added to the key
//   to find the correct rootkey. They are also only calculated once and do not
//   change over the time of a partial.
// - No key shifts affects drum parts on SC-55 (SC-55 OM page 17 & 24), but
//   part key shift affects drum parts on SC-55mk2+ (SC-55mkII OM page 21)


#include "partial.h"

#include <iostream>
#include <cmath>


namespace EmuSC {


Partial::Partial(uint8_t key, int partialId, uint16_t instrumentIndex,
		 ControlRom &ctrlRom, PcmRom &pcmRom, WaveGenerator *LFO[2],
		 Settings *settings, int8_t partId)
  : _key(key),
    _instPartial(ctrlRom.instrument(instrumentIndex).partials[partialId]),
    _index(0),
    _direction(1),
    _sample(0),
    _settings(settings),
    _partId(partId),
    _keyFreq(440 * exp(log(2) * (key - 69) / 12)),
    _expFactor(log(2) / 12000),
    _lastPos(0),
    _tvp(NULL),
    _tvf(NULL),
    _tva(NULL)
{
  _isDrum = settings->get_param(PatchParam::UseForRhythm, partId);

  // 1: Find static coarse tuning => key shifts
  int keyShift = settings->get_param(PatchParam::PitchCoarseTune, partId) -0x40;
  if (!_isDrum) {
    keyShift += settings->get_param(SystemParam::KeyShift) - 0x40 +
                settings->get_param(PatchParam::PitchKeyShift, partId) - 0x40;

  } else {
    if (ctrlRom.generation() >= ControlRom::SynthGen::SC55mk2)
      keyShift += settings->get_param(PatchParam::PitchKeyShift, partId) - 0x40;
  }

  // 2: Find sample index from break table while adjusting key with key shifts
  uint16_t sampleIndex;
  uint16_t pIndex =
    ctrlRom.instrument(instrumentIndex).partials[partialId].partialIndex;
  for (int j = 0; j < 16; j ++) {
    if (ctrlRom.partial(pIndex).breaks[j] >= (key + keyShift) ||
	ctrlRom.partial(pIndex).breaks[j] == 0x7f) {
      sampleIndex = ctrlRom.partial(pIndex).samples[j];
      if (sampleIndex == 0xffff) {              // This should never happen
	std::cerr << "libEmuSC: Internal error when reading sample index"
		  << std::endl;
	return;// TODO: Verify that we are in a usable state!
      }
      break;
    }
  }

  // 3. Update internal class data pointers
  _pcmSamples = &pcmRom.samples(sampleIndex).samplesF;
  _ctrlSample = &ctrlRom.sample(sampleIndex);

  // 4. Find actual difference in key between NoteOn and sample
  if (_isDrum) {
    _drumMap = settings->get_param(PatchParam::UseForRhythm, partId) - 1;
    _keyDiff = keyShift + settings->get_param(DrumParam::PlayKeyNumber,
					      _drumMap, key) - 0x3c;

  } else {                                        // Regular instrument
    _keyDiff = key + keyShift - _ctrlSample->rootKey;
  }

  // 5. Calculate pitch key follow
  float pitchKeyFollow = 1;
  if (_instPartial.pitchKeyFlw - 0x40 != 10)
    pitchKeyFollow += ((float) _instPartial.pitchKeyFlw - 0x4a) / 10.0;

  _staticPitchTune =
    (exp(((_instPartial.coarsePitch - 0x40 +
	   _keyDiff * pitchKeyFollow +
	   (60 - _ctrlSample->rootKey) * (1 - pitchKeyFollow)) * 100 +
	  _instPartial.finePitch - 0x40 +
	  ((_ctrlSample->pitch - 1024) / 16))
	 * log(2) / 1200))
    * 32000.0 / settings->get_param_uint32(SystemParam::SampleRate);

  // 6. Calculate volume correction
  double sampleVol = _convert_volume(_ctrlSample->volume +
				     ((_ctrlSample->fineVolume- 1024) /1000.0));
  double partialVol = _convert_volume(_instPartial.volume);
  double drumVol = 1;
  if (_isDrum)
    drumVol = _convert_volume(_settings->get_param(DrumParam::Level, _drumMap,
						   _key));
  float ctrlVol = _settings->get_param(PatchParam::Acc_AmplitudeControl, _partId) / 64.0;
  _volumeCorrection = sampleVol * partialVol * drumVol * ctrlVol;

  // 7. Calculate panpot (stereo positioning of sounds)
  if (!_isDrum)
    _panpot = (_instPartial.panpot - 0x40) / 64.0;
  else
    _panpot = (_settings->get_param(DrumParam::Panpot, _drumMap, _key) - 0x40) / 64.0;

  // 8. Create envelope classes
  // 1. Pitch: Vibrato & TVP envelope
  _tvp = new TVP(_instPartial, LFO, settings, partId);

  // 2. Filter: ?wah? & TVF envelope
  _tvf = new TVF(_instPartial, key, LFO, settings, partId);

  // 3. Volume: Tremolo & TVA envelope
  _tva = new TVA(_instPartial, key, LFO, settings, partId);
}


Partial::~Partial()
{
  delete _tvp;
  delete _tvf;
  delete _tva;
}


void Partial::stop(void)
{
  // Ignore note off for uninterruptible drums (set by drum set flag)
  if (!(_isDrum &&
	!(_settings->get_param(DrumParam::RxNoteOff,_drumMap,_key)))) {
    if (_tvp) _tvp->note_off();
    if (_tvf) _tvf->note_off();
    if (_tva) _tva->note_off();
  }
}


// Pitch is the only variable input for a note's get_next_sample
// Pitch < 0 => fixed pitch (e.g. for drums)
bool Partial::get_next_sample(float *noteSample)
{
  // Terminate this partial if its TVA envelope is finished
  if  (_tva->finished())
    return 1;

  float freqKeyTuned = _keyFreq +
    (_settings->get_param_nib16(PatchParam::PitchOffsetFine,_partId) -0x080)/10;
  float pitchOffsetHz = freqKeyTuned / _keyFreq;

  float pitchExp = _settings->get_param_32nib(SystemParam::Tune) - 0x400 +
                   (_settings->get_patch_param((int) PatchParam::ScaleTuningC +
					       (_key % 12), _partId) - 0x40)*10+
                   ((_settings->get_param_uint16(PatchParam::PitchFineTune,
						 _partId) - 16384) / 16.384);

  float pitchAdj = exp(pitchExp * _expFactor) *
                   pitchOffsetHz *
                   _settings->get_pitchBend_factor(_partId) *
                   _staticPitchTune *
                   _tvp->get_pitch();

  if (_next_sample_from_rom(pitchAdj))
    return 1;

  double sample[2] = {0, 0};
  sample[0] = _sample * 0.65;      // Reduce audio volume to avoid overflow


  // Apply volume changes
  sample[0] *= _volumeCorrection;

  // Apply TVF
// NOTE: TEMPORARILY DISABLED
//  sample[0] = _tvf->apply(sample[0]);

  // Apply TVA
  sample[0] *= _tva->get_amplification();

  // Make both channels equal before adding pan (stereo position)
  sample[1] = sample[0];

  if (_panpot < 0)
    sample[1] *= (1 + _panpot);
  else if (_panpot > 0)
    sample[0] *= (1 - _panpot);

  // Finally add samples to the sample pointers (always 2 channels / stereo)
  noteSample[0] += sample[0];
  noteSample[1] += sample[1];

  return 0;
}


bool Partial::_next_sample_from_rom(float timeStep)
{
  float loopEnd = _ctrlSample->sampleLen;
  float loopLen = _ctrlSample->loopLen;
  float loopStart = loopEnd - loopLen;

  // Half-frame "overshoot" is desired so that, in effect, the
  // first and last frames are duplicated during the ping-pong loop.
  // This is necessary to avoid a "click" at the loop point.
  //
  //                        NO                        YES
  //           ----------------------------------------------------
  //           3         •           •           • •             •
  //  sample   2       •   •       •           •     •         •
  //  index    1     •       •   •           •         •     •
  //           0   •           •           •             • •
  //           ----------------------------------------------------
  //                                time ->
  if (_index > loopEnd + 0.5f) {
    if (_ctrlSample->loopMode == 0) {
      // Normal: do nothing
    } else if (_ctrlSample->loopMode == 1) {
      // Ping-pong: switch direction
      _direction *= -1;
    } else if (_ctrlSample->loopMode == 2) {
      // One-shot: sample is finished; exit
      return 1;
    }

    // Restart loop
    _index -= (loopLen + 1);
  }

  int pos;
  if (_direction == 1) {
    // Forward loop
    pos = (int)(_index);
    _sample = _pcmSamples->at(pos);
  } else {
    // Backward loop, inverted polarity
    pos = (int)(loopStart + loopEnd - _index);
    _sample = -_pcmSamples->at(pos);
  }

  _index += timeStep;

  return 0;
}


double Partial::_convert_volume(uint8_t volume)
{
  return (0.1 * pow(2.0, (double)(volume) / 36.7111) - 0.1);
}

}
