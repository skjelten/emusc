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


#ifdef __CORE__

#include "audio_output_core.h"
#include "ex.h"

#include <iostream>


AudioOutputCore::AudioOutputCore(Config *config, Synth *synth)
  : _synth(synth),
    _channels(2),
    _sampleRate(44100)
{
  OSStatus result;
  
  AudioComponentDescription aCompDesc;
  aCompDesc.componentType = kAudioUnitType_Output;
  aCompDesc.componentSubType = kAudioUnitSubType_DefaultOutput;
  aCompDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
  aCompDesc.componentFlags = 0;
  aCompDesc.componentFlagsMask = 0;

  AudioComponent aComp = AudioComponentFindNext(NULL, &aCompDesc);
  if (aComp == NULL)
    throw (Ex(-1, "Can't find CoreAudio unit matching given description"));

  result = AudioComponentInstanceNew(aComp, &_audioUnit);
  if (result != noErr)
    throw (Ex(-1, "Couldn't create a new instance of a CoreAudio component"));

  AudioStreamBasicDescription aFormat = {0};
  aFormat.mFormatID = kAudioFormatLinearPCM;
  aFormat.mFormatFlags = kLinearPCMFormatFlagIsPacked;
  aFormat.mChannelsPerFrame = 2;
  aFormat.mSampleRate = 44100;
  aFormat.mFramesPerPacket = 1;
  aFormat.mBitsPerChannel = 16;
  aFormat.mBytesPerFrame = aFormat.mChannelsPerFrame*aFormat.mBitsPerChannel/8;
  aFormat.mFramesPerPacket = 1;
  aFormat.mBytesPerPacket = aFormat.mBytesPerFrame * aFormat.mFramesPerPacket;
  /*
  result = AudioUnitSetProperty(_audioUnit,
				kAudioUnitProperty_StreamFormat,
				kAudioUnitScope_Input, 0,
				&aFormat, sizeof(aFormat));
  if (result != noErr)
    throw (Ex(-1, "Couldn't set the data format for CoreAudio unit"));
  */
  AURenderCallbackStruct rCallback;
  rCallback.inputProc = this->callback;
  rCallback.inputProcRefCon = this;
  result = AudioUnitSetProperty(_audioUnit,
				kAudioUnitProperty_SetRenderCallback,
				kAudioUnitScope_Input, 0,
				&rCallback, sizeof(rCallback));
  if (result != noErr)
    throw (Ex(-1, "Couldn't set CoreAudio render callback"));
  
  result = AudioUnitInitialize(_audioUnit);
  if (result != noErr)
    throw (Ex(-1, "Couldn't initialize CoreAudio unit"));

  std::cout << "EmuSC: Core audio output initialized" << std::endl;
}


AudioOutputCore::~AudioOutputCore()
{
  AudioUnitUninitialize(_audioUnit);
  AudioComponentInstanceDispose(_audioUnit);
}


OSStatus AudioOutputCore::_callback(AudioUnitRenderActionFlags *ioActionFalgs,
				    const AudioTimeStamp *inTimeStamp,
				    UInt32 inBusNumber,
				    UInt32 inNumberFrames,
				    AudioBufferList *ioData)
{
  _fill_buffer(ioData, inNumberFrames);
  return noErr;
}


// Only 16 bit supported
int AudioOutputCore::_fill_buffer(AudioBufferList *data, UInt32 frames)
{
  int i = 0;
  int16_t sample[_channels];

  Float32 *left = (Float32 *) data->mBuffers[0].mData;
  Float32 *right = (Float32 *) data->mBuffers[1].mData;
	  
  for (unsigned int frame = 0; frame < frames; frame++) {
    _synth->get_next_sample(sample, _sampleRate, _channels);//FIXME 16/24/32bit
    *left = (Float32) sample[0] / (1 << 15);
    *right = (Float32) sample[1] / (1 << 15);

    left++;
    right++;
    i ++;
  }

  return i;
}


void AudioOutputCore::run(Synth *synth)
{
  OSStatus result = AudioOutputUnitStart(_audioUnit);
  if (result != noErr)
    std::cerr << "EmuSC: Couldn't start CoreAudio playback" << std::endl;
}


void AudioOutputCore::stop(void)
{
  OSStatus result = AudioOutputUnitStop(_audioUnit);
  if (result != noErr)
    std::cerr << "EmuSC: Couldn't stop CoreAudio playback" << std::endl;
}


OSStatus AudioOutputCore::callback(void *inRefCon,
				   AudioUnitRenderActionFlags *ioActionFalgs,
				   const AudioTimeStamp *inTimeStamp,
				   UInt32 inBusNumber,
				   UInt32 inNumberFrames,
				   AudioBufferList *ioData)
{
  AudioOutputCore *aoc = (AudioOutputCore *) inRefCon;
  return aoc->_callback(ioActionFalgs, inTimeStamp, inBusNumber,
			inNumberFrames, ioData);
}


#endif  // __CORE__
