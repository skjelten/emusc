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


#include "control_rom_info_dialog.h"

#include "emusc/control_rom.h"

#include <iostream>

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>


ControlRomInfoDialog::ControlRomInfoDialog(Emulator *emulator, QWidget *parent)
  : QDialog{parent}
{
  _instrumentsTab = new InstrumentsTab(emulator, this);
  _partialsTab = new PartialsTab(emulator, this);
  _samplesTab = new SamplesTab(emulator, this);
  _variationsTab = new VariationsTab(emulator, this);
  _drumSetsTab = new DrumSetsTab(emulator, this);

  _tabWidget = new QTabWidget();
  _tabWidget->addTab(_instrumentsTab, tr("Instruments"));
  _tabWidget->addTab(_partialsTab, tr("Partials"));
  _tabWidget->addTab(_samplesTab, tr("Samples"));
  _tabWidget->addTab(_variationsTab, tr("Variations"));
  _tabWidget->addTab(_drumSetsTab, tr("Drum Sets"));

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(_tabWidget);
  mainLayout->addWidget(buttonBox);

  setLayout(mainLayout);

  setWindowTitle(tr("Control ROM information"));
  setModal(false);
  resize(500, 600);

  show();
}


ControlRomInfoDialog::~ControlRomInfoDialog()
{}


void ControlRomInfoDialog::accept()
{
  delete this;
}


void ControlRomInfoDialog::setTabIndex(QString index)
{
  QStringList tokens = index.split (",");
  if (tokens.count () == 2) {
    int tab = tokens.at(0).toInt();
    int row = tokens.at(1).toInt();

    _tabWidget->setCurrentIndex(tab);
    if (tab == 0)
      _instrumentsTab->set_active_row(row);
    else if (tab == 1)
      _partialsTab->set_active_row(row);
    else if (tab == 2)
      _samplesTab->set_active_row(row);
  }
}


InstrumentsTab::InstrumentsTab(Emulator *emulator, QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  _table = new QTableView();
  _model = emulator->get_instruments_list();

  QHBoxLayout *hboxLayout = new QHBoxLayout();
  hboxLayout->addWidget(new QLabel("Search instruments:"));

  QLineEdit *searchLE = new QLineEdit();
  searchLE->setClearButtonEnabled(true);
  hboxLayout->addWidget(searchLE);
  connect(searchLE, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));

  // Hack to make y axis count from 0
  for (int i = 0; i < _model->rowCount(); i++)
    _model->setHeaderData(i, Qt::Vertical, i);

  _table->setModel(_model);
  _table->setEditTriggers(QAbstractItemView::NoEditTriggers);

  connect(_table, SIGNAL(doubleClicked(QModelIndex)),
	  this, SLOT(select_partial(QModelIndex)));
  connect(this, SIGNAL(change_tab(QString)),
	  parent, SLOT(setTabIndex(QString)));

  vboxLayout->addLayout(hboxLayout);
  vboxLayout->addWidget(_table);
  setLayout(vboxLayout);
}


InstrumentsTab::~InstrumentsTab()
{}


void InstrumentsTab::search(const QString searchStr)
{
  QString searchTerm = searchStr.trimmed();

  for (int row = 0; row < _model->rowCount(); ++row) {
    QModelIndex index = _model->index(row, 0);
    QVariant data = _model->data(index);

    if (data.toString().contains(searchTerm, Qt::CaseInsensitive)) {
      QModelIndex selection = _model->index(row, 0);
      QItemSelectionModel *selectionModel = _table->selectionModel();
      selectionModel->clearSelection();
      selectionModel->select(selection, QItemSelectionModel::Select);
      _table->scrollTo(selection);

      return; // Stop searching once the item is found
    }
  }

  // If the search term is not found, clear the selection
  _table->selectionModel()->clearSelection();
}


void InstrumentsTab::select_partial(const QModelIndex index)
{
  if (index.column() != 0 && index.data().toString() != "-") {
    QString newIndex = "1,";
    newIndex.append(index.data().toString());
    emit change_tab(newIndex);
  }
}


void InstrumentsTab::set_active_row(int row)
{
  _table->selectRow(row);
}


PartialsTab::PartialsTab(Emulator *emulator, QWidget *parent)
    : QWidget(parent)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  _table = new QTableView();
  QStandardItemModel *model = emulator->get_partials_list();

  // Hack to make y axis count from 0
  for (int i = 0; i < model->rowCount(); i++)
    model->setHeaderData(i, Qt::Vertical, i);

  _table->setModel(model);
  _table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _table->resizeColumnsToContents();

  connect(_table, SIGNAL(doubleClicked(QModelIndex)),
	  this, SLOT(select_partial(QModelIndex)));
  connect(this, SIGNAL(change_tab(QString)),
	  parent, SLOT(setTabIndex(QString)));

  vboxLayout->addWidget(_table);
  setLayout(vboxLayout);
}


PartialsTab::~PartialsTab()
{}


void PartialsTab::set_active_row(int row)
{
  _table->selectRow(row);
}


void PartialsTab::select_partial(const QModelIndex index)
{
  if (index.column() > 16 && index.data().toString() != "-") {
    QString newIndex = "2,";
    newIndex.append(index.data().toString());
    emit change_tab(newIndex);
  }
}


SamplesTab::SamplesTab(Emulator *emulator, QWidget *parent)
    : QWidget(parent)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  _table = new QTableView();
  QStandardItemModel *model = emulator->get_samples_list();

  // Hack to make y axis count from 0
  for (int i = 0; i < model->rowCount(); i++)
    model->setHeaderData(i, Qt::Vertical, i);

  _table->setModel(model);
  _table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _table->resizeColumnsToContents();
 
  vboxLayout->addWidget(_table);
  setLayout(vboxLayout);
}

SamplesTab::~SamplesTab()
{}


void SamplesTab::set_active_row(int row)
{
  _table->selectRow(row);
}


VariationsTab::VariationsTab(Emulator *emulator, QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  QTableView *table = new QTableView();
  QStandardItemModel *model = emulator->get_variations_list();

  // Hack to make y axis count from 0
  for (int i = 0; i < model->rowCount(); i++)
    model->setHeaderData(i, Qt::Vertical, i);

  table->setModel(model);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->resizeColumnsToContents();
  
  connect(table, SIGNAL(doubleClicked(QModelIndex)),
	  this, SLOT(select_partial(QModelIndex)));
  connect(this, SIGNAL(change_tab(QString)),
	  parent, SLOT(setTabIndex(QString)));

  vboxLayout->addWidget(table);
  setLayout(vboxLayout);
}

VariationsTab::~VariationsTab()
{}


void VariationsTab::select_partial(const QModelIndex index)
{
  if (index.data().toString() != "") {
    QString newIndex = "0,";
    newIndex.append(index.data().toString());
    emit change_tab(newIndex);
  }
}


DrumSetsTab::DrumSetsTab(Emulator *emulator, QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  QTableView *table = new QTableView();
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
