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


#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H


#include "emulator.h"
#include "main_window.h"
#include "rom_info.h"
#include "scene.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QString>
#include <QStringList>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QRadioButton>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTableView>
#include <QToolButton>
#include <QSpinBox>

#include "rom_info.h"

class PreferencesDialog : public QDialog
{
  Q_OBJECT

private:
  class GeneralSettings *_generalSettings;
  class AudioSettings *_audioSettings;
  class MidiSettings *_midiSettings;
  class RomSettings *_romSettings;

  QStackedWidget *_stack;
  QListWidget *_menuList;
  QListWidgetItem *_generalLW;
  QListWidgetItem *_audioLW;
  QListWidgetItem *_midiLW;
  QListWidgetItem *_romLW;

  Emulator *_emulator;

public:
  explicit PreferencesDialog(Emulator *emulator, Scene *scene, MainWindow *mWindow, QWidget *parent = nullptr);
  ~PreferencesDialog();

private slots:
  void accept(void);
  void reset(void);
};


class GeneralSettings : public QWidget
{
  Q_OBJECT

private:
  QCheckBox *_autoPowerOnCB;
  QCheckBox *_rememberLayoutCB;
  QCheckBox *_enableKbdMidi;

  QRadioButton *_emuscAnim;
  QRadioButton *_romAnim;
  QRadioButton *_noAnim;

  QPushButton *_lcdBkgColorPickB;
  QPushButton *_lcdActiveColorPickB;
  QPushButton *_lcdInactiveColorPickB;

  MainWindow *_mainWindow;
  Emulator *_emulator;
  Scene *_scene;

  void _set_button_color(QPushButton *button, QColor color);

public:
  explicit GeneralSettings(MainWindow *mainWindow, Emulator *emulator,
                           Scene *scene, QWidget *parent = nullptr);

  void reset(void);

private slots:
  void _autoPowerOn_toggled(bool checked);
  void _remember_layout_toggled(bool checked);
  void _enableKbdMidi_toggled(bool checked);
  void _lcd_bkg_colorpick_clicked(void);
  void _lcd_active_colorpick_clicked(void);
  void _lcd_inactive_colorpick_clicked(void);
  void _emuscAnim_toggled(bool checked);
  void _romAnim_toggled(bool checked);
  void _noAnim_toggled(bool checked);
};


class AudioSettings : public QWidget
{
  Q_OBJECT

private:
  QLabel *_deviceLabel;
  QLabel *_bufferTimeLabel;
  QLabel *_periodTimeLabel;
  QLabel *_sampleRateLabel;
  QLabel *_channelsLabel;

  QComboBox *_systemBox;
  QComboBox *_deviceBox;
  QComboBox *_interpolBox;

  QSpinBox *_bufferTimeSB;
  QSpinBox *_periodTimeSB;
  QSpinBox *_sampleRateSB;
  QComboBox *_channelsCB;
 
  QLabel *_filePathLabel;
  QLineEdit *_filePathLE;
  QToolButton *_fileDialogTB;

  QCheckBox *_reverseStereo;

  Emulator *_emulator;

  int _defaultBufferTime = 75000;
  int _defaultPeriodTime = 25000;
  int _defaultSampleRate = 44100;

public:
  explicit AudioSettings(Emulator *emulator, QWidget *parent = nullptr);

  void reset(void);

private slots:
  void _system_box_changed(int index);
  void _device_box_changed(int index);
  void _channels_box_changed(int index);
  void _bufferTimeSB_changed(int value);
  void _periodTimeSB_changed(int value);
  void _sampleRateSB_changed(int value);
  void _filePathLE_changed(void);
  void _open_file_path_dialog(void);
  void _interpolation_box_changed(int index);

};


class MidiSettings : public QWidget
{
  Q_OBJECT

private:
  QComboBox *_systemCB;
  QComboBox *_deviceCB;

  QListWidget *_portsListLW;

  Emulator *_emulator;

public:
  explicit MidiSettings(Emulator *emulator, Scene *scene, QWidget *parent = nullptr);

  void reset(void);

private slots:
  void _update_ports_list(void);
  void _ports_list_changed(QListWidgetItem *item);

  void _systemCB_changed(int index);
  void _deviceCB_changed(int index);
};


class RomSettings : public QWidget
{
  Q_OBJECT

private:
  QTableView *_romTableView;
  QStandardItemModel *_romModel;

  Emulator *_emulator;

  ROMInfo _romInfo;

public:
  explicit RomSettings(Emulator *emulator, QWidget *parent = nullptr);

  void reset(void);

private:
  QString _get_file_sha256(QString filePath);
  void _add_table_row(int row, QStringList text, QColor c = QColor(0, 0, 0));

  void _update_rom_table_progrom(QString fileName);
  void _update_rom_table_cpurom(QString fileName);
  void _update_rom_table_waveroms(QString fileName, int index);

private slots:
  void _table_view_clicked(const QModelIndex &index);

};


#endif // PREFERENCES_DIALOG_H
