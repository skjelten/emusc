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


#include "emulator.h"

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
#include "midi_input_keyboard.h"
#include "midi_input_win32.h"


Emulator::Emulator(void)
  : _emuscControlRom(NULL),
    _emuscPcmRom(NULL),
    _emuscSynth(NULL),
    _audioOutput(NULL),
    _midiInput(NULL),
    _ctrlRomUpdated(false)
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

  _ctrlRomModel = _emuscControlRom->get_info_model().c_str();
  _ctrlRomVersion = _emuscControlRom->get_info_version().c_str();
  _ctrlRomDate = _emuscControlRom->get_info_date().c_str();
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

  _pcmRomVersion = _emuscPcmRom->get_info_version().c_str();
  _pcmRomDate = _emuscPcmRom->get_info_date().c_str();
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
    _emuscSynth = new EmuSC::Synth(*_emuscControlRom, *_emuscPcmRom);

    _start_midi_subsystem();
    _start_audio_subsystem();

  } catch (QString errorMsg) {
    stop();
    throw(errorMsg);
  }
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
    } else if (!midiSystem.compare("keyboard", Qt::CaseInsensitive)) {
      _midiInput = new MidiInputKeyboard();
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

void Emulator::volume(int volume)
{
    if (_emuscSynth == NULL)
        return;

  uint8_t calculatedVolume = (float) volume / 100 * 127;
  _emuscSynth->volume(calculatedVolume);
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

void Emulator::all_sounds_off(void)
{
  if (_emuscSynth || _midiInput)
    _midiInput->send_midi_event(0xb0, 120, 0);
}


std::vector<float> Emulator::get_last_parts_sample(void)
{
  if (_emuscSynth)
    return _emuscSynth->get_last_part_samples();

  return std::vector<float> { 0 };
}


QVector<uint8_t> Emulator::get_intro_anim(void)
{
  if (!_emuscControlRom)
  return QVector<uint8_t> { };
  
  return QVector<uint8_t>::fromStdVector(_emuscControlRom->get_intro_anim());
}


bool Emulator::control_rom_changed(void)
{
  if (_ctrlRomUpdated) {
    _ctrlRomUpdated = false;
    return true;
  }

  return false;
}
