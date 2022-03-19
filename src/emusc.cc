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


#include "audio_output.h"
#include "audio_output_null.h"
#include "config.h"
#include "control_rom.h"
#include "midi_input.h"
#include "midi_input_keyboard.h"
#include "pcm_rom.h"
#include "synth.h"

#include "ex.h"

#include <iostream>
#include <csignal>
#include <thread>

#include <unistd.h>

#ifdef __PULSE__
#include "audio_output_pulse.h"
#endif

#ifdef __CORE__
#include "midi_input_core.h"
#include "audio_output_core.h"
#endif

#ifdef __ALSA__
#include "midi_input_alsa.h"
#include "audio_output_alsa.h"
#endif

#ifdef __WIN32__
#include <windows.h>
#include "midi_input_win32.h"
#include "audio_output_win32.h"
#endif


bool quit = false;
void ext_quit(int sig)
{
  quit = true;
}


void display_error_msg(std::string message)
{
#ifdef __WIN32__
  MessageBox(NULL, message.c_str(), "Info", MB_OK | MB_ICONERROR);
#else
  std::cerr << "EmuSC: Error during initialization:" << std::endl
	    << "  -> " << message << std::endl;
#endif
}


// Needs to be rewritten
// ControlRom & PcmRom dumping their own data to disk
void dump_rom_data(Config *config)
{
  std::cout << "This feature is currently disabled" << std::endl;
  /*
  ControlRom *ctrlRom;
  PcmRom *pcmRom;

  ctrlRom = new ControlRom(config->get("control_rom"), config->verbose());

    std::vector<std::string> romPath;
    for (int i = 1; i < 4; i ++) {
      if (!config->get(std::string("pcm_rom_" + std::to_string(i))).empty())
	romPath.push_back(config->get(std::string("pcm_rom_" +
						  std::to_string(i))));
    }

    pcmRom = new PcmRom(romPath, *ctrlRom);
  */
}


void dump_midi_files(Config *config)
{
  ControlRom *ctrlRom;
  try {
    ctrlRom = new ControlRom(config->get("control_rom"), config->verbose());

  } catch(Ex ex) {
    display_error_msg(ex.errorMsg);
    return;
  }

  ctrlRom->dump_demo_songs(config->get("dump-midi"));
  delete ctrlRom;
}


void run_synth(Config *config)
{
  Synth *synth = NULL;
  MidiInput *midiInput = NULL;
  AudioOutput *audioOutput = NULL;
  ControlRom *ctrlRom;
  PcmRom *pcmRom;

  try {
    ctrlRom = new ControlRom(config->get("control_rom"), config->verbose());

    std::vector<std::string> romPath;
    for (int i = 1; i < 5; i ++) {
      if (!config->get(std::string("pcm_rom_" + std::to_string(i))).empty())
	romPath.push_back(config->get(std::string("pcm_rom_" +
						  std::to_string(i))));
    }
    pcmRom = new PcmRom(romPath, *ctrlRom);

    // Initialize synth
    synth = new Synth(*config, *ctrlRom, *pcmRom);

    // Initialize MIDI input [ alsa | win32 | core | keyboard ]
    if (config->get("input") == "alsa") {
#ifdef __ALSA__
      midiInput = new MidiInputAlsa();
#else
      throw(Ex(-1, "'alsa' MIDI input is missing in this build"));
#endif
    } else if (config->get("input") == "win32") {
#ifdef __WIN32__
      midiInput = new MidiInputWin32(config, synth);
#else
      throw(Ex(-1, "'win32' MIDI input is missing in this build"));
#endif
    } else if (config->get("input") == "core") {
#ifdef __CORE__
      midiInput = new MidiInputCore(synth);
#else
      throw(Ex(-1, "'core' MIDI input is missing in this build"));
#endif
    } else if (config->get("input") == "keyboard") {
      midiInput = new MidiInputKeyboard();
    }
    if (!midiInput)
      throw(Ex(-1, "No valid 'input' defined, check configuration file"));

    // Initialize audio output [ alsa | pulse | win32 | core | null ]
    if (config->get("output") == "alsa") {
#ifdef __ALSA__
      audioOutput = new AudioOutputAlsa(config);
#else
      throw(Ex(-1, "'alsa' audio ouput is missing in this build"));
#endif
    } else if (config->get("output") == "win32") {
#ifdef __WIN32__
      audioOutput = new AudioOutputWin32(config);
#else
      throw(Ex(-1, "'win32' audio ouput is missing in this build"));
#endif
    } else if (config->get("output") == "core") {
#ifdef __CORE__
      audioOutput = new AudioOutputCore(config, synth);
#else
      throw(Ex(-1, "'core' audio ouput is missing in this build"));
#endif
    } else if (config->get("output") == "pulse") {
#ifdef __PULSE__
      audioOutput = new AudioOutputPulse(config, synth);
#else
      throw(Ex(-1, "'pulse' audio ouput is missing in this build"));
#endif
    } else if (config->get("output") == "null") {
      audioOutput = new AudioOutputNull(config);
    }
    if (!audioOutput)
      throw(Ex(-1, "No valid audio output defined, check configuration file"));

  } catch(Ex ex) {
    display_error_msg(ex.errorMsg);
    return;
  }
  
  std::signal(SIGINT, ext_quit);
  std::signal(SIGTERM, ext_quit);

  std::thread audioThread(&AudioOutput::run, audioOutput, synth);
  std::thread midiThread(&MidiInput::run, midiInput, synth);
  
  while(!quit) { sleep(1); }

  std::cout << "\nEmuSC: Signal INT / TERM received, exiting..." << std::endl;

  audioOutput->stop();  
  midiInput->stop();

  audioThread.join();
  midiThread.join();

  delete audioOutput;
  delete midiInput;
  delete ctrlRom;
  delete pcmRom;
  delete synth;
}


int main(int argc, char *argv[])
{
  Config *config = NULL;

  try {
    config = new Config(argc, argv);
  } catch(Ex ex) {
    display_error_msg(ex.errorMsg);
    return -1;
  }

  if (!config->get("dump-rom-data").empty()) {
    dump_rom_data(config);
  } else if (!config->get("dump-midi").empty()) {
    dump_midi_files(config);
  } else {
    run_synth(config);
  }

  delete config;
}
