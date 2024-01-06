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


#ifndef AUDIO_CONFIG_DIALOG_H
#define AUDIO_CONFIG_DIALOG_H


#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>


class AudioConfigDialog : public QDialog
{
  Q_OBJECT

private:
  QComboBox *_audioSystemBox;
  QComboBox *_audioDeviceBox;
  QLineEdit *_audioBufferTimeLE;
  QLineEdit *_audioPeriodTimeLE;
  QLineEdit *_sampleRateLE;
  QLabel *_audioDeviceLabel;
  QLabel *_bufferTimeLabel;
  QLabel *_defaultBufferTimeLabel;
  QLabel *_periodTimeLabel;
  QLabel *_defaultPeriodTimeLabel;
  QLabel *_sampleRateLabel;
  QLabel *_defaultSampleRateLabel;

public:
  explicit AudioConfigDialog(QWidget *parent = nullptr);
  virtual ~AudioConfigDialog();

private slots:
  void accept();
  void reject();

  void system_changed(int index);

};

#endif // AUDIO_CONFIG_DIALOG_H
