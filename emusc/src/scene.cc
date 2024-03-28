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


#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QGraphicsItem>
#include <QGraphicsWidget>
#include <QGraphicsPolygonItem>
#include <QGraphicsProxyWidget>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include <QToolButton>

#include "scene.h"

#include <iostream>
#include <cmath>


Scene::Scene(QWidget *parent)
  : _lcdOnBackgroundColorReset(225, 145, 15),
    _lcdOnActiveColorReset(94, 37, 28),
    _lcdOnInactiveColorReset(215, 135, 10),
    _lcdOffBackgroundColor(140, 160, 140),
    _lcdOffFontColor(80, 80, 80),
    _backgroundColor(0, 0, 0),
    _keyNoteOctave(3)
{
  setParent(parent);

  // Update elements according to stored settings
  QSettings settings;
  if (settings.contains("Synth/lcd_bkg_color"))
    _lcdOnBackgroundColor = settings.value("Synth/lcd_bkg_color").value<QColor>();
  else
    _lcdOnBackgroundColor = _lcdOnBackgroundColorReset;

  if (settings.contains("Synth/lcd_active_color"))
    _lcdOnActiveColor = settings.value("Synth/lcd_active_color").value<QColor>();
  else
    _lcdOnActiveColor = _lcdOnActiveColorReset;

  if (settings.contains("Synth/lcd_inactive_color"))
    _lcdOnInactiveColor = settings.value("Synth/lcd_inactive_color").value<QColor>();
  else
    _lcdOnInactiveColor = _lcdOnInactiveColorReset;

  _midiKbdInput = settings.value("Midi/kbd_input").toBool();

  // Set background brush to grey texture for synth object
  _backgroundBrush.setTexture(QPixmap(":/images/synth_bkg.png"));

  // Add sunken frame to LCD display
  QGraphicsRectItem *frameTop = new QGraphicsRectItem(0, 0, 510, 191);
  frameTop->setBrush(QColor(40, 40, 40));
  frameTop->setPen(QColor(0, 0, 0, 0));
  frameTop->setPos(QPointF(94, -8));
  addItem(frameTop);
  QPolygonF frameBottomPoly;
  frameBottomPoly.append(QPoint(94, 183));
  frameBottomPoly.append(QPoint(102, 175));
  frameBottomPoly.append(QPointF(596, 0));
  frameBottomPoly.append(QPointF(604, -8));
  frameBottomPoly.append(QPointF(604, 183));
  QGraphicsPolygonItem* frameBottom = new QGraphicsPolygonItem(frameBottomPoly);
  frameBottom->setBrush(QBrush(QColor(100, 100, 100, 255), Qt::SolidPattern));
  frameBottom->setPen(QColor(90, 90, 90, 90));
  addItem(frameBottom);

  // Add sunken button rectangles
  addItem(new GrooveRect(760, -10, 353, 35));
  addItem(new GrooveRect(760, 45, 353, 35));
  addItem(new GrooveRect(760, 100, 353, 35));
  addItem(new GrooveRect(760, 155, 353, 35));
  addItem(new GrooveRect(-15, 8, 100, 35));

  _add_lcd_display_items();

  // Set black vertical bar background
  QGraphicsRectItem *blackBackground = new QGraphicsRectItem(0, 0, 110, 280);
  blackBackground->setBrush(QColor(0, 0, 0));
  blackBackground->setPen(QColor(0, 0, 0, 0));
  blackBackground->setPos(QPointF(637, -50));
  addItem(blackBackground);

//    QGraphicsTextItem *logoText = new QGraphicsTextItem;
//    logoText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:18pt; font-weight:bold; color: #bbbbbb\">EmuSC</font>");
//    logoText->setPos(QPointF(500, -35));
//    addItem(logoText);

  QGraphicsTextItem *partHeaderText = new QGraphicsTextItem;
  partHeaderText->setHtml(_generate_sans_text_html("PART", 10));
  partHeaderText->setDefaultTextColor(0xbbbbbb);
  partHeaderText->setPos(QPointF(110, -30));
  addItem(partHeaderText);

  QGraphicsTextItem *instrumentHeaderText = new QGraphicsTextItem;
  instrumentHeaderText->setHtml(_generate_sans_text_html("INSTRUMENT", 10));
  instrumentHeaderText->setDefaultTextColor(0xbbbbbb);
  instrumentHeaderText->setPos(QPointF(192, -30));
  addItem(instrumentHeaderText);

  QGraphicsTextItem *partBottomText = new QGraphicsTextItem;
  partBottomText->setHtml(_generate_sans_text_html("PART", 10));
  partBottomText->setDefaultTextColor(0xbbbbbb);
  partBottomText->setPos(QPointF(385, 180));
  addItem(partBottomText);

  QGraphicsTextItem *powerText = new QGraphicsTextItem;
  powerText->setHtml(_generate_sans_text_html("POWER", 12));
  powerText->setDefaultTextColor(0xbbbbbb);
  powerText->setPos(QPointF(0, -20));
  addItem(powerText);

  QGraphicsTextItem *volumeText = new QGraphicsTextItem;
  volumeText->setHtml(_generate_sans_text_html("VOLUME", 12));
  volumeText->setDefaultTextColor(0xbbbbbb);
  volumeText->setPos(QPointF(0, 55));
  addItem(volumeText);

  _powerButton = new QPushButton();
  _powerButton->setGeometry(QRect(-5, 13, 78, 25));
  _powerButton->setStyleSheet("background-color: #111111; \
                             border-style: outset;	    \
                             border-width: 2px;		    \
                             border-radius: 5px;	    \
                             border-color: #333333;");
  _powerButton->setAttribute(Qt::WA_TranslucentBackground);
  QGraphicsProxyWidget *pwrProxy = addWidget(_powerButton);

  _volumeDial = new VolumeDial();
  _volumeDial->setGeometry(QRect(-2, 78, 75, 75));
  _volumeDial->setStyleSheet("background-color: #00000000;");
  _volumeDial->setRange(0,100);

  // Default value
  _volumeDial->setValue(settings.value("Audio/volume", 80).toInt());

  QHBoxLayout *layout = new QHBoxLayout(_volumeDial);
  layout->setContentsMargins(0, 0, 0, 0);
//       layout->addWidget(sizeGrip, 0, Qt::AlignRight | Qt::AlignBottom);

  QGraphicsWidget* parentWidget = new QGraphicsWidget();
  addItem(parentWidget);

  QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget();
  proxy->setWidget(_volumeDial);
  proxy->setParentItem(parentWidget);

  // Add all / mute buttons
  _allButton = new QPushButton();
  _allButton->setCheckable(true);
  _allButton->setGeometry(QRect(693, 0, 34, 34));
  _allButton->setStyleSheet("QPushButton { "		\
			     "color: #aaa;"		\
			     "border: 2px solid #555; " \
			     "border-radius: 17px;"     \
			     "border-style: outset;"	\
			     "background: qradialgradient(cx: 0.3, cy: -0.4," \
			     "fx: 0.3, fy: -0.4,radius: 1.35, stop: 0 #fff, stop: 1 #888);" \
			     "padding: 5px }"		\
			     "QPushButton::checked"	\
                             "{background-color : #ff7a45;}");
  _allButton->setAttribute(Qt::WA_TranslucentBackground);
  QGraphicsProxyWidget *allBtnProxy = addWidget(_allButton);

  _muteButton = new QPushButton();
  _muteButton->setCheckable(true);
  _muteButton->setGeometry(QRect(693, 50, 34, 34));
  _muteButton->setStyleSheet("QPushButton { "		\
			     "color: #aaa;"		\
			     "border: 2px solid #555; " \
			     "border-radius: 17px;"     \
			     "border-style: outset;"	\
			     "background: qradialgradient(cx: 0.3, cy: -0.4," \
			     "fx: 0.3, fy: -0.4,radius: 1.35, stop: 0 #fff, stop: 1 #888);" \
			     "padding: 5px }"		\
			     "QPushButton::checked"	\
                             "{background-color : #ff7a45;}");
  _muteButton->setAttribute(Qt::WA_TranslucentBackground);
  QGraphicsProxyWidget *muteBtnProxy = addWidget(_muteButton);

  // Add partL / mute buttons
  _partLButton = new QPushButton();
  _partLButton->setGeometry(QRect(802, -5, 26, 26));
  _partLButton->setStyleSheet("color: #aaa;"	       \
			    "border: 2px solid #555; " \
			    "border-radius: 13px;"     \
			    "border-style: outset;"    \
			      "background: black;"     \
			    "padding: 5px");
  _partLButton->setAttribute(Qt::WA_TranslucentBackground);
  _partLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  _partLButton->setIconSize(QSize(9, 9));
  _partLButton->setAutoRepeat(true);
  _partLButton->setAutoRepeatDelay(500);
  _partLButton->setAutoRepeatInterval(120);
  QGraphicsProxyWidget *partLBtnProxy = addWidget(_partLButton);

  _partRButton = new QPushButton();
  _partRButton->setGeometry(QRect(874, -5, 26, 26));
  _partRButton->setStyleSheet("color: #aaa;"	       \
			    "border: 2px solid #555; " \
			    "border-radius: 13px;"     \
			    "border-style: outset;"    \
			    "background: black;"     \
			    "padding: 5px");
  _partRButton->setAttribute(Qt::WA_TranslucentBackground);
  _partRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  _partRButton->setIconSize(QSize(9, 9));
  _partRButton->setAutoRepeat(true);
  _partRButton->setAutoRepeatDelay(500);
  _partRButton->setAutoRepeatInterval(120);
  QGraphicsProxyWidget *partRBtnProxy = addWidget(_partRButton);

  // Add instrument L/R buttons
  _instrumentLButton = new SynthButton();
  _instrumentLButton->setGeometry(QRect(945, -5, 70, 25));
  _instrumentLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  QGraphicsProxyWidget *instLBtnProxy = addWidget(_instrumentLButton);

  _instrumentRButton = new SynthButton();
  _instrumentRButton->setGeometry(QRect(1018, -5, 70, 25));
  _instrumentRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  QGraphicsProxyWidget *instRBtnProxy = addWidget(_instrumentRButton);

  // Add pan L/R buttons
  _panLButton = new SynthButton();
  _panLButton->setGeometry(QRect(945, 50, 70, 25));
  _panLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  QGraphicsProxyWidget *panLBtnProxy = addWidget(_panLButton);

  _panRButton = new SynthButton();
  _panRButton->setGeometry(QRect(1018, 50, 70, 25));
  _panRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  QGraphicsProxyWidget *panRBtnProxy = addWidget(_panRButton);

  // Add chorus L/R buttons
  _chorusLButton = new SynthButton();
  _chorusLButton->setGeometry(QRect(945, 105, 70, 25));
  _chorusLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  QGraphicsProxyWidget *chorusLBtnProxy = addWidget(_chorusLButton);

  _chorusRButton = new SynthButton();
  _chorusRButton->setGeometry(QRect(1018, 105, 70, 25));
  _chorusRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  QGraphicsProxyWidget *chorusRBtnProxy = addWidget(_chorusRButton);

  // Add midich L/R buttons
  _midichLButton = new SynthButton();
  _midichLButton->setGeometry(QRect(945, 160, 70, 25));
  _midichLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  QGraphicsProxyWidget *midichLBtnProxy = addWidget(_midichLButton);

  _midichRButton = new SynthButton();
  _midichRButton->setGeometry(QRect(1018, 160, 70, 25));
  _midichRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  QGraphicsProxyWidget *midichRBtnProxy = addWidget(_midichRButton);

  // Add level L/R buttons
  _levelLButton = new SynthButton();
  _levelLButton->setGeometry(QRect(780, 50, 70, 25));
  _levelLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  QGraphicsProxyWidget *levelLBtnProxy = addWidget(_levelLButton);

  _levelRButton = new SynthButton();
  _levelRButton->setGeometry(QRect(853, 50, 70, 25));
  _levelRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  QGraphicsProxyWidget *levelRBtnProxy = addWidget(_levelRButton);

  // Add reverb L/R buttons
  _reverbLButton = new SynthButton();
  _reverbLButton->setGeometry(QRect(780, 105, 70, 25));
  _reverbLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  QGraphicsProxyWidget *reverbLBtnProxy = addWidget(_reverbLButton);

  _reverbRButton = new SynthButton();
  _reverbRButton->setGeometry(QRect(853, 105, 70, 25));
  _reverbRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  QGraphicsProxyWidget *reverbRBtnProxy = addWidget(_reverbRButton);

  // Add keyshift L/R buttons
  _keyshiftLButton = new SynthButton();
  _keyshiftLButton->setGeometry(QRect(780, 160, 70, 25));
  _keyshiftLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  QGraphicsProxyWidget *keyshiftLBtnProxy = addWidget(_keyshiftLButton);

  _keyshiftRButton = new SynthButton();
  _keyshiftRButton->setGeometry(QRect(853, 160, 70, 25));
  _keyshiftRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  QGraphicsProxyWidget *keyshiftRBtnProxy = addWidget(_keyshiftRButton);

  QGraphicsTextItem *allBtnText = new QGraphicsTextItem;
  allBtnText->setHtml(_generate_sans_text_html("ALL", 10.5));
  allBtnText->setPos(QPointF(690 - allBtnText->boundingRect().right(), 3));
  allBtnText->setDefaultTextColor(0xbbbbbb);
  addItem(allBtnText);

  QGraphicsTextItem *muteBtnText = new QGraphicsTextItem;
  muteBtnText->setHtml(_generate_sans_text_html("MUTE", 10.5));
  muteBtnText->setPos(QPointF(690 - muteBtnText->boundingRect().right(), 56));
  muteBtnText->setDefaultTextColor(0xbbbbbb);
  addItem(muteBtnText);

  QGraphicsTextItem *partBtnText = new QGraphicsTextItem;
  partBtnText->setHtml(_generate_sans_text_html("PART", 10.5));
  partBtnText->setPos(QPointF(855 - partBtnText->boundingRect().center().x(), -33));
  partBtnText->setDefaultTextColor(0xbbbbbb);
  addItem(partBtnText);

  QGraphicsTextItem *instrumentBtnText = new QGraphicsTextItem;
  instrumentBtnText->setHtml(_generate_sans_text_html("INSTRUMENT", 10.5));
  instrumentBtnText->setPos(QPointF(1015 - instrumentBtnText->boundingRect().center().x(), -33));
  instrumentBtnText->setDefaultTextColor(0xbbbbbb);
  addItem(instrumentBtnText);

  QGraphicsTextItem *levelBtnText = new QGraphicsTextItem;
  levelBtnText->setHtml(_generate_sans_text_html("LEVEL", 10.5));
  levelBtnText->setPos(QPointF(855 - levelBtnText->boundingRect().center().x(), 22));
  levelBtnText->setDefaultTextColor(0xbbbbbb);
  addItem(levelBtnText);

  QGraphicsTextItem *panBtnText = new QGraphicsTextItem;
  panBtnText->setHtml(_generate_sans_text_html("PAN", 10.5));
  panBtnText->setPos(QPointF(1015 - panBtnText->boundingRect().center().x(), 22));
  panBtnText->setDefaultTextColor(0xbbbbbb);
  addItem(panBtnText);

  QGraphicsTextItem *reverbBtnText = new QGraphicsTextItem;
  reverbBtnText->setHtml(_generate_sans_text_html("REVERB", 10.5));
  reverbBtnText->setPos(QPointF(855 - reverbBtnText->boundingRect().center().x(), 77));
  reverbBtnText->setDefaultTextColor(0xbbbbbb);
  addItem(reverbBtnText);

  QGraphicsTextItem *chorusBtnText = new QGraphicsTextItem;
  chorusBtnText->setHtml(_generate_sans_text_html("CHORUS", 10.5));
  chorusBtnText->setPos(QPointF(1015 - chorusBtnText->boundingRect().center().x(), 77));
  chorusBtnText->setDefaultTextColor(0xbbbbbb);
  addItem(chorusBtnText);

  QGraphicsTextItem *keyshiftBtnText = new QGraphicsTextItem;
  keyshiftBtnText->setHtml(_generate_sans_text_html("KEY SHIFT", 10.5));
  keyshiftBtnText->setPos(QPointF(855 - keyshiftBtnText->boundingRect().center().x(), 132));
  keyshiftBtnText->setDefaultTextColor(0xbbbbbb);
  addItem(keyshiftBtnText);

  QGraphicsTextItem *midichBtnText = new QGraphicsTextItem;
  midichBtnText->setHtml(_generate_sans_text_html("MIDI CH", 10.5));
  midichBtnText->setPos(QPointF(1015 - midichBtnText->boundingRect().center().x(), 132));
  midichBtnText->setDefaultTextColor(0xbbbbbb);
  addItem(midichBtnText);

  // TODO: Show GM / GS logo based on ROM version
  QGraphicsPixmapItem *gmLogo = new QGraphicsPixmapItem(QPixmap(":/images/gm_logo.png"));
  gmLogo->setPos(645, 170);
  addItem(gmLogo);

  QGraphicsPixmapItem *gsLogo = new QGraphicsPixmapItem(QPixmap(":/images/gs_logo.png"));
  gsLogo->setPos(695, 170);
  addItem(gsLogo);

  // Add LED button
  _ledOnGradient = new QRadialGradient(35, 179, 20);
  _ledOnGradient->setColorAt(.0, 0x2fafff);
  _ledOnGradient->setColorAt(.6, 0x0000ff);

  _ledOffGradient = new QRadialGradient(35, 179, 20);
  _ledOffGradient->setColorAt(.0, 0x000090);
  _ledOffGradient->setColorAt(.9, Qt::black);

  _midiActLed = new QGraphicsRectItem(25, 175, 20, 8);
  _midiActLed->setPen(QColor(40, 40, 40));
  _midiActLed->setBrush(*_ledOffGradient);
  addItem(_midiActLed);

  _midiActTimer = new QTimer();
  _midiActTimer->setSingleShot(true);
  _midiActTimer->setTimerType(Qt::CoarseTimer);
  connect(_midiActTimer, SIGNAL(timeout()),
	  this, SLOT(update_midi_activity_timeout()));

  _connect_signals();
}


