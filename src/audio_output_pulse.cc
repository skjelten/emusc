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


#ifdef __PULSE__


#include "audio_output_pulse.h"
#include "ex.h"

#include <string>
#include <iostream>

#include <string.h>
#include <signal.h>
AudioOutputPulse::AudioOutputPulse(Config *config, Synth *synth)
  : _synth(synth),
    _sampleRate(44100),
    _channels(2),
    _volume(PA_VOLUME_NORM),
    _mainLoop(NULL),
    _mainLoopApi(NULL),
    _context(NULL),
    _stream(NULL)
{
  // Set sample specification
  _sampleSpec.rate = _sampleRate;
  _sampleSpec.channels = _channels;
  _sampleSpec.format = PA_SAMPLE_S16NE;

  if (!pa_sample_spec_valid(&_sampleSpec))
    throw (Ex(-1, "Pulse error: Sample spec invalid"));

  // Set up a new main loop
  if (!(_mainLoop = pa_mainloop_new()))
    throw (Ex(-1, "Pulse error: pa_mainloop_new() failed"));

  _mainLoopApi = pa_mainloop_get_api(_mainLoop);

// Fails
//  if (pa_signal_init(_mainLoopApi) == 0)
//    throw (Ex(-1, "Pulse error: pa_signal_init() failed"));

#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#endif

  // Create a new connection context
  if (!(_context = pa_context_new(_mainLoopApi, "EmuSC")))
    throw (Ex(-1, "Pulse error: pa_context_new() failed"));

  pa_context_set_state_callback(_context, context_state_callback, this);

  pa_context_flags_t f;
  if (pa_context_connect(_context, NULL, f, NULL) < 0)
    throw (Ex(-1, "Pulse error: pa_context_connect() failed"));

  char spec[PA_SAMPLE_SPEC_SNPRINT_MAX];
  pa_sample_spec_snprint(spec, sizeof(spec), &_sampleSpec);

  std::cout << "EmuSC: Audio output [PulseAudio] successfully initialized"
	    << std::endl << " -> " << spec << std::endl;
}


AudioOutputPulse::~AudioOutputPulse()
{
  if (_stream)
    pa_stream_unref(_stream);

  if (_context)
    pa_context_unref(_context);

  if (_mainLoop) {
    pa_signal_done();
    pa_mainloop_free(_mainLoop);
  }
}


// This is called whenever the context status changes
void AudioOutputPulse::_context_state_callback(pa_context *c)
{  
  if (!c) {
    std::cerr << "EmuSC: Error in context state callback" << std::endl;
    return;
  }
  
  switch (pa_context_get_state(c))
    {
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
      break;

    case PA_CONTEXT_READY: {
      pa_cvolume cv;

      // FIXME - TIL CONFIG!
      char *device = NULL;

      pa_stream_flags_t f;
      
      _stream = pa_stream_new(c, "stream_name", &_sampleSpec, NULL);
      if (!_stream) {
	std::cerr << "EmuSC: PulseAudio error when creating new stream"
		  << std::endl;
	return;
      }

      pa_stream_set_state_callback(_stream, stream_state_callback, this);
      pa_stream_set_write_callback(_stream, stream_write_callback, this);
      pa_stream_connect_playback(_stream, device, NULL, f,
				 pa_cvolume_set(&cv, _channels, _volume), NULL);

      break;
    }

    case PA_CONTEXT_TERMINATED:
      _quit = true;
      break;

    case PA_CONTEXT_FAILED:
    default:
      std::cerr << "EmuSC: PulseAudio error \"Connection failure\""
		<< std::endl;
      _quit = true;
    }
}


// This is called whenever new data may be written to the stream
void AudioOutputPulse::_stream_write_callback(pa_stream *s, size_t length)
{
  if (!s || length == 0) {
    std::cerr << "EmuSC: Error in stream write callback" << std::endl;
    return;
  }

  void *data = pa_xmalloc(length);
  int bytes = _fill_buffer((int8_t *) data, length);

  if (bytes > 0)
    pa_stream_write(s, data, (size_t) bytes, pa_xfree, 0, PA_SEEK_RELATIVE);
  else
    pa_xfree(data);

  if (0)
    std::cout << "DEBUG: Pulse write cb recieved length=" << (int) length
	      << "and wrote " << bytes << " to PA stream" << std::endl;
}


// Only 16 bit supported
int AudioOutputPulse::_fill_buffer(int8_t *data, size_t length)
{
  int i = 0;
  int16_t sample[_channels];

  int frames = length / 4;
  
  for (unsigned int frame = 0; frame < frames; frame++) {
    _synth->get_next_sample(sample, _sampleRate, _channels);// FIXME 16/24/32bit

    if (*sample > 0xffffff)
      std::cout << "Audio overflow" << std::endl;
    
    for (int channel=0; channel < _channels; channel++) {
      int16_t* dest = (int16_t *) &data[(frame * 4) + (2 * channel)];
      *dest = sample[channel];
      i += 2;
    }
  }

  return i;
}


void AudioOutputPulse::_stream_state_callback(pa_stream *s)
{
  if (!s) {
    std::cerr << "EmuSC: Stream state callback called with NULL pointer"
	      << std::endl;
    return;
  }

  switch (pa_stream_get_state(s))
    {
    case PA_STREAM_CREATING:
    case PA_STREAM_TERMINATED:
    case PA_STREAM_READY:
      break;

    case PA_STREAM_FAILED:
    default:
      std::cerr << "EmuSC: Stream error, "
		<< pa_strerror(pa_context_errno(pa_stream_get_context(s)))
		<< std::endl;
      _quit = true;
    }
}


void AudioOutputPulse::run(Synth *synth)
{
  int ret;

  if (!_quit && pa_mainloop_run(_mainLoop, &ret) < 0)
    std::cerr << "EmuSC: PulseAudio main loop failed to run" << std::endl;
}


void AudioOutputPulse::stop()
{
  pa_mainloop_quit(_mainLoop, 0);
}


void AudioOutputPulse::context_state_callback(pa_context *c, void *userdata)
{
  AudioOutputPulse *aop = (AudioOutputPulse *) userdata;
  aop->_context_state_callback(c);
}


void AudioOutputPulse::stream_write_callback(pa_stream *s, size_t length,
					     void *userdata)
{
  AudioOutputPulse *aop = (AudioOutputPulse *) userdata;
  aop->_stream_write_callback(s, length);
}


void AudioOutputPulse::stream_state_callback(pa_stream *s, void *userdata)
{
  AudioOutputPulse *aop = (AudioOutputPulse *) userdata;
  aop->_stream_state_callback(s);
}


#endif  // __PULSE__
