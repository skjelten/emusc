/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022  Håkon Skjelten
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


#include "audio_output_alsa.h"

#include <iostream>

#include <signal.h>

#include <QSettings>


AudioOutputAlsa::AudioOutputAlsa(EmuSC::Synth *synth)
  : _synth(synth),
    _bufferTime(75000),
    _periodTime(25000),
    _sampleRate(44100),
    _channels(2)
{
  QSettings settings;
  QString audioDevice = settings.value("audio/device").toString();
  _bufferTime = settings.value("audio/buffer_time").toInt();
  _periodTime = settings.value("audio/period_time").toInt();
  _sampleRate = settings.value("audio/sample_rate").toInt();

  // First find the correct device name from our description
  QString deviceName = "default";
  void **hints, **n;
  
  if (snd_device_name_hint(-1, "pcm", &hints) < 0)
    throw (QString("Selected sound device not found!"));

  n = hints;
  while (*n != NULL) {
    char *description = snd_device_name_get_hint(*n, "DESC");
    
    if (!QString(description).simplified().compare(audioDevice)) {
      char *name = snd_device_name_get_hint(*n, "NAME");
      deviceName = name;
      delete (name);
    }

    delete (description);
    n++;
  }

  snd_device_name_free_hint(hints);

  // Open the selected PCM device in playback mode
  unsigned int ret;
  if (ret = snd_pcm_open(&_pcmHandle, deviceName.toStdString().c_str(),
			 SND_PCM_STREAM_PLAYBACK, 0) < 0)
    throw(QString("[ALSA] Can't open " + deviceName + " PCM device. " +
		  QString(snd_strerror(ret))));
 
  _set_hwparams();
  _set_swparams();

  if (0) {
    snd_output_t *output = NULL;
    snd_output_stdio_attach(&output, stdout, 0);
    snd_pcm_dump(_pcmHandle, output);
  }

  synth->set_audio_format(_sampleRate, _channels);
  
  std::cout << "EmuSC: Audio output [ALSA] successfully initialized" <<std::endl
	    << " -> device=\"" << deviceName.toStdString() << "\" (16 bit, "
	    << _sampleRate << " Hz, "
	    << _channels << " channels)" << std::endl;
}


AudioOutputAlsa::~AudioOutputAlsa()
{
  stop();
  snd_pcm_close(_pcmHandle);
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
    throw(QString("[ALSA] Can't set interleaved mode. " +
    QString(snd_strerror(ret))));

  // Set to signed 16-bit format
  if (ret = snd_pcm_hw_params_set_format(_pcmHandle, hwParams,
					 SND_PCM_FORMAT_S16_LE) < 0) 
    throw(QString("[ALSA] Can't set format. " + QString(snd_strerror(ret))));
  
  // Set number of channels
  if (ret = snd_pcm_hw_params_set_channels(_pcmHandle, hwParams, 2) < 0)
    throw(QString("[ALSA] Can't set channels number. " +
		  QString(snd_strerror(ret))));

  // Set sample rate
  if (ret = snd_pcm_hw_params_set_rate_near(_pcmHandle, hwParams,
					    &_sampleRate, 0) < 0) 
    throw(QString("[ALSA] Can't set rate. " + QString(snd_strerror(ret))));

  // Set the buffer time
  if (ret = snd_pcm_hw_params_set_buffer_time_near(_pcmHandle, hwParams,
						   &_bufferTime, &dir))
    throw(QString("[ALSA] Unable to set buffer time " +
		  QString::number(_bufferTime) + " for playback. " +
		  QString(snd_strerror(ret))));

  if (ret = snd_pcm_hw_params_get_buffer_size(hwParams, &size))
    throw(QString("[ALSA] Unable to get buffer size for playback. " +
     QString(snd_strerror(ret))));
  _bufferSize = size;

  // Set the period time
  if (ret = snd_pcm_hw_params_set_period_time_near(_pcmHandle, hwParams,
						   &_periodTime, &dir))
    throw (QString("[ALSA] Unable to set period time " +
		   QString::number(_periodTime) + " for playback. " +
		   QString(snd_strerror(ret))));

  if (ret = snd_pcm_hw_params_get_period_size(hwParams, &size, &dir))
    throw(QString("[ALSA] Unable to get period size for playback. " +
		  QString(snd_strerror(ret))));

  _periodSize = size;

  // Write parameters
  if (ret = snd_pcm_hw_params(_pcmHandle, hwParams) < 0)
    throw(QString("[ALSA] Can't set harware parameters. " +
		  QString(snd_strerror(ret))));

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
    throw(QString("[ALSA] Can't set software parameters. " +
          QString(snd_strerror(ret))));

  snd_pcm_sw_params_free(swParams);
}


