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


#include "audio_output_alsa.h"
#include "ex.h"
#include "synth.h"

#include <iostream>

#include <signal.h>


AudioOutputAlsa::AudioOutputAlsa(Config *config, Synth *synth)
  : _synth(synth),
    _wakeup(0),
    _bufferTime(75000),
    _periodTime(25000),
    _sampleRate(44100),
    _channels(2)
{
  if (config->get("output_device").empty())
    _deviceName = "default";
  else
    _deviceName = config->get("output_device");

  if (!config->get("output_buffer_time").empty())
    _bufferTime = stoi(config->get("output_buffer_time"));
  if (!config->get("output_period_time").empty())
    _periodTime = stoi(config->get("output_period_time"));

  unsigned int ret;
  
  // Open the PCM device in playback mode
  if (ret = snd_pcm_open(&_pcmHandle, _deviceName.c_str(),
			 SND_PCM_STREAM_PLAYBACK, 0) < 0)  
    throw(Ex(-1, "[ALSA] Can't open " + _deviceName + " PCM device. " +
	     std::string(snd_strerror(ret))));

  _set_hwparams();
  _set_swparams();

  // Start audio callback
  if (ret = snd_async_add_pcm_handler(&_aHandler, _pcmHandle,
				      AudioOutputAlsa::alsa_callback,
				      (void*) this))
    throw(Ex(-1, "[ALSA] Unable to register async handler. " +
	     std::string(snd_strerror(ret))));

  const snd_pcm_channel_area_t *my_areas;
  snd_pcm_uframes_t offset, frames, size;
  snd_pcm_sframes_t commitres;
  int err, count;

  for (count = 0; count < 2; count++) {
    size = _periodSize;
    while (size > 0) {
      frames = size;
      err = snd_pcm_mmap_begin(_pcmHandle, &my_areas, &offset, &frames);
      if (err < 0) {
	if ((err = xrun_recovery(_pcmHandle, err)) < 0) {
	  throw (Ex(-1, "MMAP begin avail error: " +
		    std::string(snd_strerror(err))));
	}
      }

      for(unsigned int frame = 0; frame < frames; frame++) {
	int16_t sample[_channels];
	for (int ch = 0; ch < _channels; ch ++) {
	  sample[ch] = 0;
	}
	for (int channel=0; channel < _channels; channel++) {
	  int16_t* dest = (int16_t*) ( ((char*) my_areas[channel].addr)  
				       + (my_areas[channel].first >> 3)
				       + ( (my_areas[channel].step >> 3) *
					   (offset + frame) ) );

	  *dest = sample[channel];
	}
      }
      
      commitres = snd_pcm_mmap_commit(_pcmHandle, offset, frames);
      if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
//	if ((err = xrun_recovery(_pcmHandle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
//	  std::cerr << "MMAP commit error: " <<  snd_strerror(err) << std::endl;
//	  exit(EXIT_FAILURE);
//	}
      }
      size -= frames;
    }
  }


  if (ret = snd_pcm_start(_pcmHandle))
    throw(Ex(-1, "[ALSA] PCM start error. " + std::string(snd_strerror(ret))));

  synth->set_audio_format(_sampleRate, _channels);
  std::cout << "EmuSC: Audio output [ALSA] successfully initialized" <<std::endl
	    << " -> device=\"" << _deviceName << "\" (16 bit, "
	    << _sampleRate << " Hz, "
	    << _channels << " channels)" << std::endl;

  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGIO);
  pthread_sigmask(SIG_BLOCK, &sigmask, NULL);
}


AudioOutputAlsa::~AudioOutputAlsa()
{
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGIO);
  pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL);
}


