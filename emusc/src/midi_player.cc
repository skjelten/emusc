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


#include "midi_player.h"

#include <cmath>


void MidiPlayer::load_song(const MidiFile *file)
{
  stop();
  _file = file;
}


void MidiPlayer::play()
{
  if (!_file || _file->events().empty())
    return;

  State st = _state.load();
  if (st == State::Playing)
    return;

  if (st == State::Stopped) {
    _rewind();
    if (_sendResetOnPlay)
      _pendingReset = true;
  }
  _atEnd = false;
  _state = State::Playing;
}


void MidiPlayer::pause()
{
  if (_state.load() != State::Playing)
    return;
  _state = State::Paused;
  _sendAllSoundOff();
}


void MidiPlayer::stop()
{
  State st = _state.exchange(State::Stopped);
  if (st != State::Stopped)
    _sendAllSoundOff();
  _rewind();
}


void MidiPlayer::_rewind()
{
  _index = 0;
  _tickPos = 0.0;
  _tempoUs = 500000.0;               // SMF default, 120 BPM
  _delaySeconds = 0.0;
  _elapsedSeconds = 0.0;
  _positionSeconds = 0.0;
  _atEnd = false;
}


void MidiPlayer::advance(double seconds)
{
  if (_state.load() != State::Playing || !_file || seconds <= 0.0)
    return;

  if (_pendingReset.exchange(false)) {
    _sendResetSequence();
    _delaySeconds = _resetDelayMs * 0.001;
  }

  double remaining = seconds;

  if (_delaySeconds > 0.0) {
    double d = _delaySeconds < remaining ? _delaySeconds : remaining;
    _delaySeconds -= d;
    remaining -= d;
  }

  const std::vector<MidiEvent> &ev = _file->events();

  while (remaining > 0.0) {
    if (_index >= ev.size()) {
      _atEnd = true;
      _state = State::Stopped;
      break;
    }

    // Samples per tick under the current tempo
    double spt = _tempoUs / (1.0e6 * _file->division());
    double reachableTicks = remaining / spt;
    double nextTick = (double)ev[_index].tick;

    if (nextTick <= _tickPos + reachableTicks) {
      // Consume time up to the event, then dispatch it. Tempo events
      // update _tempoUs inside _dispatch, affecting subsequent math.
      double dTicks = nextTick - _tickPos;
      if (dTicks > 0.0) {
        remaining -= dTicks * spt;
        _elapsedSeconds += dTicks * spt;
        _tickPos = nextTick;
      }
      _dispatch(ev[_index]);
      _index++;
    } else {
      _tickPos += reachableTicks;
      _elapsedSeconds += remaining;
      remaining = 0.0;
    }
  }

  _positionSeconds = _elapsedSeconds;
}


void MidiPlayer::_dispatch(const MidiEvent &ev)
{
  if (ev.status == 0xff) {
    if (ev.meta == 0x51 && ev.blob.size() == 3) {
      uint32_t t = ((uint32_t) ev.blob[0] << 16) |
                   ((uint32_t) ev.blob[1] << 8) | ev.blob[2];
      if (t > 0)
        _tempoUs = (double) t;
    }

    return;
  }

  if (!_sink)
    return;

  if (ev.status == 0xf0 || ev.status == 0xf7) {
    if (!ev.blob.empty())
      _sink(ev.blob.data(), ev.blob.size());
    return;
  }

  uint8_t msg[3] = { ev.status, ev.data[0], ev.data[1] };
  uint8_t type = ev.status & 0xf0;
  _sink(msg, (type == 0xc0 || type == 0xd0) ? 2 : 3);
}


void MidiPlayer::_sendAllSoundOff()
{
  if (!_sink)
    return;

  for (uint8_t ch = 0; ch < 16; ch++) {
    uint8_t msg[3] = { (uint8_t)(0xb0 | ch), 120, 0 };   // All Sound Off
    _sink(msg, 3);
  }
}


void MidiPlayer::_sendResetSequence()
{
  if (!_sink)
    return;

  for (uint8_t ch = 0; ch < 16; ch++) {
    uint8_t off[3]  = { (uint8_t) (0xb0 | ch), 120, 0 };  // All Sound Off
    uint8_t rst[3]  = { (uint8_t) (0xb0 | ch), 121, 0 };  // Reset All Ctrls
    uint8_t bend[3] = { (uint8_t) (0xe0 | ch), 0, 64 };   // Bend to center
    _sink(off, 3);
    _sink(rst, 3);
    _sink(bend, 3);
  }

  static const uint8_t gsReset[] =
    { 0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41, 0xf7 };
  _sink(gsReset, sizeof(gsReset));
}
