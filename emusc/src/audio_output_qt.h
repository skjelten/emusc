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

#ifdef _NOT_USED_

#ifndef AUDIO_OUTPUT_QT_H
#define AUDIO_OUTPUT_QT_H


#include "audio_output.h"

#include "emusc/synth.h"

#include <string>
#include <QString>

#include <QtMultimedia/QAudioOutput>
#include <QIODevice>

#include <QtCore/QObject>
#include <QObject>
#include <QWidget>


// NOTE: Class is not finished and not in a working order!
// *******************************************************
class AudioOutputQt: public QObject,AudioOutput
{
  Q_OBJECT

private:
  QAudioDeviceInfo _deviceInfo;
  QAudioOutput*    _output;

  QAudioFormat _format;  
  QIODevice*   _auIObuffer;           // IODevice to connect to m_AudioOutput  
  signed short _aubuffer[BufferSize]; // Audio circular buffer  

  int  readpointer;
  int  writepointer;

  EmuSC::Synth *_synth;

  int _channels;
  unsigned int _sampleRate;

  unsigned int _bufferTime;       // Ring buffer length in us
  unsigned int _periodTime;       // Period time in us

  std::string _deviceName;

//  AudioOutputQt();

public:
  AudioOutputQt(EmuSC::Synth *synth);
  virtual ~AudioOutputQt();

  virtual void run(void);

private slots:
  void fill_buffer();

};


#endif  // AUDIO_OUTPUT_QT_H

#endif //_NOT_USED_ 
