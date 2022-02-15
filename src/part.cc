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


#include "part.h"

#include <algorithm>
#include <cmath>
#include <iostream>


Part::Part(uint8_t id, uint8_t mode, uint8_t type,
	   std::vector<ControlRom::Instrument>& i,
	   std::vector<ControlRom::Partial>& p,
	   std::vector<ControlRom::Sample>& s,
  	   std::vector<ControlRom::DrumSet>& d)
  : _id(id),
    _midiChannel(id),
    _instrument(0),
    _drumSet(0),
    _volume(100),
    _pan(64),
    _reverb(40),
    _chours(0),
    _keyShift(0),
    _mode(mode_Norm),
    _bendRange(2),
    _modDepth(10),
    _keyRangeL(1),         // FIXME C1
    _keyRangeH(9),         // FIXME G9
    _velSensDepth(64),
    _velSensOffset(64),
    _partialReserve(2),
    _polyMode(true),
    _vibRate(0),
    _vibDepth(0),
    _vibDelay(0),
    _cutoffFreq(0),
    _resonance(0),
    _attackTime(0),
    _decayTime(0),
    _releaseTime(0),
    _mute(false),
    _instruments(i),
    _partials(p),
    _samples(s),
    _drumSets(d)
{
  // Part 10 is factory preset for MIDI channel 10 and standard drum set
  if (id == 9)
    _mode = 1;

  // Banks used for drum sets (not found in ROM, but by using the real thing)
  // Only true for SC-55, the SC-88 has more drum sets
  _drumSetBanks.assign({0, 8, 16, 24, 25, 32, 40, 48, 56, 57, 58, 59, 60, 127});
}


Part::~Part()
{}


int Part::_clear_unused_notes(void)
{
  int i;
  std::list<Note>::const_iterator itr = _notes.begin();
  while (itr != _notes.end()) {
    if ((*itr).state == adsr_Done) {
//      std::cout << "Automatically cleard one key" << std::endl;
      itr = _notes.erase(itr);
      i++;
    } else {
      itr++;
    }
  }

  return i;
}