Scene::~Scene()
{
  QSettings settings;
  settings.setValue("Audio/volume", _volumeDial->value());
}


void Scene::_connect_signals(void)
{
  connect(_allButton, SIGNAL(clicked()), this, SIGNAL(all_button_clicked()));
  connect(_muteButton, SIGNAL(clicked()), this, SIGNAL(mute_button_clicked()));

  connect(_partLButton, SIGNAL(clicked()),
	  this, SIGNAL(partL_button_clicked()));
  connect(_partRButton, SIGNAL(clicked()),
	  this, SIGNAL(partR_button_clicked()));
  connect(_instrumentLButton, SIGNAL(clicked()),
	  this, SIGNAL(instrumentL_button_clicked()));
  connect(_instrumentRButton, SIGNAL(clicked()),
	  this, SIGNAL(instrumentR_button_clicked()));
  connect(_instrumentLButton, SIGNAL(rightClicked()),
	  this, SIGNAL(instrumentL_button_rightClicked()));
  connect(_instrumentRButton, SIGNAL(rightClicked()),
	  this, SIGNAL(instrumentR_button_rightClicked()));
  connect(_panLButton, SIGNAL(clicked()),
	  this, SIGNAL(panL_button_clicked()));
  connect(_panRButton, SIGNAL(clicked()),
	  this, SIGNAL(panR_button_clicked()));
  connect(_chorusLButton, SIGNAL(clicked()),
	  this, SIGNAL(chorusL_button_clicked()));
  connect(_chorusRButton, SIGNAL(clicked()),
	  this, SIGNAL(chorusR_button_clicked()));
  connect(_midichLButton, SIGNAL(clicked()),
	  this, SIGNAL(midichL_button_clicked()));
  connect(_midichRButton, SIGNAL(clicked()),
	  this, SIGNAL(midichR_button_clicked()));
  connect(_levelLButton, SIGNAL(clicked()),
	  this, SIGNAL(levelL_button_clicked()));
  connect(_levelRButton, SIGNAL(clicked()),
	  this, SIGNAL(levelR_button_clicked()));
  connect(_reverbLButton, SIGNAL(clicked()),
	  this, SIGNAL(reverbL_button_clicked()));
  connect(_reverbRButton, SIGNAL(clicked()),
	  this, SIGNAL(reverbR_button_clicked()));
  connect(_keyshiftLButton, SIGNAL(clicked()),
	  this, SIGNAL(keyshiftL_button_clicked()));
  connect(_keyshiftRButton, SIGNAL(clicked()),
	  this, SIGNAL(keyshiftR_button_clicked()));

  connect(_powerButton, SIGNAL(clicked()), parent(), SLOT(power_switch()));
  connect(_volumeDial, SIGNAL(valueChanged(int)),
	  this, SIGNAL(volume_changed(int)));
}