void AudioOutputAlsa::_set_hwparams()
{
  snd_pcm_hw_params_t *hwParams;
  snd_pcm_uframes_t size;
  unsigned int ret;
  int dir;
  
  // Allocate parameters object for HW and fill it with default values
  snd_pcm_hw_params_malloc(&hwParams);
  snd_pcm_hw_params_any(_pcmHandle, hwParams);

  // Set interleaved mode
  if (ret = snd_pcm_hw_params_set_access(_pcmHandle, hwParams,
					 SND_PCM_ACCESS_MMAP_INTERLEAVED) < 0) 
    throw(Ex(-1, "[ALSA] Can't set interleaved mode. " +
	     std::string(snd_strerror(ret))));

  // Set to signed 16-bit format
  if (ret = snd_pcm_hw_params_set_format(_pcmHandle, hwParams,
					 SND_PCM_FORMAT_S16_LE) < 0) 
    throw(Ex(-1, "[ALSA] Can't set format. " + std::string(snd_strerror(ret))));
  
  // Set number of channels
  if (ret = snd_pcm_hw_params_set_channels(_pcmHandle, hwParams, 2) <0) 
    throw(Ex(-1, "[ALSA] Can't set channels number. " +
	     std::string(snd_strerror(ret))));

  // Set sample rate
  if (ret = snd_pcm_hw_params_set_rate_near(_pcmHandle, hwParams,
					    &_sampleRate, 0) < 0) 
    throw(Ex(-1, "[ALSA] Can't set rate. " + std::string(snd_strerror(ret))));


  
  // Set the buffer time
  if (ret = snd_pcm_hw_params_set_buffer_time_near(_pcmHandle, hwParams,
						   &_bufferTime, &dir))
    throw(Ex(-1, "[ALSA] Unable to set buffer time " +
	     std::to_string(_bufferTime) + " for playback. " +
	     std::string(snd_strerror(ret))));

  if (ret = snd_pcm_hw_params_get_buffer_size(hwParams, &size))
    throw(Ex(-1, "[ALSA] Unable to get buffer size for playback. " +
	     std::string(snd_strerror(ret))));
  _bufferSize = size;

  // Set the period time
  if (ret = snd_pcm_hw_params_set_period_time_near(_pcmHandle, hwParams,
						   &_periodTime, &dir))
    throw(Ex(-1, "[ALSA] Unable to set period time " +
	     std::to_string(_periodTime) + " for playback. " +
	     std::string(snd_strerror(ret))));

  if (ret = snd_pcm_hw_params_get_period_size(hwParams, &size, &dir))
    throw(Ex(-1, "[ALSA] Unable to get period size for playback. " +
	     std::string(snd_strerror(ret))));

  _periodSize = size;

  // Write parameters
  if (ret = snd_pcm_hw_params(_pcmHandle, hwParams) < 0)
    throw(Ex(-1, "[ALSA] Can't set harware parameters. " +
	     std::string(snd_strerror(ret))));

  snd_pcm_hw_params_free(hwParams);
}


// Add error checking
void AudioOutputAlsa::_set_swparams()
{
  snd_pcm_sw_params_t *swParams;
  unsigned int ret;

  // Allocate parameters object for SW and fill it with default values
  snd_pcm_sw_params_malloc(&swParams);
  snd_pcm_sw_params_current(_pcmHandle, swParams);

  snd_pcm_sw_params_set_start_threshold(_pcmHandle, swParams,
					_bufferSize - _periodSize);
  snd_pcm_sw_params_set_avail_min(_pcmHandle, swParams, _periodSize);

  if (ret = snd_pcm_sw_params(_pcmHandle, swParams) < 0)
    throw(Ex(-1, "[ALSA] Can't set software parameters. " +
	     std::string(snd_strerror(ret))));

  snd_pcm_sw_params_free(swParams);
}


void AudioOutputAlsa::alsa_callback(snd_async_handler_t *aHandler)
{
  AudioOutputAlsa *aoa =
    (AudioOutputAlsa *) snd_async_handler_get_callback_private(aHandler);

  aoa->_wakeup = 1;  
}


