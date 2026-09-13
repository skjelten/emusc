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


#include "midi_player_dialog.h"

#include "emusc/control_rom.h"

#include <iostream>

#include <QApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>


MidiPlayerDialog::MidiPlayerDialog(Emulator *emulator, QWidget *parent)
  : QDialog{parent},
    _emulator(emulator),
    _playing(false),
    _paused(false)
{
  // Setup the MIDI player
  _midiPlayer.setSink([this, emulator](const uint8_t *msg, size_t len) {
    if (msg[0] == 0xf0)
      emulator->midi_input_sysex((unsigned char *) msg, len);
    else
      emulator->midi_input(msg[0], msg[1], len > 2 ? msg[2] : 0);
  });

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);

  QVBoxLayout *mainLayout = new QVBoxLayout;

  _titleLE = new QLineEdit(this);
  _titleLE->setReadOnly(true);

  QToolButton *openTB = new QToolButton(this);
  openTB->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOpenButton));
  openTB->setToolTip("Open MIDI file");

  QHBoxLayout *songSelectLayout = new QHBoxLayout;
  songSelectLayout->addWidget(openTB);
  songSelectLayout->addWidget(_titleLE);

  connect(openTB, SIGNAL(clicked()), this, SLOT(open_file()));

  QGridLayout *buttonLayout = new QGridLayout;

  _playTB = new QToolButton(this);
  _playTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));

  QToolButton *stopTB = new QToolButton(this);
  stopTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaStop));

  /* Not needed before we have a play list
  QToolButton *prevTB = new QToolButton(this);
  prevTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaSkipBackward));
  QToolButton *nextTB = new QToolButton(this);
  nextTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaSkipForward));
  */
//  buttonLayout->addWidget(prevTB, 0, 1, Qt::AlignCenter);
  buttonLayout->addWidget(_playTB, 0, 2, Qt::AlignCenter);
  buttonLayout->addWidget(stopTB, 0, 3, Qt::AlignCenter);
//  buttonLayout->addWidget(nextTB, 0, 4, Qt::AlignCenter);

  buttonLayout->setColumnStretch(0, 1);
  buttonLayout->setColumnStretch(5, 1);

  _timeSL = new QSlider(Qt::Horizontal, this);
  _timeSL->setFocusPolicy(Qt::NoFocus);
  _timeSL->setAttribute(Qt::WA_TransparentForMouseEvents);

  QHBoxLayout *timeLabelsLayout = new QHBoxLayout;
  _currentTimeL = new QLabel(this);
  _totalTimeL = new QLabel(this);
  timeLabelsLayout->addWidget(_currentTimeL);
  timeLabelsLayout->addStretch();
  timeLabelsLayout->addWidget(_totalTimeL);

  mainLayout->addLayout(songSelectLayout);
  mainLayout->addSpacing(10);
  mainLayout->addLayout(buttonLayout);
  mainLayout->addSpacing(10);
  mainLayout->addWidget(_timeSL);
  mainLayout->addLayout(timeLabelsLayout);
  mainLayout->addSpacing(15);
  mainLayout->addWidget(buttonBox);

  setLayout(mainLayout);

  setWindowTitle(tr("MIDI player dialog"));
  setModal(false);
//  resize(500, 600);

  connect(_playTB, &QToolButton::clicked, this, &MidiPlayerDialog::play);
  connect(stopTB, &QToolButton::clicked, this, &MidiPlayerDialog::stop);

  _worker = new MidiPlayerWorker(&_midiPlayer);
  _worker->moveToThread(&_thread);
  connect(&_thread, &QThread::started,  _worker, &MidiPlayerWorker::start);
  connect(&_thread, &QThread::finished, _worker, &QObject::deleteLater);
  connect(_worker, &MidiPlayerWorker::positionChanged,
	  this, &MidiPlayerDialog::update_slider);
  connect(_worker, &MidiPlayerWorker::songFinished,
	  this, &MidiPlayerDialog::song_finished);

  connect(this, &MidiPlayerDialog::playerThreadStart,
          _worker, &MidiPlayerWorker::start);
  connect(this, &MidiPlayerDialog::playerThreadStop,
          _worker, &MidiPlayerWorker::stop);  

  show();
}


MidiPlayerDialog::~MidiPlayerDialog()
{
  _midiPlayer.stop();
  _thread.quit();
  _thread.wait();
}


void MidiPlayerDialog::accept()
{
  delete this;
}


void MidiPlayerDialog::open_file()
{
  QString filePath = QFileDialog::getOpenFileName(this,
						  "Open MIDI file",
						  QDir::homePath(),
						  "MIDI-files (*.mid);;All files (*)"
						  );

  if (filePath.isEmpty())
    return;

  stop();

  std::string error;
  _midiFile.load_file(filePath.toStdString(), MidiFile::Variant::Standard,
                      &error);

  if (!error.empty()) {
    QMessageBox::critical(this, "Error loading MIDI file", error.c_str());
    return;
  }

  _midiPlayer.load_song(&_midiFile);

  QString songName = QString::fromStdString(_midiFile.song_name());

  if (songName.isEmpty()) {
    QFileInfo fileInfo(filePath);
    songName = fileInfo.fileName();
  }

  _titleLE->setText(songName);
  double songDur = _midiFile.duration_seconds();
  _timeSL->setRange(0, static_cast<int>(songDur * 1000));

  int min = static_cast<int>(songDur) / 60;
  int sec = static_cast<int>(songDur) % 60;
  QString totalTime =
    QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
  _totalTimeL->setText(totalTime);
}


void MidiPlayerDialog::play()
{
  if (!_playing) {
    _midiPlayer.play();

    _thread.start(QThread::TimeCriticalPriority);
    emit playerThreadStart();

    _playTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPause));
    _playing = true;

  } else if (!_paused) {
    _midiPlayer.pause();
    emit playerThreadStop();    
    _playTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));
    _paused = true;

  } else {
    _midiPlayer.play();
    emit playerThreadStart();
    _playTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPause));
    _paused = false;
  }
}


void MidiPlayerDialog::stop()
{
  if (!_playing)
    return;

  _playTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));
  _midiPlayer.stop();
  emit playerThreadStop();

  update_slider(0);

  _playing = false;
  _paused = false;
}


void MidiPlayerDialog::update_slider(double seconds)
{
  _timeSL->setValue(static_cast<int>(seconds * 1000));

  int min = static_cast<int>(seconds) / 60;
  int sec = static_cast<int>(seconds) % 60;
  QString currentTime =
    QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
  _currentTimeL->setText(currentTime);
}


void MidiPlayerDialog::song_finished(void)
{
  stop();
}
