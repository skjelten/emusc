/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2026  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 2.1 of the License, or
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


namespace EmuSC {


Partial::Partial(int partialId, uint8_t key, uint8_t velocity,
		 uint16_t instrumentIndex, ControlRom &ctrlRom, PcmRom &pcmRom,
		 WaveGenerator *LFO1, Settings *settings, int8_t partId)
  : _instPartial(ctrlRom.instrument(instrumentIndex).partials[partialId]),
    _settings(settings),
    _partId(partId),
    _LFO2(NULL),
    _pitch(NULL),
    _tvf(NULL),
    _tva(NULL),
    _pitchAdj(0),
    _sample(0)
{
  _drumSet = settings->get_param(PatchParam::UseForRhythm, partId);
  if (_drumSet)
    _drumRxNoteOff = _settings->get_param(DrumParam::RxNoteOff, _drumSet-1,key);

  _LFO2 = new WaveGenerator(_instPartial, ctrlRom.lookupTables,settings,partId);

  _pitch = new Pitch(ctrlRom, instrumentIndex, partialId, key, velocity, LFO1,
                     _LFO2, settings, partId);

  _tvf = new TVF(_instPartial, key, velocity, LFO1, _LFO2,
                 ctrlRom.lookupTables, settings, partId);

  int sampleIndex = _pitch->get_sample_id();
  _tva = new TVA(ctrlRom, key, velocity, sampleIndex, LFO1, _LFO2, settings,
                 partId, instrumentIndex, partialId);

  _ctrlSample = &ctrlRom.sample(sampleIndex);
  _pcmSamples = &pcmRom.samples(sampleIndex).samplesF;
  _resample = new Resample(_ctrlSample, _pcmSamples,
                           std::bind(&Partial::first_run_cb, this));

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

  delete _resample;

  delete _LFO2;
}


// Pitch is the only variable input for a note's get_next_sample
// Pitch < 0 => fixed pitch (e.g. for drums)
bool Partial::get_next_sample(float *noteSample)
{
  // Terminate this partial if its TVA envelope is finished
  if  (_tva->finished())
    return 1;

  _sample = _resample->get_next_sample(_pitchAdj);

  double sample[2] = {0, 0};
  sample[0] = _sample * 1.0;      // Reduce audio volume to avoid overflow

//  _tvf->apply(&sample[0]);
  _tva->apply(&sample[0]);

  // Finally add samples to the sample pointers (always 2 channels / stereo)
  noteSample[0] += sample[0];
  noteSample[1] += sample[1];

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


void Partial::first_run_cb(void)
{
  // Looping samples have their pitch tuned after the first loop point
  // Non-looping samples shall be terminated (if not complete already)
  if (_ctrlSample->loopMode != 2)
    _pitch->first_sample_run_complete();
  else
    _tva->set_phase(Envelope::Phase::Terminated);
}

}