void AudioOutputAlsa::_private_callback(Synth *synth)
{
  if (0)
    std::cout << "ALSA private callback" << std::endl;

  const snd_pcm_channel_area_t *areas;
  snd_pcm_uframes_t offset, frames, size;
  snd_pcm_sframes_t avail, commitres;
  int first = 0, ret;

  while (true) {
    snd_pcm_state_t state = snd_pcm_state(_pcmHandle);
    if (state == SND_PCM_STATE_XRUN) {
      if (ret = xrun_recovery(_pcmHandle, -EPIPE) < 0)
	throw(Ex(-1, "[ALSA] XRUN recovery failed: " +
		 std::string(snd_strerror(ret))));
      first = 1;

    } else if (state == SND_PCM_STATE_SUSPENDED) {
      if (ret = xrun_recovery(_pcmHandle, -ESTRPIPE) < 0)
	throw(Ex(-1, "[ALSA] SUSPEND recovery failed: " +
		 std::string(snd_strerror(ret))));
    }

    avail = snd_pcm_avail_update(_pcmHandle);
    if (avail < 0) {
      if (ret = xrun_recovery(_pcmHandle, avail) < 0)
	throw(Ex(-1, "[ALSA] avail update failed: " +
		 std::string(snd_strerror(ret))));
      first = 1;
      continue;
    }

    if (avail < _periodSize) {
      if (first) {
	first = 0;
	if (ret = snd_pcm_start(_pcmHandle) < 0)
	  throw(Ex(-1, "[ALSA] PCM start error: " +
		   std::string(snd_strerror(ret))));
      } else {
	break;
      }
      continue;
    }
    size = _periodSize;

    while (size > 0) {
      frames = size;
      if (ret = snd_pcm_mmap_begin(_pcmHandle, &areas, &offset, &frames) < 0) {
	if ((ret = xrun_recovery(_pcmHandle, ret)) < 0)
	  throw(Ex(-1, "[ALSA] MMAP begin avail error: " +
		   std::string(snd_strerror(ret))));
	first = 1;
      }

      // Get samples from synth and push to mmap'd area
      if (_fill_buffer(areas, offset, frames, synth) < 0)
	throw(Ex(-1, "Unable to get next audio samples from synth"));
      
      commitres = snd_pcm_mmap_commit(_pcmHandle, offset, frames);
      if (commitres < 0 || (snd_pcm_uframes_t) commitres != frames) {
	if ((ret = xrun_recovery(_pcmHandle, commitres>=0 ? -EPIPE : commitres))
	    < 0)
	  throw(Ex(-1, "[ALSA] MMAP commit error: " +
		   std::string(snd_strerror(ret))));
	first = 1;
      }
      size -= frames;
    }
  }
}


// Only 16 bit supported
int AudioOutputAlsa::_fill_buffer(const snd_pcm_channel_area_t *areas,
				  snd_pcm_uframes_t offset,
				  snd_pcm_uframes_t frames,
				  Synth *synth)
{
  int16_t sample[_channels];

  for (unsigned int frame = 0; frame < frames; frame++) {
    synth->get_next_sample(sample);   // FIXME: Assumes 16 bit, 44.1 kHz, 2 ch

    for (int channel=0; channel < _channels; channel++) {
      int16_t* dest =
	(int16_t*) ( ((char*) areas[channel].addr)  
		     + (areas[channel].first >> 3)
		     + ( (areas[channel].step >> 3) * (offset + frame) ) );

      *dest = sample[channel];
    }
  }

  return 0;
}


// Underrun and suspend recovery
int AudioOutputAlsa::xrun_recovery(snd_pcm_t *handle, int err)
{
  std::cerr << "EmuSC: ALSA stream recovery" << std::endl;

  if (err == -EPIPE) {        // Under-run
    err = snd_pcm_prepare(handle);
    if (err < 0)
      std::cerr << "Can't recovery from underrun, prepare failed: "
		<< snd_strerror(err) << std::endl;
    return 0;

  } else if (err == -ESTRPIPE) {
    while ((err = snd_pcm_resume(handle)) == -EAGAIN)
      sleep(1);               // Wait until the suspend flag is released
    if (err < 0) {
      err = snd_pcm_prepare(handle);
      if (err < 0)
	std::cerr << "Can't recovery from suspend, prepare failed: "
		  << snd_strerror(err) << std::endl;
    }
    return 0;
  }

  return err;
}


void AudioOutputAlsa::run(void)
{
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGIO);
  pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL);

  while (!_quit) {

    if (!_wakeup && !_quit) {
      sleep(1);
    }

    if (_wakeup) {
      _wakeup = 0;
  
     _private_callback(_synth);
    }
  }

  snd_pcm_close(_pcmHandle);
}



#endif  // __ALSA__
