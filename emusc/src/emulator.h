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


#ifndef EMULATOR_H
#define EMULATOR_H

#include "emusc/control_rom.h"
#include "emusc/pcm_rom.h"
#include "emusc/synth.h"
#include "emusc/params.h"

#include "audio_output.h"
#include "lcd_display.h"
#include "midi_input.h"
#include "scene.h"

#include <QString>
#include <QVector>
#include <QStandardItemModel>
#include <QTimer>
#include <QWidget>

#include <inttypes.h>

class EnvelopeDialog;
class LFODialog;

class Emulator : public QObject
{
  Q_OBJECT

public:
  Emulator(Scene *scene);
  virtual ~Emulator();

  void start(void);
  void stop(void);

  bool has_valid_control_rom(void);
  bool has_valid_pcm_rom(void);

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
  bool control_rom_changed(void);

  void panic(void);

  QVector<bool> get_part_amplitude_vector(void);

  void set_update_rom_state(bool state) { _updateROMs = state; }
  enum EmuSC::ControlRom::SynthGen get_synth_generation(void);

  int get_bar_display_type(void) { return _lcdDisplay->get_bar_display_type(); }
  void set_bar_display_type(int type) {_lcdDisplay->set_bar_display_type(type);}

  int get_bar_display_peak_hold(void) { return _lcdDisplay->get_bar_display_peak_hold(); }
  void set_bar_display_peak_hold(int type) { _lcdDisplay->set_bar_display_peak_hold(type);}

  void lcd_mouse_press_event(Qt::MouseButton button, const QPointF &pos);

  // libEmuSC Synth API for get & set paramters
  uint8_t  get_param(enum EmuSC::SystemParam sp);
  uint8_t* get_param_ptr(enum EmuSC::SystemParam sp);
  uint16_t get_param_32nib(enum EmuSC::SystemParam sp);
  uint8_t  get_param(enum EmuSC::PatchParam pp, int8_t part = -1);
  uint8_t* get_param_ptr(enum EmuSC::PatchParam pp, int8_t part = -1);
  uint8_t  get_param_nib16(enum EmuSC::PatchParam pp, int8_t part = -1);
  uint16_t get_param_uint14(enum EmuSC::PatchParam pp, int8_t part = -1);
  uint8_t get_patch_param(uint16_t address, int8_t part);
  uint8_t  get_param(enum EmuSC::DrumParam, uint8_t map, uint8_t key);
  int8_t* get_param_ptr(enum EmuSC::DrumParam, uint8_t map);

  void set_param(enum EmuSC::SystemParam sp, uint8_t value);
  void set_param(enum EmuSC::SystemParam sp, uint8_t *data, uint8_t size = 1);
  void set_param(enum EmuSC::SystemParam sp, uint32_t value);
  void set_param_32nib(enum EmuSC::SystemParam sp, uint16_t value);
  void set_param(enum EmuSC::PatchParam pp, uint8_t value, int8_t part = -1);
  void set_param(enum EmuSC::PatchParam pp, uint8_t *data, uint8_t size = 1,
		 int8_t part = -1);
  void set_param_uint14(enum EmuSC::PatchParam pp, uint16_t value,
			int8_t part = 1);
  void set_param_nib16(enum EmuSC::PatchParam pp, uint8_t value,
		       int8_t part = -1);
  void set_patch_param(uint16_t address, uint8_t value, int8_t part = -1);
  void set_param(enum EmuSC::DrumParam dp, uint8_t map, uint8_t key,
		 uint8_t value);
  void set_param(enum EmuSC::DrumParam dp, uint8_t map, uint8_t *data,
		 uint8_t length);

  void set_envelope_callback(int partId, EnvelopeDialog *dialog);
  void clear_envelope_callback(int partId);
  void set_lfo_callback(int partId, LFODialog *dialog);
  void clear_lfo_callback(int partId);

  void set_interpolation_mode(int mode);

signals:
  void bar_display_update(QVector<bool>*);

  void started(void);
  void stopped(void);

  void new_bar_display(QVector<bool>*);
  void display_part_updated(QString text);
  void display_instrument_updated(QString text);
  void display_level_updated(QString text);
  void display_pan_updated(QString text);
  void display_reverb_updated(QString text);
  void display_chorus_updated(QString text);
  void display_key_shift_updated(QString text);
  void display_midi_channel_updated(QString text);

  void all_button_changed(bool state);
  void mute_button_changed(bool state);

public slots:
  void select_all();
  void select_mute();

  void select_next_part();
  void select_prev_part();
  void select_next_instrument();
  void select_prev_instrument();
  void select_next_instrument_variant();
  void select_prev_instrument_variant();
  void select_next_level();
  void select_prev_level();
  void select_next_pan();
  void select_prev_pan();
  void select_next_reverb();
  void select_prev_reverb();
  void select_next_chorus();
  void select_prev_chorus();
  void select_next_key_shift();
  void select_prev_key_shift();
  void select_next_midi_channel();
  void select_prev_midi_channel();

  void change_volume(int volume);

  void play_note(uint8_t key, uint8_t velocity);

  std::vector<EmuSC::ControlRom::DrumSet> &get_drumsets_ref(void);

  void update_LCD_display(int8_t part = -1);

  void reset(void);
  void set_gs_map(void);
  void set_gs_gm_map(void);
  void set_mt32_map(void);

  bool running(void) { return _running; }
  MidiInput *get_midi_driver(void) { return _midiInput; }

  void lcd_display_init_complete(void);

private:
  Scene *_scene;

  EmuSC::ControlRom *_emuscControlRom;
  EmuSC::PcmRom *_emuscPcmRom;
  EmuSC::Synth *_emuscSynth;

  AudioOutput *_audioOutput;
  MidiInput   *_midiInput;

  LcdDisplay *_lcdDisplay;

  QString _ctrlRomModel;
  QString _ctrlRomVersion;
  QString _ctrlRomDate;
  QString _pcmRomVersion;
  QString _pcmRomDate;

  bool _updateROMs;

  uint8_t _selectedPart;

  bool _allMode;

  bool _running;

  EmuSC::Synth::SoundMap _soundMap;

  Emulator();

  void _connect_signals(void);

  void _start_midi_subsystem();
  void _start_audio_subsystem();

  void _load_control_roms(QString progPath, QString cpuPath);
  void _load_pcm_roms(QStringList romPaths);

  void _set_all(void);
  void _set_part(uint8_t value);

  void _set_instrument(uint8_t index, uint8_t bank, bool update);
  void _set_level(uint8_t value, bool update);
  void _set_pan(uint8_t value, bool update);
  void _set_reverb(uint8_t value, bool update);
  void _set_chorus(uint8_t value, bool update);
  void _set_key_shift(uint8_t value, bool update);
  void _set_midi_channel(uint8_t value, bool update);

  void _part_mod_callback(const int partId);
};


#endif // EMULATOR_H
