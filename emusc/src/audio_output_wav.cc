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

// WAV audio output writes a WAV file to disk. It currently only supports
// 16 bit, 44100 Hz, stereo output.
// The write loop tries to wait the correct number of nanoseconds and then
// ask for one frame (stereo samples) and writes them to disk.


#ifdef __WAV_AUDIO__


#include "audio_output_wav.h"

#include <QDataStream>
#include <QSettings>
#include <QString>

#include <ctime>
#include <chrono>
#include <string>
#include <iostream>

#include <string.h>
#include <signal.h>


AudioOutputWav::AudioOutputWav(EmuSC::Synth *synth)
  : _synth(synth),
    _sampleRate(44100),
    _channels(2)
{
  QSettings settings;
  QString filePath = settings.value("Audio/wav_file_path").toString();

  _wavFile = new QFile(filePath);
  if (!_wavFile->open(QIODevice::WriteOnly)) {
    delete _wavFile;
    throw(QString("EmuSC: Error opening file path " + filePath));
  }

  synth->set_audio_format(_sampleRate, _channels);
  std::cout << "EmuSC: Audio output [WAV] successfully initialized"
	    << std::endl << " -> " << "44100 Hz, 16 bit, stereo" << std::endl;
}


AudioOutputWav::~AudioOutputWav()
{
  stop();

  delete _wavFile;
}


// FIXME: Assumes 16 bit, 44.1 kHz, 2 ch
int AudioOutputWav::_fill_buffer(int8_t *data, size_t length)
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


void AudioOutputWav::start(void)
{
  _quit = false;
  _audioOutputThread = new std::thread(&AudioOutputWav::run, this);  
}


void AudioOutputWav::run(void)
{
  // Write static WAV header before starting audio loop
  const uint8_t header[] = { 'R', 'I', 'F', 'F',     0x00, 0x00, 0x00, 0x00,
			     'W', 'A', 'V', 'E',      'f',  'm',  't',  ' ',
			     0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00,
			     0x44, 0xac, 0x00, 0x00, 0x10, 0xb1, 0x02, 0x00,
			     0x04, 0x00, 0x10, 0x00,  'd',  'a',  't',  'a',
			     0x00, 0x00, 0x00, 0x00 };
  QByteArray headerArray = QByteArray::fromRawData((const char *) header,
						   sizeof(header));
  _wavFile->write(headerArray);

  int8_t data[1024];

  uint32_t numSamples = 0;
  int bytesPerSample = 4;

  QDataStream stream(_wavFile);

  std::chrono::high_resolution_clock::time_point t1;
  std::chrono::high_resolution_clock::time_point t2;
  std::chrono::duration<double> timeDiff;
  std::chrono::duration<double, std::ratio<1, 44100>> timePerSample;
  t1 = std::chrono::high_resolution_clock::now();

  while(!_quit) {
    t2 = std::chrono::high_resolution_clock::now();
    timeDiff = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);

    std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::nanoseconds>(timePerSample - timeDiff));
    
    int len = _fill_buffer(&data[0], 4);
    QByteArray dataArray = QByteArray::fromRawData((const char *)&data[0], len);
    _wavFile->write(dataArray);
    numSamples += len / 4;

    t1 += std::chrono::nanoseconds(22676);   // 22676 = ns per sample (44.1kHz)
  }

  // Update WAV header with correct sizes before closing the file
  _wavFile->seek(4);
  stream << (quint8) (((numSamples * bytesPerSample) + 36) & 0xff)
	 << (quint8) ((((numSamples * bytesPerSample) + 36) >> 8) & 0xff)
	 << (quint8) ((((numSamples * bytesPerSample) + 36) >> 16) & 0xff)
	 << (quint8) ((((numSamples * bytesPerSample) + 36) >> 24) & 0xff);

  _wavFile->seek(40);
  stream << (quint8) ((numSamples * bytesPerSample) & 0xff)
	 << (quint8) (((numSamples * bytesPerSample) >> 8) & 0xff)
	 << (quint8) (((numSamples * bytesPerSample) >> 16) & 0xff)
	 << (quint8) (((numSamples * bytesPerSample) >> 24) & 0xff);

  std::cout << "EmuSC: " << _wavFile->size() / 1000 << "kB WAV file written to "
	    << _wavFile->fileName().toStdString() << std::endl;

  _wavFile->close();
}


void AudioOutputWav::stop()
{
  _quit = true;
  
  // Wait for poll in thread to return
  if (_audioOutputThread) {
    _audioOutputThread->join();
    delete (_audioOutputThread), _audioOutputThread = NULL;
  }
}


#endif  // __WAV_AUDIO__
