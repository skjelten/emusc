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


#include <QGraphicsScene>
#include <QGraphicsItem>

#include <QGraphicsWidget>
#include <QGraphicsProxyWidget>

#include <QApplication>
#include <QMessageBox>

#include <QHBoxLayout>

#include <QToolButton>
#include <QSettings>
#include <QFont>
#include <QFontDatabase>

#include "scene.h"

#include <iostream>
#include <cmath>

#include "scene.moc"


Scene::Scene(Emulator *emulator, QWidget *parent)
  : _emulator(emulator),
    _lcdBackgroundOnColor(225, 145, 15),
    _lcdBackgroundOffColor(140, 160, 140),
    _lcdOnActiveColor(94, 37, 28),
    _lcdOnInactiveColor(215, 135, 10),
    _lcdOffFontColor(80, 80, 80),
    _lcdBarDisplayHistVect(16, {0,0,0}),
    _introFrameIndex(0)
{
  setParent(parent);

  // Set background color to grey (use bitmap for better look?)
  setBackgroundBrush(QColor(60, 60, 60));

  // Set LCD display background
  _lcdBackground = new QGraphicsRectItem(0, 0, 500, 175);
  _lcdBackground->setBrush(_lcdBackgroundOffColor);
  _lcdBackground->setPen(QColor(0, 0, 0, 0));
  _lcdBackground->setPos(QPointF(100, 0));
  addItem(_lcdBackground);

//    QGraphicsTextItem *logoText = new QGraphicsTextItem;
//    logoText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:18pt; font-weight:bold; color: #bbbbbb\">EmuSC</font>");
//    logoText->setPos(QPointF(500, -35));
//    addItem(logoText);

  QGraphicsTextItem *partHeaderText = new QGraphicsTextItem;
  partHeaderText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10pt; font-weight:normal; color: #bbbbbb\">PART</font>");
  partHeaderText->setPos(QPointF(110, -25));
  addItem(partHeaderText);

  QGraphicsTextItem *instrumentHeaderText = new QGraphicsTextItem;
  instrumentHeaderText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10pt; font-weight:normal; color: #bbbbbb\">INSTRUMENT</font>");
  instrumentHeaderText->setPos(QPointF(192, -25));
  addItem(instrumentHeaderText);

  QGraphicsTextItem *partBottomText = new QGraphicsTextItem;
  partBottomText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10pt; font-weight:normal; color: #bbbbbb\">PART</font>");
  partBottomText->setPos(QPointF(370, 175));
  addItem(partBottomText);

  QGraphicsTextItem *powerText = new QGraphicsTextItem;
  powerText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:12pt; font-weight:normal; color: #bbbbbb\">POWER</font>");
  powerText->setPos(QPointF(0, -10));
  addItem(powerText);

  QGraphicsTextItem *volumeText = new QGraphicsTextItem;
  volumeText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:12pt; font-weight:normal; color: #bbbbbb\">VOLUME</font>");
  volumeText->setPos(QPointF(0, 50));
  addItem(volumeText);

  _lcdLevelHeaderText = new QGraphicsTextItem;
  _lcdLevelHeaderText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:8pt; font-weight:normal\">LEVEL</font>");
  _lcdLevelHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdLevelHeaderText->setPos(QPointF(110, 32));
  addItem(_lcdLevelHeaderText);

  _lcdPanHeaderText = new QGraphicsTextItem;
  _lcdPanHeaderText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:8pt; font-weight:normal\">PAN</font>");
  _lcdPanHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdPanHeaderText->setPos(QPointF(192, 32));
  addItem(_lcdPanHeaderText);

  _lcdReverbHeaderText = new QGraphicsTextItem;
  _lcdReverbHeaderText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:8pt; font-weight:normal\">REVERB</font>");
  _lcdReverbHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdReverbHeaderText->setPos(QPointF(110, 76));
  addItem(_lcdReverbHeaderText);

  _lcdChorusHeaderText = new QGraphicsTextItem;
  _lcdChorusHeaderText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:8pt; font-weight:normal\">CHORUS</font>");
  _lcdChorusHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdChorusHeaderText->setPos(QPointF(192, 76));
  addItem(_lcdChorusHeaderText);

  _lcdKshiftHeaderText = new QGraphicsTextItem;
  _lcdKshiftHeaderText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:8pt; font-weight:normal\">K SHIFT</font>");
  _lcdKshiftHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdKshiftHeaderText->setPos(QPointF(110, 119));
  addItem(_lcdKshiftHeaderText);

  _lcdMidichHeaderText = new QGraphicsTextItem;
  _lcdMidichHeaderText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:8pt; font-weight:normal\">MIDI CH</font>");
  _lcdMidichHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdMidichHeaderText->setPos(QPointF(192, 119));
  addItem(_lcdMidichHeaderText);

  int id = QFontDatabase::addApplicationFont(":/fonts/retro_synth.ttf");
  if (id < 0) {
    QMessageBox::critical(parent,
			  tr("Font file not found"),
			  tr("The font file retro_synth.ttf was not found. "
			     "This font is required for running EmuSC."),
			  QMessageBox::Close);
    exit(-1); // Use exit() since the main loop has not been initiated yet
  }
  QString family = QFontDatabase::applicationFontFamilies(id).at(0);
  QFont retroSynth(family);

  _lcdInstrumentText = new QGraphicsTextItem;
  _lcdInstrumentText->setFont(retroSynth);
  _lcdInstrumentText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdInstrumentText->setPos(QPointF(192, 2));
  addItem(_lcdInstrumentText);

  _lcdPartText = new QGraphicsTextItem;
  _lcdPartText->setFont(retroSynth);
  _lcdPartText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdPartText->setPos(QPointF(110, 2));
  addItem(_lcdPartText);

  _lcdLevelText = new QGraphicsTextItem;
  _lcdLevelText->setFont(retroSynth);
  _lcdLevelText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdLevelText->setPos(QPointF(110, 46));
  addItem(_lcdLevelText);

  _lcdPanText = new QGraphicsTextItem;
  _lcdPanText->setFont(retroSynth);
  _lcdPanText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdPanText->setPos(QPointF(192, 46));
  addItem(_lcdPanText);

  _lcdReverbText = new QGraphicsTextItem;
  _lcdReverbText->setFont(retroSynth);
  _lcdReverbText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdReverbText->setPos(QPointF(110, 90));
  addItem(_lcdReverbText);

  _lcdChorusText = new QGraphicsTextItem;
  _lcdChorusText->setFont(retroSynth);
  _lcdChorusText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdChorusText->setPos(QPointF(192, 90));
  addItem(_lcdChorusText);

  _lcdKshiftText = new QGraphicsTextItem;
  _lcdKshiftText->setFont(retroSynth);
  _lcdKshiftText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdKshiftText->setPos(QPointF(110, 132));
  addItem(_lcdKshiftText);

  _lcdMidichText = new QGraphicsTextItem;
  _lcdMidichText->setFont(retroSynth);
  _lcdMidichText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdMidichText->setPos(QPointF(192, 132));
  addItem(_lcdMidichText);

  _powerButton = new QPushButton();
  _powerButton->setGeometry(QRect(-7, 13, 80, 20));
  _powerButton->setStyleSheet("background-color: #111111; \
                             border-style: outset;	    \
                             border-width: 2px;		    \
                             border-radius: 5px;	    \
                             border-color: #333333;");
  connect(_powerButton, SIGNAL(clicked()), parent, SLOT(power_switch()));
  QGraphicsProxyWidget *pwrProxy = addWidget(_powerButton);

  _volumeDial = new QDial;
  _volumeDial->setGeometry(QRect(-2, 73, 75, 75));
  _volumeDial->setStyleSheet("background-color: #3c3c3c;");
  _volumeDial->setRange(0,100);
  connect(_volumeDial, SIGNAL(valueChanged(int)), parent, SLOT(volume_changed(int)));

  QHBoxLayout *layout = new QHBoxLayout(_volumeDial);
  layout->setContentsMargins(0, 0, 0, 0);
