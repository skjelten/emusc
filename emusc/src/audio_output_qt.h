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


#ifndef AUDIO_OUTPUT_QT_H
#define AUDIO_OUTPUT_QT_H


#include "audio_output.h"

#include "emusc/synth.h"

#include <QtGlobal>
#include <QIODevice>
#include <QObject>
#include <QStringList>

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
#include <QAudioDeviceInfo>
#include <QAudioOutput>
#else
#include <QAudioDevice>
#include <QAudioSink>
#include <QMediaDevices>
#endif


class SynthGen;

class AudioOutputQt: public QObject, public AudioOutput
{
  Q_OBJECT

public:
  AudioOutputQt(EmuSC::Synth *synth);
  virtual ~AudioOutputQt();

  void start(void);
  void stop(void);

  static QStringList get_available_devices(void);

private:
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
  QScopedPointer<QAudioOutput> _audioOutput;
  QScopedPointer<SynthGen> _synthGen;
#else
  QScopedPointer<SynthGen> _synthGen;
  QScopedPointer<QAudioSink> _audioOutput;
#endif


  AudioOutputQt();

};


class SynthGen : public QIODevice
{
  Q_OBJECT

public:
  SynthGen(const QAudioFormat &format, EmuSC::Synth *synth);

  void start();
  void stop();

  qint64 readData(char *data, qint64 maxlen) override;
  qint64 writeData(const char *data, qint64 len) override;

private:
  int _sampleRate;
  int _channels;

  EmuSC::Synth *_synth;

  SynthGen();
};


#endif  // AUDIO_OUTPUT_QT_H

#endif // __QT_AUDIO__
