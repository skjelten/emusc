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


#include "emulator.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <QFile>
#include <QSettings>

#include "audio_output_alsa.h"
#include "audio_output_jack.h"
#include "audio_output_pulse.h"
#include "audio_output_wav.h"
#include "audio_output_win32.h"
#include "audio_output_core.h"
#include "audio_output_qt.h"
#include "audio_output_null.h"

#include "midi_input_alsa.h"
#include "midi_input_core.h"
#include "midi_input_win32.h"


#include "envelope_dialog.h"
#include "lfo_dialog.h"


Emulator::Emulator(Scene *scene)
  : _scene(scene),
    _emuscControlRom(NULL),
    _emuscPcmRom(NULL),
    _emuscSynth(NULL),
    _audioOutput(NULL),
    _midiInput(NULL),
    _updateROMs(false),
    _selectedPart(0),
    _allMode(false),
    _running(false)
{
  _lcdDisplay = new LcdDisplay(scene, &_emuscSynth, &_emuscControlRom);

  _connect_signals();
}


Emulator::~Emulator()
{
  delete _lcdDisplay;
}


void Emulator::_connect_signals(void)
{
  connect(_scene, SIGNAL(volume_changed(int)), this, SLOT(change_volume(int)));
  connect(_scene, SIGNAL(play_note(uint8_t,uint8_t)),
	  this, SLOT(play_note(uint8_t,uint8_t)));

  connect(_scene, SIGNAL(all_button_clicked()), this, SLOT(select_all()));
  connect(_scene, SIGNAL(mute_button_clicked()), this, SLOT(select_mute()));

  connect(_scene, SIGNAL(partL_button_clicked()),
	  this, SLOT(select_prev_part()));
  connect(_scene, SIGNAL(partR_button_clicked()),
	  this, SLOT(select_next_part()));

  connect(_scene, SIGNAL(instrumentL_button_clicked()),
	  this, SLOT(select_prev_instrument()));
  connect(_scene, SIGNAL(instrumentR_button_clicked()),
	  this, SLOT(select_next_instrument()));
  connect(_scene, SIGNAL(instrumentL_button_rightClicked()),
	  this, SLOT(select_prev_instrument_variant()));
  connect(_scene, SIGNAL(instrumentR_button_rightClicked()),
	  this, SLOT(select_next_instrument_variant()));

  connect(_scene, SIGNAL(panL_button_clicked()),
	  this, SLOT(select_prev_pan()));
  connect(_scene, SIGNAL(panR_button_clicked()),
	  this, SLOT(select_next_pan()));

  connect(_scene, SIGNAL(chorusL_button_clicked()),
	  this, SLOT(select_prev_chorus()));
  connect(_scene, SIGNAL(chorusR_button_clicked()),
	  this, SLOT(select_next_chorus()));

  connect(_scene, SIGNAL(midichL_button_clicked()),
	  this, SLOT(select_prev_midi_channel()));
  connect(_scene, SIGNAL(midichR_button_clicked()),
	  this, SLOT(select_next_midi_channel()));

  connect(_scene, SIGNAL(levelL_button_clicked()),
	  this, SLOT(select_prev_level()));
  connect(_scene, SIGNAL(levelR_button_clicked()),
	  this, SLOT(select_next_level()));

  connect(_scene, SIGNAL(reverbL_button_clicked()),
	  this, SLOT(select_prev_reverb()));
  connect(_scene, SIGNAL(reverbR_button_clicked()),
	  this, SLOT(select_next_reverb()));

  connect(_scene, SIGNAL(keyshiftL_button_clicked()),
	  this, SLOT(select_prev_key_shift()));
  connect(_scene, SIGNAL(keyshiftR_button_clicked()),
	  this, SLOT(select_next_key_shift()));

  connect(_lcdDisplay, SIGNAL(init_complete()),
	  this, SLOT(lcd_display_init_complete()));
  connect(_scene,SIGNAL(lcd_display_mouse_press_event(Qt::MouseButton,QPointF)),
	  _lcdDisplay, SLOT(mouse_press_event(Qt::MouseButton, QPointF)));
}


void Emulator::start(void)
{
  QSettings settings;

  if (_updateROMs || !_emuscControlRom || !_emuscPcmRom) {
    _updateROMs = true;

    _load_control_roms(settings.value("Rom/prog").toString(),
                       settings.value("Rom/cpu").toString());

    QStringList pcmRomFilePaths;
    pcmRomFilePaths << settings.value("Rom/wave1").toString()
		    << settings.value("Rom/wave2").toString()
		    << settings.value("Rom/wave3").toString();

    _load_pcm_roms(pcmRomFilePaths);
//    _updateROMs == false;
  }

  if (!_emuscControlRom)
    throw(QString("Invalid control ROM selected"));

  if (!_emuscPcmRom)
    throw(QString("Invalid PCM ROM(s) selected"));

  if (_emuscSynth) {
    delete _emuscSynth, _emuscSynth = NULL;
  }

  try {
    _emuscSynth = new EmuSC::Synth(*_emuscControlRom, *_emuscPcmRom, _soundMap);

    _start_audio_subsystem();
    _start_midi_subsystem();

  } catch (QString errorMsg) {
    stop();
    throw(errorMsg);
  }

  _scene->set_model_name(_emuscControlRom->model().data(),
                         _emuscControlRom->version().data());

  connect(_midiInput, SIGNAL(new_midi_message(bool, int)),
	  _scene, SLOT(update_midi_activity_led(bool, int)));

  _lcdDisplay->turn_on(control_rom_changed(),
		       settings.value("Synth/startup_animations").toString());

  QString interpol = settings.value("Audio/interpolation").toString();
  if (!interpol.compare("Nearest", Qt::CaseInsensitive))
    _emuscSynth->set_interpolation_mode(0);
  else if (!interpol.compare("Linear", Qt::CaseInsensitive))
    _emuscSynth->set_interpolation_mode(1);
  else if (!interpol.compare("Cubic", Qt::CaseInsensitive))
    _emuscSynth->set_interpolation_mode(2);

  _emuscSynth->add_part_change_callback(std::bind(&Emulator::_part_change_callback,
                                                  this,
                                                  std::placeholders::_1));

  _running = true;
  emit(started());
}


