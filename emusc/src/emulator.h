/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022  Håkon Skjelten
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


#ifndef EMULATOR_H
#define EMULATOR_H

#include "emusc/control_rom.h"
#include "emusc/pcm_rom.h"
#include "emusc/synth.h"

#include "audio_output.h"
#include "midi_input.h"

#include <QString>
#include <QVector>
#include <QStandardItemModel>

#include <inttypes.h>

class Emulator
{
public:
  Emulator(void);

  void load_control_rom(QString romPath);
  void load_pcm_rom(QVector<QString> romPaths);

  bool has_valid_control_rom(void);
  bool has_valid_pcm_rom(void);

  void start(void);
  void stop(void);

  void volume(int volume);

  QString control_rom_model(void);
  QString control_rom_version(void);
  QString control_rom_date(void);
  QString pcm_rom_version(void);
  QString pcm_rom_date(void);

  QStandardItemModel *get_instruments_list(void);
  QStandardItemModel *get_partials_list(void);
  QStandardItemModel *get_samples_list(void);
  QStandardItemModel *get_variations_list(void);
  QStandardItemModel *get_drum_sets_list(void);

  int dump_demo_songs(QString path);
  QVector<uint8_t> get_intro_anim(void);
  bool control_rom_changed(void);

  void all_sounds_off(void);

  std::vector<float> get_last_parts_sample(void);
  
private:
  EmuSC::ControlRom *_emuscControlRom;
  EmuSC::PcmRom *_emuscPcmRom;
  EmuSC::Synth *_emuscSynth;

  QString _ctrlRomModel;
  QString _ctrlRomVersion;
  QString _ctrlRomDate;
  QString _pcmRomVersion;
  QString _pcmRomDate;

  AudioOutput *_audioOutput;
  MidiInput   *_midiInput;

  bool _ctrlRomUpdated;

  void _start_midi_subsystem();
  void _start_audio_subsystem();
};


#endif // EMULATOR_H