void Scene::display_on(void)
{
  _lcdBackground->setBrush(_lcdOnBackgroundColor);

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

  // Update volume (TODO: Find a better place to do this)
  emit volume_changed(_volumeDial->value());
}


void Scene::display_off(void)
{
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
  _lcdBackground->setBrush(_lcdOffBackgroundColor);

  // And turn off buttons
  _allButton->setDown(false);
  _muteButton->setDown(false);
}


void Scene::update_lcd_bar_display(QVector<bool> *barDisplay)
{
  QVectorIterator<QGraphicsRectItem*> ivb(_volumeBars);

  for (int i = 0; i < barDisplay->size(); ++i) {
    if (barDisplay->at(i) && ivb.hasNext())
      ivb.next()->setBrush(_lcdOnActiveColor);
    else if (ivb.hasNext())
      ivb.next()->setBrush(_lcdOnInactiveColor);
  }
}


void Scene::update_all_button(bool status)
{
  _allButton->setChecked(status);
}

void Scene::update_mute_button(bool status)
{
  _muteButton->setChecked(status);
}


void Scene::update_lcd_part_text(QString text)
{
  _lcdPartText->setHtml(_generate_retro_text_html(text));
}


void Scene::update_lcd_instrument_text(QString text)
{
  _lcdInstrumentText->setHtml(_generate_retro_text_html(text));
}