// Stop synth emulation
void Emulator::stop(void)
{
  _emuscSynth->clear_part_midi_mod_callback();
  _emuscSynth->clear_part_change_callback();

  _lcdDisplay->turn_off();

  if (_midiInput)
    delete _midiInput, _midiInput = NULL;

  if (_audioOutput)
    delete _audioOutput, _audioOutput = NULL;

  if (_emuscSynth) {
    delete _emuscSynth, _emuscSynth = NULL;
  }

  _running = false;
  emit(stopped());
}


void Emulator::_load_control_roms(QString progPath, QString cpuPath)
{
  if (_emuscControlRom)
    delete _emuscControlRom, _emuscControlRom = NULL;

  try {
    if (progPath.isEmpty() || cpuPath.isEmpty()) {
      throw(QString("Emulator is unable to start since one or both of the "
		    "control ROMs (prog and CPU) are missing. "
		    "This can be done in the Preferences dialog."
		    ));
    } else {
#ifdef Q_OS_WINDOWS
      _emuscControlRom = new EmuSC::ControlRom(progPath.toLocal8Bit().constData(),
					       cpuPath.toLocal8Bit().constData());
#else
      _emuscControlRom = new EmuSC::ControlRom(progPath.toStdString(),
					       cpuPath.toStdString());
#endif
    }
  } catch (std::string errorMsg) {
    delete _emuscControlRom, _emuscControlRom = NULL;
    throw(QString("libemusc failed to load the selected control ROM:\n - ")
	  + errorMsg.c_str());
  }

  _ctrlRomModel = _emuscControlRom->model().c_str();
  _ctrlRomVersion = _emuscControlRom->version().c_str();
  _ctrlRomDate = _emuscControlRom->date().c_str();
//  _ctrlRomUpdated = true;
}


void Emulator::_load_pcm_roms(QStringList romPaths)
{
  // We depend on a valid control ROM before loading the PCM ROM
  if (!_emuscControlRom)
    throw(QString("Internal error: Control ROM must be loaded before PCM ROMs"));
  else if (romPaths.isEmpty() || romPaths.first() == "")
    throw(QString("Emulator is unable to start since no Wave ROMs have been "
		  "selected yet. This can be done in the Preferences dialog."));

  // If we already have a PCM ROM loaded, free it first
  if (_emuscPcmRom)
    delete _emuscPcmRom, _emuscPcmRom = NULL;

  std::vector<std::string> romPathsStdVect;
  for (auto &filePath : romPaths) {
    if (!filePath.isEmpty()) {
#ifdef Q_OS_WINDOWS
      romPathsStdVect.push_back(filePath.toLocal8Bit().constData());
#else
      romPathsStdVect.push_back(filePath.toStdString());
#endif
    }
  }
  try {
    _emuscPcmRom = new EmuSC::PcmRom(romPathsStdVect, *_emuscControlRom);
  } catch (std::string errorMsg) {
    delete _emuscPcmRom, _emuscPcmRom = NULL;
    throw(QString(errorMsg.c_str()));
  }

  _pcmRomVersion = _emuscPcmRom->version().c_str();
  _pcmRomDate = _emuscPcmRom->date().c_str();
}




void Emulator::lcd_display_init_complete(void)
{
  _set_part(_selectedPart = 0);
  _emuscSynth->add_part_midi_mod_callback(std::bind(&Emulator::_part_mod_callback,
						    this,
						    std::placeholders::_1));
}


void Emulator::set_envelope_callback(int partId, EnvelopeDialog *dialog)
{
#ifdef __USE_QTCHARTS__
  _emuscSynth->set_part_envelope_callback(partId,
                                          std::bind(&EnvelopeDialog::envelope_callback,
                                                    dialog,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3,
                                                    std::placeholders::_4,
                                                    std::placeholders::_5,
                                                    std::placeholders::_6));
#endif
}


void Emulator::clear_envelope_callback(int partId)
{
  if (_emuscSynth)
    _emuscSynth->clear_part_envelope_callback(partId);
}


void Emulator::set_lfo_callback(int partId, LFODialog *dialog)
{
#ifdef __USE_QTCHARTS__
  _emuscSynth->set_part_lfo_callback(partId,
                                     std::bind(&LFODialog::lfo_callback,
                                               dialog,
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               std::placeholders::_3));
#endif
}


void Emulator::clear_lfo_callback(int partId)
{
  if (_emuscSynth)
    _emuscSynth->clear_part_lfo_callback(partId);
}


void Emulator::_part_mod_callback(const int partId)
{
  if (partId == _selectedPart && !_allMode)
    _set_part(partId);
  else if (partId < 0 && _allMode)
    _set_all();
}