// Only 16 bit supported
int AudioOutputAlsa::_fill_buffer(const snd_pcm_channel_area_t *areas,
				  snd_pcm_uframes_t offset,
				  snd_pcm_uframes_t frames,
				  EmuSC::Synth *synth)
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


void AudioOutputAlsa::start(void)
{
  _quit = false;
  _audioOutputThread = new std::thread(&AudioOutputAlsa::run, this);  
}


void AudioOutputAlsa::run(void)
{
  snd_pcm_format_t format = SND_PCM_FORMAT_S16;
  signed short *samples;
  samples = (signed short *) malloc((_periodSize * _channels * snd_pcm_format_physical_width(format)) / 8);
  if (samples == NULL) {
    std::cerr << "No enough memory" << std::endl;
    exit(EXIT_FAILURE);
  }

  snd_pcm_channel_area_t *areas;
  areas = (snd_pcm_channel_area_t *) calloc(_channels, sizeof(snd_pcm_channel_area_t));
  if (areas == NULL) {
    std::cerr << "No enough memory" << std::endl;
    exit(EXIT_FAILURE);
  }

  unsigned int chn;
  for (chn = 0; chn < _channels; chn++) {
    areas[chn].addr = samples;
    areas[chn].first = chn * snd_pcm_format_physical_width(format);
    areas[chn].step = _channels * snd_pcm_format_physical_width(format);
  }

  double phase = 0;
  signed short *ptr;
  int err, cptr;
 
  while (!_quit) {
    _fill_buffer(areas, 0, _periodSize, _synth);

    ptr = samples;
    cptr = _periodSize;
    while (cptr > 0) {
      err = snd_pcm_mmap_writei(_pcmHandle, ptr, cptr);
      if (err == -EAGAIN)
	continue;
      if (err < 0) {
	if (xrun_recovery(_pcmHandle, err) < 0) {
	  std::cerr << "Write error:" << snd_strerror(err) << std::endl;
	  exit(EXIT_FAILURE);
	}
	break;                                             // Skip one period
      }
      ptr += err * _channels;
      cptr -= err;
    }
  }
}


void AudioOutputAlsa::stop(void)
{
  _quit = true;

  // Wait for poll in thread to return
  if (_audioOutputThread) {
    _audioOutputThread->join();
    delete (_audioOutputThread), _audioOutputThread = NULL;
  }
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


QStringList AudioOutputAlsa::get_available_devices(void)
{
  QStringList deviceList;
  void **hints, **n;
  char *description, *io;

  if (snd_device_name_hint(-1, "pcm", &hints) < 0)
    return deviceList;

  n = hints;

  while (*n != NULL) {
    description = snd_device_name_get_hint(*n, "DESC");
    io = snd_device_name_get_hint(*n, "IOID");
    
    if (!QString(io).compare("Output") || QString(io).isEmpty())
      deviceList << QString(description).simplified();

    delete (description);
    delete (io);

    n++;
  }

  snd_device_name_free_hint(hints);
  
  return deviceList;
}


#endif  // __ALSA_AUDIO__
