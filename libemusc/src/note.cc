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


#include "note.h"

#include <iostream>

namespace EmuSC {


Note::Note(uint8_t key, uint8_t velocity, uint16_t instrument, int drumSet,
	   ControlRom &ctrlRom, PcmRom &pcmRom, uint32_t sampleRate)
  : _key(key),
    _velocity(velocity),
    _7bScale(1/127.0)
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

	int8_t keyDiff;
	if (drumSet >= 0)
	  keyDiff = ctrlRom.drumSet(drumSet).key[key] - 0x3c;
	else
	  keyDiff = key - ctrlRom.sample(sampleIndex).rootKey;

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
  float sample[2] = {0, 0};

  // Iterate both partials
  for (int p = 0; p < 2; p ++) {

    // Skip this iteration if the partial in not used
    if  (_notePartial[p] == NULL) {
      finished[p] = 1;
      continue;
    }

    finished[p] = _notePartial[p]->get_next_sample(sample, pitchBend);
  }

  if (finished[0] == true && finished[1] == true)
    return 1;

  // Apply key velocity
  sample[0] *= _velocity * _7bScale;
  sample[1] *= _velocity * _7bScale;

  partSample[0] += sample[0];
  partSample[1] += sample[1];

  return 0;
}


int Note::get_num_partials()
{
  int numPartials = 0;
  if (_notePartial[0])
    numPartials ++;
  if (_notePartial[1])
    numPartials ++;

  return numPartials;
}

}