// Parts always produce 2 channel & 32kHz (native) output. Other channel
// numbers and sample rates are handled by the calling Synth class.
int Part::get_next_sample(int32_t *sampleOut)
{
  // Return immediately if we have no notes to play
  if (_notes.size() == 0)
    return 0;

  bool needCleanup = false;
  
  // Write silence
  sampleOut[0] = sampleOut[1] = 0;
  
  int32_t partSample[2] = { 0, 0 };
  int32_t accSample = 0;

  // Iterate through the list of active notes
  for (auto &n : _notes) {

    // Iterate both partials for each note
    for (int prt = 0; prt < 2; prt ++) {

      // Skip this iteration if the partial in not used
      if  (n.sampleNum[prt] == 0xffff || n.state == adsr_Done)
	break;

      // Update sample position going in forward direction
      if (n.sampleDirection[prt] > 0) {
	if (0)
	  std::cout << "-> FW " << std::dec << "pos=" << (int) n.samplePos[prt]
		    << " length=" << (int) _samples[n.sampleNum[prt]].sampleLen
		    << " loopMode=" << (int) _samples[n.sampleNum[prt]].loopMode
		    << " loopLength=" << _samples[n.sampleNum[prt]].loopLen
		    << " partial=" << (int) prt << std::endl;

	// Do not adjust pitch for parts configured for drum sets
	if (_mode != mode_Norm) {
	  n.samplePos[prt] += 1;

	} else {
	// Adjust sample pos. / pitch for distance between original note and key
	  int diff_t = n.key - _samples[n.sampleNum[prt]].rootKey;
	  n.samplePos[prt] += 1.0 * exp(diff_t * (log(2)/12));

	  /*
	  // Apply pitch bend messages to partial
	  if (_effects[n.midiChannel].pitchbend != 0) {
	    	  std::cout << "Pitchbend this sucker! - "
		  << _effects[i.midiChannel].pitchbend << " : "
		  << (float) 1.0 * exp(diff_t * (log(2)/12))
		  << std::endl;

		  // FIXME: Verify scale for pitchbend & fix rounding error!
		  i.samplePos[prt] += _effects[i.midiChannel].pitchbend *
		                      exp(_parts[i.part].bendRange * log(2)/12);
				      }*/
	}
	// Check for sample position passing sample boundary
	if (n.samplePos[prt] >= _samples[n.sampleNum[prt]].sampleLen) {

	  // Keep track of correct position switching position
	  int remaining = _samples[n.sampleNum[prt]].sampleLen -
	                  n.samplePos[prt];

	  // loopMode == 0 => Forward only w/loop jump back "loopLen + 1"
	  if (_samples[n.sampleNum[prt]].loopMode == 0) {
	    n.samplePos[prt] = _samples[n.sampleNum[prt]].sampleLen -
	      _samples[n.sampleNum[prt]].loopLen - 1 + remaining;

	    // loopMode == 1 => Forward-backward. Start moving backwards
	  } else if (_samples[n.sampleNum[prt]].loopMode == 1) {
	    n.samplePos[prt] = _samples[n.sampleNum[prt]].sampleLen - remaining;
	    n.sampleDirection[prt] = -1;                       // Turn backward

	    // loopMode == 2 => Forward-stop. End playback
	  } else if (_samples[n.sampleNum[prt]].loopMode == 2) {
	    if (n.samplePos[prt] >=_samples[n.sampleNum[prt]].sampleLen){
	      // Remove key
	      n.state = adsr_Done;
	      needCleanup = true;
	      continue;
	    }
	  }
	}

      // Same procedure while looping backwards  // MOVE TO SAME LOOP AS ABOVE!
      } else if (n.sampleDirection[prt] < 0) {
	if (0)
	  std::cout << "<- BW " << std::dec << "pos=" << (int) n.samplePos[prt]
		    << " length=" << (int) _samples[n.sampleNum[prt]].sampleLen 
		    << " partial=" << (int) prt << std::endl;

	// Do not adjust pitch for parts configured for drum sets
	if (_mode != mode_Norm) {
	  n.samplePos[prt] -= 1;

	} else {

	  // Adjust sample pos. / pitch for distance between original note and key
	  int diff_t = n.key - _samples[n.sampleNum[prt]].rootKey;
	  n.samplePos[prt] -= 1.0 * exp(diff_t * (log(2)/12));

	  // Apply pitch bend messages to partial
//	  if (_effects[n.midiChannel].pitchbend != 0) {
//	    n.samplePos[prt] -= _effects[n.midiChannel].pitchbend * exp(2 * log(2)/12);
//	  }
	}
	// Check for sample position passing sample boundary
	if (n.samplePos[prt] <= _samples[n.sampleNum[prt]].sampleLen -
	                        _samples[n.sampleNum[prt]].loopLen - 1) {

	  // Keep track of correct position switching position
	  int remaining = _samples[n.sampleNum[prt]].sampleLen -
	    _samples[n.sampleNum[prt]].loopLen - 1 -n.samplePos[prt];

	  // Start moving forward
	  n.samplePos[prt] = _samples[n.sampleNum[prt]].sampleLen -
	                     _samples[n.sampleNum[prt]].loopLen - 1 + remaining;
	  n.sampleDirection[prt] = 1;
	}    
      }

      // Final sample for the partial to be sent to audio out
      partSample[prt] = _samples[n.sampleNum[prt]].sampleSet[n.samplePos[prt]];

      // 1. Add volume correction for each sample
      // TODO: Add
      // * drum volum if drum
      // * ADSR volume
      // * ++
      // Reduce divisions / optimize calculations

      // WIP
      // Sample volum: 7f - 0
      float scale = 1.0 / 127.;
      float sample = partSample[prt];
      // Fine Volum correction (?):  10^((fineVolume - 0x0400) / 10000)
      float fineVol = powf(10, ((float) _samples[n.sampleNum[prt]].fineVolume -
				0x0400) / 10000.);

      sample *= scale * _samples[n.sampleNum[prt]].volume * fineVol;

//      if (drum)
//      sample *= drumvol

      partSample[prt] = sample;

      //std::cout << "finevol=" << fineVol << std::endl;

      
      
    }
    // Apply all kinds of filters and stuff on instrument (both partials)
    // -> use temp variable and update sampleData here.

    // Sample for both partials as long as key state != Done
//      if (i.state != ves_Done) {

//    uint32_t combinedSample = partSample[0] + partSample[1]; WHY NOT?
    double combinedSample = partSample[0] + partSample[1];
    
    // Add volume parameters from key and MIDI channel / part
    float velocity = (n.velocity / 127.0) * (_volume / 127.0);
    combinedSample *= velocity ;
    // Finally add to accumulated sample data
//if(i.midiChannel == 9) 
    accSample += combinedSample;
 //     }

  }
    
  sampleOut[0] += accSample;
  sampleOut[1] += accSample;

  // Add pan from part
  if (_pan > 64)
    sampleOut[0] *= 1.0 - (_pan - 64) / 63.0;
  else if (_pan < 64)
    sampleOut[1] *= ((_pan - 1) / 64.0); 

  // Remove any notes that has expired (completed the release phase)
  if (needCleanup)
    _clear_unused_notes();
  
  return 0;
}


