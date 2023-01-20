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


#ifdef __ALSA_MIDI__

#ifndef MIDI_INPUT_ALSA_H
#define MIDI_INPUT_ALSA_H


#include "midi_input.h"

#include <QStringList>

#include <alsa/asoundlib.h>

#include <thread>


class MidiInputAlsa: public MidiInput
{
private:
  snd_seq_t *_seqHandle;
  int _seqPort;

  bool _stop;
  std::thread *_eventInputThread;

public:
  MidiInputAlsa();
  virtual ~MidiInputAlsa();

  virtual void start(EmuSC::Synth *synth, QString device);
  virtual void stop(void);

  virtual void run();
  
  static QStringList get_available_devices(void);

};


#endif  // MIDI_INPUT_ALSA_H

#endif  // __ALSA_MIDI__