void Emulator::_part_change_callback(const int partId)
{
  emit part_changed(partId);
}


void Emulator::_start_midi_subsystem()
{
  QSettings settings;
  QString midiSystem = settings.value("Midi/system").toString();
  QString midiDevice = settings.value("Midi/device").toString();

  try {	    
    if (!midiSystem.compare("alsa", Qt::CaseInsensitive)) {
#ifdef __ALSA_MIDI__
      _midiInput = new MidiInputAlsa();
#else
      throw(QString("Alsa MIDI system is missing in this build"));
#endif

    } else if (!midiSystem.compare("core midi", Qt::CaseInsensitive)) {
#ifdef __CORE_MIDI__
      _midiInput = new MidiInputCore();
#else
      throw(QString("Core MIDI system is missing in this build"));
#endif

    } else if (!midiSystem.compare("win32", Qt::CaseInsensitive)) {
#ifdef __WIN32_MIDI__
      _midiInput = new MidiInputWin32();
#else
      throw(QString("Win32 MIDI system is missing in this build"));
#endif
    } else {
      throw(QString("No valid MIDI system configured. This can be done in the "
		    "Preferences dialog."));
    }
  } catch (QString errorMsg) {
    throw(QString("Failed to initialize MIDI system (%1)\nError message: %3")
	  .arg(midiSystem).arg(errorMsg));
  }

  _midiInput->start(_emuscSynth, midiDevice);
}


