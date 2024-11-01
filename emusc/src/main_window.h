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


#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include <QMainWindow>
#include <QAction>
#include <QActionGroup>
#include <QLineEdit>
#include <QGraphicsView>
#include <QMenu>
#include <QPointer>
#include <QTimer>

#include "scene.h"
#include "synth_dialog.h"
#include "emulator.h"


class MainWindow : public QMainWindow
{
  Q_OBJECT

private:
  QMenu *_fileMenu;
  QMenu *_editMenu;
  QMenu *_viewMenu;
  QMenu *_viewLayoutMenu;
  QMenu *_toolsMenu;
  QMenu *_synthMenu;
  QMenu *_synthModeMenu;
  QMenu *_helpMenu;

  QActionGroup *_layoutGroup;
  QActionGroup *_modeGroup;
  QAction *_quitAct;
  QAction *_preferencesAct;
  QAction *_normalLayoutAct;
  QAction *_compactLayoutAct;
  QAction *_viewMenubarAct;
  QAction *_viewStatusbarAct;
  QAction *_fullScreenAct;
  QAction *_resetWindowAct;
  QAction *_dumpSongsAct;
  QAction *_viewCtrlRomDataAct;
  QAction *_viewLFOsChartAct;
  QAction *_synthSettingsAct;
  QAction *_GSmodeAct;
  QAction *_GMmodeAct;
  QAction *_MT32modeAct;
  QAction *_panicAct;
  QAction *_aboutAct;

  QPointer<SynthDialog> _synthDialog;

  bool _powerState;
  Emulator *_emulator;

  Scene *_scene;
  QGraphicsView *_synthView;

  QTimer *_resizeTimer;
  bool _useNormalLayout;
  float _aspectRatio;

  bool _hasMovedEvent;

  void _create_actions(void);
  void _create_menus(void);

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void resizeEvent(QResizeEvent *event);
  void resize_timeout(void);
  void keyPressEvent(QKeyEvent *keyEvent);

  void cleanUp(void);

  void _set_normal_layout(void);
  void _set_compact_layout(void);

  void _show_menubar_clicked(bool state);
  void _show_statusbar_clicked(bool state);
  void _fullscreen_toggle(void);
  void _show_default_view(void);

  void _display_welcome_dialog(void);
  void _display_lfo_dialog(void);
  void _display_preferences_dialog(void);
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


#endif // MAIN_WINDOW_H
