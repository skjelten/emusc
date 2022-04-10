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


#include "note.h"

#include <iostream>


Note::Note(uint8_t key, uint8_t velocity, uint16_t instrument, int drumSet,
	   ControlRom &ctrlRom, PcmRom &pcmRom, uint32_t sampleRate)
  : _key(key),
    _velocity(velocity)
{
  _notePartial[0] = _notePartial[1] = NULL;

  // Every Sound Canvas uses either 1 or 2 partials for each instrument
  // Find correct original tone from break table if partial is in use
  for (int i = 0; i < 2; i ++) {
    uint16_t pIndex = ctrlRom.instrument(instrument).partials[i].partialIndex;
    if (pIndex == 0xffff)        // Partial 1 always used, but not 2. partial
      break;

    for (int j = 0; j < 16; j ++) {
      if (ctrlRom.partial(pIndex).breaks[j] >= key ||
	  ctrlRom.partial(pIndex).breaks[j] == 0x7f) {
	uint16_t sampleIndex = ctrlRom.partial(pIndex).samples[j];
	if (sampleIndex == 0xffff) {              // This should never happen
	  std::cerr << "EmuSC: Internal error when reading sample" << std::endl;
	  break;
	}
	int8_t keyDiff = key - ctrlRom.sample(sampleIndex).rootKey;
	_notePartial[i] = new NotePartial(key, keyDiff, sampleIndex, drumSet,
					  ctrlRom, pcmRom, instrument, i,
					  sampleRate);
	break;
      }
    }
  }
}


Note::~Note()
{
  delete _notePartial[0];
  delete _notePartial[1];
}


bool Note::stop(uint8_t key)
{
  // If keys match, trigger stop event for each Note Partial
  if (key == _key) {
    
    if (_notePartial[0])
      _notePartial[0]->stop();

    if (_notePartial[1])
      _notePartial[1]->stop();

    return 1;
  }

  return 0;
}


bool Note::get_next_sample(float *partSample, float pitchBend)
{
  bool finished[2] = {0, 0};

  // Temporary samples for LEFT and RIGHT channel
  float samples[2] = {0, 0};

  // Iterate both partials
  for (int p = 0; p < 2; p ++) {

    // Skip this iteration if the partial in not used
    if  (_notePartial[p] == NULL) {
      finished[p] = 1;
      continue;
    }

    // Note: Samples are added to samples
    finished[p] = _notePartial[p]->get_next_sample(samples, pitchBend);
  }

  if (finished[0] == true && finished[1] == true)
    return 1;

  // Apply velocity
  float scale = 1.0 / 127.;
  float fsample[2];
  fsample[0] = samples[0] * scale * _velocity;
  fsample[1] = samples[1] * scale * _velocity;

  partSample[0] += fsample[0];
  partSample[1] += fsample[1];

  return 0;
}
