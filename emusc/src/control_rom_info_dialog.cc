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


#include "control_rom_info_dialog.h"

#include "emusc/control_rom.h"

#include <iostream>

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTableView>
#include <QStandardItemModel>


ControlRomInfoDialog::ControlRomInfoDialog(Emulator *emulator, QWidget *parent)
  : QDialog{parent}
{
  InstrumentsTab *instrumentsTab = new InstrumentsTab(emulator);
  PartialsTab *partialsTab = new PartialsTab(emulator);
  SamplesTab *samplesTab = new SamplesTab(emulator);
  VariationsTab *variationsTab = new VariationsTab(emulator);
  DrumSetsTab *drumSetsTab = new DrumSetsTab(emulator);

  QTabWidget *tabWidget = new QTabWidget;
  tabWidget->addTab(instrumentsTab, tr("Instruments"));
  tabWidget->addTab(partialsTab, tr("Partials"));
  tabWidget->addTab(samplesTab, tr("Samples"));
  tabWidget->addTab(variationsTab, tr("Variations"));
  tabWidget->addTab(drumSetsTab, tr("Drum Sets"));

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(tabWidget);
  mainLayout->addWidget(buttonBox);

  setLayout(mainLayout);

  setWindowTitle(tr("Control ROM information"));
  setModal(true);
  resize(500, 600);
}


ControlRomInfoDialog::~ControlRomInfoDialog()
{}


InstrumentsTab::InstrumentsTab(Emulator *emulator, QWidget *parent)
    : QWidget(parent)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  QTableView *table = new QTableView();;
  QStandardItemModel *model = emulator->get_instruments_list();

  // Hack to make y axis count from 0
  for (int i = 0; i < model->rowCount(); i++)
    model->setHeaderData(i, Qt::Vertical, i);

  table->setModel(model);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  
  vboxLayout->addWidget(table);
  setLayout(vboxLayout);
}


InstrumentsTab::~InstrumentsTab()
{}


PartialsTab::PartialsTab(Emulator *emulator, QWidget *parent)
    : QWidget(parent)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  QTableView *table = new QTableView();;
  QStandardItemModel *model = emulator->get_partials_list();

  // Hack to make y axis count from 0
  for (int i = 0; i < model->rowCount(); i++)
    model->setHeaderData(i, Qt::Vertical, i);

  table->setModel(model);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->resizeColumnsToContents();
 
  vboxLayout->addWidget(table);
  setLayout(vboxLayout);
}


PartialsTab::~PartialsTab()
{}


SamplesTab::SamplesTab(Emulator *emulator, QWidget *parent)
    : QWidget(parent)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  QTableView *table = new QTableView();;
  QStandardItemModel *model = emulator->get_samples_list();

  // Hack to make y axis count from 0
  for (int i = 0; i < model->rowCount(); i++)
    model->setHeaderData(i, Qt::Vertical, i);

  table->setModel(model);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->resizeColumnsToContents();
 
  vboxLayout->addWidget(table);
  setLayout(vboxLayout);
}

SamplesTab::~SamplesTab()
{}


VariationsTab::VariationsTab(Emulator *emulator, QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  QTableView *table = new QTableView();;
  QStandardItemModel *model = emulator->get_variations_list();

  // Hack to make y axis count from 0
  for (int i = 0; i < model->rowCount(); i++)
    model->setHeaderData(i, Qt::Vertical, i);

  table->setModel(model);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->resizeColumnsToContents();
  
  vboxLayout->addWidget(table);
  setLayout(vboxLayout);
}

VariationsTab::~VariationsTab()
{}


DrumSetsTab::DrumSetsTab(Emulator *emulator, QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  QTableView *table = new QTableView();;
  QStandardItemModel *model = emulator->get_drum_sets_list();

  // Hack to make y axis count from 0
  for (int i = 0; i < model->rowCount(); i++)
    model->setHeaderData(i, Qt::Vertical, i);

  table->setModel(model);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
 
  vboxLayout->addWidget(table);
  setLayout(vboxLayout);
}


DrumSetsTab::~DrumSetsTab()
{}
