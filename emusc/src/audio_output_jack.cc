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


#ifdef __JACK_AUDIO__


#include "audio_output_jack.h"

#include <iostream>
#include <string>


AudioOutputJack::AudioOutputJack(EmuSC::Synth *synth)
  : _synth(synth),
    _sampleRate(44100),
    _channels(2)
{
  const char *server_name = NULL;

  jack_options_t options = JackNullOption;
  jack_status_t status;

  _client = jack_client_open("EmuSC", options, &status, server_name);
  if (_client == NULL) {
    if (status & JackServerFailed)
      throw(std::string("Unable to connect to JACK server"));
    else
      throw(std::string("jack_client_open() failed"));
  }

  if (status & JackServerStarted)
    std::cout << "JACK server started" << std::endl;

  if (status & JackNameNotUnique)
    std::cout << "EmuSC warning: JACK client name changed to "
	      << jack_get_client_name(_client) << std::endl;

  jack_set_process_callback(_client, callback, this);
  jack_on_shutdown(_client, shutdown, this);

  // TODO: Search for available ports and verify that we have CHANNELS available
  for (int i = 0; i < _channels; i ++) {
    std::string output = "output_" + std::to_string(i+1);

    _port[i] = jack_port_register(_client, output.c_str(),
				  JACK_DEFAULT_AUDIO_TYPE,
				  JackPortIsOutput, 0);
    if (_port[i] == NULL)
      throw(std::string("No more JACK ports available"));
  }

  _sampleRate = jack_get_sample_rate(_client);
  synth->set_audio_format(_sampleRate, _channels);

  std::cout << "EmuSC: Audio output [JACK] successfully initialized ("
	    << _sampleRate << " Hz)" << std::endl;
}


AudioOutputJack::~AudioOutputJack()
{
  stop();
  jack_client_close(_client);
}


int AudioOutputJack::callback(jack_nframes_t nframes, void *arg)
{
  AudioOutputJack *aoj = (AudioOutputJack *) arg;
  return aoj->_fill_buffer(nframes);
}


// TODO: Add support for float directly from synth->get_next_sample() to avoid
//       unnecessary conversion float -> int16 -> float
int AudioOutputJack::_fill_buffer(jack_nframes_t nframes)
{
  int16_t sample[_channels];

  jack_default_audio_sample_t *out[_channels];
  for (int i = 0; i < _channels; i ++)
    out[i] = (jack_default_audio_sample_t *) jack_port_get_buffer(_port[i],
								  nframes);

  for (unsigned int frame = 0; frame < nframes; frame++) {
    _synth->get_next_sample(sample);

    for (int i = 0; i < _channels; i ++) {      
      float *o = (float *) (out[i] + frame);
      *o =  (float) sample[i] / (1 << 15) * _volume;
    }
  }

  return 0;
}


void AudioOutputJack::shutdown(void *arg)
{
  AudioOutputJack *aoj = (AudioOutputJack *) arg;
  aoj->_shutdown();
}


// TODO: Fix proper handling of unexpected shutdowns
void AudioOutputJack::_shutdown(void)
{
  std::cout << "EmuSC error: JACK server shut down" << std::endl;
}


void AudioOutputJack::start(void)
{
  if (jack_activate(_client))
    throw(std::string("JACK Audio error: cannot activate client"));  

  const char **ports = jack_get_ports(_client, NULL, NULL,
				      JackPortIsPhysical | JackPortIsInput);
  if (ports == NULL)
    std::cerr << "EmuSC ERROR [JACK Audio]: No physical playback ports"
	      << std::endl;

  for (int i = 0; i < _channels; i ++)
    if (jack_connect(_client, jack_port_name(_port[i]), ports[i]))
      std::cerr << "EmuSC ERROR [JACK Audio]: cannot connect output ports"
		<< std::endl;

  delete ports;
}


void AudioOutputJack::stop(void)
{
  jack_deactivate(_client);
}


// TODO: Add list of running servers?
QStringList AudioOutputJack::get_available_devices(void)
{
  QStringList deviceList;

  return deviceList;
}


#endif  // __JACK_AUDIO__
