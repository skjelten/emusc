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


#include "rom_config_dialog.h"

#include <iostream>

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QPushButton>
#include <QTabWidget>
#include <QSettings>


RomConfigDialog::RomConfigDialog(Emulator *emulator, QWidget *parent)
  : QDialog{parent},
    _emulator(emulator)
{
  _controlTab = new ControlTab(emulator);
  _pcmTab = new PcmTab(emulator);

  QTabWidget *tabWidget = new QTabWidget;
  tabWidget->addTab(_controlTab, tr("Control"));
  tabWidget->addTab(_pcmTab, tr("PCM"));

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
						     |QDialogButtonBox::Cancel);

  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(tabWidget);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  setWindowTitle(tr("ROM Configuration"));
  setModal(true);
  resize(500, 200);
}


RomConfigDialog::~RomConfigDialog()
{
}


void RomConfigDialog::accept()
{
  QSettings settings;
  settings.setValue("rom/control", _controlTab->rom_path());
  settings.setValue("rom/pcm1", _pcmTab->rom_path(1));
  settings.setValue("rom/pcm2", _pcmTab->rom_path(2));
  settings.setValue("rom/pcm3", _pcmTab->rom_path(3));

  delete this;
}


void RomConfigDialog::reject()
{
  delete this;
}


ControlTab::ControlTab(Emulator *emulator, QWidget *parent)
  :  QWidget(parent),
     _emulator(emulator)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  QGridLayout *gridLayout = new QGridLayout();

  gridLayout->addWidget(new QLabel("Path to Control ROM"), 0, 0);

  _pathCtrlRomEdit = new QLineEdit(this);

  gridLayout->addWidget(_pathCtrlRomEdit, 1, 0);

  QPushButton *qpbSelectControlRom = new QPushButton("...");
  gridLayout->addWidget(qpbSelectControlRom, 1, 2);
  _statusControlRomLabel = new QLabel(this);

  vboxLayout->addLayout(gridLayout);
  vboxLayout->addSpacing(10);
  vboxLayout->addWidget(_statusControlRomLabel);
  vboxLayout->addStretch();
  setLayout(vboxLayout);

  connect(qpbSelectControlRom, SIGNAL(clicked()),
	  this, SLOT(_open_rom_file_dialog()));
  connect(_pathCtrlRomEdit, SIGNAL(textChanged(QString)),
	  this, SLOT(_verify_control_rom(QString)));

  if (!_pathCtrlRomEdit->text().isEmpty())
    _verify_control_rom("");

  // Update from settings file
  QSettings settings;
  QString romPath = settings.value("rom/control").toString();

  if (!romPath.isEmpty())
    _pathCtrlRomEdit->setText(romPath);
}


QString ControlTab::rom_path()
{
  return _pathCtrlRomEdit->text();
}


void ControlTab::_open_rom_file_dialog()
{
  QString filePath = QFileDialog::getOpenFileName(this,
						  tr("Open Image"),
						  NULL,
						  tr("ROM File (*.*)"));
  QFileInfo fileInfo(filePath);
  if (fileInfo.isFile() && fileInfo.size() > 0)
    _pathCtrlRomEdit->setText(fileInfo.absoluteFilePath());
}


void ControlTab::_verify_control_rom(QString romPath)
{
  if (romPath.isEmpty()) {
    _statusControlRomLabel->setText("");
    _pathCtrlRomEdit->setStyleSheet("QLineEdit {background-color: white;}");
    return;
  }

  try {
    _emulator->load_control_rom(_pathCtrlRomEdit->text());
  }  catch (QString errorMsg) {
    _statusControlRomLabel->setText(QString("Failed: ") + errorMsg);
    _pathCtrlRomEdit->setStyleSheet("QLineEdit {background-color: #e69797;}");
    return;
  }

  QString message = "Control ROM loaded succsessfully";
  message.append("\n  Model: ");
  message.append(_emulator->control_rom_model());
  message.append("\n  Version: ");
  message.append(_emulator->control_rom_version());
  message.append("\n  Date: ");
  message.append(_emulator->control_rom_date());

  _statusControlRomLabel->setText(message);
  _pathCtrlRomEdit->setStyleSheet("QLineEdit {background-color: #97e797;}");
}


