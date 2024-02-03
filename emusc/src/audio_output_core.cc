/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2024  Håkon Skjelten
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

#include "audio_output_core.h"

#include <QSettings>
#include <QString>

#include <iostream>


const QString CoreDefaultDevice = "System Sound Output Device";


AudioOutputCore::AudioOutputCore(EmuSC::Synth *synth)
  : _synth(synth),
    _channels(2),
    _sampleRate(44100)
{
  QSettings settings;
  QString audioDevice = settings.value("Audio/device").toString();
//  _bufferTime = settings.value("Audio/buffer_time").toInt();
//  _periodTime = settings.value("Audio/period_time").toInt();
  _sampleRate = settings.value("Audio/sample_rate").toInt();

  if (audioDevice.compare(CoreDefaultDevice))
    throw (QString("Only '" + CoreDefaultDevice +
		   "' is currently supported on macOS"));

  std::cout << "Opening CORE AUDIO with device=" << audioDevice.toStdString()
	    << std::endl;

  OSStatus result;
  
  AudioComponentDescription aCompDesc;
  aCompDesc.componentType = kAudioUnitType_Output;
  aCompDesc.componentSubType = kAudioUnitSubType_DefaultOutput;
  aCompDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
  aCompDesc.componentFlags = 0;
  aCompDesc.componentFlagsMask = 0;

  AudioComponent aComp = AudioComponentFindNext(NULL, &aCompDesc);
  if (aComp == NULL)
    throw (QString("Can't find CoreAudio unit matching given description"));

  result = AudioComponentInstanceNew(aComp, &_audioUnit);
  if (result != noErr)
    throw (QString("Couldn't create a new instance of a CoreAudio component"));

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
    throw (QString("Couldn't set the data format for CoreAudio unit"));
  */
  AURenderCallbackStruct rCallback;
  rCallback.inputProc = this->callback;
  rCallback.inputProcRefCon = this;
  result = AudioUnitSetProperty(_audioUnit,
				kAudioUnitProperty_SetRenderCallback,
				kAudioUnitScope_Input, 0,
				&rCallback, sizeof(rCallback));
  if (result != noErr)
    throw (QString("Couldn't set CoreAudio render callback"));
  
  result = AudioUnitInitialize(_audioUnit);
  if (result != noErr)
    throw (QString("Couldn't initialize CoreAudio unit"));

  synth->set_audio_format(_sampleRate, _channels);
  std::cout << "EmuSC: Audio output [Core] successfully initialized"
	    << std::endl;
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
    _synth->get_next_sample(sample);
    *left = (Float32) sample[0] / (1 << 15) * _volume;
    *right = (Float32) sample[1] / (1 << 15) * _volume;

    left++;
    right++;
    i ++;
  }

  return i;
}


void AudioOutputCore::start(void)
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


QStringList AudioOutputCore::get_available_devices(void)
{
  QStringList deviceList;
  deviceList << CoreDefaultDevice;
  
  AudioObjectPropertyAddress propertyAddress = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  UInt32 dataSize = 0;
  OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
						   &propertyAddress,
						   0,
						   NULL,
						   &dataSize);
  if (status != kAudioHardwareNoError) {
    std::cerr << "AudioObjectGetPropertyDataSize failed: " << status
	      << std::endl;
    return deviceList;
  }

  UInt32 deviceCount = (UInt32) (dataSize / sizeof(AudioDeviceID));

  AudioDeviceID *audioDevices = (AudioDeviceID *) (malloc(dataSize));
  if (!audioDevices) {
    std::cerr << "Unable to allocate memory for AudioDeviceID" << std::endl;
    return deviceList;
  }

  status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
				      &propertyAddress,
				      0,
				      NULL,
				      &dataSize,
				      audioDevices);
  if (status != kAudioHardwareNoError) {
    std::cerr << "AudioObjectGetPropertyData failed: " << status << std::endl;
    free(audioDevices);
    audioDevices = NULL;
    return deviceList;
  }

  // Look for output devices
  propertyAddress.mScope = kAudioDevicePropertyScopeOutput;

  for (UInt32 i = 0; i < deviceCount; ++i) {
  /*
  CFStringRef deviceUID = NULL;
    dataSize = sizeof(deviceUID);
    propertyAddress.mSelector = kAudioDevicePropertyDeviceUID;
    status = AudioObjectGetPropertyData(audioDevices[i],
					&propertyAddress,
					0,
					NULL,
					&dataSize,
					&deviceUID);
    if (status != kAudioHardwareNoError) {
      std::cerr << "AudioObjectGetPropertyData failed: " << status << std::endl;
      continue;
    }
  */
    // Iterate through all device names
    CFStringRef deviceName = NULL;
    dataSize = sizeof(deviceName);
    propertyAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
    status = AudioObjectGetPropertyData(audioDevices[i],
					&propertyAddress,
					0,
					NULL,
					&dataSize,
					&deviceName);
    if (status != kAudioHardwareNoError) {
      std::cerr << "AudioObjectGetPropertyData failed: " << status << std::endl;
      continue;
    }

    // Determine if the device is an output device
    dataSize = 0;
    propertyAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
    status = AudioObjectGetPropertyDataSize(audioDevices[i],
					    &propertyAddress,
					    0,
					    NULL,
					    &dataSize);
    if (status != kAudioHardwareNoError) {
      std::cerr << "AudioObjectGetPropertyDataSize failed: " << status
		<< std::endl;
      continue;
    }
    
    AudioBufferList *bufferList = (AudioBufferList *)(malloc(dataSize));
    if (!bufferList) {
      std::cerr << "Unable to allocate memory for AudioBufferList" << std::endl;
      break;
    }

    status = AudioObjectGetPropertyData(audioDevices[i],
					&propertyAddress,
					0,
					NULL,
					&dataSize,
					bufferList);
    if (status != kAudioHardwareNoError || bufferList->mNumberBuffers == 0) {
      if (status != kAudioHardwareNoError)
	std::cerr << "AudioObjectGetPropertyData failed: " << status
		  << std::endl;
      free(bufferList);
      bufferList = NULL;
      continue;
    }
    UInt32 numBuffers = bufferList->mNumberBuffers;
    
    deviceList << QString(CFStringGetCStringPtr(deviceName,
						kCFStringEncodingMacRoman));

    /*
    for (UInt8 j = 0; j < numBuffers; j++) {
      AudioBuffer ab = bufferList->mBuffers[j];
      printf("\n#Channels: %d DataByteSize: %d", ab.mNumberChannels, ab.mDataByteSize);
    }
    */
    free(bufferList), bufferList = NULL;
  }

  free(audioDevices);
    
  return deviceList;
}


#endif  // __CORE_AUDIO__
