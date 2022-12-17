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


#include "midi_config_dialog.h"
#include "midi_input_alsa.h"
#include "midi_input_core.h"
#include "midi_input_win32.h"

#include <iostream>

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QString>


MidiConfigDialog::MidiConfigDialog(QWidget *parent)
  : QDialog{parent}
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  QGridLayout *gridLayout = new QGridLayout();

  gridLayout->addWidget(new QLabel("MIDI Device"), 0, 0);
  gridLayout->addWidget(new QLabel("MIDI Connection"), 1, 0);

  _midiSystemBox = new QComboBox();
  gridLayout->addWidget(_midiSystemBox, 0, 1);
  _midiDeviceBox = new QComboBox();
  gridLayout->addWidget(_midiDeviceBox, 1, 1);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
						     QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  vboxLayout->addLayout(gridLayout);
  vboxLayout->addWidget(buttonBox);

  setLayout(vboxLayout);

  setWindowTitle("MIDI Configuration");
#ifdef __ALSA_MIDI__
  _midiSystemBox->addItem("ALSA");
#endif
#ifdef __WIN32_MIDI__
  _midiSystemBox->addItem("Win32");
#endif
#ifdef __CORE_MIDI__
  _midiSystemBox->addItem("Core");
#endif

  connect(_midiSystemBox,QOverload<int>::of(&QComboBox::currentIndexChanged),
	  this, &MidiConfigDialog::system_changed);

  // Finally read & update settings from config file
  QSettings settings;
  QString systemStr = settings.value("midi/system").toString();

  int systemIndex = _midiSystemBox->findText(systemStr);
  if (systemIndex < 0)
    std::cout << "Illegal value in midi system combobox" << std::endl;
  else
    _midiSystemBox->setCurrentIndex(systemIndex);

  system_changed(0);
}


MidiConfigDialog::~MidiConfigDialog()
{
  delete _midiSystemBox;
  delete _midiDeviceBox;
}


void MidiConfigDialog::accept()
{
  QString systemStr = _midiSystemBox->currentText();
  QString deviceStr = _midiDeviceBox->currentText();

  QSettings settings;
  if (systemStr != "")
    settings.setValue("midi/system", systemStr);
  if (deviceStr != "")
    settings.setValue("midi/device", deviceStr);

  delete this;
}


void MidiConfigDialog::reject()
{
  delete this;
}


void MidiConfigDialog::system_changed(int index)
{
  _midiDeviceBox->clear();

  if (!_midiSystemBox->currentText().compare("Core", Qt::CaseInsensitive)) {
    // List all available MIDI devices - or show a warning if none    
#ifdef __CORE_MIDI__
    QStringList devices = MidiInputCore::get_available_devices();

    for (auto d : devices)
      _midiDeviceBox->addItem(d);
#endif

  } else if (!_midiSystemBox->currentText().compare("alsa",
						    Qt::CaseInsensitive)) {
#ifdef __ALSA_MIDI__
    QStringList devices = MidiInputAlsa::get_available_devices();

    for (auto d : devices)
      _midiDeviceBox->addItem(d);
#endif

  } else if (!_midiSystemBox->currentText().compare("win32",
						    Qt::CaseInsensitive)) {
#ifdef __WIN32_MIDI__
    QStringList devices = MidiInputWin32::get_available_devices();

    for (auto d : devices)
      _midiDeviceBox->addItem(d);
#endif
  }

  // Fixme: Set to correct MIDI input device according to settings
}
