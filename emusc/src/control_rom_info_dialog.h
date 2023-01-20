/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2023  Håkon Skjelten
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


#ifndef CONTROL_ROM_INFO_DIALOG_H
#define CONTROL_ROM_INFO_DIALOG_H

#include "emulator.h"

#include <QDialog>


class ControlRomInfoDialog : public QDialog
{
  Q_OBJECT

public:
  ControlRomInfoDialog(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~ControlRomInfoDialog();

private:
  Emulator *_emulator;
};


class InstrumentsTab : public QWidget
{
  Q_OBJECT

private:

public:
  explicit InstrumentsTab(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~InstrumentsTab();
};

class PartialsTab : public QWidget
{
  Q_OBJECT

private:

public:
  explicit PartialsTab(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~PartialsTab();
};


class SamplesTab : public QWidget
{
  Q_OBJECT

private:

public:
  explicit SamplesTab(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~SamplesTab();
};



class VariationsTab : public QWidget
{
  Q_OBJECT

private:

public:
  explicit VariationsTab(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~VariationsTab();
};


class DrumSetsTab : public QWidget
{
  Q_OBJECT

private:

public:
  explicit DrumSetsTab(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~DrumSetsTab();
};


#endif // CONTROL_ROM_INFO_DIALOG_H
