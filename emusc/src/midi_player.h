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

// A minimal sequencer for MIDI files and ROM demo songs playback in EmuSC.
// Only supports play, pause, stop and song selection. It does not use any
// audio hardware, but emits raw MIDI messages thorugh a sink callback.


#ifndef MIDIPLAYER_H
#define MIDIPLAYER_H

#include "midi_file.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>

#include <QTimer>

#include <iostream>


class MidiPlayer {
public:
  // Sink receives complete, wire-format MIDI messages (2-3 bytes for
  // channel messages, full F0..F7 for SysEx).
  using MidiSink = std::function<void(const uint8_t *msg, size_t length)>;

  MidiPlayer() = default;

  void setSink(MidiSink sink)        { _sink = sink; }
  void setResetDelayMs(unsigned ms)  { _resetDelayMs = ms; }
  void setSendResetOnPlay(bool b)    { _sendResetOnPlay = b; }

  void load_song(const MidiFile *file);

  void play();
  void pause();
  void stop();

  bool isPlaying()  const { return _state.load() == State::Playing; }
  bool isPaused()   const { return _state.load() == State::Paused; }
  bool atEnd()      const { return _atEnd.load(); }

  double positionSeconds() const { return _positionSeconds.load(); }

  // Call from the audio thread, once per block, before rendering it
  void advance(double seconds);

private:
  enum class State { Stopped, Playing, Paused };

  void _dispatch(const MidiEvent &ev);
  void _sendAllSoundOff();
  void _sendResetSequence();
  void _rewind();

  MidiSink        _sink;
  const MidiFile *_file = nullptr;

  unsigned _resetDelayMs = 60;
  bool _sendResetOnPlay = true;

  std::atomic<State> _state { State::Stopped };
  std::atomic<bool>  _atEnd { false };
  std::atomic<bool>  _pendingReset { false };
  std::atomic<double> _positionSeconds { 0.0 };

  // Audio-thread-only state
  size_t _index        = 0;        // Next event to dispatch
  double _tickPos      = 0.0;      // Current position in ticks
  double _tempoUs      = 500000.0; // Current us per quarter note
  double _delaySeconds = 0.0;      // Remaining post-reset silence
  double _elapsedSeconds = 0.0;
};


class MidiPlayerWorker : public QObject
{
  Q_OBJECT
public:
  MidiPlayerWorker(MidiPlayer *player)
    : _player(player)
  {
    _timer = new QTimer(this);
    _timer->setTimerType(Qt::PreciseTimer);
    connect(_timer, &QTimer::timeout, this, &MidiPlayerWorker::tick);
  }

public slots:
  void start()
  {
    _last = std::chrono::steady_clock::now();
//    _remainder = 0.0;
    _timer->start(2);
  }


  void stop()
  {
    _timer->stop();
  }

private slots:
  void tick()
  {
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - _last).count();
    _last = now;
    _player->advance(dt);

    if (++_uiDivider >= 25) {        // ~50 ms UI updates
      _uiDivider = 0;
      emit positionChanged(_player->positionSeconds());
    }
    if (_player->atEnd())
      emit songFinished();
  }

signals:
  void positionChanged(double seconds);
  void songFinished();

private:
  MidiPlayer *_player;
  int _uiDivider = 0;
  QTimer *_timer = nullptr;
  std::chrono::steady_clock::time_point _last;
};


#endif  // MIDIPLAYER_H
