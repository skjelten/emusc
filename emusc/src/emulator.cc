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


#include "emulator.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <QSettings>

#include "audio_output_alsa.h"
#include "audio_output_pulse.h"
#include "audio_output_win32.h"
#include "audio_output_core.h"
#include "audio_output_qt.h"
#include "audio_output_null.h"

#include "midi_input_alsa.h"
#include "midi_input_core.h"
#include "midi_input_win32.h"


Emulator::Emulator(void)
  : _emuscControlRom(NULL),
    _emuscPcmRom(NULL),
    _emuscSynth(NULL),
    _audioOutput(NULL),
    _midiInput(NULL),
    _ctrlRomUpdated(false),
    _selectedPart(0),
    _lcdBarDisplayHistVect(16, {0,-1,0}),
    _introFrameIndex(0),
    _allMode(false)
{
  // Start fixed timer for LCD updates & set bar display
  _lcdDisplayTimer = new QTimer(this);

}


Emulator::~Emulator()
{
}


void Emulator::load_control_rom(QString romPath)
{
  if (_emuscControlRom)
    delete _emuscControlRom, _emuscControlRom = NULL;

  try {
    _emuscControlRom = new EmuSC::ControlRom(romPath.toStdString());
  } catch (std::string errorMsg) {
    delete _emuscControlRom, _emuscControlRom = NULL;
    throw(QString(errorMsg.c_str()));
  }

  _ctrlRomModel = _emuscControlRom->model().c_str();
  _ctrlRomVersion = _emuscControlRom->version().c_str();
  _ctrlRomDate = _emuscControlRom->date().c_str();
  _ctrlRomUpdated = true;
}


void Emulator::load_pcm_rom(QVector<QString> romPaths)
{
  // We depend on a valid control ROM before loading the PCM ROM
  if (!_emuscControlRom)
    return;

  // If we already have a PCM ROM loaded, free it first
  if (_emuscPcmRom)
    delete _emuscPcmRom, _emuscPcmRom = NULL;

  std::vector<std::string> romPathsStdVect;
  QVectorIterator<QString> i(romPaths);
  while (i.hasNext())
    romPathsStdVect.push_back(i.next().toStdString());

  try {
    _emuscPcmRom = new EmuSC::PcmRom(romPathsStdVect, *_emuscControlRom);
  } catch (std::string errorMsg) {
    delete _emuscPcmRom, _emuscPcmRom = NULL;
    throw(QString(errorMsg.c_str()));
  }

  _pcmRomVersion = _emuscPcmRom->version().c_str();
  _pcmRomDate = _emuscPcmRom->date().c_str();
}


void Emulator::start(void)
{
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

    // TODO: Set audio format -> _emuscSynth->set_audio_format(44100, 2);

  } catch (QString errorMsg) {
    stop();
    throw(errorMsg);
  }

  emit emulator_started();

  //  Disconnect any previously connected timer signal
  disconnect(_lcdDisplayTimer, SIGNAL(timeout()), NULL, NULL);

  // Set initial state of volume bar view
  // Play intro animation only at first run and only if ROM contains animation
  if (control_rom_changed()) {
    _introAnimData = get_intro_anim();

    if (_introAnimData.isEmpty()) {
      connect(_lcdDisplayTimer, SIGNAL(timeout()),
	      this, SLOT(generate_bar_display()));
      set_part(_selectedPart = 0);
      _emuscSynth->add_part_midi_mod_callback(std::bind(&Emulator::part_mod_callback,
							this,
							std::placeholders::_1));

    } else {
      connect(_lcdDisplayTimer, SIGNAL(timeout()),
	      this, SLOT(play_intro_anim_bar_display()));
    }
  } else {
    connect(_lcdDisplayTimer, SIGNAL(timeout()),
	    this, SLOT(generate_bar_display()));
    set_part(_selectedPart = 0);
    _emuscSynth->add_part_midi_mod_callback(std::bind(&Emulator::part_mod_callback,
						      this,
						      std::placeholders::_1));
  }

  // Start LCD update timer at ~16,67 FPS
  _lcdDisplayTimer->start(60);
}


