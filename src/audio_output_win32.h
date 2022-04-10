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


#ifndef __AUDIO_OUTPUT_WIN32_H__
#define __AUDIO_OUTPUT_WIN32_H__


#include "audio_output.h"
#include "config.h"
#include "synth.h"

#include <string>

#include <windows.h>
#include <mmsystem.h>


class AudioOutputWin32: public AudioOutput
{
private:
  Synth *_synth;

  HANDLE _eHandle;
  HWAVEOUT _hWave;

  int _channels;
  unsigned int _sampleRate;

  uint32_t _bufferSize;

  int _fill_buffer(char *audioBuffer);

  AudioOutputWin32();

public:
  AudioOutputWin32(Config *config, Synth *synth);
  ~AudioOutputWin32();

  void run(void);

};


#endif  // __AUDIO_OUTPUT_WIN32_H__


#endif  // __WIN32__
