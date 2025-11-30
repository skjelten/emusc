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


#include "note.h"

#include <bitset>
#include <iostream>
#include <cmath>

namespace EmuSC {


Note::Note(uint8_t key, uint8_t velocity, ControlRom &ctrlRom, PcmRom &pcmRom,
	   Settings *settings, int8_t partId)
  : _key(key),
    _sustain(false),
    _stopped(false),
    _7bScale(1/127.0),
    _settings(settings),
    _partId(partId),
    _updateSkipSamples(256.0 * (float) settings->sample_rate() / 32000.0),
    _updateSkipSamplesItr(0)
{
  _partial[0] = _partial[1] = NULL;

  // 1. Find correct instrument index for note
  // Note: toneBank is used as drumSet index for rhythm parts
  uint8_t toneBank = settings->get_param(PatchParam::ToneNumber, partId);
  uint8_t toneIndex = settings->get_param(PatchParam::ToneNumber2, partId);
  uint16_t instrumentIndex;
  if (settings->get_param(PatchParam::UseForRhythm, partId) == 0)
    instrumentIndex = ctrlRom.variation(toneBank)[toneIndex];
  else
    instrumentIndex = ctrlRom.drumSet(toneBank).preset[key];

  if (instrumentIndex == 0xffff)        // Ignore undefined instruments / drums
    return;

  // LFO1 is shared between partials
  _LFO1 = new WaveGenerator(ctrlRom.instrument(instrumentIndex),
                            ctrlRom.lookupTables, settings, partId);

  // Every instrument in the Sound Canvas line has up to two partials
  std::bitset<2> partialBits(ctrlRom.instrument(instrumentIndex).partialsUsed);
  if (partialBits.test(0))
    _partial[0] = new Partial(0, key, velocity, instrumentIndex, ctrlRom,
			      pcmRom, _LFO1, settings, partId);
  if (partialBits.test(1))
    _partial[1] = new Partial(1, key, velocity, instrumentIndex, ctrlRom,
			      pcmRom, _LFO1, settings, partId);
}


Note::~Note()
{
  delete _partial[0];
  delete _partial[1];

  delete _LFO1;
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

  // Update all note parameters every 256th samples @32k samples/s => 125 Hz
  if (_updateSkipSamplesItr-- == 0) {
    _updateSkipSamplesItr = _updateSkipSamples;

    if (_LFO1) _LFO1->update();
    if (_partial[0]) _partial[0]->update();
    if (_partial[1]) _partial[1]->update();
  }

  // Temporary samples for LEFT and RIGHT channel
  float sample[2] = {0, 0};

  // Iterate both partials
  for (int p = 0; p < 2; p ++) {
    if  (_partial[p] == NULL)
      finished[p] = 1;
    else
      finished[p] = _partial[p]->get_next_sample(sample);
  }

  if (finished[0] == true && finished[1] == true)
    return 1;

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


float Note::get_current_lfo(int lfo)
{
  if (lfo == 0)
    return _LFO1->value();
  if (lfo == 1 && _partial[0])
    return _partial[0]->get_current_lfo();
  if (lfo == 2 && _partial[1])
    return _partial[1]->get_current_lfo();

  return std::nanf("");
}


float Note::get_current_tvp(bool partial)
{
  if (!_partial[partial])
    return 0;

  return _partial[partial]->get_current_tvp();
}


float Note::get_current_tvf(bool partial)
{
  if (!_partial[partial])
    return 0;

  return _partial[partial]->get_current_tvf();
}


float Note::get_current_tva(bool partial)
{
  if (!_partial[partial])
    return 0;

  return _partial[partial]->get_current_tva();
}

}
