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


#include "status_bar.h"

#include <QFontMetrics>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QRect>

#include <algorithm>
#include <cmath>


StatusBar::StatusBar(Emulator *emulator, QWidget *parent)
  : QStatusBar(parent)
{
  // Create the vertical separator line
  _separator = new QFrame(this);
  _separator->setFrameShape(QFrame::VLine);
  _separator->setFrameShadow(QFrame::Sunken);

  connect(emulator, SIGNAL(started()), this, SLOT(_emu_started()));
  connect(emulator, SIGNAL(stopped()), this, SLOT(_emu_stopped()));

  showMessage(tr("Power off: Press SPACE to turn on"));
}


void StatusBar::addPermanentSeparator(void)
{
  addPermanentWidget(_separator);
}


void StatusBar::_emu_started(void)
{
  clearMessage();
}


void StatusBar::_emu_stopped(void)
{
  showMessage(tr("Power off: Press SPACE to turn on"));
}


SBMidiPortMsg::SBMidiPortMsg(Emulator *emulator, QWidget *parent)
  : QLabel(parent)
{
  connect(emulator, SIGNAL(midi_port_changed(QString)),
          this, SLOT(_new_port(QString)));

  setToolTip("Active MIDI port for incoming connections");
}


void SBMidiPortMsg::_new_port(QString port)
{
  if (port != "")
    setText("MIDI #" + port);
  else
    setText("");
}


SBMidiActLed::SBMidiActLed(Emulator *emulator, QWidget *parent)
  : QLabel(parent)
{
  setMinimumWidth(height());

  _actTimer = new QTimer();
  _actTimer->setSingleShot(true);
  _actTimer->setTimerType(Qt::CoarseTimer);

  connect(_actTimer, SIGNAL(timeout()), this, SLOT(_activity_timeout()));
  connect(emulator, SIGNAL(new_midi_message(bool, int)),
          this, SLOT(_new_activity(bool, int)));

  _ledOff = "QLabel { border: 1px solid #001122; border-radius: 3px;"
            "background-color: qradialgradient(cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5,"
            "stop:0 #113388, stop:1 #051a44);}";

  _ledOn  = "QLabel {border: 1px solid #0088cc; border-radius: 3px;"
            "background-color: qradialgradient(cx:0.5, cy:0.5, radius:0.6, fx:0.5, fy:0.5,"
            "stop:0 #00e5ff, stop:0.4 #00aaff, stop:1 #0033aa);}"; 

  _anim = new QVariantAnimation(this);
  _anim->setStartValue(QColor("#113388"));
  _anim->setEndValue(QColor("#00e5ff"));

  connect(_anim, &QVariantAnimation::valueChanged, this,
          &SBMidiActLed::_update_anim_color);

  setStyleSheet(_ledOff);

  setToolTip("MIDI activity");
}


void SBMidiActLed::_update_anim_color(const QVariant &value)
{
  QColor centerColor = value.value<QColor>();

  if (centerColor == QColor("#113388")) {
    setStyleSheet(_ledOff);
  } else {
    QString currentStyle = QString("QLabel {border: 1px solid #0088cc; "
                                   "border-radius: 3px; "
                                   "background-color: qradialgradient(cx:0.5, cy:0.5, radius:0.6, fx:0.5, fy:0.5, "
                                   "stop:0 %1, stop:0.5 #0055cc, stop:1 #051a44);}"
                                   ).arg(centerColor.name());

    setStyleSheet(currentStyle);
  }
}


void SBMidiActLed::set_state(bool state)
{
  if (state) {
    _actTimer->stop();

    _anim->stop();
    setStyleSheet(_ledOn); 

    _actTimer->start(150);

  } else {
    _anim->setDuration(300);
    _anim->setDirection(QAbstractAnimation::Backward);
    _anim->start();
  }
}


void SBMidiActLed::_new_activity(bool sysEx, int length)
{
  Q_UNUSED(sysEx); Q_UNUSED(length);
  set_state(true);
}


void SBMidiActLed::_activity_timeout(void)
{
  set_state(false);
}


SBVolumeMeter::SBVolumeMeter(QWidget *parent)
  : QWidget(parent),
    _dispLeft(0), _dispRight(0),
    _peakLeft(0), _peakRight(0),
    _peakHoldLeft(0), _peakHoldRight(0),
    _clipLatched(false),
    _scaledWidth(_minWidth),
    _fixedHeight(16),
    _paintedLeft(-1), _paintedRight(-1),
    _paintedPeakLeft(-1), _paintedPeakRight(-1),
    _paintedClip(false)
{
  _fixedHeight = std::max(14, fontMetrics().height());
  setFixedHeight(_fixedHeight);

  setMinimumWidth(_minWidth);
  setMaximumWidth(_minWidth);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  
  setToolTip("Output level (click to clear clip indicator)");
}


QSize SBVolumeMeter::sizeHint(void) const
{
  return QSize(_scaledWidth, _fixedHeight);
}


QSize SBVolumeMeter::minimumSizeHint(void) const
{
  return QSize(_minWidth, _fixedHeight);
}


void SBVolumeMeter::set_available_width(int windowWidth)
{
  int width = std::clamp((int) (windowWidth * _widthFraction),
                         _minWidth, _maxWidth);

  if (width == _scaledWidth)
    return;

  _scaledWidth = width;
  setMaximumWidth(_scaledWidth);
  updateGeometry();
}


