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


#ifdef __CORE_AUDIO__

#ifndef AUDIO_OUTPUT_CORE_H
#define AUDIO_OUTPUT_CORE_H


#include "audio_output.h"

#include "emusc/synth.h"

#include <QStringList>

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>

#include <AudioUnit/AudioUnit.h>


class AudioOutputCore: public AudioOutput
{
private:
  AudioUnit _audioUnit;

  EmuSC::Synth *_synth;
  uint8_t _channels;
  uint32_t _sampleRate;
  
  OSStatus _callback(AudioUnitRenderActionFlags *ioActionFalgs,
		     const AudioTimeStamp *inTimeStamp,
		     UInt32 inBusNumber,
		     UInt32 inNumberFrames,
		     AudioBufferList *ioData);
  int _fill_buffer(AudioBufferList *data, UInt32 frames);
  
  AudioOutputCore();

public:
  AudioOutputCore(EmuSC::Synth *synth);
  ~AudioOutputCore();

  void start(void);
  void stop(void);
  static OSStatus callback(void *inRefCon,
			   AudioUnitRenderActionFlags *ioActionFalgs,
			   const AudioTimeStamp *inTimeStamp,
			   uint32_t inBusNumber,
			   uint32_t inNumberFrames,
			   AudioBufferList *ioData);

  static QStringList get_available_devices(void);
};


#endif  // AUDIO_OUTPUT_CORE_H

#endif  // __CORE_AUDIO__