//       layout->addWidget(sizeGrip, 0, Qt::AlignRight | Qt::AlignBottom);

  QGraphicsWidget* parentWidget = new QGraphicsWidget();
  addItem(parentWidget);

  QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget();
  proxy->setWidget(_volumeDial);
  proxy->setParentItem(parentWidget);

  // Populate volume bars with text and bars
  for (int i = 0; i < 16; i ++) {
    QGraphicsTextItem *partNumber = new QGraphicsTextItem;
    partNumber->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:8pt; font-weight:normal\">" + QString::number(i+1) + " </font>");
    partNumber->setDefaultTextColor(QColor(80, 80, 80));

    if (i < 9)
      partNumber->setPos(QPointF(296.5 + 18 * i, 156));
    else
      partNumber->setPos(QPointF(296 + 15.7 * i + 2 * i, 156));

    addItem(partNumber);
    _partNumText.append(partNumber);
    
    for (int j = 0; j < 16; j ++) {
      qreal x = 295 + i*18;
      qreal y = 155 - j*7;

      QGraphicsRectItem* item = new QGraphicsRectItem(0,0,16,6);
      item->setBrush(QBrush(QColor(0, 0, 0, 0)));
      item->setPen(QColor(0, 0, 0, 0));
      item->setPos(QPointF(x, y));
      addItem(item);
      _volumeBars.append(item);
    }
  }

  // Populate filled volume bars circles
  for (int i = 0; i < 11; i ++) {
    QGraphicsEllipseItem *circle = new QGraphicsEllipseItem;
    if (i == 0 || i == 5 || i == 10)
      circle->setRect(287, 155 - i * 10.4, 4.5, 4.5);
    else
      circle->setRect(287 + (1.5 / 2), 155 - i * 10, 3, 3);

    circle->setBrush(_lcdOffFontColor);
    circle->setPen(QColor(0, 0, 0, 0));
    addItem(circle);
    _volumeCircles.append(circle);
  }

  // Update elements according to stored settings
  QSettings settings;
  _volumeDial->setValue(settings.value("volume").toInt());
}