void Scene::update_lcd_level_text(QString text)
{
  _lcdLevelText->setHtml(_generate_retro_text_html(text));
}


void Scene::update_lcd_pan_text(QString text)
{
  _lcdPanText->setHtml(_generate_retro_text_html(text));
}


void Scene::update_lcd_reverb_text(QString text)
{
  _lcdReverbText->setHtml(_generate_retro_text_html(text));
}


void Scene::update_lcd_chorus_text(QString text)
{
  _lcdChorusText->setHtml(_generate_retro_text_html(text));
}


void Scene::update_lcd_kshift_text(QString text)
{
  _lcdKshiftText->setHtml(_generate_retro_text_html(text));
}

void Scene::update_lcd_midich_text(QString text)
{
  _lcdMidichText->setHtml(_generate_retro_text_html(text));
}


void Scene::update_midi_activity_led(bool sysex, int length)
{
  _midiActLed->setBrush(*_ledOnGradient);
  _midiActTimer->start(300);
}


void Scene::update_midi_activity_timeout(void)
{
  _midiActLed->setBrush(*_ledOffGradient);
}

void Scene::set_lcd_bkg_on_color(QColor color)
{
  _lcdOnBackgroundColor = color;
  _lcdBackground->setBrush(color);
  _lcdBackground->update();
}