PcmTab::PcmTab(Emulator *emulator, QWidget *parent)
  : QWidget(parent),
    _emulator(emulator)
{
  QLabel *pathPcmRom1Label = new QLabel(tr("Path to PCM ROM #1"));
  _pathPcmRom1Edit = new QLineEdit();
  QPushButton *selectControlRom1Btn = new QPushButton("...");

  QLabel *pathPcmRom2Label = new QLabel(tr("Path to PCM ROM #2"));
  _pathPcmRom2Edit = new QLineEdit();
  QPushButton *selectControlRom2Btn = new QPushButton("...");

  QLabel *pathPcmRom3Label = new QLabel(tr("Path to PCM ROM #3"));
  _pathPcmRom3Edit = new QLineEdit();
  QPushButton *selectControlRom3Btn = new QPushButton("...");

  QVBoxLayout *mainLayout = new QVBoxLayout;
  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(pathPcmRom1Label, 0, 0);
  gridLayout->addWidget(_pathPcmRom1Edit, 1, 0);
  gridLayout->addWidget(selectControlRom1Btn, 1, 1);
  gridLayout->addWidget(pathPcmRom2Label, 2, 0);
  gridLayout->addWidget(_pathPcmRom2Edit, 3, 0);
  gridLayout->addWidget(selectControlRom2Btn, 3, 1);
  gridLayout->addWidget(pathPcmRom3Label, 4, 0);
  gridLayout->addWidget(_pathPcmRom3Edit, 5, 0);
  gridLayout->addWidget(selectControlRom3Btn, 5, 1);

  mainLayout->addLayout(gridLayout);
  mainLayout->addSpacing(10);

  _statusPcmRomLabel = new QLabel();
  mainLayout->addWidget(_statusPcmRomLabel);
  mainLayout->addStretch();
  setLayout(mainLayout);

  // Set signal & slots
  connect(selectControlRom1Btn, SIGNAL(clicked()),
	  this, SLOT(_open_rom1_file_dialog()));
  connect(selectControlRom2Btn, SIGNAL(clicked()),
	  this, SLOT(_open_rom2_file_dialog()));
  connect(selectControlRom3Btn, SIGNAL(clicked()),
	  this, SLOT(_open_rom3_file_dialog()));

  connect(_pathPcmRom1Edit, SIGNAL(textChanged(QString)),
	  this, SLOT(_verify_pcm_rom(QString)));
  connect(_pathPcmRom2Edit, SIGNAL(textChanged(QString)),
	  this, SLOT(_verify_pcm_rom(QString)));
  connect(_pathPcmRom3Edit, SIGNAL(textChanged(QString)),
	  this, SLOT(_verify_pcm_rom(QString)));

  // Update path fields from settings file
  QSettings settings;
  QFileInfo pcm1Info(settings.value("rom/pcm1").toString());
  QFileInfo pcm2Info(settings.value("rom/pcm2").toString());
  QFileInfo pcm3Info(settings.value("rom/pcm3").toString());

  if (pcm1Info.isFile())
    _pathPcmRom1Edit->setText(pcm1Info.absoluteFilePath());
  if (pcm2Info.isFile())
    _pathPcmRom2Edit->setText(pcm2Info.absoluteFilePath());
  if (pcm3Info.isFile())
    _pathPcmRom3Edit->setText(pcm3Info.absoluteFilePath());
}


QString PcmTab::rom_path(int num)
{
  if (num == 1)
    return _pathPcmRom1Edit->text();
  else if (num == 2)
    return _pathPcmRom2Edit->text();
  else if (num == 3)
    return _pathPcmRom3Edit->text();

  return "";
}


void PcmTab::_open_rom1_file_dialog()
{
  QString filePath = QFileDialog::getOpenFileName(this,
						  tr("Open ROM file"),
						  NULL,
						  tr("ROM File (*.*)"));
  _pathPcmRom1Edit->setText(filePath);
}


void PcmTab::_open_rom2_file_dialog()
{
  QString filePath = QFileDialog::getOpenFileName(this,
						  tr("Open ROM file"),
						  NULL,
						  tr("ROM File (*.*)"));
  _pathPcmRom2Edit->setText(filePath);
}


void PcmTab::_open_rom3_file_dialog()
{
  QString filePath = QFileDialog::getOpenFileName(this,
						  tr("Open ROM file"),
						  NULL,
						  tr("ROM File (*.*)"));
  _pathPcmRom3Edit->setText(filePath);
}


