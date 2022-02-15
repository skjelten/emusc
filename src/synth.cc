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

// Much of the source code for decoding and storing ROM data is based on
// the SC55 Soundfont Converter written by Kitrinx and NewRisingSun.
// https://github.com/Kitrinx/SC55_Soundfont


#include "synth.h"
#include "ex.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <mutex>

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


Synth::Synth(Config &config)
  : _config(config),
    _bank(0),
    _effects(16),
    _volume(127),
    _pan(0),
    _reverb(64),
    _chours(64),
    _keyShift(0),
    _masterTune(440.0),
    _reverbType(5),
    _choursType(3)
{
  ControlRom controlRom(config);

  controlRom.get_instruments(_instruments);
  controlRom.get_partials(_partials);
  controlRom.get_samples(_samples);
  controlRom.get_variations(_variations);
  controlRom.get_drumSet(_drumSets);

  PcmRom pcmRom(config);

  // Add raw samples data from the PCM ROM to our samples vector
  for (ControlRom::Sample &s : _samples)
    pcmRom.get_samples(&s.sampleSet, s.address, s.sampleLen);

  std::cout << "EmuSC: Found " << _instruments.size() << " instruments, "
	    << _partials.size() << " parts, "
	    << _samples.size() << " samples and "
	    << _drumSets.size() << " drum sets" << std::endl;
  
  // Note: SC-88 also has a "SC-55" mode
  std::string mode = _config.get("mode");
  std::transform(mode.begin(), mode.end(), mode.begin(), ::toupper);
  if (!mode.empty() && mode != "MT32" & mode != "GS")
    throw (Ex(-1, "Unsupported mode " + mode + ". [GS | MT32]"));
  if (mode == "GS" || mode.empty())
    _mode = mode_GS;
  else
    _mode = mode_MT32;

  for (int i = 0; i < 16; i++) {
    Part part(i, _mode, 0, _instruments, _partials, _samples, _drumSets);
    _parts.push_back(part);
  }
  if (_mode == mode_GS)
    std::cout << "EmuSC: GS mode initialized" << std::endl;
  else
    std::cout << "EmuSC: MT-32 mode initialized (not impl. yet!)" << std::endl;

  // Mute selected parts
  if (!_config.get_vect_int("mute").empty()) {
    std::vector<int> muteVect = _config.get_vect_int("mute");
    for (auto i : muteVect) _parts[i].set_mute(true);
  } else if (!_config.get_vect_int("mute-except").empty()) {
    std::vector<int> muteExVect = _config.get_vect_int("mute-except");
    for (auto &p : _parts) p.set_mute(true);
    for (auto i : muteExVect) _parts[i].set_mute(false);
  }

  if (!_config.get_vect_int("mute").empty() ||
      !_config.get_vect_int("mute-except").empty()) {
    std::cout << "EmuSC: Muted parts [ ";
    for (std::size_t i = 0; i < _parts.size(); i++)
      if (_parts[i].mute()) std::cout << i << " ";
    std::cout << "]" << std::endl;
  }

  // Dump data to disk if requested -- call from separate function
  if (!config.get("dump-pcm-rom").empty())
    pcmRom.dump_rom(config.get("dump-pcm-rom"));
  if (!config.get("dump-rom-data").empty())
    _dump_rom_data(config.get("dump-rom-data"));
  if (!config.get("dump-midi").empty())
    controlRom.dump_demo_songs(config.get("dump-midi"));
}


Synth::~Synth()
{}