void Scene::set_lcd_active_on_color(QColor color)
{
  _lcdOnActiveColor = color;

  _lcdInstrumentText->setDefaultTextColor(color);
  _lcdPartText->setDefaultTextColor(color);
  _lcdLevelText->setDefaultTextColor(color);
  _lcdPanText->setDefaultTextColor(color);
  _lcdReverbText->setDefaultTextColor(color);
  _lcdChorusText->setDefaultTextColor(color);
  _lcdKshiftText->setDefaultTextColor(color);
  _lcdMidichText->setDefaultTextColor(color);

  _lcdLevelHeaderText->setDefaultTextColor(color);
  _lcdPanHeaderText->setDefaultTextColor(color);
  _lcdReverbHeaderText->setDefaultTextColor(color);
  _lcdChorusHeaderText->setDefaultTextColor(color);
  _lcdKshiftHeaderText->setDefaultTextColor(color);
  _lcdMidichHeaderText->setDefaultTextColor(color);

  QVectorIterator<QGraphicsEllipseItem*> ic(_volumeCircles);
  while (ic.hasNext())
    ic.next()->setBrush(QBrush(color));

  QVectorIterator<QGraphicsTextItem*> it(_partNumText);
  while (it.hasNext())
    it.next()->setDefaultTextColor(color);
}


