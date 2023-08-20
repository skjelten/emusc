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


#include "main_window.h"
#include "rom_config_dialog.h"
#include "audio_config_dialog.h"
#include "midi_config_dialog.h"
#include "control_rom_info_dialog.h"
#include "scene.h"
#include "synth_dialog.h"

#include "emusc/control_rom.h"

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

#include "config.h"


MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent),
    _emulator(NULL),
    _scene(NULL),
    _powerState(0)
{
  resize(1150, 280);

  _create_actions();
  _create_menus();

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
  _synthModeMenu->setEnabled(_emulator->has_valid_control_rom());

  if (_emulator->has_valid_control_rom() &&
      _emulator->get_synth_generation() > EmuSC::ControlRom::SynthGen::SC55)
    _GMmodeAct->setVisible(true);
  else
    _GMmodeAct->setVisible(false);

  statusBar()->hide();

  setCentralWidget(gView);

  if (QCoreApplication::arguments().contains("-p") ||
      QCoreApplication::arguments().contains("--power-on"))
    power_switch(true);
}


MainWindow::~MainWindow()
{
  delete _scene;
}


void MainWindow::cleanUp()
{
  if (_synthDialog)
    _synthDialog->close();

  power_switch(0);
}


void MainWindow::_create_actions(void)
{
  _quitAct = new QAction("&Quit", this);
  _quitAct->setShortcut(tr("CTRL+Q"));
  connect(_quitAct, &QAction::triggered, this, &QApplication::quit);

  _dumpSongsAct = new QAction("&Dump MIDI files to disk", this);
  connect(_dumpSongsAct, &QAction::triggered,
	  this, &MainWindow::_dump_demo_songs);

  _viewCtrlRomDataAct = new QAction("&View control ROM data", this);
  connect(_viewCtrlRomDataAct, &QAction::triggered,
	  this, &MainWindow::_display_control_rom_info);

  _synthSettingsAct = new QAction("&Settings...", this);
  _synthSettingsAct->setShortcut(tr("CTRL+S"));
  _synthSettingsAct->setEnabled(false);
  connect(_synthSettingsAct, &QAction::triggered,
	  this, &MainWindow::_display_synth_dialog);

  _GSmodeAct = new QAction("&GS", this);
  _GSmodeAct->setCheckable(true);
  connect(_GSmodeAct, &QAction::triggered,
	  this, &MainWindow::_set_gs_map);
  _GMmodeAct = new QAction("G&S (GM mode)", this);
  _GMmodeAct->setCheckable(true);
  _GMmodeAct->setVisible(false);
  connect(_GMmodeAct, &QAction::triggered,
	  this, &MainWindow::_set_gs_gm_map);
  _MT32modeAct = new QAction("&MT32", this);
  _MT32modeAct->setCheckable(true);
  connect(_MT32modeAct, &QAction::triggered,
	  this, &MainWindow::_set_mt32_map);

  _modeGroup = new QActionGroup(this);
  _modeGroup->addAction(_GSmodeAct);
  _modeGroup->addAction(_GMmodeAct);
  _modeGroup->addAction(_MT32modeAct);
  _GSmodeAct->setChecked(true);

  _panicAct = new QAction("&Panic", this);
  _panicAct->setShortcut(tr("CTRL+P"));
  _panicAct->setEnabled(false);
  connect(_panicAct, &QAction::triggered,
	  this, &MainWindow::_panic);

  _audioAct = new QAction("&Audio Configuration...", this);
  _audioAct->setShortcut(tr("CTRL+A"));
  connect(_audioAct, &QAction::triggered,
	  this, &MainWindow::_display_audio_dialog);

  _midiAct = new QAction("&MIDI Configuration...", this);
  _midiAct->setShortcut(tr("CTRL+M"));
  connect(_midiAct, &QAction::triggered,
	  this, &MainWindow::_display_midi_dialog);

  _romAct = new QAction("&ROM Configuration...", this);
  _romAct->setShortcut(tr("CTRL+R"));
  connect(_romAct, &QAction::triggered,
	  this, &MainWindow::_display_rom_dialog);

  _aboutAct = new QAction("&About", this);
  connect(_aboutAct, &QAction::triggered,
	  this, &MainWindow::_display_about_dialog);
}


void MainWindow::_create_menus(void)
{
  _fileMenu = menuBar()->addMenu("&File");
  _fileMenu->addAction(_quitAct);

  _toolsMenu = menuBar()->addMenu("&Tools");
  _toolsMenu->addAction(_dumpSongsAct);
  _toolsMenu->addAction(_viewCtrlRomDataAct);

  _synthMenu = menuBar()->addMenu("&Synth");
  _synthMenu->addAction(_synthSettingsAct);

  // TODO: Add SC-88 modes
  _synthModeMenu = _synthMenu->addMenu("Sound Map");
  _synthModeMenu->addAction(_GSmodeAct);
  _synthModeMenu->addAction(_GMmodeAct);
  _synthModeMenu->addAction(_MT32modeAct);
  _synthModeMenu->setEnabled(false);

  _synthMenu->addSeparator();
  _synthMenu->addAction(_panicAct);

  _optionsMenu = menuBar()->addMenu("&Options");
  _optionsMenu->addAction(_audioAct);
  _optionsMenu->addAction(_midiAct);
  _optionsMenu->addAction(_romAct);

  _helpMenu = menuBar()->addMenu("&Help");
  _helpMenu->addAction(_aboutAct);
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
  romDialog->exec();

  _viewCtrlRomDataAct->setEnabled(_emulator->has_valid_control_rom());
  _synthModeMenu->setEnabled(_emulator->has_valid_control_rom());

  if (_emulator->has_valid_control_rom() &&
      _emulator->get_synth_generation() > EmuSC::ControlRom::SynthGen::SC55)
    _GMmodeAct->setVisible(true);
  else
    _GMmodeAct->setVisible(false);
}


void MainWindow::_display_synth_dialog()
{
  _synthDialog = new SynthDialog(_emulator, _scene, this);
  _synthDialog->setModal(false);
  _synthDialog->show();
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
			     "Copyright (C) 2022 Håkon Skjelten\n"
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

    _panicAct->setDisabled(false);
    _synthSettingsAct->setDisabled(false);
    _audioAct->setDisabled(true);
    _midiAct->setDisabled(true);
    _romAct->setDisabled(true);

    _powerState = 1;

  } else if ((newPowerState == 0 && _powerState == 1) ||
	     (newPowerState < 0 && _powerState == 1)) {
    qDebug() << "POWER OFF";
    _powerState = 0;

    _emulator->stop();

    // TODO: Force close synth settings dialog

    _panicAct->setDisabled(true);
    _synthSettingsAct->setDisabled(true);
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


void MainWindow::_panic(void)
{
  if (_emulator)
    _emulator->panic();
}


void MainWindow::_set_gs_map(void)
{
  if (_emulator)
    _emulator->set_gs_map();
}


void MainWindow::_set_gs_gm_map(void)
{
  if (_emulator)
    _emulator->set_gs_gm_map();
}


void MainWindow::_set_mt32_map(void)
{
  if (_emulator)
    _emulator->set_mt32_map();
}
