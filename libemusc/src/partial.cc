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


Partial::Partial(int partialId, uint8_t key, uint8_t velocity,
		 uint16_t instrumentIndex, ControlRom &ctrlRom, PcmRom &pcmRom,
		 WaveGenerator *LFO1, Settings *settings, int8_t partId)
  : _instPartial(ctrlRom.instrument(instrumentIndex).partials[partialId]),
    _lastPos(0),
    _index(0),
    _isLooping(false),
    _firstSampleIt(true),
    _settings(settings),
    _partId(partId),
    _LFO2(NULL),
    _pitch(NULL),
    _tvf(NULL),
    _tva(NULL),
    _interpMode(static_cast<Settings::InterpMode>(settings->interpolation_mode())),
    _sample(0)
{
  _drumSet = settings->get_param(PatchParam::UseForRhythm, partId);
  if (_drumSet)
    _drumRxNoteOff = _settings->get_param(DrumParam::RxNoteOff, _drumSet-1,key);

  _LFO2 = new WaveGenerator(_instPartial, ctrlRom.lookupTables,settings,partId);

  _pitch = new Pitch(ctrlRom, instrumentIndex, partialId, key, velocity, LFO1,
                     _LFO2, settings, partId);

  int sampleIndex = _pitch->get_sample_id();
  _pcmSamples = &pcmRom.samples(sampleIndex).samplesF;
  _ctrlSample = &ctrlRom.sample(sampleIndex);

  _tvf = new TVF(_instPartial, key, velocity, LFO1, _LFO2,
                 ctrlRom.lookupTables, settings, partId);

  _tva = new TVA(ctrlRom, key, velocity, sampleIndex, LFO1, _LFO2, settings,
                 partId, instrumentIndex, partialId);

  // FIXME: A few sample definitions in the SC-55 ROM have loop length >
  // sample length. This makes EmuSC crash as it loops outside range. The
  // following hack prevents a crash, but audio is wrong for these samples.
  // TODO: Figure out why this works on the real hardware.
  // Example: Concert Cym. (Con_sym), #59 of Orchestra drumkit
  if (_ctrlSample->loopLen > _ctrlSample->sampleLen) {
    std::cerr << "libEmuSC: Internal error, loop length > sample length!"
              << std::endl << " => loop length = sample length" << std::endl;
    _ctrlSample->loopLen = _ctrlSample->sampleLen;
  }
}


Partial::~Partial()
{
  delete _pitch;
  delete _tvf;
  delete _tva;

  delete _LFO2;
}


// Pitch is the only variable input for a note's get_next_sample
// Pitch < 0 => fixed pitch (e.g. for drums)
bool Partial::get_next_sample(float *noteSample)
{
  // Terminate this partial if its TVA envelope is finished
  if  (_tva->finished())
    return 1;

  if (_next_sample_from_rom())
    return 1;

  double sample[2] = {0, 0};
  sample[0] = _sample * 0.5;      // Reduce audio volume to avoid overflow

//  _tvf->apply(&sample[0]);
  _tva->apply(&sample[0]);

  // Finally add samples to the sample pointers (always 2 channels / stereo)
  noteSample[0] += sample[0];
  noteSample[1] += sample[1];

  return 0;
}


bool Partial::_next_sample_from_rom()
{
  float loopEnd = _ctrlSample->sampleLen;
  float loopLen = _ctrlSample->loopLen;
  float loopStart = loopEnd - loopLen;
  const double *coeffs;

  if (_index > loopStart && _ctrlSample->loopMode != 2) _isLooping = true;

  if (_index >= loopEnd + 1.f) {
    if (_firstSampleIt) {
      _firstSampleIt = false;
      _pitch->first_sample_run_complete();
    }

    // This could probably be replaced by first_loop_complete to TVA/F
    if (_ctrlSample->loopMode == 2) {
      // One-shot: sample is finished; exit
      return 1;
    }

    // Restart loop
    _index -= (loopLen + 1.f);

    _phase = (int) _index << 14;
  }

  int idx0 = (int) floor(_index);
  float frac = _index - floor(_index);

  switch (_interpMode) {
    case Settings::InterpMode::Nearest: {
      _sample = _pcmSamples->at(idx0);
      break;
    }
    case Settings::InterpMode::Linear: {
      int idx1 = idx0 + 1;
      if (idx1 > (int) loopEnd) {
        idx1 = _isLooping ? (int) loopStart : idx0;
      }

      coeffs = interp_coeff_linear[emusc_float_to_row(frac)];
      _sample = coeffs[0] * _pcmSamples->at(idx0) +
                coeffs[1] * _pcmSamples->at(idx1);
      break;
    }
    case Settings::InterpMode::Cubic: {
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

  _phase += _pitchAdj;
  _index = _phase >> 14;

  return 0;
}


void Partial::stop(void)
{
  // Ignore note off for uninterruptible drums (set by drum set flag)
  if (!(_drumSet && !_drumRxNoteOff)) {
    if (_pitch) _pitch->note_off();
    if (_tvf) _tvf->note_off();
    if (_tva) _tva->note_off();
  }
}


// Update parameters every 256th sample @32k
void Partial::update(void)
{
  if (_pitch) {
    _pitch->update();
    _pitchAdj = _settings->get_pitchBend_factor(_partId) *
                _pitch->get_scaled_phase_increment();
  }
  if (_tvf) _tvf->update();
  if (_tva) _tva->update();

  if (_LFO2) _LFO2->update();
}

}