void Scene::set_lcd_inactive_on_color(QColor color)
{
  _lcdOnInactiveColor = color;
}


QString Scene::_generate_sans_text_html(QString text, float size)
{
#ifdef Q_OS_MACOS
  if (size == 8)
    size = 10;
  else if (size == 10)
    size = 13.5;
  else if (size == 10.5)
    size = 13.5;
  else if (size == 12)
    size = 16;
#endif

  return QString(QString("<html><head><body style=\" white-space: pre-wrap; "
                         "font-family:Sans Serif; font-style:normal; "
                         "text-decoration:none;\">"
                         "<font style=\"font-size:")
		 + QString::number(size, 'f', 1)
		 + QString("pt; font-weight:normal;\">")
                 + text
                 + QString("</font>"));
}


QString Scene::_generate_retro_text_html(QString text)
{
#ifdef Q_OS_MACOS
  return QString(QString("<html><head><body style=\" white-space: pre-wrap; "
                         "letter-spacing: 4px; font-style:normal; "
                         "text-decoration:none;\">"
                         "<font style=\"font-size:35pt; font-weight:thin;\">")
                 + text
                 + QString("</font>"));
#endif

  return QString(QString("<html><head><body style=\" white-space: pre-wrap; "
                         "letter-spacing: 4px; font-style:normal; "
                         "text-decoration:none;\">"
                         "<font style=\"font-size:27pt; font-weight:normal;\">")
                 + text
                 + QString("</font>"));
}


void Scene::drawBackground(QPainter *painter, const QRectF &rect)
{
  // Black background
  painter->setBrush(_backgroundColor);
  painter->drawRect(rect);

  // Bakground texture over the synth rectangle area
  painter->setBrush(_backgroundBrush);
  painter->drawRect(-50, -50, 1200, 280);
}