bool Synth::_dump_rom_data(std::string rootPath)
{
  if (rootPath.back() != '/')
    rootPath.append("/");
  rootPath.append("emusc-pcm-dump");

  std::cout << "EmuSC: Dumping ROM data to " << rootPath << std::endl;
  
  if (mkdir(rootPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    throw (Ex(-1, "Failed to create directory " + rootPath));

  int i = 0;
  std::string drumSetDir = rootPath + "/drumsets";
  if (mkdir(drumSetDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    throw (Ex(-1, "Failed to create directory " + drumSetDir));

  for(const auto& d: _drumSets) {
    std::string drumSetPath = rootPath + "/drumsets/" + _drumSets[i].name;
    mkdir(drumSetPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    
    std::ofstream file(std::string(drumSetPath + "/drumsets.ini"));
    file << "Preset=";
    for (int x = 0; x < 128; x++)
      file << (int) _drumSets[i].preset[x] << ",";
    file << std::endl << "Volume=";
    for (int x = 0; x < 128; x++)
      file << (int) _drumSets[i].volume[x] << ",";
    file << std::endl << "Key=";
    for (int x = 0; x < 128; x++)
      file << (int) _drumSets[i].key[x] << ",";
    file << std::endl << "Assign group=";
    for (int x = 0; x < 128; x++)
      file << (int) _drumSets[i].assignGroup[x] << ",";
    file << std::endl << "Panpot=";
    for (int x = 0; x < 128; x++)
      file << (int) _drumSets[i].panpot[x] << ",";
    file << std::endl << "Reverb=";
    for (int x = 0; x < 128; x++)
      file << (int) _drumSets[i].reverb[x] << ",";
    file << std::endl << "Chorus=";
    for (int x = 0; x < 128; x++)
      file << (int) _drumSets[i].chorus[x] << ",";
    file << std::endl << "Flags=";
    for (int x = 0; x < 128; x++)
      file << (int) _drumSets[i].flags[x] << ",";
    file << std::endl;

    file.close();
    i++;
  }

  std::string sampleDir = rootPath + "/samples";
  if (mkdir(sampleDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    throw (Ex(-1, "Failed to create directory " + sampleDir));  

  i=0;
  for(const auto& prt: _partials) {
    char setNum[5];
    snprintf(setNum, 5, "%.4hx", i++);
    sampleDir.assign(rootPath + "/samples/set_" + setNum);

    if (mkdir(sampleDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
      throw (Ex(-1, "Failed to create dir: " + sampleDir));

    std::ofstream name(std::string(sampleDir + "/" + prt.name + ".name"));
    name.close();
    
    std::ofstream file(std::string(sampleDir + "/samples.ini"));
    file << prt.name << std::endl << "breaks=";
    for (int x = 0; x < 16; x++)
      file << (int) prt.breaks[x] << ",";
    file << std::endl << "samples=";
    for (int x = 0; x < 16; x++)
      file << (int) prt.samples[x] << ",";
    file << std::endl;
    file.close();

    for (int x = 0; x < 16; x++)
      if (prt.samples[x] != 0xffff)
	_export_sample_24(_samples[prt.samples[x]].sampleSet,
			  sampleDir + "/samples_" + std::to_string(x) +".wav");
  }

  // Dump instrument (preset) table
  std::string presetsDir = rootPath + "/presets";
  if (mkdir(presetsDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    throw (Ex(-1, "Failed to create directory " + presetsDir));
  i = 0;
  for(const auto& inst: _instruments) {
    char prNum[5];
    snprintf(prNum, 5, "%.4hx", i++);
    std::string presetPath = presetsDir + "/" + prNum;
    if (mkdir(presetPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
      throw (Ex(-1, "Failed to create directory " + presetPath));

    std::ofstream name(std::string(presetPath + "/" + inst.name + ".name"));
    name.close();

    std::ofstream file(std::string(presetPath + "/preset.ini"));
    file << inst.name << std::endl;
    file.close();

    std::string partialDir = presetPath + "/partials";
    if (mkdir(partialDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
      throw (Ex(-1, "Failed to create directory " + partialDir));

    for (int x = 0; x < 2; x++) {
      if (inst.partials[x].partialIndex == 0xffff)
	continue;
      
      std::string partDir = partialDir + "/" + std::to_string(x+1);
      if (mkdir(partDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
	throw (Ex(-1, "Failed to create directory " + partDir));

      std::ofstream file(std::string(partDir + "/partial.ini"));
      file << "Partial index=" << inst.partials[x].partialIndex << std::endl;
      file.close(); // Add a lot of more data
    }
  }

  // Dump variations table
  std::string variationDir = rootPath + "/variations";
  if (mkdir(variationDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    throw (Ex(-1, "Failed to create directory " + variationDir));  
  i = 0;
  for(const auto& v: _variations) {
    char chNum[3];
    snprintf(chNum, 3, "%.2hx", i);
    std::string variationPath = variationDir + "/channel_" + chNum;
    if (mkdir(variationPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
      throw (Ex(-1, "Failed to create directory " + variationPath));

    std::ofstream file(std::string(variationPath + "/variations.ini"));
    for (int x = 0; x < 128; x++) {
      if (_variations[i].variation[x] != 0xffff) {
	file << "[" << i << "]=" << _variations[i].variation[x] << std::endl;

	// Try to add sym-links for file systems that supports it
	char prNum[5];
	snprintf(prNum, 5, "%.4x", _variations[i].variation[x]);
	std::string target("../../presets/");
	target.append(prNum);
	std::string linkPath(variationPath + "/instrument_"+std::to_string(i));
	// Ignore error on symlinks if file system does not support it
	int ret = symlink(target.c_str(), linkPath.c_str());
      }
    }
    file.close();
    i++;
  }

  return 0;
}


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


// Finds correct instrument variation from variations table
// Implemented according to SC-55 Owner's Manual page 42-45
uint16_t Synth::_get_instrument_from_variations(int bank, int preset)
{
  uint16_t instrument = _variations[bank].variation[preset];

  if (bank < 63 && preset < 120)
    while (instrument == 0xffff)
      instrument = _variations[--bank].variation[preset];

  return instrument;
}


void Synth::midi_input(struct MidiInput::MidiEvent *midiEvent)
{
  midiMutex.lock();

  switch (midiEvent->type)
    {
    case MidiInput::se_PrgChange:
      if (_config.verbose())
	std::cout << "EmuSC MIDI: Program change [ch=" <<(int)midiEvent->channel
		  << " bank=" << (int) _bank
		  << " program=" << (int) midiEvent->data1 << "]" << std::endl;
      {
	uint16_t instrument = _get_instrument_from_variations(_bank,
							      midiEvent->data1);
	for (auto &p : _parts)
	  p.set_program(midiEvent->channel, midiEvent->data1, instrument);
      }
      break;

    case MidiInput::se_CtrlChange:
      if (_config.verbose())
	std::cout << "EmuSC MIDI: Control change [ch=" <<(int)midiEvent->channel
		  << " status byte=" << (int) midiEvent->status
		  << " data1=0x" <<  (int) midiEvent->data1
		  << " data2=0x" << (int) midiEvent->data2 << "]" << std::endl;
      {
	// MIDI CC messages
	enum Part::ControlMsg cMsg = Part::cmsg_Unknown;
	if (midiEvent->status == 0) {                          // Bank select
	  _bank = midiEvent->data1;
	} else if (midiEvent->status == 7) {                   // Ch volume
	  cMsg = Part::cmsg_Volume;
	} else if (midiEvent->status == 10) {                  // Ch pan
	  cMsg = Part::cmsg_Pan;
	}

	if (cMsg == Part::cmsg_Unknown)
	  break;

	for (auto &p : _parts)
	  p.set_control(cMsg, midiEvent->channel, midiEvent->data1);
      }
      break;
    
    case MidiInput::se_PitchBend:                        // Data -8192 <-> 8192
      if (_config.verbose())
	std::cout << "EmuSC MIDI: Pitchbend [ch=" << (int) midiEvent->channel
		  << " data=" << (int) midiEvent->data << "]" << std::endl;
      _effects[midiEvent->channel].pitchbend = midiEvent->data / 8192.0;
      break;

    case MidiInput::se_ChPressure:                        // Data -8192 <-> 8192
      if (_config.verbose())
	std::cout << "EmuSC MIDI: Ch. pressure [ch=" << (int) midiEvent->channel
		  << " data=" << (int) midiEvent->data << "]" << std::endl;
      // TODO: Not implemented
      break;

    case MidiInput::se_KeyPressure:
      if (_config.verbose())
	std::cout << "EmuSC MIDI: Key pressure [ch=" << (int) midiEvent->channel
		  << " data=" << (int) midiEvent->data << "]" << std::endl;
      // TODO: Not implemented
      break;

    case MidiInput::se_NoteOn:
      if (_config.verbose())
	std::cout << "EmuSC MIDI: Note On [ch=" << (int) midiEvent->channel
		  << " data1=" << std::hex << (int) midiEvent->data1
		  << " data2=" << std::hex << (int) midiEvent->data2 << "]"
		  << std::endl; 
      
      if (!midiEvent->data2) {          // Note On with velocity = 0 => Note Off
	for (auto &p: _parts)
	  p.delete_note(midiEvent->channel, midiEvent->data1);
      } else {
	for (auto &p: _parts)
	  p.add_note(midiEvent->channel, midiEvent->data1, midiEvent->data2);
      }
      break;

    case MidiInput::se_NoteOff: 
      if (_config.verbose())
	std::cout << "EmuSC MIDI: Note Off [ch=" << (int) midiEvent->channel
		  << " data1=" << std::hex << (int) midiEvent->data1
		  << " data2=" << std::hex << (int) midiEvent->data2 << "]"
		  << std::endl; 
      
      // TODO: Change state to ADSR release
      for (auto &p: _parts)
	p.delete_note(midiEvent->channel, midiEvent->data1);
     break;

    case MidiInput::se_Sysex:
      if (_config.verbose()) {
	std::cout << "EmuSC MIDI: Sysex message [length="
		  << (int) midiEvent->data1 << " data:";
	for (int i = 0; i < midiEvent->data1; i++)
	  std::cout << " " << std::hex << std::setw(2) <<(int)midiEvent->ptr[i];
	std::cout << "]" << std::dec << std::endl;
      }
	// TODO: Not implemented
      break;

    case MidiInput::se_Cancel:
      if (_config.verbose())
	std::cout << "EmuSC MIDI: Clear all notes" << std::endl;
	for (auto &p: _parts)
	p.clear_all_notes();
      break;
     
    default:
      std::cout << "EmuSC MIDI: Unknown event received" << std::endl;
      break;
    }

  midiMutex.unlock();
}


int Synth::get_next_sample(int16_t *sampleOut, int sampleRate, int channels)
{
  int32_t partSample[2];
  int32_t accumulatedSample[2] = { 0, 0 };

  // Write silence
  for (int i = 0; i < channels; i++) {
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


  // Apply all kinds of filters and stuff on global data
  // See 
  _volume = 100;

  // Apply pan
  if (_pan > 0)
    accumulatedSample[0] *= 1.0 - (_pan / 63.0);
  else if (_pan < 0)
    accumulatedSample[1] *= 1.0 - (abs(_pan) / 63.0);

  // Apply master volume and sample rate conversion
  accumulatedSample[0] *= (_volume / 127.0) * (32000.0 / sampleRate);
  accumulatedSample[1] *= (_volume / 127.0) * (32000.0 / sampleRate);
  
  // Send to audio output. FIXME: Add support for PAN (stereo)!
  for (int c = 0; c < channels; c++)
    sampleOut[c] += (int16_t) (accumulatedSample[c] >> 16);  // >> 16 original

  return 0;
}
