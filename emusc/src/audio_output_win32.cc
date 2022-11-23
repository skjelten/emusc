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


#ifdef __WIN32_AUDIO__


#include "audio_output_win32.h"

#include <iostream>

#include <QSettings>


AudioOutputWin32::AudioOutputWin32(EmuSC::Synth *synth)
  : _synth(synth),
    _channels(2)
{
  QSettings settings;
  QString device = settings.value("audio/device").toString();
  int bufferTime = settings.value("audio/buffer_time").toInt();
  int periodTime = settings.value("audio/period_time").toInt();
  _sampleRate = settings.value("audio/sample_rate").toInt();

  WAVEFORMATEX pwfx;
  pwfx.wFormatTag = WAVE_FORMAT_PCM;
  pwfx.nChannels = _channels;
  pwfx.nSamplesPerSec = _sampleRate;
  pwfx.nBlockAlign = 4;
  pwfx.wBitsPerSample = 16;
  pwfx.nAvgBytesPerSec = _sampleRate * pwfx.nBlockAlign;

  UINT n = waveOutGetNumDevs();
  UINT deviceID = -1;

  for (int i = 0; i < n; ++i) {
    WAVEOUTCAPS caps;
    MMRESULT res = waveOutGetDevCaps(i, &caps, sizeof(WAVEOUTCAPS));
    if (res == MMSYSERR_NOERROR && !device.compare(caps.szPname)) {
      deviceID = i;
      break;
    }
  }

  if (deviceID < 0)
    throw(QString("MIDI device '" + device + "' not found!"));

  _eHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (_eHandle == NULL)
    throw (QString("Win32 audio driver failed to create callback event"));

  MMRESULT res = waveOutOpen(&_hWave, WAVE_MAPPER, &pwfx, (DWORD_PTR) _eHandle,
			     (DWORD_PTR) NULL, CALLBACK_EVENT | WAVE_ALLOWSYNC);
  QString error;
  switch (res)
    {
    case MMSYSERR_ALLOCATED: error = "MMSYSERR_ALLOCATED";
      break;
    case MMSYSERR_BADDEVICEID: error = "MMSYSERR_BADDEVICEID";
      break;
    case MMSYSERR_NODRIVER: error = "MMSYSERR_NODRIVER";
      break;
    case MMSYSERR_NOMEM: error = "MMSYSERR_NOMEM";
      break;
    case WAVERR_BADFORMAT: error = "WAVERR_BADFORMAT";
      break;
    case WAVERR_SYNC: error = "WAVERR_SYNC";
      break;
    }

  if (res != MMSYSERR_NOERROR) {
    throw (QString("Failed to initialize WIN32 audio output driver [" +
		   error + "]"));
  }

  _bufferSize = ((pwfx.nAvgBytesPerSec * (bufferTime / 1000)) / (1000 * 2)) &~3;
  synth->set_audio_format(_sampleRate, _channels);
}


AudioOutputWin32::~AudioOutputWin32()
{
  CloseHandle(_eHandle);
}


void AudioOutputWin32::start(void)
{
  _quit = false;
  _audioOutputThread = new std::thread(&AudioOutputWin32::run, this);  
}


void AudioOutputWin32::run(void)
{
  MMRESULT res;
  char audioBuffer[2][_bufferSize];
  WAVEHDR wHeader[2];

  for (int i = 0; i < 2; i++) {
    wHeader[i].lpData = (LPSTR) audioBuffer[i];
    wHeader[i].dwBufferLength = _bufferSize;
    wHeader[i].dwLoops = 0;
    wHeader[i].dwFlags = 0;

    ResetEvent(_eHandle);

    res = waveOutPrepareHeader(_hWave, &wHeader[i], sizeof(WAVEHDR));
    if (res != MMSYSERR_NOERROR)
      std::cerr << "EmuSC: Error when preparing waveOut header" << std::endl;

    _fill_buffer(audioBuffer[i]);

    res = waveOutWrite(_hWave, &wHeader[i], sizeof(WAVEHDR));
    if (res != MMSYSERR_NOERROR)
      std::cerr << "EmuSC: Error when writing audio wave data" << std::endl;
  }

  bool index = 0;
  while (!_quit) {
    DWORD event = WaitForSingleObject(_eHandle, INFINITE);
    ResetEvent(_eHandle);

    if (event == WAIT_OBJECT_0) {
      _fill_buffer(audioBuffer[index]);

      res = waveOutWrite(_hWave, &wHeader[index], sizeof(WAVEHDR));
      if (res != MMSYSERR_NOERROR)
	std::cerr << "EmuSC: Error while writing audio wave data (WIN32)"
		  << std::endl;

      index = !index;
    }
  }

  waveOutUnprepareHeader(_hWave, &wHeader[0], sizeof(WAVEHDR));
  waveOutUnprepareHeader(_hWave, &wHeader[1], sizeof(WAVEHDR));
}


void AudioOutputWin32::stop(void)
{
  _quit = true;

  // Wait for poll in thread to return
  if (_audioOutputThread)
    _audioOutputThread->join();
}


// Only 16 bit supported
int AudioOutputWin32::_fill_buffer(char *audioBuffer)
{
  int i = 0;
  int16_t sample[_channels];

  int frames = _bufferSize / (2 * _channels);
  
  for (unsigned int frame = 0; frame < frames; frame++) {
    _synth->get_next_sample(sample);
    
    for (int channel=0; channel < _channels; channel++) {
      int16_t* dest = (int16_t*) &audioBuffer[(frame * 4) + (2 * channel)];
      *dest = sample[channel];
      i += 2;
    }
  }

  return i;
}


QStringList AudioOutputWin32::get_available_devices(void)
{
  QStringList deviceList;
  UINT n = waveOutGetNumDevs();

  for (int i = 0; i < n; ++i) {
    WAVEOUTCAPS caps;
    MMRESULT res = waveOutGetDevCaps(i, &caps, sizeof(WAVEOUTCAPS));
    if (res == MMSYSERR_NOERROR)
      deviceList << caps.szPname;
  }

  return deviceList;
}



#endif  // __WIN32_AUDIO__