void Emulator::_start_audio_subsystem(void)
{
  QSettings settings;

  if (!settings.contains("Audio/system"))
    throw(QString("Audio system not configured. This can be done in the "
		  "Preferences dialog."));

  QString audioSystem = settings.value("Audio/system").toString();

  try {
    if (!audioSystem.compare("alsa", Qt::CaseInsensitive)) {
#ifdef __ALSA_AUDIO__
      _audioOutput = new AudioOutputAlsa(_emuscSynth);
#else
      throw(QString("'Alsa' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("jack", Qt::CaseInsensitive)) {
#ifdef __JACK_AUDIO__
      _audioOutput = new AudioOutputJack(_emuscSynth);
#else
      throw(QString("'JACK' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("pulse", Qt::CaseInsensitive)) {
#ifdef __PULSE_AUDIO__
      _audioOutput = new AudioOutputPulse(_emuscSynth);
#else
      throw(QString("'Pulse' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("qt", Qt::CaseInsensitive)) {
#ifdef __QT_AUDIO__
      _audioOutput = new AudioOutputQt(_emuscSynth);
#else
      throw(QString("'Qt' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("win32", Qt::CaseInsensitive)) {
#ifdef __WIN32_AUDIO__
      _audioOutput = new AudioOutputWin32(_emuscSynth);
#else
      throw(QString("'Win32' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("wav", Qt::CaseInsensitive)) {
#ifdef __WAV_AUDIO__
      _audioOutput = new AudioOutputWav(_emuscSynth);
#else
      throw(QString("'WAV' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("core audio", Qt::CaseInsensitive)) {
#ifdef __CORE_AUDIO__
      _audioOutput = new AudioOutputCore(_emuscSynth);
#else
      throw(QString("'Core Audio' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("null", Qt::CaseInsensitive)) {
      _audioOutput = new AudioOutputNull(_emuscSynth);
    }

    if (_audioOutput == NULL)
      throw(QString("Unknown audio system"));

  } catch (QString errorMsg) {
    // Delete? stop()?
    throw(QString("Failed to initialize audio system (%1)\nError message: %2")
	  .arg(audioSystem).arg(errorMsg));
  }

  _audioOutput->start();
}


QString Emulator::control_rom_model(void)
{
    return QString(_ctrlRomModel);
}


QString Emulator::control_rom_version(void)
{
    return QString(_ctrlRomVersion);
}


QString Emulator::control_rom_date(void)
{
    return QString(_ctrlRomDate);
}


QString Emulator::pcm_rom_version(void)
{
    return QString(_pcmRomVersion);
}


QString Emulator::pcm_rom_date(void)
{
    return QString(_pcmRomDate);
}

bool Emulator::has_valid_control_rom(void)
{
 if (_emuscControlRom)
     return true;

 return false;
}


bool Emulator::has_valid_pcm_rom(void)
{
 if (_emuscPcmRom)
     return true;

 return false;
}


QStandardItemModel *Emulator::get_instruments_list(void)
{
  if (!_emuscControlRom)
    return NULL;

  std::vector<std::vector<std::string>> instListVector;
  instListVector = _emuscControlRom->get_instruments_list();

  QStringList headers;
  for (auto itr : instListVector[0])
    headers << itr.c_str();

  int numRows = instListVector.size() - 1;
  int numColumns = headers.size();
  QStandardItemModel *model = new QStandardItemModel(numRows, numColumns);

  model->setHorizontalHeaderLabels(headers);

  int row = -1;
  for(auto itr : instListVector) {
    if (row >= 0)                                    // Ignore header row
    for (int i = 0; i < numColumns; i++) {
      QModelIndex index = model->index(row, i, QModelIndex());
      if (i > 0 && itr[i] == "65535")
	model->setData(index, QVariant("-"));
      else
	model->setData(index, QVariant(itr[i].c_str()));
    }
    row ++;
  }

  return model;
}


QStandardItemModel *Emulator::get_partials_list(void)
{
  if (!_emuscControlRom)
    return NULL;

  std::vector<std::vector<std::string>> partListVector;
  partListVector = _emuscControlRom->get_partials_list();

  QStringList headers;
  for (auto itr : partListVector[0])
    headers << itr.c_str();

  int numRows = partListVector.size() - 1;
  int numColumns = headers.size();
  QStandardItemModel *model = new QStandardItemModel(numRows, numColumns);

  model->setHorizontalHeaderLabels(headers);

  int row = -1;
  for(auto itr : partListVector) {
    if (row >= 0)                                    // Ignore header row
      for (int i = 0; i < numColumns; i++) {
	QModelIndex index = model->index(row, i, QModelIndex());
	if ((i < 17 && i > 0 && itr[i] == "127") ||
	    (i > 16 && itr[i] == "65535"))
	  model->setData(index, QVariant("-"));
	else
	  model->setData(index, QVariant(itr[i].c_str()));
      }

    row ++;
  }

  return model;
}


QStandardItemModel *Emulator::get_samples_list(void)
{
  if (!_emuscControlRom)
    return NULL;

  std::vector<std::vector<std::string>> samplesListVector;
  samplesListVector = _emuscControlRom->get_samples_list();
  
  QStringList headers;
  for (auto itr : samplesListVector[0])
    headers << itr.c_str();

  int numRows = samplesListVector.size() - 1;
  int numColumns = headers.size();
  QStandardItemModel *model = new QStandardItemModel(numRows, numColumns);

  model->setHorizontalHeaderLabels(headers);

  int row = -1;
  for (auto itr : samplesListVector) {
    if (row >= 0)                                    // Ignore header row
      for (int i = 0; i < numColumns; i++) {
	QModelIndex index = model->index(row, i, QModelIndex());
	model->setData(index, QVariant(itr[i].c_str()));
      }

    row ++;
  }

  return model;
}


QStandardItemModel *Emulator::get_variations_list(void)
{
  if (!_emuscControlRom)
    return NULL;

  std::vector<std::vector<std::string>> varListVector;
  varListVector = _emuscControlRom->get_variations_list();
 
  QStringList headers;
  for (auto itr : varListVector[0])
    headers << itr.c_str();

  int numRows = varListVector.size() - 1;
  int numColumns = headers.size();
  QStandardItemModel *model = new QStandardItemModel(numRows, numColumns);

  model->setHorizontalHeaderLabels(headers);

  int row = -1;
  for(auto itr : varListVector) {
    if (row >= 0)                                    // Ignore header row
      for (int i = 0; i < numColumns; i++) {
	QModelIndex index = model->index(row, i, QModelIndex());
	if (itr[i] != "65535")
	  model->setData(index, QVariant(itr[i].c_str()));
	else
	  model->setData(index, QVariant(""));
      }

    row ++;
  }

  return model;
}


QStandardItemModel *Emulator::get_drum_sets_list(void)
{
  if (!_emuscControlRom)
    return NULL;

  std::vector<std::string> drumSetsVector;
  drumSetsVector = _emuscControlRom->get_drum_sets_list();

  QStringList headers;
  headers << drumSetsVector[0].c_str();

  int numRows = drumSetsVector.size() - 1;
  int numColumns = headers.size();
  QStandardItemModel *model = new QStandardItemModel(numRows, numColumns);

  model->setHorizontalHeaderLabels(headers);

  int row = -1;
  for(auto itr : drumSetsVector) {
    if (row >= 0) {                                   // Ignore header row
      QModelIndex index = model->index(row, 0, QModelIndex());
      model->setData(index, QVariant(itr.c_str()));
    }

    row ++;
  }

  return model;
}


int Emulator::dump_demo_songs(QString path)
{
  if (!_emuscControlRom)
    return 0;

  return _emuscControlRom->dump_demo_songs(path.toStdString());
}

void Emulator::panic(void)
{
  if (_emuscSynth)
    _emuscSynth->panic();
}


bool Emulator::control_rom_changed(void)
{
  if (_updateROMs) {
    _updateROMs = false;
    return true;
  }

  return false;
}


void Emulator::select_all()
{
  if (_emuscSynth == NULL)
    return;

  _allMode = !_allMode;

  emit(all_button_changed(_allMode));

  if (_allMode)
    _set_all();
  else
    _set_part(_selectedPart);
}


void Emulator::select_mute()
{
  if (_emuscSynth == NULL)
    return;

  bool currentMute = _emuscSynth->get_part_mute(_selectedPart);
  _emuscSynth->set_part_mute(_selectedPart, !currentMute);

  emit(mute_button_changed(!currentMute));
}


void Emulator::select_prev_part()
{
  if (_emuscSynth == NULL || _selectedPart == 0 || _allMode)
    return;

  _set_part(--_selectedPart);
}


void Emulator::select_next_part()
{
  if (_emuscSynth == NULL || _selectedPart >= 15 || _allMode)
    return;

  _set_part(++_selectedPart);
}


// Update all LCD part information with global (ALL) settings
// Used when ALL button is used to change settings affecting all parts
void Emulator::_set_all(void)
{
  if (!_emuscSynth)
    return;

  emit display_part_updated("ALL");
  emit display_instrument_updated("- SOUND Canvas -");

  _set_level(_emuscSynth->get_param(EmuSC::SystemParam::Volume), false);
  _set_pan(_emuscSynth->get_param(EmuSC::SystemParam::Pan), false);
  _set_reverb(_emuscSynth->get_param(EmuSC::PatchParam::ReverbLevel), false);
  _set_chorus(_emuscSynth->get_param(EmuSC::PatchParam::ChorusLevel), false);
  _set_key_shift(_emuscSynth->get_param(EmuSC::SystemParam::KeyShift), false);

  emit display_midi_channel_updated(" 17");

  emit (all_button_changed(_allMode));
}


// Update all LCD part information and button state of mute
// Used when part is changed and all information must be updated
void Emulator::_set_part(uint8_t value)
{
  if (!_emuscSynth)
    return;

  QString str = QStringLiteral("%1").arg(value + 1, 2, 10, QLatin1Char('0'));
  str.prepend(' ');
  _lcdDisplay->set_part(str);

  uint8_t *toneNumber =
    _emuscSynth->get_param_ptr(EmuSC::PatchParam::ToneNumber, value);
  _set_instrument(toneNumber[1], toneNumber[0], false);
  _set_level(_emuscSynth->get_param(EmuSC::PatchParam::PartLevel, value),false);
  _set_pan(_emuscSynth->get_param(EmuSC::PatchParam::PartPanpot, value), false);
  _set_reverb(_emuscSynth->get_param(EmuSC::PatchParam::ReverbSendLevel, value),
	      false);
  _set_chorus(_emuscSynth->get_param(EmuSC::PatchParam::ChorusSendLevel, value),
	      false);
  _set_key_shift(_emuscSynth->get_param(EmuSC::PatchParam::PitchKeyShift,value),
		 false);
  _set_midi_channel(_emuscSynth->get_param(EmuSC::PatchParam::RxChannel, value),
		    false);


  emit (mute_button_changed(_emuscSynth->get_part_mute(value)));
}


void Emulator::select_prev_instrument()
{
  if (!_emuscSynth || _allMode)
    return;

  uint8_t *toneNumber =
    _emuscSynth->get_param_ptr(EmuSC::PatchParam::ToneNumber, _selectedPart);
  uint8_t bank = toneNumber[0];
  uint8_t index = toneNumber[1];
  _set_instrument(index, bank, false);

  // Instrument
  if (!_emuscSynth->get_param(EmuSC::PatchParam::UseForRhythm, _selectedPart)) {
    const std::array<uint16_t, 128> &var = _emuscControlRom->variation(bank);
    for (int i = index - 1; i >= 0; i--) {
      if (var[i] != 0xffff) {
	_set_instrument(i, bank, true);
	break;
      }
    }

  // Drum set
  } else {
    const std::array<uint8_t, 128> &drumSetsLUT = _emuscControlRom->get_drum_sets_LUT();

    for (int i = index - 1; i >= 0; i--) {
      if (drumSetsLUT[i] != 0xff) {
        _set_instrument(i, 0, true);
        break;
      }
    }
  }
}


void Emulator::select_next_instrument()
{
  if (!_emuscSynth || _allMode)
    return;

  uint8_t *toneNumber =
    _emuscSynth->get_param_ptr(EmuSC::PatchParam::ToneNumber, _selectedPart);
  uint8_t bank = toneNumber[0];
  uint8_t index = toneNumber[1];
  _set_instrument(index, bank, false);

  // Instrument
  if (!_emuscSynth->get_param(EmuSC::PatchParam::UseForRhythm, _selectedPart)) {
    const std::array<uint16_t, 128> &var = _emuscControlRom->variation(bank);
    for (int i = index + 1; i < (int) var.size(); i++) {
      if (var[i] != 0xffff) {
	_set_instrument(i, bank, true);
	break;
      }
    }

  // Drum set
  } else {
    const std::array<uint8_t, 128> &drumSetsLUT = _emuscControlRom->get_drum_sets_LUT();

    for (int i = index + 1; i < (int) drumSetsLUT.size(); i++) {
      if (drumSetsLUT[i] != 0xff) {
        _set_instrument(i, 0, true);
        break;
      }
    }
  }
}


void Emulator::select_next_instrument_variant()
{
  if (!_emuscSynth || _allMode)
    return;

  uint8_t *toneNumber =
    _emuscSynth->get_param_ptr(EmuSC::PatchParam::ToneNumber, _selectedPart);
  uint8_t bank = toneNumber[0];
  uint8_t index = toneNumber[1];

  // Only relevant for instrument, not drums
  if (!_emuscSynth->get_param(EmuSC::PatchParam::UseForRhythm, _selectedPart)) {
    for (int i = bank + 1; i < 128; i++) {
      const std::array<uint16_t, 128> &var = _emuscControlRom->variation(i);
      if (var[index] != 0xffff) {
	_set_instrument(index, i, true);
	break;
      }
    }
  }
}


void Emulator::select_prev_instrument_variant()
{
  if (!_emuscSynth || _allMode)
    return;

  uint8_t *toneNumber =
    _emuscSynth->get_param_ptr(EmuSC::PatchParam::ToneNumber, _selectedPart);
  uint8_t bank = toneNumber[0];
  uint8_t index = toneNumber[1];

  // Only relevant for instrument, not drums
  if (!_emuscSynth->get_param(EmuSC::PatchParam::UseForRhythm, _selectedPart)) {
    for (int i = bank - 1; i >= 0; i--) {
      const std::array<uint16_t, 128> &var = _emuscControlRom->variation(i);
      if (var[index] != 0xffff) {
	_set_instrument(index, i, true);
	break;
      }
    }
  }
}


void Emulator::_set_instrument(uint8_t index, uint8_t bank, bool update)
{
  if ((!_emuscSynth && update ) || !_emuscControlRom)
    return;

  QString str;
  uint8_t rhythm =
    _emuscSynth->get_param(EmuSC::PatchParam::UseForRhythm,_selectedPart);
  if (!rhythm) {
    if (update)
      _emuscSynth->set_part_instrument(_selectedPart, index, bank);

    uint16_t instrument = _emuscControlRom->variation(bank)[index];
    str = QStringLiteral("%1").arg(index + 1, 3, 10, QLatin1Char('0'));
    if (bank == 0)
      str.append(" ");
    else if (bank == 127)
      str.append("#");
    else
      str.append("+");
    str.append(QString(_emuscControlRom->instrument(instrument).name.c_str()));

  // Drum set
  } else {
    if (update)
      _emuscSynth->set_part_instrument(_selectedPart, index, bank);

    std::string name((char *) get_param_ptr(EmuSC::DrumParam::DrumsMapName,
					    rhythm - 1), 12);
    str = QStringLiteral("%1*").arg(index + 1, 3, 10, QLatin1Char('0'));
    str.append(QString(name.c_str()));
  }

  _lcdDisplay->set_instrument(str);
}


void Emulator::select_prev_level(void)
{
  if (_emuscSynth == NULL)
    return;

  uint8_t currentLevel;

  if (_allMode)
    currentLevel = _emuscSynth->get_param(EmuSC::SystemParam::Volume);
  else
    currentLevel = _emuscSynth->get_param(EmuSC::PatchParam::PartLevel,
					  _selectedPart);

  if (currentLevel > 0)
    _set_level(currentLevel - 1, true);
}


void Emulator::select_next_level(void)
{
  if (_emuscSynth == NULL)
    return;

  uint8_t currentLevel;

  if (_allMode)
    currentLevel = _emuscSynth->get_param(EmuSC::SystemParam::Volume);
  else
    currentLevel = _emuscSynth->get_param(EmuSC::PatchParam::PartLevel,
					  _selectedPart);

  if (currentLevel < 127)
    _set_level(currentLevel + 1, true);
}


void Emulator::_set_level(uint8_t value, bool update)
{
  if (!_emuscSynth && update)
    return;

  if (update) {
    if (_allMode)
      _emuscSynth->set_param(EmuSC::SystemParam::Volume, value);
    else
      _emuscSynth->set_param(EmuSC::PatchParam::PartLevel, value,_selectedPart);
  }

  QString str = QStringLiteral("%1").arg(value, 3, 10,QLatin1Char(' '));
  _lcdDisplay->set_level(str);
}


void Emulator::select_prev_pan()
{
  if (_emuscSynth == NULL)
    return;

  uint8_t currentPan;

  if (_allMode)
    currentPan = _emuscSynth->get_param(EmuSC::SystemParam::Pan);
  else
    currentPan = _emuscSynth->get_param(EmuSC::PatchParam::PartPanpot,
					      _selectedPart);

  if ((_allMode && currentPan > 1) || (!_allMode && currentPan > 0))
    _set_pan(currentPan - 1, true);
}


void Emulator::select_next_pan()
{
  if (_emuscSynth == NULL)
    return;

  uint8_t currentPan;;

  if (_allMode)
    currentPan = _emuscSynth->get_param(EmuSC::SystemParam::Pan);
  else
    currentPan = _emuscSynth->get_param(EmuSC::PatchParam::PartPanpot,
					      _selectedPart);

  if (currentPan < 127)
    _set_pan(currentPan + 1, true);
}


void Emulator::_set_pan(uint8_t value, bool update)
{
  if (!_emuscSynth)
    return;

  if (update) {
    if (_allMode)
      _emuscSynth->set_param(EmuSC::SystemParam::Pan, value);
    else
      _emuscSynth->set_param(EmuSC::PatchParam::PartPanpot, value, _selectedPart);
  }

  QString str = QStringLiteral("%1").arg(abs(value - 64),3,10,QLatin1Char(' '));

  // Special case: pan == 0 => random
  if (value  == 0)
    str = "Rnd";
  else if (value < 64)
    str.replace(0, 1, 'L');
  else if (value > 64)
    str.replace(0, 1, 'R');

  _lcdDisplay->set_pan(str);
}


void Emulator::select_prev_reverb()
{
  if (_emuscSynth == NULL)
    return;

  uint8_t currentReverb;

  if (_allMode)
    currentReverb =_emuscSynth->get_param(EmuSC::PatchParam::ReverbLevel);
  else
    currentReverb = _emuscSynth->get_param(EmuSC::PatchParam::ReverbSendLevel,
					   _selectedPart);

  if (currentReverb > 0)
    _set_reverb(currentReverb - 1, true);
}


void Emulator::select_next_reverb()
{
  if (_emuscSynth == NULL)
    return;

  uint8_t currentReverb;

  if (_allMode)
    currentReverb =_emuscSynth->get_param(EmuSC::PatchParam::ReverbLevel);
  else
    currentReverb = _emuscSynth->get_param(EmuSC::PatchParam::ReverbSendLevel,
					   _selectedPart);

  if (currentReverb < 127)
    _set_reverb(currentReverb + 1, true);
}


void Emulator::_set_reverb(uint8_t value, bool update)
{
  if (!_emuscSynth && update)
    return;

  if (update) {
    if (_allMode)
      _emuscSynth->set_param(EmuSC::PatchParam::ReverbLevel, value);
    else
      _emuscSynth->set_param(EmuSC::PatchParam::ReverbSendLevel, value,
			     _selectedPart);
  }

  QString str = QStringLiteral("%1").arg(value, 3, 10, QLatin1Char(' '));
  _lcdDisplay->set_reverb(str);
}


void Emulator::select_prev_chorus()
{
  if (_emuscSynth == NULL)
    return;

  uint8_t currentChorus;

  if (_allMode)
    currentChorus =_emuscSynth->get_param(EmuSC::PatchParam::ChorusLevel);
  else
    currentChorus = _emuscSynth->get_param(EmuSC::PatchParam::ChorusSendLevel,
					   _selectedPart);

  if (currentChorus > 0)
    _set_chorus(currentChorus - 1, true);
}


void Emulator::select_next_chorus()
{
  if (_emuscSynth == NULL)
    return;

  uint8_t currentChorus;

  if (_allMode)
    currentChorus =_emuscSynth->get_param(EmuSC::PatchParam::ChorusLevel);
  else
    currentChorus = _emuscSynth->get_param(EmuSC::PatchParam::ChorusSendLevel,
					   _selectedPart);

  if (currentChorus < 127)
    _set_chorus(currentChorus + 1, true);
}


void Emulator::_set_chorus(uint8_t value, bool update)
{
  if (!_emuscSynth && update)
    return;

  if (update) {
    if (_allMode)
      _emuscSynth->set_param(EmuSC::PatchParam::ChorusLevel, value);
    else
      _emuscSynth->set_param(EmuSC::PatchParam::ChorusSendLevel, value,
			     _selectedPart);
  }

  QString str = QStringLiteral("%1").arg(value, 3, 10, QLatin1Char(' '));
  _lcdDisplay->set_chorus(str);
}


void Emulator::select_prev_key_shift()
{
  if (_emuscSynth == NULL)
    return;

  int8_t currentKeyShift;

  if (_allMode)
    currentKeyShift = _emuscSynth->get_param(EmuSC::SystemParam::KeyShift);
  else
    currentKeyShift = _emuscSynth->get_param(EmuSC::PatchParam::PitchKeyShift,
					     _selectedPart);

  if (currentKeyShift > 0x28)
    _set_key_shift(currentKeyShift - 1, true);
}


void Emulator::select_next_key_shift()
{
  if (_emuscSynth == NULL)
    return;

  int8_t currentKeyShift;

  if (_allMode)
    currentKeyShift = _emuscSynth->get_param(EmuSC::SystemParam::KeyShift);
  else
    currentKeyShift = _emuscSynth->get_param(EmuSC::PatchParam::PitchKeyShift,
					     _selectedPart);

  if (currentKeyShift < 0x58)
    _set_key_shift(currentKeyShift + 1, true);
}


void Emulator::_set_key_shift(uint8_t value, bool update)
{
  if (!_emuscSynth && update)
    return;

  if (update) {
    if (_allMode)
      _emuscSynth->set_param(EmuSC::SystemParam::KeyShift, value);
    else
      _emuscSynth->set_param(EmuSC::PatchParam::PitchKeyShift, value,
			     _selectedPart);
  }

  int8_t aValue = value - 0x40;   // Adjust value for default value 0x40 => 0
  QString str = QStringLiteral("%1").arg(abs(aValue), 3, 10, QLatin1Char(' '));
  if (aValue > 0)
    str.replace(0, 1, '+');
  else if (aValue < 0)
    str.replace(0, 1, '-');
  _lcdDisplay->set_kshift(str);
}


void Emulator::select_prev_midi_channel()
{
  if (_emuscSynth == NULL || _allMode)
    return;

  uint8_t currentMidiChannel =
    _emuscSynth->get_param(EmuSC::PatchParam::RxChannel, _selectedPart);
  if (currentMidiChannel > 0)
    _set_midi_channel(currentMidiChannel - 1, true);
}


void Emulator::select_next_midi_channel()
{
  if (_emuscSynth == NULL || _allMode)
    return;

  uint8_t currentMidiChannel =
    _emuscSynth->get_param(EmuSC::PatchParam::RxChannel, _selectedPart);
  if (currentMidiChannel < 16)
    _set_midi_channel(currentMidiChannel + 1, true);
}


void Emulator::_set_midi_channel(uint8_t value, bool update)
{
  if (!_emuscSynth && update)
    return;

  if (update)
    _emuscSynth->set_param(EmuSC::PatchParam::RxChannel, value, _selectedPart);

  QString str;
  if (value < 16) {
    str = QStringLiteral("%1").arg(value + 1, 2, 10, QLatin1Char('0'));
    str.prepend(' ');
  } else {
    str = "Off";
  }
  _lcdDisplay->set_midich(str);
}


// Volume in percent
void Emulator::change_volume(int volume)
{
  if (_audioOutput == NULL)
    return;

  if (volume < 0)
    volume = 0;
  else if (volume > 100)
    volume = 100;

  _audioOutput->set_volume(volume / 100.0);
}


uint8_t Emulator::get_param(enum EmuSC::SystemParam sp)
{
  return _emuscSynth->get_param(sp);
}

uint8_t* Emulator::get_param_ptr(enum EmuSC::SystemParam sp)
{
  return _emuscSynth->get_param_ptr(sp);
}


uint16_t Emulator::get_param_32nib(enum EmuSC::SystemParam sp)
{
  return _emuscSynth->get_param_32nib(sp);
}


uint8_t Emulator::get_param(enum EmuSC::PatchParam pp, int8_t part)
{
  return _emuscSynth->get_param(pp, part);
}


uint8_t* Emulator::get_param_ptr(enum EmuSC::PatchParam pp, int8_t part)
{
  return _emuscSynth->get_param_ptr(pp, part);
}


uint8_t Emulator::get_param_nib16(enum EmuSC::PatchParam pp, int8_t part)
{
  return _emuscSynth->get_param_nib16(pp, part);
}


uint16_t Emulator::get_param_uint14(enum EmuSC::PatchParam pp, int8_t part)
{
  return _emuscSynth->get_param_uint14(pp, part);
}


uint8_t Emulator::get_patch_param(uint16_t address, int8_t part)
{
  return _emuscSynth->get_patch_param(address, part);
}


uint8_t Emulator::get_param(enum EmuSC::DrumParam dp, uint8_t map, uint8_t key)
{
  return _emuscSynth->get_param(dp, map, key);
}


int8_t* Emulator::get_param_ptr(enum EmuSC::DrumParam dp, uint8_t map)
{
  return _emuscSynth->get_param_ptr(dp, map);
}


void Emulator::set_param(enum EmuSC::SystemParam sp, uint8_t value)
{
  _emuscSynth->set_param(sp, value);
}


void Emulator::set_param(enum EmuSC::SystemParam sp, uint8_t *data,uint8_t size)
{
  _emuscSynth->set_param(sp, data, size);
}


void Emulator::set_param(enum EmuSC::SystemParam sp, uint32_t value)
{
  _emuscSynth->set_param(sp, value);
}


void Emulator::set_param_32nib(enum EmuSC::SystemParam sp, uint16_t value)
{
  _emuscSynth->set_param_32nib(sp, value);
}


void Emulator::set_param(enum EmuSC::PatchParam pp, uint8_t value, int8_t part)
{
  _emuscSynth->set_param(pp, value, part);
}


void Emulator::set_param(enum EmuSC::PatchParam sp, uint8_t *data, uint8_t size,
			 int8_t part)
{
  _emuscSynth->set_param(sp, data, size, part);
}

void Emulator::set_param_uint14(enum EmuSC::PatchParam pp, uint16_t value,
				int8_t part)
{
  _emuscSynth->set_param_uint14(pp, value, part);
}


void Emulator::set_param_nib16(enum EmuSC::PatchParam pp, uint8_t value, int8_t part)
{
  _emuscSynth->set_param_nib16(pp, value, part);
}


void Emulator::set_patch_param(uint16_t address, uint8_t value, int8_t part)
{
  _emuscSynth->set_patch_param(address, value, part);
}


void Emulator::set_param(enum EmuSC::DrumParam dp, uint8_t map, uint8_t key,
			 uint8_t value)
{
  _emuscSynth->set_param(dp, map, key, value);
}


void Emulator::set_param(enum EmuSC::DrumParam dp, uint8_t map, uint8_t *data,
			 uint8_t length)
{
  _emuscSynth->set_param(dp, map, data, length);
}


void Emulator::set_interpolation_mode(int mode)
{
  if (_emuscSynth)
    _emuscSynth->set_interpolation_mode(mode);
}


void Emulator::play_note(uint8_t key, uint8_t velocity)
{
  if (_emuscSynth)
    _emuscSynth->midi_input((0x90 | _selectedPart), key, velocity);
}


std::vector<EmuSC::ControlRom::DrumSet> &Emulator::get_drumsets_ref(void)
{
  return _emuscControlRom->get_drumsets_ref();
}


void Emulator::update_LCD_display(int8_t part)
{
  if (part < 0 && _allMode)
    _set_all();
  else if (part == _selectedPart && !_allMode)
    _set_part(part);
  else if (part < 0 && !_allMode)
    _set_part(_selectedPart);
}


enum EmuSC::ControlRom::SynthGen Emulator::get_synth_generation(void)
{
  return _emuscControlRom->generation();
}


void Emulator::reset(void)
{
  if (_emuscSynth)
    _emuscSynth->reset(_soundMap);
}


void Emulator::set_gs_map(void)
{
  _soundMap = EmuSC::Synth::SoundMap::GS;
  reset();
  _selectedPart = 0;
  update_LCD_display();
}


void Emulator::set_gs_gm_map(void)
{
  _soundMap = EmuSC::Synth::SoundMap::GS_GM;
  reset();
  _selectedPart = 0;
  update_LCD_display();
}


void Emulator::set_mt32_map(void)
{
  _soundMap = EmuSC::Synth::SoundMap::MT32;
  reset();
  _selectedPart = 0;
  update_LCD_display();
}


EmuSC::ControlRom::Instrument &Emulator::get_instrument_rom(int bank, int index)
{
  if (!_emuscControlRom)
    throw (QString("No instrument available"));

  uint16_t instrument = _emuscControlRom->variation(bank)[index];

  return _emuscControlRom->instrument(instrument);
}


int Emulator::get_lfo_rate_LUT(int index)
{
  if (!_emuscControlRom)
    throw (QString("Internal error: Control ROM not available!"));
  else if (index < 0 || index > 127)
    throw (QString("Internal error: LFO Rate lookup out of range!"));

  return _emuscControlRom->lookupTables.LFORate[index];
}


int Emulator::get_lfo_delay_fade_LUT(int index)
{
  if (!_emuscControlRom)
    throw (QString("Internal error: Control ROM not available"));
  else if (index < 0 || index > 127)
    throw (QString("Internal error: LFO Delay / Fade lookup out of range!"));

  return _emuscControlRom->lookupTables.LFODelayTime[index];
}
