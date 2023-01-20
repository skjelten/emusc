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


#include "part.h"

#include <algorithm>
#include <cmath>
#include <iostream>


namespace EmuSC {

Part::Part(uint8_t id, uint8_t mode, uint8_t type, Settings *settings,
	   ControlRom &ctrlRom, PcmRom &pcmRom)
  : _id(id),
    _settings(settings),
    _ctrlRom(ctrlRom),
    _pcmRom(pcmRom),
    _7bScale(1/127.0)
{
  // TODO: Rename mode => synthMode and set proper defaults for MT32 mode

  reset();
}


Part::~Part()
{
  clear_all_notes();
}


// Parts always produce 2 channel & 32kHz (native) output. Other channel
// numbers and sample rates are handled by the calling Synth class.
int Part::get_next_sample(float *sampleOut)
{
  // Return immediately if we have no notes to play
  if (_notes.size() == 0)
    return 0;

  float partSample[2] = { 0, 0 };
  float accSample = 0;

  // Get next sample from active notes, delete those which are finished
  std::list<Note*>::iterator itr = _notes.begin();
  while (itr != _notes.end()) {
    bool finished = (*itr)->get_next_sample(partSample, _pitchBend, _modWheel);

    if (finished) {
//      std::cout << "Both partials have finished -> delete note" << std::endl;
      delete *itr;
      itr = _notes.erase(itr);
    } else {
      ++itr;
    }
  }

  // Apply volume from part (MIDI channel) and expression (CM11)
  partSample[0] *= _settings->get_param(PatchParam::PartLevel, _id) *
    _7bScale * (_expression * _7bScale);
  partSample[1] *= _settings->get_param(PatchParam::PartLevel, _id) *
    _7bScale * (_expression * _7bScale);

  // Store last (highest) value for future queries (typically for bar display)
  _lastPeakSample =
    (_lastPeakSample >= partSample[0]) ? _lastPeakSample : partSample[0];
  
  // Apply pan from part (MIDI Channel)
  uint8_t panpot = _settings->get_param(PatchParam::PartPanpot, _id);
  if (panpot > 64)
    partSample[0] *= 1.0 - (panpot - 64) / 63.0;
  else if (panpot < 64)
    partSample[1] *= (panpot - 1) / 64.0;

  sampleOut[0] += partSample[0];
  sampleOut[1] += partSample[1];

  return 0;
}


float Part::get_last_peak_sample(void)
{
  if (_mute)
    return -1;

  float ret = std::abs(_lastPeakSample);
  _lastPeakSample = 0;

  return ret;
}


int Part::get_num_partials(void)
{
  if (_notes.size() == 0)
    return 0;

  int numPartials = 0;
  for (auto &n: _notes)
    numPartials += n->get_num_partials();

  return numPartials;
}


// Should mute => not accept key - or play silently in the background?
int Part::add_note(uint8_t key, uint8_t keyVelocity)
{
  // 1. Check if part is muted or rxNoteMessage is disabled
  // FIXME: Verify that this is correct behavior for mute
  if (_mute || !_settings->get_param(PatchParam::RxNoteMessage, _id))
    return 0;

  // 2. Check if key is outside part configured key range
  if (key < _settings->get_param(PatchParam::KeyRangeLow, _id) ||
      key > _settings->get_param(PatchParam::KeyRangeHigh, _id))
    return 0;

  // 3. Find partial(s) used by instrument or drum set
  uint16_t instrumentIndex;
  int drumSet;
  if (_settings->get_param(PatchParam::UseForRhythm, _id) == mode_Norm) {
    uint8_t toneBank = _settings->get_param(PatchParam::ToneNumber, _id);
    uint8_t toneIndex = _settings->get_param(PatchParam::ToneNumber2,_id);
    instrumentIndex = _ctrlRom.variation(toneBank)[toneIndex];
    drumSet = -1;
  } else {
    instrumentIndex = _ctrlRom.drumSet(_drumSet).preset[key];
    drumSet = _drumSet;
  }

  if (instrumentIndex == 0xffff)        // Ignore undefined instruments / drums
    return 0;

  // 4. If note is a drum -> check if drum accepts note on
  if (drumSet >= 0 && !(_ctrlRom.drumSet(drumSet).flags[key] & 0x10))
    return 0;

  // 5. Calculate corrected key velocity based on velocity sens depth & offset
  //    according to description in SC-55 owner's manual page 38
  uint8_t velSensDepth =
    _settings->get_param(PatchParam::VelocitySenseDepth, _id);
  uint8_t velSensOffset =
    _settings->get_param(PatchParam::VelocitySenseOffset, _id);
  float v = keyVelocity * (velSensDepth / 64.0);
  if (velSensOffset >= 64)
    v += velSensOffset - 64;
  else
    v *= (velSensOffset + 64) / 127.0;
  uint8_t velocity = (v <= 127) ? std::roundf(v) : 127;

  // 6. Remove all existing notes if part is in mono mode according to the
  //    SC-55 owner's manual page 39
  if (_settings->get_param(PatchParam::PolyMode, _id) == false &&
      _settings->get_param(PatchParam::UseForRhythm, _id) == 0)
    clear_all_notes();

  // 7. Create new note and set default values (note: using pointers)
  uint8_t keyShift = _settings->get_param(SystemParam::KeyShift) - 0x40 +
    _settings->get_param(PatchParam::PitchKeyShift, _id) - 0x40;

  Note *n = new Note(key, keyShift, velocity, instrumentIndex,
		     drumSet, _ctrlRom, _pcmRom, _settings, _id);
  _notes.push_back(n);

  if (0)
    std::cout << "EmuSC: New note [ part=" << (int) _id
	      << " key=" << (int) key
	      << " velocity=" << (int) velocity
	      << " preset=" << _ctrlRom.instrument(instrumentIndex).name
	      << " ]" << std::endl;
  
    return 1;
}


int Part::stop_note(uint8_t key)
{
  // 1. Check if CM64 is active (Hold Pedal) and store notes if true
  if (_holdPedal) {
    _holdPedalKeys.push_back(key);
    return 0;
  }

  // 2. Else iterate through notes list and send stop signal (-> release)
  int i;
  for (auto &n : _notes) {
    bool ret = n->stop(key);
    i += ret;
  }

  return i;
}


int Part::clear_all_notes(void)
{
  int i = _notes.size();
  for (auto n : _notes)
    delete n;

  _notes.clear();

  return i;
}


void Part::reset(void)
{
  clear_all_notes();

  _drumSet = 0;
  _modDepth = 10;
  _partialReserve = 2;

  // Other default settings for all parts
  _pitchBend = 1;
  _mute = false;
  _modulation = 0;
  _expression = 127;
  _holdPedal = false;
  _lastPeakSample = 0;

  _rpnMSB = rpn_None;
  _rpnLSB = rpn_None;
  _masterFineTuning = 0x2000;
  _masterCoarseTuning = 0x40;
}


// [index, bank] is the [x,y] coordinate in the variation table
// For drum sets, index is the program number in the drum set bank
int Part::set_program(uint8_t index, uint8_t bank)
{
  if (!_settings->get_param(PatchParam::RxProgramChange, _id))
    return -1;

  // Finds correct instrument variation from variations table
  // Implemented according to SC-55 Owner's Manual page 42-45
  if (_settings->get_param(PatchParam::UseForRhythm, _id) == mode_Norm) {
    uint16_t instrument = _ctrlRom.variation(bank)[index];
    if (bank < 63 && index < 120)
      while (instrument == 0xffff)
	instrument = _ctrlRom.variation(--bank)[index];

  // If part is used for drums, select correct drum set
  } else {
    const std::vector<int> &drumSetBank = _ctrlRom.drum_set_bank();
    std::vector<int>::const_iterator it = std::find(drumSetBank.begin(),
						    drumSetBank.end(),
						    (int) index);
    if (it != drumSetBank.end()) {
      _drumSet = (int8_t) std::distance(drumSetBank.begin(), it);
    } else {
      std::cerr << "EmuSC: Illegal program for drum set (" << (int) index << ")"
		<< std::endl;
      return 0;
    }
  }

  _settings->set_param(PatchParam::ToneNumber, bank, _id);
  _settings->set_param(PatchParam::ToneNumber2, index, _id);

  return 1;
}


int Part::set_control(enum ControlMsg m, uint8_t value)
{
  if (!_settings->get_param(PatchParam::RxControlChange, _id))
    return -1;

  if (m == cmsg_Volume && _settings->get_param(PatchParam::RxVolume, _id)) {
    _settings->set_param(PatchParam::PartLevel, value, _id);
  } else if (m == cmsg_ModWheel &&
	     _settings->get_param(PatchParam::RxModulation, _id)) {
    _modulation = value;

    // TODO: Figure out a proper way to calculate modWheel pitch value and
    //       where this calculation should be done
    _modWheel = value * 0.0003;
  } else if (m == cmsg_Pan && _settings->get_param(PatchParam::RxPanpot, _id)) {
    uint8_t panpot = (value == 0) ? 1 : value;
    _settings->set_param(PatchParam::PartPanpot, panpot, _id);
  } else if (m == cmsg_Expression &&
	     _settings->get_param(PatchParam::RxExpression, _id)) {
    _expression =  value;
  } else if (m == cmsg_HoldPedal &&
	     _settings->get_param(PatchParam::RxHold1, _id)) {
    _holdPedal =  (value >= 64) ? true : false;
    if (_holdPedal == false) {
      for (auto &k : _holdPedalKeys)
	stop_note(k);
      _holdPedalKeys.clear();
    }
  } else if (m == cmsg_Portamento &&
	     _settings->get_param(PatchParam::RxPortamento, _id)) {
    _portamento =  (value >= 64) ? true : false;
  } else if (m == cmsg_PortamentoTime) {
    _portamentoTime =  value;
  } else if (m == cmsg_Reverb) {
    _settings->set_param(PatchParam::ReverbSendLevel, value, _id);
  } else if (m == cmsg_Chorus) {
    _settings->set_param(PatchParam::ChorusSendLevel, value, _id);
  } else if (m == cmsg_RPN_LSB && _settings->get_param(PatchParam::RxRPN, _id)){
    if (value == 0x00 || value == 0x01 || value == 0x02 || value == 0x7f)
      _rpnLSB = (RPN) value;
  } else if (m == cmsg_RPN_MSB && _settings->get_param(PatchParam::RxRPN, _id)){
    if (value == 0x00 || value == 0x01 || value == 0x02 || value == 0x7f)
      _rpnMSB = (RPN) value;
  } else if (m == cmsg_DataEntry_LSB) {
    if (_rpnLSB == rpn_MasterFineTuning)
      _masterFineTuning |= value;
  } else if (m == cmsg_DataEntry_MSB) {
    if (_rpnMSB == rpn_PitchBendSensitivity)
      _settings->set_param(PatchParam::PB_PitchControl,  value, _id);
    else if (_rpnMSB == rpn_MasterFineTuning)
      _masterFineTuning |= value << 8;
    else if (_rpnMSB == rpn_MasterCoarseTuning)
      _masterCoarseTuning  = value;
  }

  return 1;
}


// Note: One semitone = log(2)/12
void Part::set_pitchBend(int16_t pitchBend)
{
  if (_settings->get_param(PatchParam::RxPitchBend, _id)) {
    uint8_t bendRange =
      _settings->get_param(PatchParam::PB_PitchControl, _id) - 64;
    _pitchBend = exp(((pitchBend - 8192) / 8192.0) * bendRange * (log(2) /12));

    if (0)
      std::cout << "libEmuSC: Set pitch bend [part=" << std::dec << (int) _id
		<< "] -> " << pitchBend << " => " << _pitchBend << " [ range:"
		<< (int) bendRange << " ]" << std::endl;
  }
}


uint16_t Part::_native_endian_uint16(uint8_t *ptr)
{
  if (_le_native())
    return (ptr[0] << 8 | ptr[1]);

  return (ptr[1] << 8 | ptr[0]);
}

}
