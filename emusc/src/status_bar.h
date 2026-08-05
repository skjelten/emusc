/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2026  Håkon Skjelten
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


#ifndef STATUS_BAR_H
#define STATUS_BAR_H


#include "emulator.h"
#include "level_meter.h"

#include <QColor>
#include <QFrame>
#include <QLabel>
#include <QPainter>
#include <QSize>
#include <QStatusBar>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>


class StatusBar : public QStatusBar
{
  Q_OBJECT

public:
  StatusBar(Emulator *emulator, QWidget *parent = nullptr);
  ~StatusBar() {}

  void addPermanentSeparator();

private slots:
  void _emu_started(void);
  void _emu_stopped(void);

private:
  QFrame *_separator;
};


class SBMidiPortMsg : public QLabel
{
  Q_OBJECT

public:
  SBMidiPortMsg(Emulator *emulator, QWidget *parent = nullptr);
  ~SBMidiPortMsg() {}

private slots:
  void _new_port(QString port);
};


class SBMidiActLed : public QLabel
{
  Q_OBJECT

public:
  SBMidiActLed(Emulator *emulator, QWidget *parent = nullptr);
  ~SBMidiActLed() {}

  void set_state(bool state);

private slots:
  void _activity_timeout(void);
  void _new_activity(bool sysEx, int length);
  void _update_anim_color(const QVariant &value);

private:
  QString _ledOff;
  QString _ledOn;

  QTimer *_actTimer;

  QVariantAnimation *_anim;
};


/* Stereo RMS + peak hold meter for the status bar.
 *
 * Knows nothing about where the numbers come from - MainWindow pushes a
 * LevelMeter::Levels struct on a timer and the widget handles dB conversion,
 * ballistics and painting.
 */
class SBVolumeMeter : public QWidget
{
  Q_OBJECT

public:
  SBVolumeMeter(QWidget *parent = nullptr);
  ~SBVolumeMeter() {}

  QSize sizeHint(void) const override;
  QSize minimumSizeHint(void) const override;

  void set_available_width(int windowWidth);

  static constexpr int updateIntervalMs = 40;

public slots:
  void push_levels(const LevelMeter::Levels &levels);
  void reset(void);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

private:
  float _dispLeft, _dispRight;               // Bar position [0-1]
  float _peakLeft, _peakRight;               // Peak hold position [0-1]
  int _peakHoldLeft, _peakHoldRight;         // Remaining hold in timer ticks

  bool _clipLatched;

  int _scaledWidth;
  int _fixedHeight;

  static constexpr float _widthFraction = 0.18f;
  static constexpr int _minWidth = 80;
  static constexpr int _maxWidth = 400;

  float _paintedLeft, _paintedRight;
  float _paintedPeakLeft, _paintedPeakRight;
  bool _paintedClip;

  static constexpr float _floorDb = 60.0f;
  static constexpr float _release = 0.30f;
  static constexpr int _peakHoldTicks = 1500 / updateIntervalMs;
  static constexpr float _peakFall = (20.0f / _floorDb) *
                                     (updateIntervalMs / 1000.0f);

  static float _to_pos(float dB);

  void _draw_channel(QPainter &painter, const QRect &rect,
                     float level, float peak);
};


#endif  // STATUS_BAR_H