// Should mute => not accept key - or play silently in the background?
int Part::add_note(uint8_t midiChannel, uint8_t key, uint8_t velocity)
{
  // 1. Check if this message is relevant for this part
  if (midiChannel != _midiChannel || _mute)
    return 0;

  // 2. Create new note and set default values
  struct Note n;
  n.key = key;
  n.velocity = velocity;
  n.state = adsr_Attack;
  
  // 3. Find partial(s) used by instrument or drum set
  int i;
  if (_mode == mode_Norm)
    i = _instrument;
  else
    i = _drumSets[_drumSet].preset[key];

  if (i == 0xffff)                     // Ignore undefined instruments / drums
    return 0;

  uint16_t partial[2];
  partial[0] = _instruments[i].partials[0].partialIndex;
  partial[1] = _instruments[i].partials[1].partialIndex;

    // 4. Find correct original tone from break table  TODO->one loop!
  for (int x = 0; x < 16; x ++) {
    if (_partials[partial[0]].breaks[x] >= key ||
	_partials[partial[0]].breaks[x] == 0x7f) {
      n.sampleNum[0] = _partials[partial[0]].samples[x];
      n.samplePos[0] = 0;
      n.sampleDirection[0] = 1;
      break;
    }
  }
  if (partial[1] == 0xffff) {
    n.sampleNum[1] = 0xffff;
  } else {
    for (int x = 0; x < 16; x ++) {
      if (_partials[partial[1]].breaks[x] >= key ||
	  _partials[partial[1]].breaks[x] == 0x7f) {
	n.sampleNum[1] = _partials[partial[1]].samples[x];
	n.samplePos[1] = 0;
	n.sampleDirection[1] = 1;
	break;
      }
    }
  }

  _notes.push_back(n);

  if (0)
  if (_mode == mode_Norm)
    std::cout << "EmuSC: DEBUG New instr. note: part=" << (int) _id
	      << " key=" << (int) key
	      << " velocity=" << (int) velocity
//	      << " partial[0]=" << _instruments[_instrument].partials[0].partialIndex
//	      << " partial[1]=" << _instruments[_instrument].partials[1].partialIndex
	      << " loopMode=" << (int) _samples[_partials[_instruments[_instrument].partials[0].partialIndex].samples[0]].loopMode
//	      << " part0Name=" << _partials[_instruments[_instrument].partials[0].partialIndex].name
//     	      << " part0sample0=" << (std::hex) << (int) _partials[_instruments[_instrument].partials[0].partialIndex].samples[0]
      	      << " name=" << _instruments[_instrument].name << std::endl;
  else
    std::cout << "EmuSC: DEBUG New drum note: part=" << (int) _id
	      << " key=" << (int) key
	      << " velocity=" << (int) velocity
	      << " prt2=" << partial[1]
	      << " lm[0]=" << (int) _samples[_partials[_instruments[_instrument].partials[0].partialIndex].samples[0]].loopMode
	      << " index=" << _drumSets[_drumSet].preset[key]
	      << " drum set=" << _drumSets[_drumSet].name
	      << "(" << (int) _drumSet << ")"
	      << " name=" << _instruments[_drumSets[_drumSet].preset[key]].name
	      << std::endl;
  
    return 1;
}


int Part::delete_note(uint8_t midiChannel, uint8_t key)
{
  // 1. Check if this message is relevant for this part. Check:Hanging note bug?
  if (midiChannel != _midiChannel)
    return 0;

  // 2. Iterate through notes list and set them to release phase. FIXME!
  int i;
  std::list<Note>::const_iterator itr = _notes.begin();
  while (itr != _notes.end()) {
    // Check if we have the right instrument / drum
    if ((*itr).key == key) {

      // Check if we are dealing with drums, and if yes check note off flag
      // FIXME: Figure out how to properly end looping sounds
      if (_mode != mode_Norm && !(_drumSets[_drumSet].flags[key] & 0x01)
	  && _samples[(*itr).sampleNum[0]].loopMode == 2) {  // This is a hack!
	++itr;

      // We have an interuptable instrument / drum -> erase (FIXME: -> Release)
      } else { 
	itr = _notes.erase(itr);
	i++;
      }
    } else {
      ++itr;
    }
  }
    
  return i;
}


int Part::clear_all_notes(void)
{
  int i = _notes.size();
  _notes.clear();

  return i;
}


int Part::set_program(uint8_t midiChannel, uint8_t index, uint16_t instrument)
{
  if (midiChannel != _midiChannel)
    return 0;

  if (_mode == mode_Norm) {
    _instrument = instrument;
  } else {
    std::list<int>::iterator it = std::find(_drumSetBanks.begin(),
					    _drumSetBanks.end(),
					    (int) index);
    if (it != _drumSetBanks.end())
      _drumSet = (int8_t) std::distance(_drumSetBanks.begin(), it);
    else
      std::cerr << "EmuSC: Illegal program for drum set (" << (int) index << ")"
		<< std::endl;
  }

  return 1;
}


int Part::set_control(enum ControlMsg m, uint8_t midiChannel, uint8_t value)
{
  if (midiChannel != _midiChannel)
    return 0;

  if (m == cmsg_Volume)
    _volume = value;
  else if (m == cmsg_Pan)
    _pan = (value == 0) ? 1 : value;

  return 1;
}