Scene::~Scene()
{
  QSettings settings;
  settings.setValue("volume", _volumeDial->value());
 }


void Scene::display_on(void)
{
  _lcdBackground->setBrush(_lcdBackgroundOnColor);

  // Set all static text in LCD to correct color
  QVectorIterator<QGraphicsTextItem*> it(_partNumText);
  while (it.hasNext())
    it.next()->setDefaultTextColor(_lcdOnActiveColor);

  _lcdLevelHeaderText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdPanHeaderText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdReverbHeaderText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdChorusHeaderText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdKshiftHeaderText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdMidichHeaderText->setDefaultTextColor(_lcdOnActiveColor);

  // Set part volume bar circles to correct color
  QVectorIterator<QGraphicsEllipseItem*> ic(_volumeCircles);
  while (ic.hasNext())
    ic.next()->setBrush(QBrush(_lcdOnActiveColor));

  // Set part volume bars to correct color
  QVectorIterator<QGraphicsRectItem*> ir(_volumeBars);
  while (ir.hasNext())
    ir.next()->setBrush(QBrush(_lcdOnInactiveColor));

  // Set initial LCD text messages
  update_lcd_instrument_text(" SOUND Canvas **");
  update_lcd_part_text(" **");

  // Start fixed timer for LCD updates & set bar display
  _lcdDisplayTimer = new QTimer(this);

  // Set initial state of volume bars
  if (_emulator->control_rom_changed()) {
    _introAnimData = _emulator->get_intro_anim();
    if (_introAnimData.isEmpty())
      connect(_lcdDisplayTimer, &QTimer::timeout,
	      this, QOverload<>::of(&Scene::update_bar_display));
    else
      connect(_lcdDisplayTimer, &QTimer::timeout,
	      this, QOverload<>::of(&Scene::update_intro_anim));

  } else {
    connect(_lcdDisplayTimer, &QTimer::timeout,
	    this, QOverload<>::of(&Scene::update_bar_display));
  }

  // Start LCD update timer at ~16,67 FPS
  _lcdDisplayTimer->start(60);

  // Do we need this?
  _init_lcd_display();
}


