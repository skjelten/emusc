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


#include "note.h"

#include <iostream>

namespace EmuSC {


Note::Note(uint8_t key, uint8_t velocity, ControlRom &ctrlRom, PcmRom &pcmRom,
	   Settings *settings, int8_t partId)
  : _key(key),
    _velocity(velocity),
    _sustain(false),
    _stopped(false),
    _7bScale(1/127.0)
{
  _partial[0] = _partial[1] = NULL;

  // 1. Find correct instrument index for note
  // Note: toneBank is used for drum set number for rhythm parts
  uint8_t toneBank = settings->get_param(PatchParam::ToneNumber, partId);
  uint8_t toneIndex = settings->get_param(PatchParam::ToneNumber2, partId);
  uint16_t instrumentIndex;
  if (settings->get_param(PatchParam::UseForRhythm, partId) == 0)
    instrumentIndex = ctrlRom.variation(toneBank)[toneIndex];
  else
    instrumentIndex = ctrlRom.drumSet(toneBank).preset[key];

  if (instrumentIndex == 0xffff)        // Ignore undefined instruments / drums
    return;

  // Every Sound Canvas uses either 1 or 2 partials for each instrument
  for (int i = 0; i < 2; i ++) {
    uint16_t pIndex = ctrlRom.instrument(instrumentIndex).partials[i].partialIndex;
    if (pIndex == 0xffff)        // Partial 1 always used, but not 2. partial
      break;

    _partial[i] = new Partial(key, i, instrumentIndex, ctrlRom,
			      pcmRom, settings, partId);
  }
}


Note::~Note()
{
  delete _partial[0];
  delete _partial[1];
}


void Note::stop(void)
{
  if (_sustain) {                       // Hold pedal (hold1) or Sostenuto
    _stopped = true;

  } else {
    if (_partial[0])
      _partial[0]->stop();

    if (_partial[1])
      _partial[1]->stop();
  }
}


void Note::stop(uint8_t key)
{
  if (key == _key) {
    if (_sustain)                       // Hold pedal (hold1) or Sostenuto
      _stopped = true;

    if (_partial[0])
      _partial[0]->stop();

    if (_partial[1])
      _partial[1]->stop();
  }
}


void Note::sustain(bool state)
{
  _sustain = state;

  if (state == false && _stopped == true)
    stop(_key);
}


bool Note::get_next_sample(float *partSample)
{
  bool finished[2] = {0, 0};

  // Temporary samples for LEFT and RIGHT channel
  float sample[2] = {0, 0};

  // Iterate both partials
  for (int p = 0; p < 2; p ++) {

    // Skip this iteration if the partial in not used
    if  (_partial[p] == NULL) {
      finished[p] = 1;
      continue;
    }

    finished[p] = _partial[p]->get_next_sample(sample);
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
  if (_partial[0])
    numPartials ++;
  if (_partial[1])
    numPartials ++;

  return numPartials;
}

}
