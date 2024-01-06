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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <QAction>
#include <QLineEdit>
#include <QPointer>

#include "scene.h"
#include "synth_dialog.h"
#include "emulator.h"


class MainWindow : public QMainWindow
{
  Q_OBJECT

private:
  QMenu *_fileMenu;
  QMenu *_toolsMenu;
  QMenu *_synthMenu;
  QMenu *_synthModeMenu;
  QMenu *_optionsMenu;
  QMenu *_helpMenu;

  QActionGroup *_modeGroup;
  QAction *_quitAct;
  QAction *_dumpSongsAct;
  QAction *_viewCtrlRomDataAct;
  QAction *_synthSettingsAct;
  QAction *_GSmodeAct;
  QAction *_GMmodeAct;
  QAction *_MT32modeAct;
  QAction *_panicAct;
  QAction *_audioAct;
  QAction *_midiAct;
  QAction *_romAct;
  QAction *_aboutAct;

  QPointer<SynthDialog> _synthDialog;

  Scene *_scene;

  bool _powerState;
  Emulator *_emulator;

  void _create_actions(void);
  void _create_menus(void);

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void cleanUp(void);

  void _display_audio_dialog(void);
  void _display_midi_dialog(void);
  void _display_rom_dialog(void);
  void _display_synth_dialog(void);
  void _display_about_dialog(void);

  void _dump_demo_songs(void);
  void _display_control_rom_info(void);
  void _panic(void);

  void _set_gs_map(void);
  void _set_gs_gm_map(void);
  void _set_mt32_map(void);

  void power_switch(int state = -1);
};

#endif // MAINWINDOW_H
