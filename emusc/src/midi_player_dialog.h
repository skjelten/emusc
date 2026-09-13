/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2026  Håkon Skjelten
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


#ifndef MIDI_PLAYER_DIALOG_H
#define MIDI_PLAYER_DIALOG_H


#include "emulator.h"
#include "midi_file.h"
#include "midi_player.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QString>
#include <QToolButton>
#include <QThread>


class MidiPlayerDialog : public QDialog
{
  Q_OBJECT

public:
  MidiPlayerDialog(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~MidiPlayerDialog();

private:
  Emulator *_emulator;

  bool _playing;
  bool _paused;

  QThread _thread;
  MidiPlayerWorker *_worker;

  MidiFile _midiFile;
  MidiPlayer _midiPlayer;

  QToolButton *_playTB;
  QLineEdit *_titleLE;
  QSlider *_timeSL;
  QLabel *_currentTimeL;
  QLabel *_totalTimeL;

public slots:
  void accept(void);

  void play(void);
  void stop(void);

  void open_file(void);

  void update_slider(double seconds);
  void song_finished(void);

signals:
  void playerThreadStart(void);
  void playerThreadStop(void);
};


#endif // MIDI_PLAYER_DIALOG_H
