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
// - Pitch corrections in this class should perhaps be moved to the TVP class


#include "partial.h"
#include "resample.h"

#include <iostream>
#include <cmath>

namespace {
struct InterpIndexes {
  int i0, i1, i2, i3;
};

static InterpIndexes get_cubic_indexes(int i, int loopStart, int loopEnd, bool isLooping) {
  if (i == loopEnd - 1) {
    return {i-1, i, isLooping ? loopStart : i+1, isLooping ? loopStart+1 : i+1};
  } else if (i == loopEnd) {
    return {i-1, i, isLooping ? loopStart : i, isLooping ? loopStart+1 : i};
  } else if (i == 0) {
    return {isLooping ? loopEnd : i, i, i+1, i+2};
  } else {
    return {i-1, i, i+1, i+2};
  }
}
}

namespace EmuSC {


Partial::Partial(uint8_t key, int partialId, uint16_t instrumentIndex,
		 ControlRom &ctrlRom, PcmRom &pcmRom, WaveGenerator *LFO[2],
		 Settings *settings, int8_t partId)
  : _key(key),
    _keyFreq(440 * exp(log(2) * (key - 69) / 12)),
    _instPartial(ctrlRom.instrument(instrumentIndex).partials[partialId]),
    _lastPos(0),
    _index(0),
    _isLooping(false),
    _expFactor(log(2) / 12000),
    _settings(settings),
    _partId(partId),
    _tvp(NULL),
    _tvf(NULL),
    _tva(NULL),
    _sample(0),
    _updatePeriod(settings->get_param_uint32(SystemParam::SampleRate) / 128)
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

  // 6. Calculate static volume correction
  double instVol = _convert_volume(ctrlRom.instrument(instrumentIndex).volume);
  double sampleVol = _convert_volume(_ctrlSample->volume +
				     ((_ctrlSample->fineVolume- 1024) /1000.0));
  double partialVol = _convert_volume(_instPartial.volume);
  _volumeCorrection = instVol * sampleVol * partialVol;

  // 7. Create TVP/F/A & envelope classes
  _tvp = new TVP(_instPartial, LFO, settings, partId);
  _tvf = new TVF(_instPartial, key, LFO, settings, partId);
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

  // TODO: Figure out how often this should be executed (new thread?)
  if (!(_updateTimeout++ % _updatePeriod))
    _update_params();

  float pitchAdj = _pitchExp *
                   _pitchOffsetHz *
                   _settings->get_pitchBend_factor(_partId) *
                   _staticPitchTune *
                   _tvp->get_pitch();

  if (_next_sample_from_rom(pitchAdj))
    return 1;

  double sample[2] = {0, 0};
  sample[0] = _sample * 0.65;      // Reduce audio volume to avoid overflow

  // Apply volume changes
  sample[0] *= _volumeCorrection;

  _tvf->apply(&sample[0]);
  _tva->apply(&sample[0]);

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
  const double *coeffs;

  if (_index > loopStart && _ctrlSample->loopMode != 2) _isLooping = true;

  if (_index >= loopEnd + 1.f) {
    if (_ctrlSample->loopMode == 2) {
      // One-shot: sample is finished; exit
      return 1;
    }

    // Restart loop
    _index -= (loopLen + 1.f);
  }

  int idx0 = (int) floor(_index);
  float frac = _index - floor(_index);

  // TODO: Make interpolation mode configurable.
  InterpMode interpMode = InterpMode::Cubic;
  switch (interpMode) {
    case InterpMode::Nearest: {
      // Nearest neighbor
      _sample = _pcmSamples->at(idx0);
      break;
    }
    case InterpMode::Linear: {
      // Linear interpolation
      int idx1 = idx0 + 1;
      if (idx1 > (int) loopEnd) {
        idx1 = _isLooping ? (int) loopStart : idx0;
      }

      coeffs = interp_coeff_linear[emusc_float_to_row(frac)];
      _sample = coeffs[0] * _pcmSamples->at(idx0) +
                coeffs[1] * _pcmSamples->at(idx1);
      break;
    }
    case InterpMode::Cubic: {
      // Cubic interpolation
      auto indexes = get_cubic_indexes(idx0,
                                       _ctrlSample->sampleLen - _ctrlSample->loopLen,
                                       _ctrlSample->sampleLen,
                                       _isLooping);

      coeffs = interp_coeff_cubic[emusc_float_to_row(frac)];
      _sample = coeffs[0] * _pcmSamples->at(indexes.i0) +
                coeffs[1] * _pcmSamples->at(indexes.i1) +
                coeffs[2] * _pcmSamples->at(indexes.i2) +
                coeffs[3] * _pcmSamples->at(indexes.i3);
      break;
    }
  }
  _index += timeStep;

  return 0;
}


double Partial::_convert_volume(uint8_t volume)
{
  return (0.1 * pow(2.0, (double)(volume) / 36.7111) - 0.1);
}


void Partial::_update_params(void)
{
  float freqKeyTuned = _keyFreq +
    (_settings->get_param_nib16(PatchParam::PitchOffsetFine,_partId) -0x080)/10;
  _pitchOffsetHz = freqKeyTuned / _keyFreq;

  _pitchExp = exp((_settings->get_param_32nib(SystemParam::Tune) - 0x400 +
                   (_settings->get_patch_param((int) PatchParam::ScaleTuningC +
                                               (_key % 12),_partId) - 0x40)*10 +
                   ((_settings->get_param_uint16(PatchParam::PitchFineTune,
                                                 _partId) - 16384)
                    / 16.384)) *
                  _expFactor);

  if (_tvp) _tvp->update_params();
  if (_tvf) _tvf->update_params();
  if (_tva) _tva->update_params();
}

}
