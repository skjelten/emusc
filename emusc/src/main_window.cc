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


#include "main_window.h"
#include "rom_config_dialog.h"
#include "audio_config_dialog.h"
#include "midi_config_dialog.h"
#include "control_rom_info_dialog.h"
#include "scene.h"

#include <iostream>

#include <QApplication>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>

#include <QVBoxLayout>
#include <QGridLayout>
#include <QDialog>
#include <QFileDialog>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLineEdit>

#include <QGraphicsView>

#include <QDebug>
#include <QSettings>

#include "../config.h"

#include "main_window.moc"


MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent),
    _emulator(NULL),
    _scene(NULL),
    _powerState(0)
{
  resize(1150, 280);

  QMenu *fileMenu = menuBar()->addMenu("&File");
  QAction *quitAct = new QAction("&Quit", this);
  quitAct->setShortcut(tr("CTRL+Q"));
  fileMenu->addAction(quitAct);

  QMenu *toolsMenu = menuBar()->addMenu("&Tools");
  QAction *dumpSongsAct = new QAction("&Dump MIDI files to disk", this);
  toolsMenu->addAction(dumpSongsAct);
  _viewCtrlRomDataAct = new QAction("&View control ROM data", this);
  toolsMenu->addAction(_viewCtrlRomDataAct);
  _allSoundsOffAct = new QAction("&Send 'All sounds off'", this);
  _allSoundsOffAct->setEnabled(false);
  toolsMenu->addAction(_allSoundsOffAct);

  QMenu *optionsMenu = menuBar()->addMenu("&Options");
  _audioAct = new QAction("&Audio Configuration...", this);
  _audioAct->setShortcut(tr("CTRL+A"));
  optionsMenu->addAction(_audioAct);
  _midiAct = new QAction("&MIDI Configuration...", this);
  _midiAct->setShortcut(tr("CTRL+M"));
  optionsMenu->addAction(_midiAct);
  _romAct = new QAction("&ROM Configuration...", this);
  _romAct->setShortcut(tr("CTRL+R"));
  optionsMenu->addAction(_romAct);

  QMenu *helpMenu = menuBar()->addMenu("&Help");
  QAction *aboutAct = new QAction("&About", this);
  helpMenu->addAction(aboutAct);

  _emulator = new Emulator();
  _scene = new Scene(_emulator, this);

  auto *gView = new QGraphicsView(this);
  gView->setScene(_scene);

  _scene->setSceneRect(0, -10, 1100, 200);
//    gView->fitInView(_scene->sceneRect(), Qt::KeepAspectRatio);

  QSettings settings;
  QString ctrlRomPath = settings.value("rom/control").toString();
  QString pcm1RomPath = settings.value("rom/pcm1").toString();
  QString pcm2RomPath = settings.value("rom/pcm2").toString();
  QString pcm3RomPath = settings.value("rom/pcm3").toString();

  QVector<QString> pcmRomPaths;
  pcmRomPaths.push_back(pcm1RomPath);
  if (!pcm2RomPath.isEmpty()) pcmRomPaths.push_back(pcm2RomPath);
  if (!pcm3RomPath.isEmpty()) pcmRomPaths.push_back(pcm3RomPath);

  if (!ctrlRomPath.isEmpty()) {
    try {
      _emulator->load_control_rom(ctrlRomPath);
      _emulator->load_pcm_rom(pcmRomPaths);
    } catch (QString errorMsg) { std::cout << "Do nothing" << std::endl; }
  }
  _viewCtrlRomDataAct->setEnabled(_emulator->has_valid_control_rom());
  statusBar()->hide();

  connect(quitAct, &QAction::triggered, this, &QApplication::quit);
  connect(_audioAct, &QAction::triggered,
	  this, &MainWindow::_display_audio_dialog);
  connect(_midiAct, &QAction::triggered,
	  this, &MainWindow::_display_midi_dialog);
  connect(_romAct, &QAction::triggered,
	  this, &MainWindow::_display_rom_dialog);
  connect(aboutAct, &QAction::triggered,
	  this, &MainWindow::_display_about_dialog);
  connect(dumpSongsAct, &QAction::triggered,
	  this, &MainWindow::_dump_demo_songs);
  connect(_viewCtrlRomDataAct, &QAction::triggered,
	  this, &MainWindow::_display_control_rom_info);
  connect(_allSoundsOffAct, &QAction::triggered,
	  this, &MainWindow::_send_all_sounds_off);

  setCentralWidget(gView);

  if (QCoreApplication::arguments().contains("-p") ||
      QCoreApplication::arguments().contains("--poweron"))
    power_switch(true);
}


