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


#include "bar_display.h"

#include <inttypes.h>

#include <iostream>

#include <QDataStream>
#include <QFile>
#include <QDebug>


BarDisplay::BarDisplay(EmuSC::Synth **synth, EmuSC::ControlRom **controlRom)
  : _emuscSynth(synth),
    _emuscControlRom(controlRom),
    _barDisplayHistVect(16, {0,-1,0}),
    _playAnimation(false),
    _animFrameIndex(0),
    _type(1),
    _peakHold(1)
{
  _barDisplay.reserve(256);
//  _barDisplay.fill(1, 256);

  // Start fixed timer for LCD updates & set bar display
  _updateTimer = new QTimer(this);
  connect(_updateTimer, SIGNAL(timeout()), this, SLOT(update()));
}


BarDisplay::~BarDisplay()
{
  delete _updateTimer;
}


void BarDisplay::start(void)
{
  // Start LCD update timer at ~16,67 FPS
  _updateTimer->start(60);
}


void BarDisplay::stop(void)
{
  _updateTimer->stop();
}


void BarDisplay::play_intro_animations(QString startupAnimSetting)
{
  if (startupAnimSetting == "rom") {
    _animationList.push_back(BarDisplay::Animation::ROM);
    _playAnimation = true;

  } else if (startupAnimSetting == "all") {
    _animationList.push_back(BarDisplay::Animation::EMUSC);

    if ((*_emuscControlRom)->intro_anim_available()) {
      _animationList.push_back(BarDisplay::Animation::ROM);

    } else {
      if ((*_emuscControlRom)->model() == "SC-55")
	_animationList.push_back(BarDisplay::Animation::SC55);
//      else if ((*_emuscControlRom)->model().c_str() == "SCC1")
//      else if ((*_emuscControlRom)->model().c_str() == "SCC1A")
//      else if ((*_emuscControlRom)->model().c_str() == "SCB")
//
    }

    _playAnimation = true;
  }
}


void BarDisplay::update(void)
{
  if (!_emuscSynth) {
    std::cerr << "Internal error: EmuSC Synth object deleted, "
	      << "but LCD display is still active" << std::endl;
    return;
  }

  if (_playAnimation) {
    _update_animation();
  } else {
    _currentPartsAmp = (*_emuscSynth)->get_parts_last_peak_sample();
    _update_volume();
  }

  emit update(&_barDisplay);
}


void BarDisplay::set_type(int type)
{
  if (type >= 1 && type <= 8)
    _type = type;
}

void BarDisplay::set_peak_hold(int type)
{
  if (type >= 0 && type <= 3)
    _peakHold = type;
}


void BarDisplay::_update_animation(void)
{
  if (_animFrameIndex == 0)
    _load_next_animation();

  _barDisplay.fill(0, 16 * 16);
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 5; x++) {
      if (_animBuffer[_animFrameIndex + y] & (1 << x))        // First 5 parts
	_barDisplay[16 * 4 - x * 16 + 15 - y] = true;
      else
	_barDisplay[16 * 4 - x * 16 + 15 - y] = false;
            
      if (_animBuffer[_animFrameIndex + y + 16] & (1 << x))   // Parts 5-10
	_barDisplay[16 * 9 - x * 16 + 15 - y] = true;
      else
	_barDisplay[16 * 9 - x * 16 + 15 - y] = false;
      
      if (_animBuffer[_animFrameIndex + y + 32] & (1 << x))   // Parts 10-15
	_barDisplay[16 * 14 - x * 16 + 15 - y] = true;
      else
	_barDisplay[16 * 14 - x * 16 + 15 - y] = false;

      if (_animBuffer[_animFrameIndex + y + 48])              // Part 16
	_barDisplay[16 * 15 + 15 - y] = true;
      else
	_barDisplay[16 * 15 + 15 - y] = false;
    }
  }
    
  // Each frame is 64 bytes (even though there is only 32 bytes of image data)
  _animFrameIndex += 64;

  if (_animFrameIndex >= _animBuffer.size()) {
    _animFrameIndex = 0;

    if (_animationList.isEmpty()) {
      _animBuffer.clear();
      _animBuffer.squeeze();
      _playAnimation = false;

      emit animations_complete();
    }
  }
}


