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


#include "synth.h"
#include "part.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <mutex>

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "../config.h"


namespace EmuSC {

Synth::Synth(ControlRom &controlRom, PcmRom &pcmRom, Mode mode)
  : _mode(mode),
    _bank(0),
    _volume(100),
    _pan(0),
    _reverb(64),
    _chours(64),
    _keyShift(0),
    _masterTune(440.0),
    _reverbType(5),
    _choursType(3),
    _sampleRate(0),
    _channels(0),
    _ctrlRom(controlRom)
{
  // FIXME: Mode is currently ignored

  // Initialize all parts
  for (int i = 0; i < 16; i++) {
    Part part(i, _mode, 0, controlRom, pcmRom);
    _parts.push_back(part);
  }
  if (_mode == scm_GS)
    std::cout << "EmuSC: GS mode initialized" << std::endl;
  else if (_mode == scm_MT32)
    std::cout << "EmuSC: MT-32 mode initialized (not impl. yet!)" << std::endl;
}


Synth::~Synth()
{
  _parts.clear();
}


void Synth::mute(void)
{
  for (auto &p : _parts) p.set_mute(true);
}


void Synth::unmute(void)
{
  for (auto &p : _parts) p.set_mute(true);
}


void Synth::mute_parts(std::vector<uint8_t> mutePartsList)
{
  for (auto i : mutePartsList) _parts[i].set_mute(true);
}


void Synth::unmute_parts(std::vector<uint8_t> mutePartsList)
{
  for (auto i : mutePartsList) _parts[i].set_mute(false);
}


void Synth::volume(uint8_t volume)
{
  _volume = (volume > 127) ? 127 : volume;
}


void Synth::pan(uint8_t pan)
{
  std::cout << "libEmuSC: set pan=" << (int) pan << std::endl;
  if (pan > 127)
    _pan = 127;
  else
    _pan = pan;
}


void Synth::_add_note(uint8_t midiChannel, uint8_t key, uint8_t velocity)
{
  int partialsUsed = 0;
  for (auto &p: _parts)
    partialsUsed += p.get_num_partials();

  // TODO: Prioritize parts / MIDI channels based on info in owners manual
  // FIXME: Reduce voice count to 24/28 when volume envelope is corrected!
  if (_ctrlRom.synthModel == ControlRom::sm_SC55 && partialsUsed >= 50) //24)
    std::cout << "EmuSC: New note on ignored due to SC55 voice limit (24)"
	      << std::endl;
  else if (_ctrlRom.synthModel == ControlRom::sm_SC55mkII && partialsUsed >= 50) // 28)
    std::cout << "EmuSC: New note on ignored due to SC55mkII voice limit (28)"
	      << std::endl;
  else
    for (auto &p: _parts)
      p.add_note(midiChannel, key, velocity, _sampleRate);
}

/* Not used -> PcmRom as part of sample dump to disk
int Synth::_export_sample_24(std::vector<int32_t> &sampleSet,
			     std::string filename)
{
  int bytesPerSample = 3;
  int numSamples = sampleSet.size();
  uint8_t waveHeader[] = { 'R', 'I', 'F', 'F',      0x00, 0x00, 0x00, 0x00,
                           'W', 'A', 'V', 'E',       'f',  'm',  't',  ' ',
			   0x10, 0x00, 0x00, 0x00,  0x01, 0x00, 0x01, 0x00,
		           0x00, 0x7d, 0x00, 0x00,  0x00, 0x77, 0x01, 0x00,
	      	           0x03, 0x00, 0x18, 0x00,   'd',  'a',  't',  'a',
       		           0x00, 0x00, 0x00, 0x00 };

  waveHeader[4] = ((numSamples * bytesPerSample) + 36) & 0xff;
  waveHeader[5] = (((numSamples * bytesPerSample) + 36) >> 8) & 0xff;
  waveHeader[6] = (((numSamples * bytesPerSample) + 36) >> 16) & 0xff;
  waveHeader[7] = (((numSamples * bytesPerSample) + 36) >> 24) & 0xff;

  waveHeader[40] = (numSamples * bytesPerSample) & 0xff;
  waveHeader[41] = ((numSamples * bytesPerSample) >> 8) & 0xff;
  waveHeader[42] = ((numSamples * bytesPerSample) >> 16) & 0xff;
  waveHeader[43] = ((numSamples * bytesPerSample) >> 24) & 0xff;

  std::ofstream waveFile(filename, std::ios::binary | std::ios::out);
  if (!waveFile.is_open())
    return -1;

  waveFile.write((char *) waveHeader, 44);

  uint8_t sample[bytesPerSample];
  for (uint32_t s : sampleSet) {
    sample[0] = (uint8_t) (s >> 8); 
    sample[1] = (uint8_t) (s >> 16);
    sample[2] = (uint8_t) (s >> 24);
    
    waveFile.write((char *) sample, 3);
  }

  waveFile.close();
  if (!waveFile)
    return -1;

  return 0;
}
*/


void Synth::midi_input(uint8_t status, uint8_t data1, uint8_t data2)
{    
  uint8_t channel = status & 0x0f;

  midiMutex.lock();
	
  switch (status & 0xf0)
    {
    case midi_NoteOff:
      if (0)
	std::cout << "EmuSC MIDI: Note Off, ch=" << (int) channel
		  << " key=" << (int) data1 << std::endl;

      for (auto &p: _parts)
	if (p.midi_channel() == channel)
	  p.stop_note(channel, data1);
      break;

    case midi_NoteOn:
      if (0)
	std::cout << "EmuSC MIDI: Note On, ch=" << (int) channel
		  << " key=" << (int) data1 << std::endl;
      if (!data2) {                   // Note On with velocity = 0 => Note Off
	for (auto &p: _parts)
	  if (p.midi_channel() == channel)
	    p.stop_note(channel, data1);
      } else {
	_add_note(channel, data1, data2);
      }
      break;

    case midi_KeyPressure:
      if (0)
	std::cout << "EmuSC MIDI: Key pressure (AfterTouch), ch="
		  << (int) channel
		  << " -NOT IMPLEMENTED YET-" << std::endl;
      // TODO: Not implemented
      break;

    case midi_CtrlChange:
      if (0)
	std::cout << "EmuSC MIDI: CtrlChange, ch=" << (int) channel
		  << " ctrlMsgNum=" << std::dec << (int) data1
		  << ", value=" << (int) data2
		  << std::endl;
      {
	// MIDI CC messages
	enum Part::ControlMsg cMsg = Part::cmsg_Unknown;
	if (data1 == 0) {                                     // Bank select
	  _bank = data1;
	  break;
	} else if (data1 == 1) {
	  cMsg = Part::cmsg_ModWheel;
	} else if (data1 == 5) {
	  cMsg = Part::cmsg_PortamentoTime;
	} else if (data1 == 6 || data1 == 38) {               // Data entry
	} else if (data1 == 7) {
	  cMsg = Part::cmsg_Volume;
	} else if (data1 == 10) {
	  cMsg = Part::cmsg_Pan;
	} else if (data1 == 11) {
	  cMsg = Part::cmsg_Expression;
	} else if (data1 == 64) {
	  cMsg = Part::cmsg_HoldPedal;
	} else if (data1 == 65) {
	  cMsg = Part::cmsg_Portamento;
	} else if (data1 == 66) {                             // Sostenuto
	} else if (data1 == 67) {                             // Soft
	} else if (data1 == 91) {
	  cMsg = Part::cmsg_Reverb;
	} else if (data1 == 93) {
	  cMsg = Part::cmsg_Chorus;
	} else if (data1 == 98 || data1 == 99) {              // NRPN
	} else if (data1 == 100 || data1 == 101) {            // RPN
	} else if (data1 == 120 ||
		   data1 == 123 ||
		   data1 == 124 ||
		   data1 == 125 ||
  		   data1 == 126 ||
		   data1 == 127) {
	  if (0)
	    std::cout << "EmuSC MIDI: Clear all notes" << std::endl;

	  for (auto &p: _parts)
	    p.clear_all_notes();

	  break;
	} else {
	  if (0)
	    std::cout << "Warning: CtrlChange message not supported by "
		      << "Sound Canvas received. Ignored."
		      << std::endl;
	  break;
	}

	if (cMsg == Part::cmsg_Unknown) {
	  if (0)
	    std::cout << "Warning: CtrlChange message not implemented by "
		      << "libEmuSC receviced. Ignored."
		      << std::endl;
	  break;
	}

	for (auto &p : _parts) {
	  if (p.midi_channel() == channel) {
	    p.set_control(cMsg, channel, data2);

	    for (const auto &cb : _partMidiModCallbacks)
	      cb(p.id());
	  }
	}
      }

      break;

    case midi_PrgChange:
      if (0)
	std::cout << "EmuSC MIDI: Program change, ch=" << (int) channel
		  << std::endl;
      {
	for (auto &p : _parts)
	  if (p.midi_channel() == channel) {
	    p.set_program(channel, data1, _bank);

	    for (const auto &cb : _partMidiModCallbacks)
	      cb(p.id());
	  }
      }

      break;

    case midi_ChPressure:                         // Data 0 <-> 16383
      if (0)
	std::cout << "EmuSC MIDI: Channel. pressure, ch=" << (int) channel
		  << " CH PRESSURE NOT IMPLEMENTED YET" << std::endl;
      // TODO: Not implemented
      break;

    case midi_PitchBend:                          // Data 0 <-> 16383
      if (0)
	std::cout << "EmuSC MIDI: Pitchbend, ch=" << (int) channel
		  << " value=" << (int) ((data1 & 0x7f) | ((data2 & 0x7f) << 7))
		  << std::endl;
      {
	for (auto &p: _parts) {
//	  if (_ctrlRom.synthModel == ControlRom::sm_SC55)   // 12 bit resolution
//	    p.set_pitchBend(channel, (data1 & 0x70) | ((data2 & 0x7f) << 7));
//	  else
	    p.set_pitchBend(channel, (data1 & 0x7f) | ((data2 & 0x7f) << 7));
	}
      }
      break;

    default:
      std::cout << "EmuSC MIDI: Unknown event received" << std::endl;
      break;
    }

      /*     
      if (0) {
	std::cout << "EmuSC MIDI: Sysex message [length="
		  << (int) midiEvent->data1 << " data:";
	for (int i = 0; i < midiEvent->data1; i++)
	  std::cout << " " << std::hex << std::setw(2) <<(int)midiEvent->ptr[i];
	std::cout << "]" << std::dec << std::endl;
      }
	// TODO: Not implemented
      break;
      */

  midiMutex.unlock();
}


int Synth::get_next_sample(int16_t *sampleOut)
{
  float partSample[2];
  float accumulatedSample[2] = { 0, 0 };

  // Write silence
  for (int i = 0; i < _channels; i++) {
    sampleOut[i] = 0;
  }

  midiMutex.lock();

  // Iterate all parts and ask for next sample
  for (auto &p : _parts) {
    partSample[0] = partSample[1] = 0;
    p.get_next_sample(partSample);

    accumulatedSample[0] += partSample[0];
    accumulatedSample[1] += partSample[1];
  }

  // Finished working MIDI data
  midiMutex.unlock();


  // Apply sample effects that applies to "system" level (all parts & notes)

  /* Fix global pan (how does it add up to exising pan?). Use a MIDI channel 17?
  // Apply pan
  if (_pan > 0)
    accumulatedSample[0] *= 1.0 - (_pan / 63.0);
  else if (_pan < 0)
    accumulatedSample[1] *= 1.0 - (abs(_pan) / 63.0);
  */

  // Apply master volume conversion
  accumulatedSample[0] *= (_volume / 127.0);
  accumulatedSample[1] *= (_volume / 127.0);

  // Check if sound is too loud => clipping
  if (accumulatedSample[0] > 1 || accumulatedSample[0] < -1) {
    std::cout << "EmuSC: Warning - audio clipped (too loud)" << std::endl;
    accumulatedSample[0] = (accumulatedSample[0] > 1) ? 1 : -1;
  }
  if (accumulatedSample[1] > 1 || accumulatedSample[1] < -1) {
    std::cout << "EmuSC: Warning - audio clipped (too loud)" << std::endl;
    accumulatedSample[1] = (accumulatedSample[1] > 1) ? 1 : -1;
  }

  // Convert to 16 bit and update sample data in audio output driver
  for (int c = 0; c < _channels; c++)
    sampleOut[c] += (int16_t) (accumulatedSample[c] * 0xffff);

  return 0;
}


std::vector<float> Synth::get_parts_last_peak_sample(void)
{
  std::vector<float> partVolumes;
  
  for (auto &p : _parts)
    partVolumes.push_back(p.get_last_peak_sample());
  
  return partVolumes;
}


void Synth::set_audio_format(uint32_t sampleRate, uint8_t channels)
{
  _sampleRate = sampleRate;
  _channels = channels;
}


std::string Synth::version(void)
{
  return VERSION;
}


bool Synth::get_part_mute(uint8_t partId)
{
  return _parts[partId].mute();
}

uint16_t Synth::get_part_instrument(uint8_t partId)
{
  return _parts[partId].instrument();
}

uint8_t Synth::get_part_level(uint8_t partId)
{
  return _parts[partId].level();
}

int8_t Synth::get_part_pan(uint8_t partId)
{
  return _parts[partId].pan();
}

uint8_t Synth::get_part_reverb(uint8_t partId)
{
  return _parts[partId].reverb();
}

uint8_t Synth::get_part_chorus(uint8_t partId)
{
  return _parts[partId].chorus();
}

int8_t Synth::get_part_key_shift(uint8_t partId)
{
  return _parts[partId].key_shift();
}

uint8_t Synth::get_part_midi_channel(uint8_t partId)
{
  return _parts[partId].midi_channel();
}

// Update part state; needed for adapting to button inputs
void Synth::set_part_mute(uint8_t partId, bool mute)
{
  _parts[partId].set_mute(mute);
}

void Synth::set_part_instrument(uint8_t partId, uint16_t instrument)
{
  _parts[partId].set_instrument(instrument);
}


void Synth::set_part_level(uint8_t partId, uint8_t level)
{
  _parts[partId].set_level(level);
}


void Synth::set_part_pan(uint8_t partId, uint8_t pan)
{
  _parts[partId].set_pan(pan);
}


void Synth::set_part_reverb(uint8_t partId, uint8_t reverb)
{
  _parts[partId].set_reverb(reverb);
}


void Synth::set_part_chorus(uint8_t partId, uint8_t chorus)
{
  _parts[partId].set_chorus(chorus);
}


void Synth::set_part_key_shift(uint8_t partId, int8_t keyShift)
{
  _parts[partId].set_key_shift(keyShift);
}


void Synth::set_part_midi_channel(uint8_t partId, uint8_t midiChannel)
{
  _parts[partId].set_midi_channel(midiChannel);
}


void Synth::add_part_midi_mod_callback(std::function<void(const int)> callback)
{
  _partMidiModCallbacks.push_back(callback);
}


void Synth::clear_part_midi_mod_callback(void)
{
  _partMidiModCallbacks.clear();
}

}
