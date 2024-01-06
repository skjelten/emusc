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

#ifdef _NOT_USED_

#include "audio_output_qt.h"

#include <iostream>

#include <signal.h>

#include <QSettings>
#include <QtEndian>
#include <QDebug>


AudioOutputQt::AudioOutputQt(EmuSC::Synth *synth)
  : _synth(synth),
    _bufferTime(75000),
    _periodTime(25000),
    _sampleRate(44100),
    _channels(2)
{
    QSettings settings;
    QString deviceName = settings.value("audio/device").toString();
    _bufferTime = settings.value("audio/buffer_time").toInt();
    _periodTime = settings.value("audio/period_time").toInt();
    _sampleRate = settings.value("audio/sample_rate").toInt();

    _format.setSampleRate(_sampleRate);
    _format.setChannelCount(2);
    _format.setSampleSize(16);
    _format.setCodec("audio/pcm");
    _format.setByteOrder(QAudioFormat::LittleEndian);
    _format.setSampleType(QAudioFormat::SignedInt);

    // Quick hack
     foreach(const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
         if (deviceInfo.deviceName() == deviceName) {
             std::cout << "WE FOUNDIT!" << deviceInfo.deviceName().toStdString() << std::endl;
            _deviceInfo = deviceInfo;
         }

     _output = NULL;

qDebug() << "But does the info really hold a value? Lets check!";
qDebug() << "Check: " << _deviceInfo.deviceName();
qDebug() << "Success!!";


   if (!_deviceInfo.isFormatSupported(_format)) {
     qDebug() << "Default format not supported - trying to use nearest";
     _format = _deviceInfo.nearestFormat(_format);
   }
   qDebug() << "Success2!!";

   _output = new QAudioOutput(_deviceInfo, _format, this);
   qDebug() << "Success3!!";
   _output->setNotifyInterval(50);
   qDebug() << "Success4!!";
   _output->setBufferSize(131072); //in bytes
   qDebug() << "Success5!!";
   QObject::connect(_output, SIGNAL(notify()), this, SLOT(fill_buffer()));
   //auIObuffer->open(QIODevice::ReadOnly);  
   qDebug() << "Success6!!";

   _auIObuffer = _output->start();
   // We will delete this later, but for the moment we store a 0.1s tone on the aubuffer  
   // As the buffer is 1 second long, the buffer tone will be played periodically every 1s,  
   // after the buffer wraps around.  
   for (int sample=0; sample<BufferSize; sample++) {  
        signed short value=0;  
        if ((sample>20000) && (sample<20000+(DurationSeconds*DataFrequencyHz))) {
          float time = (float) sample/DataFrequencyHz ;  
          float x = 100; //sin *pi stuff
          value = static_cast<signed short>(x * 32767);  
        }  
        _aubuffer[sample] = value;
   }  

   qDebug() << "Success7!!";
   writepointer=0;
   readpointer=0;  


/*
    _deviceName = audioDevice.toStdString();
    if (_deviceName.empty())
        _deviceName = "default";

    std::cout << "Prøver å åpne audeio device=" << _deviceName << std::endl;

  unsigned int ret;
  
  // Start audio callback
  if (ret = snd_async_add_pcm_handler(&_aHandler, _pcmHandle,
				      AudioOutputQt::alsa_callback,
				      (void*) this))
    throw(std::string("[ALSA] Unable to register async handler. " +
          std::string(snd_strerror(ret))));

  const snd_pcm_channel_area_t *my_areas;
  snd_pcm_uframes_t offset, frames, size;
  snd_pcm_sframes_t commitres;
  int err, count;


*/
    // Update with real numbers. DELETE?
  _sampleRate = _format.sampleRate();
  _channels = _format.channelCount();

  synth->set_audio_format(_sampleRate, _channels);
  std::cout << "EmuSC: Audio output [QT] successfully initialized" <<std::endl
        << " -> device=\"" << _deviceInfo.deviceName().toStdString() << "\" (16 bit, "
        << _sampleRate << " Hz, "
        << _channels << " channels)" << std::endl;

}


AudioOutputQt::~AudioOutputQt()
{
}


void AudioOutputQt::run(void)
{
std::cout << "QT-thread is started! - but needed? We leave right now :)" << std::endl;

//     _private_callback(_synth);

}


/* Only 16 bit supported - FROM ALSA
int AudioOutputQt::_fill_buffer(const snd_pcm_channel_area_t *areas,
                  snd_pcm_uframes_t offset,
                  snd_pcm_uframes_t frames,
                  EmuSC::Synth *synth)
{
  int16_t sample[_channels];

  for (unsigned int frame = 0; frame < frames; frame++) {
    synth->get_next_sample(sample);   // FIXME: Assumes 16 bit, 44.1 kHz, 2 ch

    for (int channel=0; channel < _channels; channel++) {
      int16_t* dest =
    (int16_t*) ( ((char*) areas[channel].addr)
             + (areas[channel].first >> 3)
             + ( (areas[channel].step >> 3) * (offset + frame) ) );

      *dest = sample[channel];
    }
  }

  return 0;
}
*/


// When the audio calls for this routine, we are not sure how much do we need to write in.
// We need to make sure that what we send is n-sync with what is in the buffer already...  
void AudioOutputQt::fill_buffer()
 {  
    qDebug() << "AudioOutputQt::fill_buffer().. notify event!!";

      int emptyBytes = _output->bytesFree(); // Check how many empty bytes are in the device buffer
      int periodSize = _output->periodSize(); // Check the ideal chunk size, in bytes
      int chunks = emptyBytes/periodSize;  

      while (chunks){  
           if (readpointer+periodSize/2<=BufferSize)  
           // The data we need does not wrap the buffer  
           {  
                _auIObuffer->write((const char*) &_aubuffer[readpointer], periodSize);
                readpointer+=periodSize/2;  
                if (readpointer>BufferSize-1) readpointer=0;  
//                string_display.insertPlainText("<");
                }  
           else  
           // Part of the data is before and part after the buffer wrapping  
           {  
                signed short int_buffer[periodSize/2];  
                // We want to make a single write of periodSize but  
                // data is broken in two pieces...  
                for (int sample=0;sample<BufferSize-readpointer;sample++)  
                     int_buffer[sample]=_aubuffer[readpointer+sample];
                     for (int sample=0;sample<readpointer+periodSize/2-BufferSize;sample++)  
                          int_buffer[BufferSize-readpointer+sample]=_aubuffer[sample];
                          _auIObuffer->write((const char*) &int_buffer, periodSize);
                          readpointer+=periodSize/2-BufferSize;  
//                          string_display.insertPlainText(">");  
                     }  
//           string_display.insertPlainText(QString::number(chunks));  
//           string_display.insertPlainText(" ");  
           --chunks;  
      }  
 }

#endif // _NOT_USED_