// Default bar display: Bars increase upwards towards max amplitude
void BarDisplay::_update_volume(void)
{
  _barDisplay.clear();

  bool on = (_type <= 4) ? true : false;
  bool off = !on;
  bool showBars = (_type % 2) ? true : false;
  bool upwards = (_type == 1 || _type == 2 ||
		  _type == 5 || _type == 6) ? true : false;
  int partNum = 0;
  for (auto &p : _currentPartsAmp) {
    int height = p * 100 * 0.7;  // FIXME: Audio values follows function?
    if (height > 16) height = 16;

    // We need to store values to support the peak hold feature
    if (_barDisplayHistVect[partNum].value <= height) {
      _barDisplayHistVect[partNum].value = height;
      _barDisplayHistVect[partNum].time = 0;
      _barDisplayHistVect[partNum].falling = false;
    } else {
      _barDisplayHistVect[partNum].time++;
    }

    if (_barDisplayHistVect[partNum].falling == false &&
	_barDisplayHistVect[partNum].time > 16 &&             // First fall
	_barDisplayHistVect[partNum].value > 0 &&
	(_peakHold == 1 || _peakHold == 3)) {
      _barDisplayHistVect[partNum].falling = true;
      _barDisplayHistVect[partNum].value--;
      _barDisplayHistVect[partNum].time = 0;

    } else if (_barDisplayHistVect[partNum].falling == false &&
	       _barDisplayHistVect[partNum].time > 16 &&      // No fall
	       _barDisplayHistVect[partNum].value > 0 &&
	       (_peakHold == 0 || _peakHold == 2)) {
      _barDisplayHistVect[partNum].falling = false;
      _barDisplayHistVect[partNum].value = 0;
      _barDisplayHistVect[partNum].time = 0;

    } else if (_barDisplayHistVect[partNum].falling == true &&
	       _barDisplayHistVect[partNum].time > 2 &&       // Fall speed
	       _barDisplayHistVect[partNum].value > 0) {
      _barDisplayHistVect[partNum].value--;
      _barDisplayHistVect[partNum].time = 0;
    }

    for (int i = 1; i <= 16; i++) {
      int j = (upwards) ? i : 17 - i;
      if ((j == 1 && height >= 0) ||                  // First line, no input
	  (showBars && height > j) ||                 // All bars up to height
	  (_peakHold &&
	   _barDisplayHistVect[partNum].value == j))  // Draw peak hold bar
	_barDisplay.push_back(on);
      else
	_barDisplay.push_back(off);
    }

    partNum ++;
  }
}


void BarDisplay::_load_next_animation(void)
{
  if (_animationList.isEmpty()) {
    std::cerr << "EmuSC internal error: Animation list already empty"
	      << std::endl;
    return;
  }

  switch (_animationList.takeFirst())
    {
    case Animation::ROM:
      if (_set_animBuffer_from_control_rom(0))
	std::cerr << "EmuSC internal error: Unable to read animation from "
		  << "Control ROM"  << std::endl;
      else
	_playAnimation = true;
      break;

    case Animation::ROM2:
      if (_set_animBuffer_from_control_rom(1))
	std::cerr << "EmuSC internal error: Unable to read animation from "
		  << "Control ROM"  << std::endl;
      else
	_playAnimation = true;
      break;

    case Animation::EMUSC:
      if (_set_animBuffer_from_resource(":/animations/emusc.anm"))
	std::cerr << "EmuSC internal error: Animation resource not found"
		  << std::endl;	
      else
	_playAnimation = true;
      break;

    case Animation::SC55:
      if (_set_animBuffer_from_resource(":/animations/sc-55.anm"))
	std::cerr << "EmuSC internal error: Animation resource not found"
		  << std::endl;	
      else
	_playAnimation = true;
      break;

    default:
      std::cerr << "EmuSC internal error: Unknown animation for BarDisplay"
		<< std::endl;
      break;
    }
}


bool BarDisplay::_set_animBuffer_from_control_rom(int index)
{
  if (!_emuscControlRom)
    return 1;

#if QT_VERSION <= QT_VERSION_CHECK(5,14,0)
  _animBuffer = QVector<uint8_t>::fromStdVector((*_emuscControlRom)->get_intro_anim(index));
#else
  std::vector<uint8_t> anim = (*_emuscControlRom)->get_intro_anim(index);
  _animBuffer = QVector<uint8_t>(anim.begin(), anim.end());
#endif

  return 0;
}


bool BarDisplay::_set_animBuffer_from_resource(QString animPath)
{
  QFile f(animPath);
  if (f.open(QIODevice::ReadOnly)) {
    _animBuffer.resize(f.size());
    QDataStream fileStream(&f);
    fileStream.readRawData((char *) &_animBuffer[0], f.size());
    f.close();

    return 0;
  }

  return 1;
}


void BarDisplay::mouse_press_event(Qt::MouseButton button, const QPointF &pos)
{
  if (button == Qt::LeftButton) {
    _type ++;
    if (_type > 8)
      _type = 1;

  } else if (button == Qt::RightButton) {
    _type --;
    if (_type < 1)
      _type = 8;

  } else if (button == Qt::MiddleButton) {
    _peakHold ++;
    if (_peakHold > 3)
      _peakHold = 0;

  } else if (button == Qt::BackButton) {
    _type = 1;
    _peakHold = 1;
  }
}
