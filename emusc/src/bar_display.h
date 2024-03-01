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


#ifndef BAR_DISPLAY_H
#define BAR_DISPLAY_H


#include "emusc/synth.h"
#include "emusc/control_rom.h"

#include <inttypes.h>

#include <QVector>
#include <QString>
#include <QObject>
#include <QPointF>
#include <QTimer>


class BarDisplay : public QObject
{
  Q_OBJECT

public:
  BarDisplay(EmuSC::Synth **synth, EmuSC::ControlRom **controlRom);
  virtual ~BarDisplay(void);

  void start(void);
  void stop(void);

  enum class Animation {
    ROM    = 0,
    ROM2   = 1,
    EMUSC  = 2,
    SC55   = 3
  };
  void play_intro_animations(QString startupAnimSetting);
  
  void mouse_press_event(Qt::MouseButton button, const QPointF &pos);

public slots:
  void update(void);

private:
  EmuSC::Synth **_emuscSynth;
  EmuSC::ControlRom **_emuscControlRom;

  QTimer *_updateTimer;

  int _barDisplayType;                             // Type 1 - 8       [1]
  int _barDisplayPeakHoldType;                     // Off / Type 1 - 3 [1]

  QVector<bool> _barDisplay;
  std::array<float, 16> _currentPartsAmp;

  bool _playAnimation;
  QVector<enum Animation> _animationList;
  QVector<uint8_t> _animBuffer;

  struct lcdBarDisplayPartHist {
    bool falling;
    int value;
    int time;
  };
  QVector<struct lcdBarDisplayPartHist> _barDisplayHistVect;

  unsigned int _animIndex;
  unsigned int _animFrameIndex;

  BarDisplay();

  void _load_next_animation(void);
  bool _set_animBuffer_from_control_rom(int index = 0);
  bool _set_animBuffer_from_resource(QString animPath);

  void _update_animation(void);
  void _update_volume(void);

signals:
  void update(QVector<bool>*);
  void animations_complete(void);

};


#endif // BAR_DISPLAY_H
