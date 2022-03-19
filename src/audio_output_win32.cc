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


#ifdef __WIN32__


#include "audio_output_win32.h"
#include "ex.h"
#include "synth.h"

#include <iostream>


AudioOutputWin32::AudioOutputWin32(Config *config)
  : _channels(2),
    _sampleRate(44100)
{
  uint32_t bufferTime = 75000;
  if (!config->get("output_buffer_time").empty())
    bufferTime = stoi(config->get("output_buffer_time"));

  // Set correct audio device
  UINT deviceId = WAVE_MAPPER;
  if (!config->get("audio_device").empty())
    deviceId = stol(config->get("audio_device"));

  _eHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (_eHandle == NULL)
    throw (Ex(-1, "Win32 audio driver failed to create callback event"));

  WAVEFORMATEX pwfx;
  pwfx.wFormatTag = WAVE_FORMAT_PCM;
  pwfx.nChannels = _channels;
  pwfx.nSamplesPerSec = _sampleRate;
  pwfx.nBlockAlign = 4;
  pwfx.wBitsPerSample = 16;
  pwfx.nAvgBytesPerSec = _sampleRate * pwfx.nBlockAlign;

  MMRESULT res = waveOutOpen(&_hWave, WAVE_MAPPER, &pwfx, (DWORD_PTR) _eHandle,
			     (DWORD_PTR) NULL, CALLBACK_EVENT | WAVE_ALLOWSYNC);
  std::string error;
  switch (res)
    {
    case MMSYSERR_ALLOCATED: error.assign("MMSYSERR_ALLOCATED");
      break;
    case MMSYSERR_BADDEVICEID: error.assign("MMSYSERR_BADDEVICEID");
      break;
    case MMSYSERR_NODRIVER: error.assign("MMSYSERR_NODRIVER");
      break;
    case MMSYSERR_NOMEM: error.assign("MMSYSERR_NOMEM");
      break;
    case WAVERR_BADFORMAT: error.assign("WAVERR_BADFORMAT");
      break;
    case WAVERR_SYNC: error.assign("WAVERR_SYNC");
      break;
    }

  if (res != MMSYSERR_NOERROR)
    throw (Ex(-1, "Failed to initialize WIN32 audio output driver [" +
	      error + "]"));

  _bufferSize = ((pwfx.nAvgBytesPerSec * (bufferTime / 1000)) / (1000 * 2)) &~3;

  std::string deviceName = "default";
  WAVEOUTCAPS woc;
  if (!waveOutGetDevCaps(deviceId, &woc, sizeof(WAVEOUTCAPS)))
    deviceName.assign(woc.szPname);

  std::cout << "EmuSC: Audio output [Win32] successfully initialized"
	    << std::endl
	    << " -> device=\"" << deviceName << "\" ("
	    << pwfx.wBitsPerSample << " bit, "
	    << pwfx.nSamplesPerSec << " Hz, "
	    << pwfx.nChannels << " channels)" << std::endl;

  if (config->verbose())
    std::cout << "EmuSC: Win32 audio buffer = " << _bufferSize << "B"
	      << std::endl;
}


AudioOutputWin32::~AudioOutputWin32()
{
  CloseHandle(_eHandle);
}


void AudioOutputWin32::run(Synth *synth)
{
  char audioBuffer[2][_bufferSize];
  WAVEHDR wHeader[2];

  for (int i = 0; i < 2; i++) {
    wHeader[i].lpData = (LPSTR) audioBuffer[i];
    wHeader[i].dwBufferLength = _bufferSize;
    wHeader[i].dwFlags = 0;
    waveOutPrepareHeader(_hWave, &wHeader[i], sizeof(WAVEHDR));
    _fill_buffer(audioBuffer[i], synth);
    waveOutWrite(_hWave, &wHeader[i], sizeof(WAVEHDR));
  }

  bool index = 0;
  while (!_quit) {
    DWORD res = WaitForSingleObject(_eHandle, INFINITE);
    ResetEvent(_eHandle);

    switch(res)
      {
      case WAIT_OBJECT_0:
	{
	  waveOutUnprepareHeader(_hWave, &wHeader[index], sizeof(WAVEHDR));
	  wHeader[index].lpData = audioBuffer[index];
	  wHeader[index].dwBufferLength = _bufferSize;
	  wHeader[index].dwFlags = 0;
	  waveOutPrepareHeader(_hWave, &wHeader[index], sizeof(WAVEHDR));
	  _fill_buffer(audioBuffer[index], synth);
	  waveOutWrite(_hWave, &wHeader[index], sizeof(WAVEHDR));

	  index = !index;
	  break;
	}
      default:                   // Fixme: Add proper error messages and actions
	{
	  std::cerr << "Error during Win32 audio thread!" << std::endl;
	}
      }
  }

  waveOutUnprepareHeader(_hWave, &wHeader[0], sizeof(WAVEHDR));
  waveOutUnprepareHeader(_hWave, &wHeader[1], sizeof(WAVEHDR));
}


// Only 16 bit supported
int AudioOutputWin32::_fill_buffer(char *audioBuffer, Synth *synth)
{
  int i = 0;
  int16_t sample[_channels];

  int frames = _bufferSize / (2 * _channels);
  
  for (unsigned int frame = 0; frame < frames; frame++) {
    synth->get_next_sample(sample, _sampleRate, _channels);
    
    for (int channel=0; channel < _channels; channel++) {
      int16_t* dest = (int16_t*) &audioBuffer[(frame * 4) + (2 * channel)];
      *dest = sample[channel];
      i += 2;
    }
  }

  return i;
}


#endif  // __WIN32__
