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


#ifdef __ALSA_AUDIO__


#ifndef AUDIO_OUTPUT_ALSA_H
#define AUDIO_OUTPUT_ALSA_H


#include "audio_output.h"

#include "emusc/synth.h"

#include <string>
#include <thread>

#include <QString>
#include <QStringList>

#include <alsa/asoundlib.h>


class AudioOutputAlsa: public AudioOutput
{
private:
  snd_pcm_t *_pcmHandle;

  EmuSC::Synth *_synth;

  std::thread *_audioOutputThread;
  
  int _channels;
  unsigned int _sampleRate;

  unsigned int _bufferTime;       // Ring buffer length in us
  unsigned int _periodTime;       // Period time in us

  snd_pcm_sframes_t _bufferSize;
  snd_pcm_sframes_t _periodSize;
  
  std::string _deviceName;
  
  void _set_hwparams(void);
  void _set_swparams(void);
  
  int _fill_buffer(const snd_pcm_channel_area_t *areas,
		   snd_pcm_uframes_t offset,
		   snd_pcm_uframes_t frames,
		   EmuSC::Synth *synth);

  AudioOutputAlsa();

public:
  AudioOutputAlsa(EmuSC::Synth *synth);
  virtual ~AudioOutputAlsa();

  static int xrun_recovery(snd_pcm_t *handle, int err);

  void start(void);
  void stop(void);

  void run(void);

  static QStringList get_available_devices(void);
  
};


#endif  // AUDIO_OUTPUT_ALSA_H


#endif  // __ALSA_AUDIO__