MainWindow::~MainWindow()
{
  delete _scene;
}


void MainWindow::cleanUp()
{
  power_switch(0);
}


void MainWindow::_display_audio_dialog()
{
  AudioConfigDialog *audioDialog = new AudioConfigDialog();
  audioDialog->show();
}


void MainWindow::_display_midi_dialog()
{
  MidiConfigDialog *midiDialog = new MidiConfigDialog();
  midiDialog->show();
}


void MainWindow::_display_rom_dialog()
{
  RomConfigDialog *romDialog = new RomConfigDialog(_emulator);
  romDialog->show();
}


void MainWindow::_display_about_dialog()
{
  QString libemuscVersion(EmuSC::Synth::version().c_str());

  QMessageBox::about(this, tr("About EmuSC"),
		     QString("EmuSC is a Roland Sound Canvas emulator\n"
			     "\n"
			     "EmuSC version " VERSION "\n"
			     "libEmuSC version " + libemuscVersion + "\n"
			     "libQT version " QT_VERSION_STR "\n"
			     "\n"
			     "Copyright (C) 2002 Håkon Skjelten\n"
			     "\n"
			     "Licensed under GPL v3 or any later version"));
}


// state < 0 => toggle, state == 0 => turn off, state > 0 => turn on
void MainWindow::power_switch(int newPowerState)
{
  if ((newPowerState > 0 && _powerState == 0) ||
      (newPowerState < 0 && _powerState == 0)) {
    qDebug() << "POWER ON";

    try {
      _emulator->start();
    } catch (QString errorMsg) {
      QMessageBox::critical(this,
			    tr("Failed to start emulator"),
			    errorMsg,
			    QMessageBox::Close);
      return;
    }

    _allSoundsOffAct->setDisabled(false);
    _audioAct->setDisabled(true);
    _midiAct->setDisabled(true);
    _romAct->setDisabled(true);

    _powerState = 1;

  } else if ((newPowerState == 0 && _powerState == 1) ||
	     (newPowerState < 0 && _powerState == 1)) {
    qDebug() << "POWER OFF";
    _powerState = 0;

    _emulator->stop();

    _allSoundsOffAct->setDisabled(true);
    _audioAct->setDisabled(false);
    _midiAct->setDisabled(false);
    _romAct->setDisabled(false);
  }
}


void MainWindow::_dump_demo_songs(void)
{
  if (!_emulator)
    return;
  
  QString path = QFileDialog::getExistingDirectory(this);
  int numSongs = _emulator->dump_demo_songs(path);

  if (numSongs)
    QMessageBox::information(this,
			     tr("Demo songs"),
			     QString::number(numSongs) +
			     " demo songs extracted from controROM",
			     QMessageBox::Close);
  else
    QMessageBox::warning(this,
			 tr("Demo songs"),
			 tr("No demo songs found in control ROM"),
			 QMessageBox::Close);
}

void MainWindow::_display_control_rom_info(void)
{
  ControlRomInfoDialog *dialog = new ControlRomInfoDialog(_emulator);
  dialog->show();
}


void MainWindow::_send_all_sounds_off(void)
{
  if (_emulator)
    _emulator->all_sounds_off();
}
