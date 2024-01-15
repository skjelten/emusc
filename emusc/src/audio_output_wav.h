/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2024  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC. If not, see <http://www.gnu.org/licenses/>.
 */


#ifdef __WAV_AUDIO__


#ifndef AUDIO_OUTPUT_WAV_H
#define AUDIO_OUTPUT_WAV_H


#include "audio_output.h"

#include "emusc/synth.h"

#include <thread>


class AudioOutputWav: public AudioOutput
{
public:
  AudioOutputWav(EmuSC::Synth *synth);
  ~AudioOutputWav();

  void start();
  void run(void);
  void stop();

private:
  EmuSC::Synth *_synth;

  std::thread *_audioOutputThread;

  int _channels;
  unsigned int _sampleRate;

  int _fill_buffer(int8_t *data, size_t length);

  AudioOutputWav();

};


#endif  // AUDIO_OUTPUT_WAV_H


#endif  // __WAV_AUDIO__
