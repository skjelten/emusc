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


#ifndef ROM_CONFIG_DIALOG_H
#define ROM_CONFIG_DIALOG_H


#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QString>

#include "emulator.h"


class RomConfigDialog : public QDialog
{
  Q_OBJECT

private:
  class ControlTab *_controlTab;
  class PcmTab *_pcmTab;

  Emulator *_emulator;

public:
  explicit RomConfigDialog(Emulator *emulator, QWidget *parent = nullptr);
  ~RomConfigDialog();

private slots:
  void accept();
  void reject();
};


class ControlTab : public QWidget
{
  Q_OBJECT

private:
  QLineEdit *_pathCtrlRomEdit;
  QLabel *_statusControlRomLabel;

  Emulator *_emulator;

public:
  explicit ControlTab(Emulator *emulator, QWidget *parent = nullptr);
  QString rom_path(void);

private slots:
  void _open_rom_file_dialog(void);
  void _verify_control_rom(QString romPath);
};


class PcmTab : public QWidget
{
  Q_OBJECT

private:
  QLineEdit *_pathPcmRom1Edit;
  QLineEdit *_pathPcmRom2Edit;
  QLineEdit *_pathPcmRom3Edit;
  QLabel *_statusPcmRomLabel;

  Emulator *_emulator;

public:
  explicit PcmTab(Emulator *emulator, QWidget *parent = nullptr);
  QString rom_path(int num);

private slots:
  void _open_rom1_file_dialog(void);
  void _open_rom2_file_dialog(void);
  void _open_rom3_file_dialog(void);
  void _verify_pcm_rom(QString romPath);
};


#endif // ROM_CONFIG_DIALOG_H