void Scene::keyPressEvent(QKeyEvent *keyEvent)
{
  // Ignore repeating key events generated from keys being held down
  if (keyEvent->isAutoRepeat())
    return;

  // Ignore (send forward) key events not handled by Scene, but by MainWindow
  if (keyEvent->key() == Qt::Key_Escape || keyEvent->key() == Qt::Key_F11) {
    keyEvent->ignore();
    return;
  }

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

  // The remaining keys are only used of keyboard MIDI input is enabled
  if (!_midiKbdInput)
    return;

  if (keyEvent->key() == Qt::Key_Q) {
    if (_keyNoteOctave < 10)
      _keyNoteOctave++;
  } else if (keyEvent->key() == Qt::Key_A) {
    if (_keyNoteOctave > 0)
      _keyNoteOctave--;

  } else if (keyEvent->key() == Qt::Key_Z) {
    emit play_note(_keyNoteOctave * 12 + 0, 120);
  } else if (keyEvent->key() == Qt::Key_S) {
    emit play_note(_keyNoteOctave * 12 + 1, 120);
  } else if (keyEvent->key() == Qt::Key_X) {
    emit play_note(_keyNoteOctave * 12 + 2, 120);
  } else if (keyEvent->key() == Qt::Key_D) {
    emit play_note(_keyNoteOctave * 12 + 3, 120);
  } else if (keyEvent->key() == Qt::Key_C) {
    emit play_note(_keyNoteOctave * 12 + 4, 120);
  } else if (keyEvent->key() == Qt::Key_V) {
    emit play_note(_keyNoteOctave * 12 + 5, 120);
  } else if (keyEvent->key() == Qt::Key_G) {
    emit play_note(_keyNoteOctave * 12 + 6, 120);
  } else if (keyEvent->key() == Qt::Key_B) {
    emit play_note(_keyNoteOctave * 12 + 7, 120);
  } else if (keyEvent->key() == Qt::Key_H) {
    emit play_note(_keyNoteOctave * 12 + 8, 120);
  } else if (keyEvent->key() == Qt::Key_N) {
    emit play_note(_keyNoteOctave * 12 + 9, 120);
  } else if (keyEvent->key() == Qt::Key_J) {
    emit play_note(_keyNoteOctave * 12 + 10, 120);
  } else if (keyEvent->key() == Qt::Key_M) {
    emit play_note(_keyNoteOctave * 12 + 11, 120);
  }
}


void Scene::keyReleaseEvent(QKeyEvent *keyEvent)
{
  if (!_midiKbdInput)
    return;

  // Ignore repeating key events generated from keys being held down
  if (keyEvent->isAutoRepeat())
    return;

  if (keyEvent->key() == Qt::Key_Z) {
    emit play_note(_keyNoteOctave * 12 + 0, 0);
  } else if (keyEvent->key() == Qt::Key_S) {
    emit play_note(_keyNoteOctave * 12 + 1, 0);
  } else if (keyEvent->key() == Qt::Key_X) {
    emit play_note(_keyNoteOctave * 12 + 2, 0);
  } else if (keyEvent->key() == Qt::Key_D) {
    emit play_note(_keyNoteOctave * 12 + 3, 0);
  } else if (keyEvent->key() == Qt::Key_C) {
    emit play_note(_keyNoteOctave * 12 + 4, 0);
  } else if (keyEvent->key() == Qt::Key_V) {
    emit play_note(_keyNoteOctave * 12 + 5, 0);
  } else if (keyEvent->key() == Qt::Key_G) {
    emit play_note(_keyNoteOctave * 12 + 6, 0);
  } else if (keyEvent->key() == Qt::Key_B) {
    emit play_note(_keyNoteOctave * 12 + 7, 0);
  } else if (keyEvent->key() == Qt::Key_H) {
    emit play_note(_keyNoteOctave * 12 + 8, 0);
  } else if (keyEvent->key() == Qt::Key_N) {
    emit play_note(_keyNoteOctave * 12 + 9, 0);
  } else if (keyEvent->key() == Qt::Key_J) {
    emit play_note(_keyNoteOctave * 12 + 10, 0);
  } else if (keyEvent->key() == Qt::Key_M) {
    emit play_note(_keyNoteOctave * 12 + 11, 0);
  }
}


void Scene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  // LCD display coordinates
  if (event->scenePos().x() > 100 && event->scenePos().y() > 0 &&
      event->scenePos().x() < 595 && event->scenePos().y() < 175) {
    emit lcd_display_mouse_press_event(event->button(), event->scenePos());
  } else {
    QGraphicsScene::mousePressEvent(event);
  }
}