void Scene::display_off(void)
{
  _lcdDisplayTimer->stop();
  _introFrameIndex = 0;

  // Set all static text in LCD to correct color
  QVectorIterator<QGraphicsTextItem*> it(_partNumText);
  while (it.hasNext())
    it.next()->setDefaultTextColor(_lcdOffFontColor);

  _lcdLevelHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdPanHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdReverbHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdChorusHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdKshiftHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdMidichHeaderText->setDefaultTextColor(_lcdOffFontColor);

  // Set part volume bar circles to correct color
  QVectorIterator<QGraphicsEllipseItem*> ic(_volumeCircles);
  while (ic.hasNext())
    ic.next()->setBrush(QBrush(_lcdOffFontColor));

  // Set part volume bars to correct color
  QVectorIterator<QGraphicsRectItem*> i(_volumeBars);
  while (i.hasNext())
    i.next()->setBrush(QBrush(QColor(0, 0, 0, 0)));

  // Turn off all non-static text
  update_lcd_instrument_text("");
  update_lcd_part_text("");
  update_lcd_level_text("");
  update_lcd_pan_text("");
  update_lcd_reverb_text("");
  update_lcd_chorus_text("");
  update_lcd_kshift_text("");
  update_lcd_midich_text("");

  // Finally turn off backlight
  _lcdBackground->setBrush(_lcdBackgroundOffColor);
}


void Scene::update_intro_anim()
{
  for (int y = 0; y < 16; y++) {

    // First 5 parts
    for (int x = 0; x < 5; x++) {
      if (_introAnimData[_introFrameIndex + y] & (1 << x))
	_volumeBars[16 * 4 - x * 16 + 15 - y]->setBrush(_lcdOnActiveColor);
      else
	_volumeBars[16 * 4 - x * 16 + 15 - y]->setBrush(_lcdOnInactiveColor);
    }

    // Parts 5-10
    for (int x = 0; x < 5; x++) {
      if (_introAnimData[_introFrameIndex + y + 16] & (1 << x))
	_volumeBars[16 * 9 - x * 16 + 15 - y]->setBrush(_lcdOnActiveColor);
      else
	_volumeBars[16 * 9 - x * 16 + 15 - y]->setBrush(_lcdOnInactiveColor);
    }

    // Parts 10-15
    for (int x = 0; x < 5; x++) {
      if (_introAnimData[_introFrameIndex + y + 32] & (1 << x))
	_volumeBars[16 * 14 - x * 16 + 15 - y]->setBrush(_lcdOnActiveColor);
      else
	_volumeBars[16 * 14 - x * 16 + 15 - y]->setBrush(_lcdOnInactiveColor);
    }

    // FIXME: Part 16 is missing
  }

  // Each frame is 64 bytes (even though there is only 32 bytes of image data)
  _introFrameIndex += 64;

  if (_introFrameIndex >= _introAnimData.size()) {
    disconnect(_lcdDisplayTimer, &QTimer::timeout,
	       this, QOverload<>::of(&Scene::update_intro_anim));
    connect(_lcdDisplayTimer, &QTimer::timeout,
	    this, QOverload<>::of(&Scene::update_bar_display));
  }
}


void Scene::update_bar_display(void)
{
  QVectorIterator<QGraphicsRectItem*> ivb(_volumeBars);
  std::vector<float> partsSample = _emulator->get_last_parts_sample();

  int partNum = 0;
  for (auto &p : partsSample) {
    int height = abs(p * 100) * 0.7;  // FIXME: Audio values follows function?
    if (height > 15) height = 15;

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

    for (int i = 0; i < 16; i++) {
      if (ivb.hasNext() && (i == 0) ||              // Muted parts => hide
	  (height > i || _lcdBarDisplayHistVect[partNum].value == i))
	ivb.next()->setBrush(_lcdOnActiveColor);
      else if (ivb.hasNext())
	ivb.next()->setBrush(_lcdOnInactiveColor);
    }

    partNum ++;
  }
}


