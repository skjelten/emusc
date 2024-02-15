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


#include "main_window.h"
#include "preferences_dialog.h"
#include "control_rom_info_dialog.h"
#include "scene.h"
#include "synth_dialog.h"

#include "emusc/control_rom.h"

#include <iostream>

#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QGraphicsView>
#include <QDebug>
#include <QSettings>
#include <QStatusBar>

#include "config.h"


MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent),
    _emulator(NULL),
    _scene(NULL),
    _powerState(0),
    _hasMovedEvent(false),
    _aspectRatio(1150/258.0)
{
  // TODO: Update minumum size based on *bars and compact mode state
  setMinimumSize(300, 120);

  _create_actions();
  _create_menus();

  _emulator = new Emulator();
  _scene = new Scene(_emulator, this);
  _scene->setSceneRect(0, -10, 1100, 200);

  _synthView = new QGraphicsView(this);
  _synthView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  _synthView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  _synthView->setScene(_scene);

  _viewCtrlRomDataAct->setEnabled(_emulator->has_valid_control_rom());
  _synthModeMenu->setEnabled(_emulator->has_valid_control_rom());

  if (_emulator->has_valid_control_rom() &&
      _emulator->get_synth_generation() > EmuSC::ControlRom::SynthGen::SC55)
    _GMmodeAct->setVisible(true);
  else
    _GMmodeAct->setVisible(false);

  QSettings settings;
  statusBar()->hide();
  if (settings.value("Synth/show_statusbar").toBool())
    show_statusbar(1);

  if (settings.value("Synth/use_compact_view").toBool())
    show_compact_view(true);

  setCentralWidget(_synthView);

  if (settings.value("Synth/auto_power_on").toBool() ||
      QCoreApplication::arguments().contains("-p") ||
      QCoreApplication::arguments().contains("--power-on"))
    power_switch(true);

  // TODO: Add restore geometry as a preferences setting?
  // restoreGeometry(settings.value("GUI/geometry").toByteArray());

  _synthView->fitInView(_scene->sceneRect().x(),
			_scene->sceneRect().y(),
			_scene->sceneRect().width() + 50,
			_scene->sceneRect().height() + 50,
			Qt::KeepAspectRatio);
  resize(1150, 280);  // FIXME
  installEventFilter(this);
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

//  QSettings settings;
//  settings.setValue("GUI/geometry", saveGeometry());
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
  _synthSettingsAct->setMenuRole(QAction::NoRole);
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

  _preferencesAct = new QAction("P&references...", this);
  _preferencesAct->setShortcut(tr("CTRL+R"));
  _preferencesAct->setMenuRole(QAction::PreferencesRole);
  connect(_preferencesAct, &QAction::triggered,
	  this, &MainWindow::_display_preferences_dialog);

  _aboutAct = new QAction("&About", this);
  connect(_aboutAct, &QAction::triggered,
	  this, &MainWindow::_display_about_dialog);
}


void MainWindow::_create_menus(void)
{
  _fileMenu = menuBar()->addMenu("&File");
  _fileMenu->addAction(_quitAct);

#ifdef Q_OS_MACOS                              // Hide empty Edit menu on MacOS
  _fileMenu->addAction(_preferencesAct);
#else
  _editMenu = menuBar()->addMenu("&Edit");
  _editMenu->addAction(_preferencesAct);
#endif

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

  _helpMenu = menuBar()->addMenu("&Help");
  _helpMenu->addAction(_aboutAct);
}


void MainWindow::_display_preferences_dialog()
{
  PreferencesDialog *preferencesDialog = new PreferencesDialog(_emulator, _scene, this);
  preferencesDialog->exec();


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

    connect(_emulator->get_midi_driver(), SIGNAL(new_midi_message(bool, int)),
	    _scene, SLOT(update_midi_activity_led(bool, int)));

    _powerState = 1;

  } else if ((newPowerState == 0 && _powerState == 1) ||
	     (newPowerState < 0 && _powerState == 1)) {
    _powerState = 0;

    _emulator->stop();

    // TODO: Force close synth settings dialog

    _panicAct->setDisabled(true);
    _synthSettingsAct->setDisabled(true);
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


void MainWindow::show_statusbar(bool state)
{
  // TODO: Find a better way to compensate for changes in MainWindow size
  if (state) {
    resize(size().width(), size().height() + statusBar()->height());
    statusBar()->show();
  } else {
    resize(size().width(), size().height() - statusBar()->height());
    statusBar()->hide();
  }
}


void MainWindow::show_compact_view(bool state)
{
  int mbHeight = menuBar()->isVisible() ? menuBar()->height() : 0;
  int sbHeight = statusBar()->isVisible() ? statusBar()->height() : 0;

  if (state) {
    _scene->setSceneRect(0, -10, 605, 200);
    _aspectRatio = 660 / 258.0;
    resize(660, 258 + mbHeight + sbHeight);
  } else {
    _scene->setSceneRect(0, -10, 1100, 200);
    _aspectRatio = 1150 / 258.0;
    resize(1150, 258 + mbHeight + sbHeight);
  }
}


void MainWindow::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);

  _synthView->fitInView(_scene->sceneRect().x(),
			_scene->sceneRect().y(),
			_scene->sceneRect().width() + 50,
			_scene->sceneRect().height() + 50,
			Qt::KeepAspectRatio);
}


// TODO: Fix resize events based on drag handle in statusbar
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Resize) {
    _hasMovedEvent = true;

  } else if (_hasMovedEvent && event->type() == QEvent::WindowActivate) {
    _synthView->fitInView(_scene->sceneRect().x(),
			  _scene->sceneRect().y(),
			  _scene->sceneRect().width() + 50,
			  _scene->sceneRect().height() + 50,
			  Qt::KeepAspectRatio);

    int mbHeight = menuBar()->isVisible() ? menuBar()->height() : 0;
    int sbHeight = statusBar()->isVisible() ? statusBar()->height() : 0;

    if (_synthView->width() >
	_aspectRatio * (_synthView->height() + mbHeight + sbHeight)) {
      resize(_synthView->height() * _aspectRatio,
	     _synthView->height() + mbHeight + sbHeight);
    } else {
      resize(_synthView->width(),
	     _synthView->width() * (1 / _aspectRatio) + mbHeight + sbHeight);
    }

    _hasMovedEvent = false;
  }

  return QWidget::eventFilter(obj, event);
}