void Scene::_add_lcd_display_items(void)
{
  // Set LCD display background
  _lcdBackground = new QGraphicsRectItem(0, 0, 494, 175);
  _lcdBackground->setBrush(_lcdOffBackgroundColor);
  _lcdBackground->setPen(QColor(0, 0, 0, 0));
  _lcdBackground->setPos(QPointF(102, 0));
  addItem(_lcdBackground);

  // Set LCD static text items
  _lcdLevelHeaderText = new QGraphicsTextItem;
  _lcdLevelHeaderText->setHtml(_generate_sans_text_html("LEVEL", 8));
  _lcdLevelHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdLevelHeaderText->setPos(QPointF(110, 32));
  addItem(_lcdLevelHeaderText);

  _lcdPanHeaderText = new QGraphicsTextItem;
  _lcdPanHeaderText->setHtml(_generate_sans_text_html("PAN", 8));
  _lcdPanHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdPanHeaderText->setPos(QPointF(192, 32));
  addItem(_lcdPanHeaderText);

  _lcdReverbHeaderText = new QGraphicsTextItem;
  _lcdReverbHeaderText->setHtml(_generate_sans_text_html("REVERB", 8));
  _lcdReverbHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdReverbHeaderText->setPos(QPointF(110, 76));
  addItem(_lcdReverbHeaderText);

  _lcdChorusHeaderText = new QGraphicsTextItem;
  _lcdChorusHeaderText->setHtml(_generate_sans_text_html("CHORUS", 8));
  _lcdChorusHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdChorusHeaderText->setPos(QPointF(192, 76));
  addItem(_lcdChorusHeaderText);

  _lcdKshiftHeaderText = new QGraphicsTextItem;
  _lcdKshiftHeaderText->setHtml(_generate_sans_text_html("K SHIFT", 8));
  _lcdKshiftHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdKshiftHeaderText->setPos(QPointF(110, 119));
  addItem(_lcdKshiftHeaderText);

  _lcdMidichHeaderText = new QGraphicsTextItem;
  _lcdMidichHeaderText->setHtml(_generate_sans_text_html("MIDI CH", 8));
  _lcdMidichHeaderText->setDefaultTextColor(_lcdOffFontColor);
  _lcdMidichHeaderText->setPos(QPointF(192, 119));
  addItem(_lcdMidichHeaderText);

  // Set LCD dynamic text items
  int id = QFontDatabase::addApplicationFont(":/fonts/retro_synth.ttf");
  if (id < 0) {
    QMessageBox::critical((QWidget *) parent(),
			  tr("Font file not found"),
			  tr("The font file retro_synth.ttf was not found. "
			     "This font is required for running EmuSC."),
			  QMessageBox::Close);
    exit(-1); // Use exit() since the main loop has not been initiated yet
  }
  QString family = QFontDatabase::applicationFontFamilies(id).at(0);
  QFont retroSynth(family);

  _lcdPartText = new QGraphicsTextItem;
  _lcdPartText->setFont(retroSynth);
  _lcdPartText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdPartText->setPos(QPointF(110, 2));
  addItem(_lcdPartText);

  _lcdInstrumentText = new QGraphicsTextItem;
  _lcdInstrumentText->setFont(retroSynth);
  _lcdInstrumentText->setDefaultTextColor(_lcdOnActiveColor);
  _lcdInstrumentText->setPos(QPointF(192, 2));
  addItem(_lcdInstrumentText);

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

  // Setup LCD bar display
  for (int i = 0; i < 16; i ++) {
    QGraphicsTextItem *partNumber = new QGraphicsTextItem;
    partNumber->setHtml(_generate_sans_text_html(QString::number(i+1), 8));
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
}


SynthButton::SynthButton(QWidget *parent)
  : QPushButton(parent)
{
  setStyleSheet("color: #aaa;"			\
		"border: 2px solid #555; "	\
		"border-radius: 5px;"		\
		"border-style: outset;"		\
		"background: black;"		\
		"padding: 5px");
  setAttribute(Qt::WA_TranslucentBackground);
  setAutoRepeat(true);
  setAutoRepeatDelay(500);
  setAutoRepeatInterval(120);
  setIconSize(QSize(9, 9));
}


SynthButton::~SynthButton()
{}


void SynthButton::mousePressEvent(QMouseEvent *event)
{
  if(event->button() == Qt::RightButton)
    emit rightClicked();
  else
    QPushButton::mousePressEvent(event);
}


GrooveRect::GrooveRect(qreal x, qreal y, qreal w, qreal h,QGraphicsItem *parent)
  : QGraphicsRectItem(x, y, w, h, parent)
{}


void GrooveRect::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  QLinearGradient gradient(rect().x(), rect().y(), rect().x(), rect().y() + rect().height());
  gradient.setColorAt(0, QColor(0x00, 0x00, 0x00, 0x3f));
  gradient.setColorAt(1, QColor(0xff, 0xff, 0xff, 0x3f));
  QBrush brush(gradient);
  painter->setBrush(brush);
  painter->setPen(QColor(0, 0, 0, 0));

  painter->drawRoundedRect(rect(), 16, 16);
}


VolumeDial::VolumeDial(QWidget *parent)
{}

void VolumeDial::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Max / min dots
  painter.setBrush(QColor(0xbb, 0xbb, 0xbb));
  painter.setPen(QColor(0, 0, 0, 0));
  painter.drawEllipse(15, 70, 5, 5);
  painter.drawEllipse(55, 70, 5, 5);

  // Background
  painter.setBrush(QColor(0x2b, 0x2b, 0x2b));
  painter.drawEllipse(5, 5, 65, 65);

  // Knob
  QPointF center(width() / 2., height() / 2.);
  painter.translate(center);
  painter.rotate(-150 + 3 * value());
  painter.setBrush(QColor(0, 0, 0));
  painter.drawEllipse(-25, -25, 50, 50);
  painter.setBrush(QColor(0xa9, 0xa9, 0x70));
  painter.drawRect(-1, -25, 5, 12);
}