void Emulator::part_mod_callback(const int partId)
{
  if (partId == _selectedPart && !_allMode)
    set_part(partId);
  else if (partId < 0 && _allMode)
    set_all();
}


void Emulator::_start_midi_subsystem()
{
  QSettings settings;
  QString midiSystem = settings.value("midi/system").toString();
  QString midiDevice = settings.value("midi/device").toString();

  try {	    
    if (!midiSystem.compare("alsa", Qt::CaseInsensitive)) {
#ifdef __ALSA_MIDI__
      _midiInput = new MidiInputAlsa();
#else
      throw(QString("Alsa MIDI system is missing in this build"));
#endif

    } else if (!midiSystem.compare("core", Qt::CaseInsensitive)) {
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
      throw(QString("No valid MIDI system configured"));
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
  QString audioSystem = settings.value("audio/system").toString();

  try {
    if (!audioSystem.compare("alsa", Qt::CaseInsensitive)) {
#ifdef __ALSA_AUDIO__
      _audioOutput = new AudioOutputAlsa(_emuscSynth);
#else
      throw(QString("'Alsa' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("pulse", Qt::CaseInsensitive)) {
#ifdef __PULSE_AUDIO__
      _audioOutput = new AudioOutputPulse(_emuscSynth);
#else
      throw(QString("'Pulse' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("win32", Qt::CaseInsensitive)) {
#ifdef __WIN32_AUDIO__
      _audioOutput = new AudioOutputWin32(_emuscSynth);
#else
      throw(QString("'Win32' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("core", Qt::CaseInsensitive)) {
#ifdef __CORE_AUDIO__
      _audioOutput = new AudioOutputCore(_emuscSynth);
#else
      throw(QString("'Core' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("qt", Qt::CaseInsensitive)) {
#ifdef aQT_MULTIMEDIA_LIB
      _audioOutput = new AudioOutputQt(_emuscSynth);
#else
      throw(QString("'Qt' audio ouput is missing in this build"));
#endif

    } else if (!audioSystem.compare("null", Qt::CaseInsensitive)) {
      _audioOutput = new AudioOutputNull(_emuscSynth);
    }

  } catch (QString errorMsg) {
    // Delete? stop()?
    throw(QString("Failed to initialize audio system (%1)\nError message: %2")
	  .arg(audioSystem).arg(errorMsg));
  }

  _audioOutput->start();
}


// Stop synth emulation
void Emulator::stop(void)
{
  _emuscSynth->clear_part_midi_mod_callback();

  emit emulator_stopped();
  _lcdDisplayTimer->stop();
  _introFrameIndex = 0;

  if (_midiInput)
    delete _midiInput, _midiInput = NULL;

  if (_audioOutput)
    delete _audioOutput, _audioOutput = NULL;

  if (_emuscSynth) {
    delete _emuscSynth, _emuscSynth = NULL;
  }
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


QVector<uint8_t> Emulator::get_intro_anim(void)
{
  if (!_emuscControlRom)
  return QVector<uint8_t> { };

#if QT_VERSION <= QT_VERSION_CHECK(5,14,0)
  return QVector<uint8_t>::fromStdVector(_emuscControlRom->get_intro_anim());
#endif

  std::vector<uint8_t> intro = _emuscControlRom->get_intro_anim();
  return QVector<uint8_t>(intro.begin(), intro.end());
}


bool Emulator::control_rom_changed(void)
{
  if (_ctrlRomUpdated) {
    _ctrlRomUpdated = false;
    return true;
  }

  return false;
}


// Generate a binary map of the bar display
// Height of 0-15 indicates bar 1-16, height < 0 indicates no bars (muted)
void Emulator::generate_bar_display(void)
{
  if (!_emuscSynth)
    return;

  _barDisplay.clear();        // clear() preserves memory allocated for vector

  int partNum = 0;
  std::vector<float> partsAmp = _emuscSynth->get_parts_last_peak_sample();
  for (auto &p : partsAmp) {
    int height = p * 100 * 0.7;  // FIXME: Audio values follows function?
    if (height > 16) height = 16;

    // We need to store values to support the top point to fall slower
    if (_lcdBarDisplayHistVect[partNum].value <= height) {
      _lcdBarDisplayHistVect[partNum].value = height;
      _lcdBarDisplayHistVect[partNum].time = 0;
      _lcdBarDisplayHistVect[partNum].falling = false;
    } else {
      _lcdBarDisplayHistVect[partNum].time++;
    }

    if (_lcdBarDisplayHistVect[partNum].falling == false &&
	_lcdBarDisplayHistVect[partNum].time > 16 &&             // First fall
	_lcdBarDisplayHistVect[partNum].value > 0) {
      _lcdBarDisplayHistVect[partNum].falling = true;
      _lcdBarDisplayHistVect[partNum].value--;
      _lcdBarDisplayHistVect[partNum].time = 0;

    } else if (_lcdBarDisplayHistVect[partNum].falling == true &&
	       _lcdBarDisplayHistVect[partNum].time > 2 &&       // Fall speed
	       _lcdBarDisplayHistVect[partNum].value > 0) {
      _lcdBarDisplayHistVect[partNum].value--;
      _lcdBarDisplayHistVect[partNum].time = 0;
    }

    for (int i = 1; i <= 16; i++) {
      if ((i == 1 && height >= 0) ||                  // First line, no input
	  height > i ||                               // Higher ampl. than bar
	  _lcdBarDisplayHistVect[partNum].value == i) // Hist draw bar
	_barDisplay.push_back(true);
      else
	_barDisplay.push_back(false);
    }

    partNum ++;
  }

  emit new_bar_display(&_barDisplay);
}


void Emulator::play_intro_anim_bar_display(void)
{
  // Update part display before showing intro animation
  if (_introFrameIndex == 0) {
    if (_ctrlRomModel == "SC-55mkII") {
      emit display_part_updated(" **");
      emit display_instrument_updated(" SOUND CANVAS **");
      emit display_level_updated("SC-");
      emit display_pan_updated("55 ");
      emit display_chorus_updated("mk$");
    }
  }

  _barDisplay.fill(0, 16 * 16);

  for (int y = 0; y < 16; y++) {

    // First 5 parts
    for (int x = 0; x < 5; x++) {
      if (_introAnimData[_introFrameIndex + y] & (1 << x))
	_barDisplay[16 * 4 - x * 16 + 15 - y] = true;
      else
	_barDisplay[16 * 4 - x * 16 + 15 - y] = false;
    }

    // Parts 5-10
    for (int x = 0; x < 5; x++) {
      if (_introAnimData[_introFrameIndex + y + 16] & (1 << x))
	_barDisplay[16 * 9 - x * 16 + 15 - y] = true;
      else
	_barDisplay[16 * 9 - x * 16 + 15 - y] = false;
    }

    // Parts 10-15
    for (int x = 0; x < 5; x++) {
      if (_introAnimData[_introFrameIndex + y + 32] & (1 << x))
	_barDisplay[16 * 14 - x * 16 + 15 - y] = true;
      else
	_barDisplay[16 * 14 - x * 16 + 15 - y] = false;
    }

    // Part 16
    if (_introAnimData[_introFrameIndex + y + 48])
      _barDisplay[16 * 15 + 15 - y] = true;
    else
      _barDisplay[16 * 15 + 15 - y] = false;
  }

  emit new_bar_display(&_barDisplay);

  // Each frame is 64 bytes (even though there is only 32 bytes of image data)
  _introFrameIndex += 64;

  if (_introFrameIndex >= _introAnimData.size()) {
    disconnect(_lcdDisplayTimer, SIGNAL(timeout()),
	       this, SLOT(play_intro_anim_bar_display()));
    connect(_lcdDisplayTimer, SIGNAL(timeout()),
	    this, SLOT(generate_bar_display()));
    set_part(_selectedPart = 0);
    _emuscSynth->add_part_midi_mod_callback(std::bind(&Emulator::part_mod_callback,
						      this,
						      std::placeholders::_1));
  }
}


void Emulator::select_all()
{
  if (_emuscSynth == NULL)
    return;

  _allMode = !_allMode;

  emit(all_button_changed(_allMode));

  if (_allMode)
    set_all();
  else
    set_part(_selectedPart);
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

  set_part(--_selectedPart);
}


void Emulator::select_next_part()
{
  if (_emuscSynth == NULL || _selectedPart >= 15 || _allMode)
    return;

  set_part(++_selectedPart);
}


// Update all LCD part information with global (ALL) settings
// Used when ALL button is used to change settings affecting all parts
void Emulator::set_all(void)
{
  if (!_emuscSynth)
    return;

  emit display_part_updated("ALL");
  emit display_instrument_updated("- SOUND Canvas -");

  set_level(_emuscSynth->get_param(EmuSC::SystemParam::Volume), false);
  set_pan(_emuscSynth->get_param(EmuSC::SystemParam::Pan), false);
  set_reverb(_emuscSynth->get_param(EmuSC::PatchParam::ReverbLevel), false);
  set_chorus(_emuscSynth->get_param(EmuSC::PatchParam::ChorusLevel), false);
  set_key_shift(_emuscSynth->get_param(EmuSC::SystemParam::KeyShift), false);

  emit display_midi_channel_updated(" 17");

  emit (all_button_changed(_allMode));
}


// Update all LCD part information and button state of mute
// Used when part is changed and all information must be updated
void Emulator::set_part(uint8_t value)
{
  if (!_emuscSynth)
    return;

  QString str = QStringLiteral("%1").arg(value + 1, 2, 10, QLatin1Char('0'));
  str.prepend(' ');
  emit display_part_updated(str);

  uint8_t *toneNumber =
    _emuscSynth->get_param_ptr(EmuSC::PatchParam::ToneNumber, value);
  set_instrument(toneNumber[1], toneNumber[0], false);
  set_level(_emuscSynth->get_param(EmuSC::PatchParam::PartLevel, value), false);
  set_pan(_emuscSynth->get_param(EmuSC::PatchParam::PartPanpot, value), false);
  set_reverb(_emuscSynth->get_param(EmuSC::PatchParam::ReverbSendLevel, value),
	     false);
  set_chorus(_emuscSynth->get_param(EmuSC::PatchParam::ChorusSendLevel, value),
	     false);
  set_key_shift(_emuscSynth->get_param(EmuSC::PatchParam::PitchKeyShift, value),
		false);
  set_midi_channel(_emuscSynth->get_param(EmuSC::PatchParam::RxChannel, value),
		   false);


  emit (mute_button_changed(_emuscSynth->get_part_mute(value)));
}


void Emulator::select_prev_instrument()
{
  if (_emuscSynth == NULL || _allMode)
    return;

  uint8_t *toneNumber =
    _emuscSynth->get_param_ptr(EmuSC::PatchParam::ToneNumber, _selectedPart);
  uint8_t bank = toneNumber[0];
  uint8_t index = toneNumber[1];

  set_instrument(index, bank, false);

  // Instrument
  if (_emuscSynth->get_param(EmuSC::PatchParam::UseForRhythm,_selectedPart)==0){
    const std::array<uint16_t, 128> &var = _emuscControlRom->variation(bank);
    for (int i = index - 1; i >= 0; i--) {
      if (var[i] != 0xffff) {
	set_instrument(i, bank, true);
	break;
      }
    }

  // Drum set
  } else {
    const std::vector<int> &drumSetBank = _emuscControlRom->drum_set_bank();
    std::vector<int>::const_iterator it = std::find(drumSetBank.begin(),
						    drumSetBank.end(),
						    (int) index);
    if (it != drumSetBank.begin())
      set_instrument(drumSetBank[std::distance(drumSetBank.begin(), it) - 1],
		     0, true);
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
  set_instrument(index, bank, false);

  // Instrument
  if (_emuscSynth->get_param(EmuSC::PatchParam::UseForRhythm,_selectedPart)==0){
    const std::array<uint16_t, 128> &var = _emuscControlRom->variation(bank);
    for (int i = index + 1; i < var.size(); i++) {
      if (var[i] != 0xffff) {
	set_instrument(i, bank, true);
	break;
      }
    }

  // Drum set
  } else {
    const std::vector<int> &drumSetBank = _emuscControlRom->drum_set_bank();
    std::vector<int>::const_iterator it = std::find(drumSetBank.begin(),
						    drumSetBank.end(),
						    (int) toneNumber[1]);
    if (it != drumSetBank.end() - 1)
      set_instrument(drumSetBank[std::distance(drumSetBank.begin(), it) + 1],
		     0, true);
  }
}

void Emulator::set_instrument(uint8_t index, uint8_t bank, bool update)
{
  if ((!_emuscSynth && update ) || !_emuscControlRom)
    return;

  QString str;
  if (_emuscSynth->get_param(EmuSC::PatchParam::UseForRhythm,_selectedPart)==0){
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
  } else {
    const std::vector<int> &drumSetBank = _emuscControlRom->drum_set_bank();
    std::vector<int>::const_iterator it = std::find(drumSetBank.begin(),
						    drumSetBank.end(),
						    (int) index);
    if (it == drumSetBank.end())
      return;

    uint8_t vi = std::distance(drumSetBank.begin(), it);

    if (update)
      _emuscSynth->set_part_instrument(_selectedPart, index, bank);

    str = QStringLiteral("%1*").arg(index + 1, 3, 10, QLatin1Char('0'));
    str.append(QString(_emuscControlRom->drumSet(vi).name.c_str()));
  }

  emit display_instrument_updated(str);
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
    set_level(currentLevel - 1, true);
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
    set_level(currentLevel + 1, true);
}


void Emulator::set_level(uint8_t value, bool update)
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
  emit display_level_updated(str);
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
    set_pan(currentPan - 1, true);
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
    set_pan(currentPan + 1, true);
}


void Emulator::set_pan(uint8_t value, bool update)
{
  if (!_emuscSynth)
    return;

  if (update)
    if (_allMode)
      _emuscSynth->set_param(EmuSC::SystemParam::Pan, value);
    else
      _emuscSynth->set_param(EmuSC::PatchParam::PartPanpot, value, _selectedPart);

  QString str = QStringLiteral("%1").arg(abs(value - 64),3,10,QLatin1Char(' '));

  // Special case: pan == 0 => random
  if (value  == 0)
    str = "Rnd";
  else if (value < 64)
    str.replace(0, 1, 'L');
  else if (value > 64)
    str.replace(0, 1, 'R');

  emit display_pan_updated(str);
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
    set_reverb(currentReverb - 1, true);
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
    set_reverb(currentReverb + 1, true);
}


void Emulator::set_reverb(uint8_t value, bool update)
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
  emit display_reverb_updated(str);
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
    set_chorus(currentChorus - 1, true);
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
    set_chorus(currentChorus + 1, true);
}


void Emulator::set_chorus(uint8_t value, bool update)
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
  emit display_chorus_updated(str);
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
    set_key_shift(currentKeyShift - 1, true);
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
    set_key_shift(currentKeyShift + 1, true);
}


void Emulator::set_key_shift(uint8_t value, bool update)
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
  emit display_key_shift_updated(str);
}


void Emulator::select_prev_midi_channel()
{
  if (_emuscSynth == NULL || _allMode)
    return;

  uint8_t currentMidiChannel =
    _emuscSynth->get_param(EmuSC::PatchParam::RxChannel, _selectedPart);
  if (currentMidiChannel > 0)
    set_midi_channel(currentMidiChannel - 1, true);
}


void Emulator::select_next_midi_channel()
{
  if (_emuscSynth == NULL || _allMode)
    return;

  uint8_t currentMidiChannel =
    _emuscSynth->get_param(EmuSC::PatchParam::RxChannel, _selectedPart);
  std::cout << "currentMidiChannel=" << (int) currentMidiChannel << std::endl;
  if (currentMidiChannel < 16)
    set_midi_channel(currentMidiChannel + 1, true);
}


void Emulator::set_midi_channel(uint8_t value, bool update)
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
  emit display_midi_channel_updated(str);
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
  if (part < 0 && _allMode) {
    set_all();
  } else {
    if (part == _selectedPart && !_allMode)
      set_part(part);
  }
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
