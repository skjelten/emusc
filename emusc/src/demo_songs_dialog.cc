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


#include "demo_songs_dialog.h"

#include "emusc/control_rom.h"

#include <iostream>

#include <QApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QStyle>


DemoSongsDialog::DemoSongsDialog(Emulator *emulator, QWidget *parent)
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

  _demoSongsCB = new QComboBox(this);

  QToolButton *dumpTB = new QToolButton(this);
  dumpTB->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogSaveButton));
  dumpTB->setToolTip("Dump song to disk");

  QHBoxLayout *songSelectLayout = new QHBoxLayout;
  songSelectLayout->addWidget(new QLabel("Demo song"));
  songSelectLayout->addWidget(_demoSongsCB);
  songSelectLayout->addWidget(dumpTB);

  connect(_demoSongsCB, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(demo_song_changed(int)));
  connect(dumpTB, SIGNAL(clicked()), this, SLOT(dump_demo_song()));

  QGridLayout *buttonLayout = new QGridLayout;

  _playTB = new QToolButton(this);
  _playTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));

  QToolButton *stopTB = new QToolButton(this);
  stopTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaStop));

  QToolButton *prevTB = new QToolButton(this);
  prevTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaSkipBackward));
  QToolButton *nextTB = new QToolButton(this);
  nextTB->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaSkipForward));

  buttonLayout->addWidget(prevTB, 0, 1, Qt::AlignCenter);
  buttonLayout->addWidget(_playTB, 0, 2, Qt::AlignCenter);
  buttonLayout->addWidget(stopTB, 0, 3, Qt::AlignCenter);
  buttonLayout->addWidget(nextTB, 0, 4, Qt::AlignCenter);

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

  setWindowTitle(tr("Demo songs dialog"));
  setModal(false);
//  resize(500, 600);

  connect(_playTB, &QToolButton::clicked, this, &DemoSongsDialog::play);
  connect(stopTB, &QToolButton::clicked, this, &DemoSongsDialog::stop);
  connect(prevTB, &QToolButton::clicked, this, &DemoSongsDialog::previous);
  connect(nextTB, &QToolButton::clicked, this, &DemoSongsDialog::next);

  // Get demo songs from Control ROM
  const std::vector<EmuSC::ControlRom::DemoSong> demoSongs =
    emulator->get_demo_songs();

  if (!emulator->control_rom_model().compare("SC-55mkII"))
    _mkII = true;
  else
    _mkII = false;

  int index = 0;
  for (const EmuSC::ControlRom::DemoSong &s : demoSongs) {
    _midiFile[index].load(s.data.data(), s.data.size());

    QString name = s.name.empty() ?
      QString::fromStdString(_midiFile[index].song_name()) :
      QString::fromStdString(s.name);
    _demoSongsCB->addItem(name, index);

    index ++;
  }

  _worker = new MidiPlayerWorker(&_midiPlayer);
  _worker->moveToThread(&_thread);
  connect(&_thread, &QThread::started,  _worker, &MidiPlayerWorker::start);
  connect(&_thread, &QThread::finished, _worker, &QObject::deleteLater);
  connect(_worker, &MidiPlayerWorker::positionChanged,
	  this, &DemoSongsDialog::update_slider);
  connect(_worker, &MidiPlayerWorker::songFinished,
	  this, &DemoSongsDialog::song_finished);

  connect(this, &DemoSongsDialog::playerThreadStart,
          _worker, &MidiPlayerWorker::start);
  connect(this, &DemoSongsDialog::playerThreadStop,
          _worker, &MidiPlayerWorker::stop);  

  show();
}


DemoSongsDialog::~DemoSongsDialog()
{
  _midiPlayer.stop();
  _thread.quit();
  _thread.wait();
}


void DemoSongsDialog::accept()
{
  delete this;
}


void DemoSongsDialog::play()
{
  if (!_playing) {
    _midiPlayer.load_song(&_midiFile[_demoSongsCB->currentIndex()]);
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


void DemoSongsDialog::stop(void)
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


void DemoSongsDialog::next(void)
{
  int index = _demoSongsCB->currentIndex();
  if (index < _demoSongsCB->count() - 1) {
    if (_playing) {
      demo_song_changed(index + 1);
      play();
    } else {
      demo_song_changed(index + 1);
    }
  }
}


void DemoSongsDialog::previous(void)
{
  int index = _demoSongsCB->currentIndex();
  if (index > 0) {
    if (_playing) {
      demo_song_changed(index - 1);
      play();
    } else {
      demo_song_changed(index - 1);
    }
  }  
}


void DemoSongsDialog::dump_demo_song(void)
{
  if (!_emulator)
    return;

  QString filename = QString::fromStdString(_midiFile[_demoSongsCB->currentIndex()].song_name()) + ".mid";

  QString filePath = QFileDialog::getSaveFileName(this,
						  "Store MIDI file from ROM",
						  QDir::home().filePath(filename),
						  "MIDI-files (*.mid);;All files (*)"
						  );

  if (!filePath.isEmpty()) {
    std::string error;
    _midiFile[_demoSongsCB->currentIndex()].export_file(filePath.toStdString(),
							std::string(),
                                                        &error);
  }  
}


void DemoSongsDialog::demo_song_changed(int index)
{
  stop();

  _demoSongsCB->setCurrentIndex(index);
  double songDur = _midiFile[index].duration_seconds();
  _timeSL->setRange(0, static_cast<int>(songDur * 1000));

  int min = static_cast<int>(songDur) / 60;
  int sec = static_cast<int>(songDur) % 60;
  QString totalTime =
    QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
  _totalTimeL->setText(totalTime);
}


void DemoSongsDialog::update_slider(double seconds)
{
  _timeSL->setValue(static_cast<int>(seconds * 1000));

  int min = static_cast<int>(seconds) / 60;
  int sec = static_cast<int>(seconds) % 60;
  QString currentTime =
    QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
  _currentTimeL->setText(currentTime);
}


void DemoSongsDialog::song_finished(void)
{
  stop();
}
