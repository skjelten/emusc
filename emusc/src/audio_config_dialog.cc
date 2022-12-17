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


#include "audio_config_dialog.h"
#include "audio_output_alsa.h"
#include "audio_output_core.h"
#include "audio_output_pulse.h"
#include "audio_output_win32.h"

#include <iostream>

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QIntValidator>


AudioConfigDialog::AudioConfigDialog(QWidget *parent)
  : QDialog{parent}
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  QGridLayout *gridLayout = new QGridLayout();

  gridLayout->addWidget(new QLabel("Audio system"), 0, 0);

  _audioDeviceLabel = new QLabel("Audio device");
  gridLayout->addWidget(_audioDeviceLabel, 1, 0);

  _bufferTimeLabel = new QLabel("Buffer time (µs)");
  gridLayout->addWidget(_bufferTimeLabel, 2, 0);

  _periodTimeLabel = new QLabel("Period time (µs)");
  gridLayout->addWidget(_periodTimeLabel, 3, 0);

  _defaultBufferTimeLabel = new QLabel("<html><body style=\"font-style:italic;\">"
				       "Default: 75000</body></html>");
  gridLayout->addWidget(_defaultBufferTimeLabel, 2, 2);

  _defaultPeriodTimeLabel = new QLabel("<html><body style=\"font-style:italic;\">"
				       "Default: 25000</body></html>");
  gridLayout->addWidget(_defaultPeriodTimeLabel, 3, 2);

  _sampleRateLabel = new QLabel("Sample rate (Hz)");
  gridLayout->addWidget(_sampleRateLabel, 4, 0);

  _defaultSampleRateLabel = new QLabel("<html><body style=\"font-style:italic;\">"
				       "Default: 44100</body></html>");
  gridLayout->addWidget(_defaultSampleRateLabel, 4, 2);

  _audioSystemBox = new QComboBox();
  gridLayout->addWidget(_audioSystemBox, 0, 1);

  _audioDeviceBox = new QComboBox();
  gridLayout->addWidget(_audioDeviceBox, 1, 1);

  _audioBufferTimeLE = new QLineEdit();
  gridLayout->addWidget(_audioBufferTimeLE, 2, 1);

  _audioPeriodTimeLE = new QLineEdit();
  gridLayout->addWidget(_audioPeriodTimeLE, 3, 1);

  _sampleRateLE = new QLineEdit();
  gridLayout->addWidget(_sampleRateLE, 4, 1);

  QValidator *validator = new QIntValidator(1, 1000000, this);
  _audioBufferTimeLE->setValidator(validator);
  _audioPeriodTimeLE->setValidator(validator);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
						     QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  vboxLayout->addLayout(gridLayout);
  vboxLayout->addWidget(buttonBox);

  setLayout(vboxLayout);

  setWindowTitle("Audio Configuration");

#ifdef __ALSA_AUDIO__
  _audioSystemBox->addItem("ALSA");
#endif
#ifdef __PULSE_AUDIO__
  _audioSystemBox->addItem("Pulse");
#endif
#ifdef __WIN32_AUDIO__
  _audioSystemBox->addItem("Win32");
#endif
#ifdef __CORE_AUDIO__
  _audioSystemBox->addItem("Core");
#endif
  _audioSystemBox->addItem("Null");
//  _audioSystemBox->addItem("Qt");

  connect(_audioSystemBox,QOverload<int>::of(&QComboBox::currentIndexChanged),
	  this, &AudioConfigDialog::system_changed);

  // Finally read & update settings from config file
  QSettings settings;

  _audioSystemBox->setCurrentText(settings.value("audio/system").toString());

  int bufferTime = settings.value("audio/buffer_time").toInt();
  if (!bufferTime) bufferTime = 75000;
  int periodTime = settings.value("audio/period_time").toInt();
  if (!periodTime) periodTime = 25000;
  int sampleRate = settings.value("audio/sample_rate").toInt();
  if (!sampleRate) sampleRate = 44100;

  _audioBufferTimeLE->setText(QString::number(bufferTime));
  _audioPeriodTimeLE->setText(QString::number(periodTime));
  _sampleRateLE->setText(QString::number(sampleRate));

  // Trigger an update of the dialog widgets including the _audioDeviceBox
  system_changed(0);
}


AudioConfigDialog::~AudioConfigDialog()
{}


void AudioConfigDialog::accept()
{
  QSettings settings;
  settings.setValue("audio/system", _audioSystemBox->currentText());
  settings.setValue("audio/device", _audioDeviceBox->currentText());

  settings.setValue("audio/buffer_time", _audioBufferTimeLE->text());
  settings.setValue("audio/period_time", _audioPeriodTimeLE->text());
  settings.setValue("audio/sample_rate", _sampleRateLE->text());

  delete this;
}


void AudioConfigDialog::reject()
{
  delete this;
}


void AudioConfigDialog::system_changed(int index)
{
  _audioDeviceBox->clear();

  if (!_audioSystemBox->currentText().compare("alsa", Qt::CaseInsensitive)) {
#ifdef __ALSA_AUDIO__
    QStringList devices = AudioOutputAlsa::get_available_devices();
    for (auto d : devices)
      _audioDeviceBox->addItem(d);
#endif

//  } else if (!_audioSystemBox->currentText().compare("Qt", Qt::CaseInsensitive)) {
//    const auto deviceInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
//    for (const QAudioDeviceInfo &deviceInfo : deviceInfos)
//      _audioDeviceBox->addItem(deviceInfo.deviceName());

  } else if (!_audioSystemBox->currentText().compare("pulse",
						     Qt::CaseInsensitive)) {
    _audioDeviceBox->addItem("default");

  } else if (!_audioSystemBox->currentText().compare("win32",
						     Qt::CaseInsensitive)) {

#ifdef __WIN32_AUDIO__
    QStringList devices = AudioOutputWin32::get_available_devices();
    for (auto d : devices)
      _audioDeviceBox->addItem(d);
#endif

  } else if (!_audioSystemBox->currentText().compare("core",
						     Qt::CaseInsensitive)) {

#ifdef __CORE_AUDIO__
    QStringList devices = AudioOutputCore::get_available_devices();
    for (auto d : devices)
      _audioDeviceBox->addItem(d);
#endif
  }

  QSettings settings;
  _audioDeviceBox->setCurrentText(settings.value("audio/device").toString());
}
