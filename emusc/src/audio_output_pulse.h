/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2023  Håkon Skjelten
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


#ifdef __PULSE_AUDIO__


#ifndef AUDIO_OUTPUT_PULSE_H
#define AUDIO_OUTPUT_PULSE_H


#include "audio_output.h"

#include "emusc/synth.h"

#include <thread>

#include <pulse/pulseaudio.h>


class AudioOutputPulse: public AudioOutput
{
private:
  AudioOutputPulse();

  EmuSC::Synth *_synth;
  
  std::thread *_audioOutputThread;

  uint32_t _sampleRate;
  uint8_t _channels;

  pa_mainloop* _mainLoop;
  pa_mainloop_api *_mainLoopApi;

  pa_sample_spec _sampleSpec;
  pa_context *_context;
  pa_stream *_stream;

  pa_volume_t _volume;

  // Private callbacks
  void _context_state_callback(pa_context *c);
  void _stream_write_callback(pa_stream *s,size_t length);
  void _stream_state_callback(pa_stream *s);

  int _fill_buffer(int8_t *data, size_t length);

public:
  AudioOutputPulse(EmuSC::Synth *synth);
  ~AudioOutputPulse();

  void start();
  void run(void);
  void stop(void);

  // Static functions for external callbacks -> calling private callbacks
  static void context_state_callback(pa_context *c, void *userdata);
  static void stream_write_callback(pa_stream *s,size_t length, void *userdata);
  static void stream_state_callback(pa_stream *s, void *userdata);

};


#endif  // AUDIO_OUTPUT_PULSE_H


#endif  // __PULSE__
