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
#include "midi_input.h"
#include "midi_input_keyboard.h"
#include "synth.h"

#include "ex.h"

#include <iostream>
#include <csignal>
#include <thread>

#include <unistd.h>

#ifdef __PULSE__
#include "audio_output_pulse.h"
#endif

#ifdef __ALSA__
#include "midi_input_alsa.h"
#include "audio_output_alsa.h"
#endif

#ifdef __WIN32__
#include <windows.h>
#include "audio_output_win32.h"
#endif


bool quit = false;
void ext_quit(int sig)
{
  quit = true;
}


int main(int argc, char *argv[])
{
  Config *config = NULL;
  Synth *synth = NULL;
  MidiInput *midiInput = NULL;
  AudioOutput *audioOutput = NULL;

  try {
    // Open configuration and parse parameters
    config = new Config(argc, argv);

    // Initialize synth including ROM files and internal data structurs
    synth = new Synth(*config);

    // If we are only doing disk dump of various parts, exit immediately
    if (!config->get("dump-pcm-rom").empty() ||
	!config->get("dump-rom-data").empty() ||
      	!config->get("dump-midi").empty())
      exit(0);

    // Initialize MIDI input [alsa | keyboard]
    if (config->get("input") == "alsa") {
#ifdef __ALSA__
      midiInput = new MidiInputAlsa();
#else
      throw(Ex(-1, "'alsa' MIDI input is missing in this build"));
#endif
    } else if (config->get("input") == "keyboard") {
      midiInput = new MidiInputKeyboard();
    }
    if (!midiInput)
      throw(Ex(-1, "No valid 'input' defined, check configuration file"));

    // Initialize audio output [alsa]
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
    if (!ex.errorNr) {
#ifdef __WIN32__
      MessageBox(NULL, ex.errorMsg.c_str(), "Info", MB_OK | MB_ICONERROR);
#else
      std::cout << ex.errorMsg << std::endl << std::endl;
#endif
      return 0;
    }

#ifdef __WIN32__
    MessageBox(NULL, ex.errorMsg.c_str(), "Info", MB_OK | MB_ICONERROR);
#else
    std::cout << "EmuSC: Error during initialization:" << std::endl
	      << "  -> " << ex.errorMsg << std::endl;
#endif
    return -1;
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
  delete synth;
  delete config;
}
