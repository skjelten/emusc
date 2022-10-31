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


#ifndef AUDIO_OUTPUT_WIN32_H
#define AUDIO_OUTPUT_WIN32_H


#include "audio_output.h"

#include "emusc/synth.h"

#include <thread>

#include <QStringList>
#include <QString>

#include <windows.h>
#include <mmsystem.h>


class AudioOutputWin32: public AudioOutput
{
private:
  EmuSC::Synth *_synth;

  std::thread *_audioOutputThread;

  HANDLE _eHandle;
  HWAVEOUT _hWave;

  int _channels;
  unsigned int _sampleRate;

  uint32_t _bufferSize;

  int _fill_buffer(char *audioBuffer);

  AudioOutputWin32();

public:
  AudioOutputWin32(EmuSC::Synth *synth);
  ~AudioOutputWin32();

  void start();
  void run(void);
  void stop();
  
  static QStringList get_available_devices(void);

};


#endif  // AUDIO_OUTPUT_WIN32_H


#endif  // __WIN32_AUDIO__