float SBVolumeMeter::_to_pos(float dB)
{
  return std::clamp((dB + _floorDb) / _floorDb, 0.0f, 1.0f);
}


void SBVolumeMeter::push_levels(const LevelMeter::Levels &levels)
{
  float targetLeft = 0.0f, targetRight = 0.0f;

  if (levels.valid) {         // Mean square is a power quantity (10 * log10)
    targetLeft  = _to_pos(10.0f * std::log10(levels.msLeft  + 1e-12f));
    targetRight = _to_pos(10.0f * std::log10(levels.msRight + 1e-12f));
  }

  _dispLeft = (targetLeft > _dispLeft) ?
    targetLeft : _dispLeft + (targetLeft - _dispLeft) * _release;
  _dispRight = (targetRight > _dispRight) ?
    targetRight : _dispRight + (targetRight - _dispRight) * _release;

  if (levels.valid) {         // Peak is an amplitude quantity (20 * log10)
    float peakLeft  = _to_pos(20.0f * std::log10(levels.peakLeft  + 1e-12f));
    float peakRight = _to_pos(20.0f * std::log10(levels.peakRight + 1e-12f));

    if (peakLeft >= _peakLeft) {
      _peakLeft = peakLeft;
      _peakHoldLeft = _peakHoldTicks;
    }
    if (peakRight >= _peakRight) {
      _peakRight = peakRight;
      _peakHoldRight = _peakHoldTicks;
    }
  }

  if (_peakHoldLeft > 0)
    _peakHoldLeft --;
  else
    _peakLeft = std::max(0.0f, _peakLeft - _peakFall);

  if (_peakHoldRight > 0)
    _peakHoldRight --;
  else
    _peakRight = std::max(0.0f, _peakRight - _peakFall);

  if (levels.clip)
    _clipLatched = true;

  // Don't repaint a meter that is already sitting still at silence
  const float epsilon = 0.002f;
  if (std::fabs(_dispLeft - _paintedLeft) > epsilon ||
      std::fabs(_dispRight - _paintedRight) > epsilon ||
      std::fabs(_peakLeft - _paintedPeakLeft) > epsilon ||
      std::fabs(_peakRight - _paintedPeakRight) > epsilon ||
      _clipLatched != _paintedClip)
    update();
}


void SBVolumeMeter::reset(void)
{
  _dispLeft = _dispRight = 0;
  _peakLeft = _peakRight = 0;
  _peakHoldLeft = _peakHoldRight = 0;
  _clipLatched = false;

  update();
}


void SBVolumeMeter::mousePressEvent(QMouseEvent *event)
{
  if (_clipLatched) {
    _clipLatched = false;
    update();
  }

  QWidget::mousePressEvent(event);
}


void SBVolumeMeter::_draw_channel(QPainter &painter, const QRect &rect,
                                float level, float peak)
{
  painter.fillRect(rect, QColor("#001100"));

  QLinearGradient gradient(rect.left(), 0, rect.right(), 0);
  gradient.setColorAt(0.00, QColor("#1E9E1E"));
  gradient.setColorAt(0.70, QColor("#33FF33"));     // -18 dB
  gradient.setColorAt(0.75, QColor("#CCFF33"));
  gradient.setColorAt(0.90, QColor("#FFCC33"));     //  -6 dB
  gradient.setColorAt(0.95, QColor("#FF3333"));
  gradient.setColorAt(1.00, QColor("#FF3333"));

  int fillWidth = qRound(level * rect.width());
  if (fillWidth > 0)
    painter.fillRect(QRect(rect.left(), rect.top(), fillWidth, rect.height()),
                     gradient);

  if (peak > 0.0f) {
    int tickWidth = std::max(2, rect.width() / 100);
    int x = rect.left() + qRound(peak * (rect.width() - tickWidth));
    painter.fillRect(QRect(x, rect.top(), tickWidth, rect.height()),
                     QColor("#FFFFFF"));
  }
}


void SBVolumeMeter::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, false);

  const int barGap = 2;
  const int clipWidth = std::max(6, _fixedHeight / 2);
  const int barHeight = std::max(3, (_fixedHeight - barGap - 4) / 2);

  int meterWidth = width() - clipWidth - 3;
  int top = (height() - (2 * barHeight + barGap)) / 2;

  _draw_channel(painter, QRect(0, top, meterWidth, barHeight),
                _dispLeft, _peakLeft);
  _draw_channel(painter, QRect(0, top + barHeight + barGap,
                               meterWidth, barHeight),
                _dispRight, _peakRight);

  QRect clipRect(width() - clipWidth, top,
                 clipWidth, 2 * barHeight + barGap);
  painter.fillRect(clipRect, _clipLatched ? QColor("#FF3333")
                                          : QColor("#330000"));

  painter.setPen(QColor("#555555"));
  painter.drawRect(QRect(0, top, meterWidth, 2 * barHeight + barGap)
                   .adjusted(0, 0, -1, -1));
  painter.drawRect(clipRect.adjusted(0, 0, -1, -1));

  _paintedLeft = _dispLeft;
  _paintedRight = _dispRight;
  _paintedPeakLeft = _peakLeft;
  _paintedPeakRight = _peakRight;
  _paintedClip = _clipLatched;
}
