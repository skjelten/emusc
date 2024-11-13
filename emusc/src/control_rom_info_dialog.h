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


#ifndef CONTROL_ROM_INFO_DIALOG_H
#define CONTROL_ROM_INFO_DIALOG_H

#include "emulator.h"

#include <QDialog>
#include <QStandardItemModel>
#include <QString>
#include <QTableView>
#include <QTabWidget>


class ControlRomInfoDialog : public QDialog
{
  Q_OBJECT

public:
  ControlRomInfoDialog(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~ControlRomInfoDialog();

private:
  Emulator *_emulator;

  class InstrumentsTab *_instrumentsTab;
  class PartialsTab *_partialsTab;
  class SamplesTab *_samplesTab;
  class VariationsTab *_variationsTab;
  class DrumSetsTab *_drumSetsTab;
  QTabWidget *_tabWidget;

public slots:
  void accept(void);
  void setTabIndex(QString index);
};


class InstrumentsTab : public QWidget
{
  Q_OBJECT

private:
  QTableView *_table;
  QStandardItemModel *_model;

private slots:
  void search(QString searchStr);
  void select_partial(const QModelIndex index);

public:
  explicit InstrumentsTab(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~InstrumentsTab();

  void set_active_row(int row);

signals:
  void change_tab(QString index);
};

class PartialsTab : public QWidget
{
  Q_OBJECT

private:
  QTableView *_table;

private slots:
  void select_partial(const QModelIndex index);

public:
  explicit PartialsTab(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~PartialsTab();

  void set_active_row(int row);

signals:
  void change_tab(QString index);
};


class SamplesTab : public QWidget
{
  Q_OBJECT

private:
  QTableView *_table;

public:
  explicit SamplesTab(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~SamplesTab();

  void set_active_row(int row);
};



class VariationsTab : public QWidget
{
  Q_OBJECT

private:

private slots:
  void select_partial(const QModelIndex index);

public:
  explicit VariationsTab(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~VariationsTab();

signals:
  void change_tab(QString index);
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