void Scene::update_lcd_instrument_text(QString text)
{
    _lcdInstrumentText->setHtml(QString("<html><head><body style=\" white-space: pre-wrap; letter-spacing: 4px; font-style:normal; text-decoration:none;\"><font style=\"font-size:27pt; font-weight:normal;\">") + text + QString("</font>"));
}


void Scene::update_lcd_part_text(QString text)
{
    _lcdPartText->setHtml(QString("<html><head><body style=\" white-space: pre-wrap; letter-spacing: 4px; font-style:normal; text-decoration:none;\"><font style=\"font-size:27pt; font-weight:normal;\">") + text + QString("</font>"));

}

void Scene::update_lcd_level_text(QString text)
{
    _lcdLevelText->setHtml(QString("<html><head><body style=\" white-space: pre-wrap; letter-spacing: 4px; font-style:normal; text-decoration:none;\"><font style=\"font-size:27pt; font-weight:normal;\">") + text + QString("</font>"));

}


void Scene::update_lcd_pan_text(QString text)
{
    _lcdPanText->setHtml(QString("<html><head><body style=\" white-space: pre-wrap; letter-spacing: 4px; font-style:normal; text-decoration:none;\"><font style=\"font-size:27pt; font-weight:normal;\">") + text + QString("</font>"));
}


void Scene::update_lcd_reverb_text(QString text)
{
    _lcdReverbText->setHtml(QString("<html><head><body style=\" white-space: pre-wrap; letter-spacing: 4px; font-style:normal; text-decoration:none;\"><font style=\"font-size:27pt; font-weight:normal;\">") + text + QString("</font>"));
}


void Scene::update_lcd_chorus_text(QString text)
{
    _lcdChorusText->setHtml(QString("<html><head><body style=\" white-space: pre-wrap; letter-spacing: 4px; font-style:normal; text-decoration:none;\"><font style=\"font-size:27pt; font-weight:normal;\">") + text + QString("</font>"));
}


void Scene::update_lcd_kshift_text(QString text)
{
    _lcdKshiftText->setHtml(QString("<html><head><body style=\" white-space: pre-wrap; letter-spacing: 4px; font-style:normal; text-decoration:none;\"><font style=\"font-size:27pt; font-weight:normal;\">") + text + QString("</font>"));
}

void Scene::update_lcd_midich_text(QString text)
{
    _lcdMidichText->setHtml(QString("<html><head><body style=\" white-space: pre-wrap; letter-spacing: 4px; font-style:normal; text-decoration:none;\"><font style=\"font-size:27pt; font-weight:normal;\">") + text + QString("</font>"));
}


// This is just temporary until we have a proper state machine
void Scene::_init_lcd_display()
{
  update_lcd_instrument_text("EmuSC v. 0.01");
  update_lcd_part_text(" 01");
  update_lcd_level_text("100");
  update_lcd_pan_text("  0");
  update_lcd_reverb_text("  0");
  update_lcd_chorus_text("  0");
  update_lcd_kshift_text("  0");
  update_lcd_midich_text(" 01");
}


void Scene::keyPressEvent(QKeyEvent *keyEvent)
{
  if (keyEvent->key() == Qt::Key_Plus) {
    int volume = _volumeDial->value();
    int newVolume = (volume <= 95) ? volume + 5 : 100;
    _volumeDial->setValue(newVolume);

  } else if (keyEvent->key() == Qt::Key_Minus) {
    int volume = _volumeDial->value();
    int newVolume = (volume >= 5) ? volume - 5 : 0;
    _volumeDial->setValue(newVolume);

  } else if (keyEvent->key() == Qt::Key_Space) {
    _powerButton->click();
  }
}
