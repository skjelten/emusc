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


#ifdef __ALSA__


#ifndef __AUDIO_OUTPUT_ALSA_H__
#define __AUDIO_OUTPUT_ALSA_H__


#include "audio_output.h"
#include "config.h"
#include "synth.h"

#include <string>

#include <alsa/asoundlib.h>


class AudioOutputAlsa: public AudioOutput
{
private:
  snd_pcm_t *_pcmHandle;
  snd_async_handler_t *_aHandler;

  volatile int _wakeup;
  
  int _channels;
  unsigned int _sampleRate;

  unsigned int _bufferTime;       // Ring buffer length in us
  unsigned int _periodTime;       // Period time in us

  snd_pcm_sframes_t _bufferSize;
  snd_pcm_sframes_t _periodSize;
  
  std::string _deviceName;
  
  void _set_hwparams(void);
  void _set_swparams(void);
  
  void _private_callback(Synth *synth);
  int _fill_buffer(const snd_pcm_channel_area_t *areas,
		   snd_pcm_uframes_t offset,
		   snd_pcm_uframes_t frames,
		   Synth *synth);
  
  AudioOutputAlsa();

public:
  AudioOutputAlsa(Config *config);
  virtual ~AudioOutputAlsa();

  static int xrun_recovery(snd_pcm_t *handle, int err);
  static void alsa_callback(snd_async_handler_t *aHandler);

  virtual void run(Synth *synth);

};


#endif  // __AUDIO_OUTPUT_ALSA_H__


#endif  // __ALSA__
