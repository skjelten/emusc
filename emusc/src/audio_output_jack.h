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


#ifdef __JACK_AUDIO__


#ifndef AUDIO_OUTPUT_JACK_H
#define AUDIO_OUTPUT_JACK_H


#include "audio_output.h"

#include "emusc/synth.h"

#include <QString>
#include <QStringList>

#include <jack/jack.h>


class AudioOutputJack: public AudioOutput
{
private:
  jack_port_t *_port[2];
  jack_client_t *_client;

  EmuSC::Synth *_synth;

  int _channels;
  unsigned int _sampleRate;

  int _fill_buffer(jack_nframes_t nframes);

  void _shutdown(void);

  AudioOutputJack();

public:
  AudioOutputJack(EmuSC::Synth *synth);
  virtual ~AudioOutputJack();

  static int callback(jack_nframes_t nframes, void *arg);
  static void shutdown(void *arg);

  void start(void);
  void stop(void);

  static QStringList get_available_devices(void);

};


#endif  // AUDIO_OUTPUT_JACK_H


#endif  // __JACK_AUDIO__
