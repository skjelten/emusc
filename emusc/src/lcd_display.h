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


#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H


#include "emusc/synth.h"
#include "emusc/control_rom.h"

#include "bar_display.h"
#include "scene.h"

#include <inttypes.h>

#include <QVector>
#include <QString>
#include <QObject>
#include <QPointF>
#include <QTimer>


class LcdDisplay : public QObject
{
  Q_OBJECT

public:
  LcdDisplay(Scene *scene, EmuSC::Synth **synth,EmuSC::ControlRom **controlRom);
  virtual ~LcdDisplay();
  
  void turn_on(bool newRom, QString startupAnimSetting);
  void turn_off(void);

  void set_part(QString text);
  void set_instrument(QString text);
  void set_level(QString text);
  void set_pan(QString text);
  void set_reverb(QString text);
  void set_chorus(QString text);
  void set_kshift(QString text);
  void set_midich(QString text);

  int get_bar_display_type(void) { return _barDisplay->get_type(); }
  void set_bar_display_type(int type) { _barDisplay->set_type(type); }

  int get_bar_display_peak_hold(void) { return _barDisplay->get_peak_hold(); }
  void set_bar_display_peak_hold(int type) { _barDisplay->set_peak_hold(type); }

public slots:
  void mouse_press_event(Qt::MouseButton button, const QPointF &pos);

private:
  Scene *_scene;

  EmuSC::Synth **_emuscSynth;
  EmuSC::ControlRom **_emuscControlRom;

  BarDisplay *_barDisplay;

  LcdDisplay();

  void _connect_signals(void);

signals:
  void init_complete(void);

  void update_part_text(QString text);
  void update_instrument_text(QString text);
  void update_level_text(QString text);
  void update_pan_text(QString text);
  void update_reverb_text(QString text);
  void update_chorus_text(QString text);
  void update_kshift_text(QString text);
  void update_midich_text(QString text);
};


#endif // LCD_DISPLAY_H
