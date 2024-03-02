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


#include "lcd_display.h"

#include <iostream>


LcdDisplay::LcdDisplay(Scene *scene, EmuSC::Synth **synth,
		       EmuSC::ControlRom **controlRom)
  : _scene(scene),
    _emuscSynth(synth),
    _emuscControlRom(controlRom)
{
  _barDisplay = new BarDisplay(synth, controlRom);

  _connect_signals();
}


LcdDisplay::~LcdDisplay()
{
  delete _barDisplay;
}


void LcdDisplay::turn_on(bool newRom, QString startupAnimSetting)
{
  if (!(*_emuscControlRom)) {
    std::cerr << "EmuSC internal error: Tried to use LCD display with "
	      << "uninitialized Control Rom class" << std::endl;
    return;
  }

  _scene->display_on();

  if (!newRom ||
      (startupAnimSetting != "rom" && startupAnimSetting != "all") ||
      (startupAnimSetting == "rom" && (*_emuscControlRom)->intro_anim_available())) {
    _barDisplay->start();
    emit init_complete();
    return;
  }

  QString _ctrlRomModel = (*_emuscControlRom)->model().c_str();

  set_part(" **");
  set_instrument(" SOUND CANVAS **");

  if (_ctrlRomModel == "SC-55") {
    set_level("SC-");
    set_pan("55 ");

  } else  if (_ctrlRomModel == "SC-55mkII") {
    set_level("SC-");
    set_pan("55 ");
    set_chorus("mk$");

  } else  if (_ctrlRomModel == "SCC1") {
    set_level("SCC");
    set_pan("1 ");
  }

  _barDisplay->play_intro_animations(startupAnimSetting);
  _barDisplay->start();
}


void LcdDisplay::turn_off(void)
{
  _barDisplay->stop();
  _scene->display_off();
}


void LcdDisplay::set_part(QString text)
{
  emit update_part_text(text);
}


void LcdDisplay::set_instrument(QString text)
{
  emit update_instrument_text(text);
}


void LcdDisplay::set_level(QString text)
{
  emit update_level_text(text);
}


void LcdDisplay::set_pan(QString text)
{
  emit update_pan_text(text);
}


void LcdDisplay::set_reverb(QString text)
{
  emit update_reverb_text(text);
}


void LcdDisplay::set_chorus(QString text)
{
  emit update_chorus_text(text);
}


void LcdDisplay::set_kshift(QString text)
{
  emit update_kshift_text(text);
}


void LcdDisplay::set_midich(QString text)
{
  emit update_midich_text(text);
}


void LcdDisplay::mouse_press_event(Qt::MouseButton button, const QPointF &pos)
{
  // Bar display
  if (pos.x() > 288 && pos.y() > 51 &&
      pos.x() < 582 && pos.y() < 162) {
    if (_barDisplay)
      _barDisplay->mouse_press_event(button, pos);
  }
}


void LcdDisplay::_connect_signals(void)
{
  connect(this, SIGNAL(update_part_text(QString)),
	  _scene, SLOT(update_lcd_part_text(QString)));
  connect(this, SIGNAL(update_instrument_text(QString)),
	  _scene, SLOT(update_lcd_instrument_text(QString)));
  connect(this, SIGNAL(update_level_text(QString)),
	  _scene, SLOT(update_lcd_level_text(QString)));
  connect(this, SIGNAL(update_pan_text(QString)),
	  _scene, SLOT(update_lcd_pan_text(QString)));
  connect(this, SIGNAL(update_reverb_text(QString)),
	  _scene, SLOT(update_lcd_reverb_text(QString)));
  connect(this, SIGNAL(update_chorus_text(QString)),
	  _scene, SLOT(update_lcd_chorus_text(QString)));
  connect(this, SIGNAL(update_kshift_text(QString)),
	  _scene, SLOT(update_lcd_kshift_text(QString)));
  connect(this, SIGNAL(update_midich_text(QString)),
	  _scene, SLOT(update_lcd_midich_text(QString)));

  connect(_barDisplay, SIGNAL(update(QVector<bool>*)),
	  _scene, SLOT(update_lcd_bar_display(QVector<bool>*)));
  connect(_barDisplay, SIGNAL(animations_complete()),
	  this, SIGNAL(init_complete()));
}
