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


#include "synth.h"
#include "part.h"
#include "settings.h"

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <mutex>

#include "config.h"


namespace EmuSC {

Synth::Synth(ControlRom &controlRom, PcmRom &pcmRom, Mode mode)
  : _mode(mode),
    _sampleRate(0),
    _channels(0),
    _ctrlRom(controlRom)
{
  _settings = new Settings();

  // FIXME: Mode is currently ignored -> read from settings
  reset();
  
  // Initialize all parts
  for (int i = 0; i < 16; i++) {
    Part part(i, _mode, 0, _settings, controlRom, pcmRom);
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
  delete _settings;
}


void Synth::reset(bool resetParts)
{
  if (resetParts)
    for (auto &p : _parts) p.reset();   //? TODO: CLEAN UP

  _bank = 0;
  _settings->reset(Settings::Mode::GS);
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


void Synth::_add_note(uint8_t midiChannel, uint8_t key, uint8_t velocity)
{
  int partialsUsed = 0;
  for (auto &p: _parts)
    partialsUsed += p.get_num_partials();

  // TODO: Prioritize parts / MIDI channels based on info in owners manual
  // FIXME: Reduce voice count when volume envelope is corrected!
  if (partialsUsed > _ctrlRom.max_polyphony() * 2)
    std::cout << "EmuSC: New note on ignored due to voice limit"
	      << std::endl;
  else
    for (auto &p: _parts)
      if (p.midi_channel() == midiChannel)
	p.add_note(key, velocity);
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
	  p.stop_note(data1);
      break;

    case midi_NoteOn:
      if (!_sampleRate) {
	std::cerr << "LibEmuSC error: MIDI note on received, but audio format "
		  << "has not been set. Note on ignored."
		  << std::endl;
	break;
      }

      if (0)
	std::cout << "EmuSC MIDI: Note On, ch=" << (int) channel
		  << " key=" << (int) data1 << std::endl;
      if (!data2) {                   // Note On with velocity = 0 => Note Off
	for (auto &p: _parts)
	  if (p.midi_channel() == channel)
	    p.stop_note(data1);
      } else {
	_add_note(channel, data1, data2);
      }
      break;

    case midi_KeyPressure:
      if (1)
	std::cout << "EmuSC MIDI: Key pressure (AfterTouch), ch="
		  << (int) channel << ", key=" << (int) data1
		  << ", value=" << (int) data2  << " -NOT IMPLEMENTED-"
		  << std::endl;
      // TODO: Not implemented
      break;

    case midi_CtrlChange:
      if (1)
	std::cout << "libEmuSC: New CtrlChange MIDI event, ch=" << (int) channel
		  << " CC Number=" << std::dec << (int) data1
		  << ", value=" << (int) data2 << std::endl;
      {
	enum Part::ControlMsg cMsg;
	if (data1 == 0) {                                   // Bank select
	  _bank = data2;
	  break;

	} else if (data1 == 1) {                            // Modulation
	  for (auto &p: _parts) {
	    if (p.midi_channel() == channel &&
		_settings->get_param(PatchParam::RxModulation, p.id()))
	      _settings->set_param(PatchParam::Modulation, data2, p.id());
	  }
	  break;

	} else if (data1 == 5) {                            // Portamento time
	  for (auto &p: _parts) {
	    if (p.midi_channel() == channel)
	      _settings->set_param(PatchParam::PortamentoTime, data2, p.id());
	  }
	  break;

	} else if (data1 == 6) {                            // Data entry MSB
	  cMsg = Part::cmsg_DataEntry_MSB;

	} else if (data1 == 7) {                            // Volume
	  for (auto &p: _parts) {
	    if (p.midi_channel() == channel &&
		_settings->get_param(PatchParam::RxVolume, p.id())) {
	      _settings->set_param(PatchParam::PartLevel, data2, p.id());
	      for (const auto &cb : _partMidiModCallbacks)
		cb(p.id());
	    }
	  }
	  break;

	} else if (data1 == 10) {                           // Panpot
	  for (auto &p: _parts) {
	    if (p.midi_channel() == channel &&
		_settings->get_param(PatchParam::RxPanpot, p.id())) {
	      _settings->set_param(PatchParam::PartPanpot, data2, p.id());
	      for (const auto &cb : _partMidiModCallbacks)
		cb(p.id());
	    }
	  }
	  break;

	} else if (data1 == 11) {                           // Expression
	  for (auto &p: _parts) {
	    if (p.midi_channel() == channel &&
		_settings->get_param(PatchParam::RxExpression, p.id()))
	      _settings->set_param(PatchParam::Expression, data2, p.id());
	  }
	  break;

	} else if (data1 == 38) {                           // Data entry LSB
	  cMsg = Part::cmsg_DataEntry_LSB;

	} else if (data1 == 64) {                           // Hold1
	  for (auto &p: _parts) {
	    if (p.midi_channel() == channel &&
		_settings->get_param(PatchParam::RxHold1, p.id())) {
	      if (data2 < 64)
		_settings->set_param(PatchParam::Hold1, (uint8_t) 0, p.id());
	      else
		_settings->set_param(PatchParam::Hold1, (uint8_t) 1, p.id());
	    }
	  } // Note: SC-88 Pro seems to use full 7 bit value for Hold1
	  break;

	} else if (data1 == 65) {                           // Portamento
	  for (auto &p: _parts) {
	    if (p.midi_channel() == channel &&
		_settings->get_param(PatchParam::RxPortamento, p.id())) {
	      if (data2 < 64)
		_settings->set_param(PatchParam::Portamento, (uint8_t)0,p.id());
	      else
		_settings->set_param(PatchParam::Portamento, (uint8_t)1,p.id());
	    }
	  }
	  break;

	} else if (data1 == 66) {                           // Sostenuto
	  for (auto &p: _parts) {
	    if (p.midi_channel() == channel &&
		_settings->get_param(PatchParam::RxSostenuto, p.id())) {
	      if (data2 < 64)
		_settings->set_param(PatchParam::Sostenuto, (uint8_t)0,p.id());
	      else
		_settings->set_param(PatchParam::Sostenuto, (uint8_t)1,p.id());
	    }
	  }
	  break;

	} else if (data1 == 67) {                           // Soft
	  for (auto &p: _parts) {
	    if (p.midi_channel() == channel &&
		_settings->get_param(PatchParam::RxSoft, p.id())) {
	      if (data2 < 64)
		_settings->set_param(PatchParam::Soft, (uint8_t)0,p.id());
	      else
		_settings->set_param(PatchParam::Soft, (uint8_t)1,p.id());
	    }
	  }
	  break;

	} else if (data1 == 91) {                           // Reverb
	  for (auto &p: _parts) {
	    if (p.midi_channel() == channel) {
	      _settings->set_param(PatchParam::ReverbSendLevel, data2, p.id());
	      for (const auto &cb : _partMidiModCallbacks)
		cb(p.id());
	    }
	  }
	  break;

	  } else if (data1 == 93) {                         // Chorus
	  for (auto &p: _parts) {
	    if (p.midi_channel() == channel) {
	      _settings->set_param(PatchParam::ChorusSendLevel, data2, p.id());
	      for (const auto &cb : _partMidiModCallbacks)
		cb(p.id());
	    }
	  }
	  break;

	} else if (data1 == 98 || data1 == 99) {            // NRPN
	  if (0)
	    std::cout << "libEmuSC: NRPN message ignored ("
		      << (int) data1 << ", " << (int) data2 << ")" << std::endl;
	} else if (data1 == 100) {                          // RPN LSB
	  cMsg = Part::cmsg_RPN_LSB;
	} else if (data1 == 101) {                          // RPN MSB
	  cMsg = Part::cmsg_RPN_MSB;
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
	}

	for (auto &p: _parts) {
	  if (_settings->get_param(PatchParam::CC1ControllerNumber, p.id())
	      == data1)
	    _settings->set_param(PatchParam::CC1Controller, data2,p.id());

	  if (_settings->get_param(PatchParam::CC2ControllerNumber, p.id())
	      == data1)
	    _settings->set_param(PatchParam::CC2Controller, data2,p.id());
	}

	if (cMsg == Part::cmsg_Unknown) {       // REMOVE
	  break;
	}

	for (auto &p : _parts) {
	  if (p.midi_channel() == channel) {
	    p.set_control(cMsg, data2);
	  }
	}
      }

      break;

    case midi_PrgChange:
      if (0)
	std::cout << "EmuSC MIDI: Program change, ch=" << (int) channel
		  << " preset=" << (int) data1 << std::endl;
      {
	for (auto &p : _parts)
	  if (p.midi_channel() == channel) {
	    p.set_program(data1, _bank);

	    for (const auto &cb : _partMidiModCallbacks)
	      cb(p.id());
	  }
      }

      break;

    case midi_ChPressure:
      for (auto &p: _parts) {
	if (p.midi_channel() == channel &&
	    _settings->get_param(PatchParam::RxChPressure, p.id())) {

	  set_param(PatchParam::ChannelPressure, data1, p.id());

	  if (1)
	    std::cout << "libEmuSC chPressure [" << p.id() << "] = "
		      << _settings->get_param(PatchParam::ChannelPressure,
					      p.id())
		      << std::endl;
	}
      }
      break;

    case midi_PitchBend:
      {
	for (auto &p: _parts) {
	  if (p.midi_channel() == channel &&
	      _settings->get_param(PatchParam::RxPitchBend, p.id())) {

	    set_patch_param((uint16_t) PatchParam::PitchBend,
			    (uint8_t) ((data2 & 0x7f) >> 1), p.id());

	    // SC-55 line has 12 bit resolution on pitch wheel (that is 14 bit)
//	  if (_ctrlRom.synthModel == ControlRom::ss_SC55)
	    set_patch_param((uint16_t) PatchParam::PitchBend + 1,
			    (uint8_t) ((data1 & 0x7c) | (data2 << 7)), p.id());
//	    else               // SC-88 line has normal 14 bit resolution
//	    set_patch_param((uint16_t) PatchParam::PitchBend + 1,
//			    (uint8_t) ((data1 & 0x7f) | (data2 << 7)), p.id());

	    if (1)
	      std::cout << "libEmuSC: CTRL Pitch wheel [" << (int) p.id()
			<< "] = " <<
		_settings->get_param_uint16(PatchParam::PitchBend, p.id()) -8192
			<< std::endl;
	  }
	}
      }
      break;

    default:
      std::cout << "EmuSC MIDI: Unknown event received" << std::endl;
      break;
    }

  midiMutex.unlock();
}


void Synth::midi_input_sysex(uint8_t *data, uint16_t length)
{
  // First check if SysEx messages has been disabled
  if (!_settings->get_param(SystemParam::RxSysEx))
    return;
  
  // Verify correct SysEx status codes and Manufacturer ID: Roland = 0x41
  if (data[0] != 0xf0 || data[1] != 0x41 || data[length - 1] != 0xf7)
    return;

  // Verify correct SysEx Device ID
  if (data[2] != _settings->get_param(SystemParam::DeviceID) - 1)
    return;
  
  // Verify valid Model IDs: GSstandard (0x42) or SC-55/88 (0x45)
  if (data[3] != 0x42 && data[3] != 0x45)
    return;

  // Verify checksum (assuming 1 byte Device ID)
  int checksum = 0;
  for (int i = 5; i < length - 2; i++)
    checksum += (int) data[i];
  while (checksum >= 128)
    checksum -= 128;
  if (data[length - 2] != 128 - checksum) {
    std::cerr << "libEmuSC: Roland SysEx message received with corrupt "
	      << "checksum. Message discarded." << std::endl;
    return;
  }

  if (1) {
    std::cout << "libEmuSC: Valid SysEx  message received: ";
    for (int i = 0; i < length; i ++)
      std::cout << std::hex << (int) data[i] << " " << std::flush;
    std::cout << std::endl;
  }

  if (data[4] == 0x11) {
    std::cerr << "SysEx responses are not implemented yet" << std::endl;
    return;
  }

  midiMutex.lock();

  // Request data 1 (RQ1)
//  if (data[4] == 0x11)
//  _midi_input_sysex_RQ1(&data[5], length - 5 - 2); // Add reply data buffer

  // Data set 1 (DT1)
  if (data[4] == 0x12)
    _midi_input_sysex_DT1(data[3], &data[5], length - 5 - 2);

  midiMutex.unlock();
}

// TODO: Verify length when expexted data length is known!!
void Synth::_midi_input_sysex_DT1(uint8_t model, uint8_t *data, uint16_t length)
{
  if (length < 4)
    return;

  int p = 0;
  if (model == 0x42) {

    // System parameters
    if (data[0] == 0x40 && data[1] == 0x00) {

      // First handle the special case: Reset to the GSstandard mode message
      if (data[2] == 0x7f) {
	reset(true);
	return;
      }

      uint8_t dataLength;             // Based on SysEx chart in Owner's Manual
      if (data[2] == (int) SystemParam::Tune)
	dataLength = 4;
      else
	dataLength = 1;

      if (length - dataLength != 3) {
	std::cerr << "libemusc: Roland SysEx message has invalid data length! "
		  << "Message discarded." << std::endl;
	return;
      }

      _settings->set_system_param(data[2], &data[3], dataLength);

      for (const auto &cb : _partMidiModCallbacks)     // Update user interface
	cb(-1);

    // Patch parameters part 1: Address space 40 01 XX
    } else if (data[0] == 0x40 && data[1] == 0x01) {

      uint8_t dataLength = 1;          // Based on SysEx chart in Owner's Manual
      if (data[2] == 0x00 || data[2] == 0x10)
	dataLength = 0x10;

      if (length - dataLength != 3) {
	std::cerr << "libemusc: Roland SysEx message has invalid data length! "
		  << "Message discarded." << std::endl;
	return;
      }

      uint16_t a = data[2] | (data[1] << 8);
      _settings->set_patch_param(a, &data[3], dataLength);

      for (const auto &cb : _partMidiModCallbacks)     // Update user interface
	cb(-1);

    // Patch parameters part 2: Address spcae 40 1P XX (P = Part)
    } else if (data[0] == 0x40 && (data[1] & 0x10)) {

      // Set length of message based on SysEx chart in Owner's Manual
      uint8_t dataLength;
      switch (data[2])
	{
	case 0x00:
	case 0x17:
	  dataLength = 2;
	  break;
	case 0x40:
	  dataLength = 12;
	  break;
	default:
	  dataLength = 1;
	}

      if (length - dataLength != 3) {
	std::cerr << "libemusc: Roland SysEx message has invalid data length! "
		  << "Message discarded." << std::endl;
	return;
      }

      uint16_t address = data[2] | (data[1] << 8);
      _settings->set_patch_param(address, &data[3], dataLength);

      // Update user interface
      for (const auto &cb : _partMidiModCallbacks)
	cb(Settings::convert_from_roland_part_id(data[1] & 0x0f));

    // Part parameters, Block 2/2: Address 40 2P XX (P = Part)
    } else if (data[0] == 0x40 && (data[1] & 0x20)) {

      // All relevant messages has data length of 1 byte
      if (length != 4) {
	std::cerr << "libemusc: Roland SysEx message has invalid data length! "
		  << "Message discarded." << std::endl;
	return;
      }

      uint16_t address = data[2] | (data[1] << 8);
      _settings->set_patch_param(address, &data[3], 1);

    // Drum parameters: Address 41 MX XX (M = Map)
    } else if (data[0] == 0x41 && !(data[1] & 0xe0)) {

      // All relevant messages has data length of 1 byte
      if (length != 4) {
	std::cerr << "libemusc: Roland SysEx message has invalid data length! "
		  << "Message discarded." << std::endl;
	return;
      }

      bool map = data[1] & 0x10;
      std::vector<ControlRom::DrumSet> &drumset = _ctrlRom.get_drumsets_ref();

      std::cout << "Work in progress MAP=" << map << std::endl;

      // TODO: Allow to change name and key number (pitch)
//      switch (data[1] & 0x0f)
//      {}
    }
  }
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

  // Apply pan
  uint8_t pan = _settings->get_param(SystemParam::Pan);
  if (pan > 64)
    accumulatedSample[0] *= 1.0 - (pan - 64) / 63.0;
  else if (pan < 64)
    accumulatedSample[1] *= ((pan - 1) / 64.0);

  // Apply master volume conversion
  uint8_t volume = _settings->get_param(SystemParam::Volume);

  accumulatedSample[0] *= (volume / 127.0);
  accumulatedSample[1] *= (volume / 127.0);

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
  _settings->set_param_uint32(SystemParam::SampleRate, sampleRate);
  _settings->set_param(SystemParam::Channels, channels);

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

void Synth::set_part_mute(uint8_t partId, bool mute)
{
  _parts[partId].set_mute(mute);
}

void Synth::set_part_instrument(uint8_t partId, uint8_t index, uint8_t bank)
{
  _parts[partId].set_program(index, bank);
}


void Synth::add_part_midi_mod_callback(std::function<void(const int)> callback)
{
  _partMidiModCallbacks.push_back(callback);
}


void Synth::clear_part_midi_mod_callback(void)
{
  _partMidiModCallbacks.clear();
}


uint8_t Synth::get_param(enum SystemParam sp)
{
  return _settings->get_param(sp);
}


uint8_t* Synth::get_param_ptr(enum SystemParam sp)
{
  return _settings->get_param_ptr(sp);
}


uint16_t Synth::get_param_32nib(enum SystemParam sp)
{
  return _settings->get_param_32nib(sp);
}


uint8_t  Synth::get_param(enum PatchParam pp, int8_t part)
{
  return _settings->get_param(pp, part);
}


uint8_t* Synth::get_param_ptr(enum PatchParam pp, int8_t part)
{
  return _settings->get_param_ptr(pp, part);
}


uint8_t Synth::get_patch_param(uint16_t address, int8_t part)
{
  return _settings->get_patch_param(address, part);
}


void Synth::set_param(enum SystemParam sp, uint8_t value)
{
  _settings->set_param(sp, value);
}


void Synth::set_param(enum SystemParam sp, uint32_t value)
{
  _settings->set_param(sp, value);
}


void Synth::set_param(enum SystemParam sp, uint8_t *data, uint8_t size)
{
  _settings->set_param(sp, data, size);

}


void Synth::set_param_32nib(enum SystemParam sp, uint16_t value)
{
  _settings->set_param_32nib(sp, value);
}


void Synth::set_param(enum PatchParam pp, uint8_t value, int8_t part)
{
  _settings->set_param(pp, value, part);
}


void Synth::set_param(enum PatchParam pp, uint8_t *data, uint8_t size,
		      int8_t part)
{
  _settings->set_param(pp, data, size, part);
}


void Synth::set_patch_param(uint16_t address, uint8_t value, int8_t part)
{
  _settings->set_patch_param(address, value, part);
}

}
