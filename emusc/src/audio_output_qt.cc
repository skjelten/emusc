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


#ifdef __QT_AUDIO__


#include "audio_output_qt.h"

#include <iostream>

#include <QSettings>
#include <QString>


AudioOutputQt::AudioOutputQt(EmuSC::Synth *synth)
{
  QSettings settings;
  QString deviceName = settings.value("audio/device").toString();
  int sampleRate = settings.value("audio/sample_rate").toInt();
  int bufferTime = settings.value("audio/buffer_time").toInt();
  int channels = 2;

  QAudioFormat format;
  format.setSampleRate(sampleRate);
  format.setChannelCount(channels);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
  const auto deviceInfos =
    QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
  for (const QAudioDeviceInfo &deviceInfo : deviceInfos) {
    if (deviceInfo.deviceName() == deviceName) {
      format.setSampleSize(16);
      format.setCodec("audio/pcm");
      format.setSampleType(QAudioFormat::SignedInt);

      if (!deviceInfo.isFormatSupported(format))
	throw (QString("Raw audio format not supported by backend"));

      _audioOutput.reset(new QAudioOutput(deviceInfo, format, this));
      break;
    }
  }

#else
  QMediaDevices *deviceInfo = new QMediaDevices(this);
  const QList<QAudioDevice> devices = deviceInfo->audioOutputs();
  for (const QAudioDevice &deviceInfo : devices) {
    if (deviceInfo.description() == deviceName) {
      format.setSampleFormat(QAudioFormat::Int16);

      if (!deviceInfo.isFormatSupported(format))
	throw (QString("Raw audio format not supported by backend"));

      _audioOutput.reset(new QAudioSink(deviceInfo, format));
      break;
    }
  }
#endif

  float msFrac = bufferTime / 1000;
  int bufferSize = msFrac * 2 * channels * sampleRate / 1000;
  _audioOutput.data()->setBufferSize(bufferSize);
  _synthGen.reset(new SynthGen(format, synth));

  synth->set_audio_format(sampleRate, channels);

  std::cout << "EmuSC: Audio output [QT] successfully initialized" <<std::endl
	    << " -> Device = " << deviceName.toStdString() << std::endl
	    << " -> Format = 16 bit, " << sampleRate << " Hz, "
	    << channels << " channels" << std::endl;
}


AudioOutputQt::~AudioOutputQt()
{
}


void AudioOutputQt::start(void)
{
  _synthGen.data()->start();
  _audioOutput->start(_synthGen.data());
}

void AudioOutputQt::stop(void)
{
  _audioOutput->stop();
  _synthGen.data()->stop();
}


QStringList AudioOutputQt::get_available_devices(void)
{
  QStringList deviceList;

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
  const auto deviceInfos =
    QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
  for (const QAudioDeviceInfo &deviceInfo : deviceInfos)
    deviceList << deviceInfo.deviceName();
#else
  QMediaDevices mediaDevices;
  const auto devices = mediaDevices.audioOutputs();
  for (auto &deviceInfo : devices)
    deviceList << deviceInfo.description();
#endif

  return deviceList;
}


// Synth Generator class (QIODevice):
// Requests samples from libemusc and copies to audio buffer
SynthGen::SynthGen(const QAudioFormat &format, EmuSC::Synth *synth)
  : _sampleRate(format.sampleRate()),
    _channels(format.channelCount()),
    _synth(synth)
{}

void SynthGen::start()
{
  open(QIODevice::ReadOnly);
}

void SynthGen::stop()
{
  close();
}

qint64 SynthGen::readData(char *data, qint64 length)
{
  int i = 0;
  int16_t sample[_channels];

  int frames = length / 4;

  for (unsigned int frame = 0; frame < frames; frame++) {
    _synth->get_next_sample(sample);

    for (int channel = 0; channel < _channels; channel++) {
      int16_t* dest = (int16_t *) &data[(frame * 4) + (2 * channel)];
      *dest = sample[channel];
      i += 2;
    }
  }

  return i;
}

qint64 SynthGen::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}


#endif // __QT_AUDIO__