void PcmTab::_verify_pcm_rom(QString romPath)
{
  // First check each entry for files that has the proper first 6 bytes
  if (_pathPcmRom1Edit->text().isEmpty()) {
    _pathPcmRom1Edit->setStyleSheet("QLineEdit {background-color: white;}");
  } else {
    QFile pcmRom1File(_pathPcmRom1Edit->text());
    if (pcmRom1File.open(QIODevice::ReadOnly)) {
      QByteArray bytes = pcmRom1File.read(6);
      if (!bytes.compare("ROLAND", Qt::CaseInsensitive) &&
	  !(pcmRom1File.size() % 1048576))
	_pathPcmRom1Edit->setStyleSheet("QLineEdit {background-color: #97e797;}");
      else
	_pathPcmRom1Edit->setStyleSheet("QLineEdit {background-color: #e69797;}");
    } else {
      _pathPcmRom1Edit->setStyleSheet("QLineEdit {background-color: #e69797;}");
    }
  }
  if (_pathPcmRom2Edit->text().isEmpty()) {
    _pathPcmRom2Edit->setStyleSheet("QLineEdit {background-color: white;}");
  } else {
    QFile pcmRom2File(_pathPcmRom2Edit->text());
    if (pcmRom2File.open(QIODevice::ReadOnly)) {
      QByteArray bytes = pcmRom2File.read(6);
      if (!bytes.compare("ROLAND", Qt::CaseInsensitive) &&
	  !(pcmRom2File.size() % 1048576))
	_pathPcmRom2Edit->setStyleSheet("QLineEdit {background-color: #97e797;}");
      else
	_pathPcmRom2Edit->setStyleSheet("QLineEdit {background-color: #e69797;}");
    } else {
      _pathPcmRom2Edit->setStyleSheet("QLineEdit {background-color: #e69797;}");
    }
  }
  if (_pathPcmRom3Edit->text().isEmpty()) {
    _pathPcmRom3Edit->setStyleSheet("QLineEdit {background-color: white;}");
  } else {
    QFile pcmRom3File(_pathPcmRom3Edit->text());
    if (pcmRom3File.open(QIODevice::ReadOnly)) {
      QByteArray bytes = pcmRom3File.read(6);
      if (!bytes.compare("ROLAND", Qt::CaseInsensitive) &&
	  !(pcmRom3File.size() % 1048576))
	_pathPcmRom3Edit->setStyleSheet("QLineEdit {background-color: #97e797;}");
      else
	_pathPcmRom3Edit->setStyleSheet("QLineEdit {background-color: #e69797;}");
    } else {
      _pathPcmRom3Edit->setStyleSheet("QLineEdit {background-color: #e69797;}");
    }
  }

  if (_pathPcmRom1Edit->text().isEmpty() &&
      _pathPcmRom2Edit->text().isEmpty() &&
      _pathPcmRom3Edit->text().isEmpty()) {
    _statusPcmRomLabel->setText("");
    return;
  }

  QFileInfo rom1Info(_pathPcmRom1Edit->text());
  QFileInfo rom2Info(_pathPcmRom2Edit->text());
  QFileInfo rom3Info(_pathPcmRom3Edit->text());

  if ((rom1Info.size() + rom2Info.size() + rom3Info.size()) != 3145728) {
    _statusPcmRomLabel->setText("Selected ROM file(s) are NOT recognized as a complete PCM ROM set");
  } else {
    //        _statusPcmRomLabel->setText("Selected ROM file(s) seems to fit a PCM library");
    QVector<QString> pcmRomPaths;
    pcmRomPaths.push_back(_pathPcmRom1Edit->text());
    if (!_pathPcmRom2Edit->text().isEmpty()) pcmRomPaths.push_back(_pathPcmRom2Edit->text());
    if (!_pathPcmRom3Edit->text().isEmpty()) pcmRomPaths.push_back(_pathPcmRom3Edit->text());
    _emulator->load_pcm_rom(pcmRomPaths);
    
    if (_emulator->has_valid_pcm_rom()) {
      QString message = "PCM ROM(s) loaded successfully";
      message.append("\n  Version: ");
      message.append(_emulator->pcm_rom_version());
      message.append("\n  Date: ");
      message.append(_emulator->pcm_rom_date());
      _statusPcmRomLabel->setText(message);

    } else {
      _statusPcmRomLabel->setText("PCM ROMs add up to the correct size, but wrong content.\n"
				  "Perhaps you have the wrong order?");
    }
  }
}
