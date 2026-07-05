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


#include <QApplication>
#include <QFontDatabase>
#include <QGraphicsItem>
#include <QGraphicsWidget>
#include <QGraphicsPolygonItem>
#include <QGraphicsProxyWidget>
#include <QHBoxLayout>
#include <QMessageBox>
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
    _keyNoteOctave(4)
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

  _midiKbdInput = settings.value("kbd_midi_input").toBool();

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
  addItem(new GrooveRect(760, -10, 313, 35));
  addItem(new GrooveRect(760,  45, 313, 35));
  addItem(new GrooveRect(760, 100, 313, 35));
  addItem(new GrooveRect(760, 155, 313, 35));
  addItem(new GrooveRect(-15,   8, 100, 35));

  _add_lcd_display_items();

  // Set black vertical bar background
  QGraphicsRectItem *blackBackground = new QGraphicsRectItem(0, 0, 113, 280);
  blackBackground->setBrush(QColor(0, 0, 0));
  blackBackground->setPos(QPointF(636, -50));
  blackBackground->setPen(Qt::NoPen);

  // Add some transparent transitions to the vertical bar
  QLinearGradient gradient(0, 0, 113, 0);
  gradient.setColorAt(0.0, Qt::transparent);
  gradient.setColorAt(0.03, Qt::black);
  gradient.setColorAt(0.97, Qt::black);
  gradient.setColorAt(1.0, Qt::transparent);
  blackBackground->setBrush(QBrush(gradient));

  addItem(blackBackground);

//    QGraphicsTextItem *logoText = new QGraphicsTextItem;
//    logoText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:18pt; font-weight:bold; color: #bbbbbb\">EmuSC</font>");
//    logoText->setPos(QPointF(500, -35));
//    addItem(logoText);

  // Note: This stays hidden and is turned on if mk2 ROM is selected on power on
  _sc55mk2Text = new QGraphicsSvgItem(":/text/sc-55mkII.svg");
  _sc55mk2Text->setScale(1.85);
  _sc55mk2Text->setPos(440, 192);
  _sc55mk2Text->setOpacity(0.85);
  _sc55mk2Text->setVisible(false);
  addItem(_sc55mk2Text);

  QGraphicsSvgItem *partTopText = new QGraphicsSvgItem(":/text/part.svg");
  partTopText->setScale(1.02);
  partTopText->setPos(110, -21);
  partTopText->setOpacity(0.85);
  addItem(partTopText);

  QGraphicsSvgItem *partBottomText = new QGraphicsSvgItem(":/text/part.svg");
  partBottomText->setScale(1.02);
  partBottomText->setPos(392, 187);
  partBottomText->setOpacity(0.85);
  addItem(partBottomText);

  QGraphicsSvgItem *instrumentTopText = new QGraphicsSvgItem(":/text/instrument.svg");
  instrumentTopText->setScale(1.02);
  instrumentTopText->setPos(200, -21);
  instrumentTopText->setOpacity(0.85);
  addItem(instrumentTopText);

  QGraphicsSvgItem *powerText = new QGraphicsSvgItem(":/text/power.svg");
  powerText->setScale(1.38);
  powerText->setPos(0, -13);
  powerText->setOpacity(0.85);
  addItem(powerText);

  QGraphicsSvgItem *volumeText = new QGraphicsSvgItem(":/text/volume.svg");
  volumeText->setScale(1.38);
  volumeText->setPos(-5, 65);
  volumeText->setOpacity(0.85);
  addItem(volumeText);

  _powerButton = new QPushButton();
  _powerButton->setGeometry(QRect(-5, 13, 78, 25));
  _powerButton->setAttribute(Qt::WA_TranslucentBackground);
  _powerButton->setObjectName("PowerButton");
  QGraphicsProxyWidget *pwrProxy = addWidget(_powerButton);

  _volumeDial = new VolumeDial();
  _volumeDial->setGeometry(QRect(-5, 85, 80, 84));
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
  _allButton->setGeometry(QRect(700, -7, 30, 30));
  _allButton->setAttribute(Qt::WA_TranslucentBackground);
  _allButton->setObjectName("AllButton");

  QGraphicsProxyWidget *allBtnProxy = addWidget(_allButton);

  _muteButton = new QPushButton();
  _muteButton->setCheckable(true);
  _muteButton->setGeometry(QRect(700, 43, 30, 30));
  _muteButton->setAttribute(Qt::WA_TranslucentBackground);
  _muteButton->setObjectName("MuteButton");

  QGraphicsProxyWidget *muteBtnProxy = addWidget(_muteButton);

  // Drop down shadows behind synth buttons
  RoundedRectItem * partLBtnShadow = new RoundedRectItem(797, -4.5, 24, 24, 12);
  RoundedRectItem * partRBtnShadow = new RoundedRectItem(857, -4.5, 24, 24, 12);
  RoundedRectItem * instBtnShadow = new RoundedRectItem(932, -4.5, 124, 24, 2);
  RoundedRectItem * levelBtnShadow = new RoundedRectItem(777, 50.5, 124, 24, 2);
  RoundedRectItem * panBtnShadow = new RoundedRectItem(932, 50.5, 124, 24, 2);
  RoundedRectItem * reverbBtnShadow = new RoundedRectItem(777, 105.5, 124, 24, 2);
  RoundedRectItem * chorusBtnShadow = new RoundedRectItem(932, 105.5, 124, 24, 2);
  RoundedRectItem * kShiftBtnShadow = new RoundedRectItem(777, 160.5, 124, 24, 2);
  RoundedRectItem * midiChBtnShadow = new RoundedRectItem(932, 160.5, 124, 24, 2);
  addItem(partLBtnShadow);
  addItem(partRBtnShadow);
  addItem(instBtnShadow);
  addItem(levelBtnShadow);
  addItem(panBtnShadow);
  addItem(reverbBtnShadow);
  addItem(chorusBtnShadow);
  addItem(kShiftBtnShadow);
  addItem(midiChBtnShadow);

  // Add part L/R round buttons
  _partLButton = new SynthButton(SynthButton::Left, SynthButton::Round);
  _partLButton->setGeometry(QRect(799, -3, 20, 20));
  _partLButton->setAttribute(Qt::WA_TranslucentBackground);
  _partLButton->setObjectName("Round");
  _partLButton->setAutoRepeat(true);
  _partLButton->setAutoRepeatDelay(500);
  _partLButton->setAutoRepeatInterval(120);
  QGraphicsProxyWidget *partLBtnProxy = addWidget(_partLButton);

  _partRButton = new SynthButton(SynthButton::Right, SynthButton::Round);
  _partRButton->setGeometry(QRect(859, -3, 20, 20));
  _partRButton->setAttribute(Qt::WA_TranslucentBackground);
  _partRButton->setObjectName("Round");
  _partRButton->setAutoRepeat(true);
  _partRButton->setAutoRepeatDelay(500);
  _partRButton->setAutoRepeatInterval(120);
  QGraphicsProxyWidget *partRBtnProxy = addWidget(_partRButton);

  // Add instrument L/R buttons
  _instrumentLButton = new SynthButton(SynthButton::Left);
  _instrumentLButton->setGeometry(QRect(935, -2, 58, 19));
  QGraphicsProxyWidget *instLBtnProxy = addWidget(_instrumentLButton);

  _instrumentRButton = new SynthButton(SynthButton::Right);
  _instrumentRButton->setGeometry(QRect(995, -2, 58, 19));
  QGraphicsProxyWidget *instRBtnProxy = addWidget(_instrumentRButton);

  // Add level L/R buttons
  _levelLButton = new SynthButton(SynthButton::Left);
  _levelLButton->setGeometry(QRect(780, 53, 58, 19));
  QGraphicsProxyWidget *levelLBtnProxy = addWidget(_levelLButton);

  _levelRButton = new SynthButton(SynthButton::Right);
  _levelRButton->setGeometry(QRect(840, 53, 58, 19));
  QGraphicsProxyWidget *levelRBtnProxy = addWidget(_levelRButton);

  // Add pan L/R buttons
  _panLButton = new SynthButton(SynthButton::Left);
  _panLButton->setGeometry(QRect(935, 53, 58, 19));
  QGraphicsProxyWidget *panLBtnProxy = addWidget(_panLButton);

  _panRButton = new SynthButton(SynthButton::Right);
  _panRButton->setGeometry(QRect(995, 53, 58, 19));
  QGraphicsProxyWidget *panRBtnProxy = addWidget(_panRButton);

  // Add reverb L/R buttons
  _reverbLButton = new SynthButton(SynthButton::Left);
  _reverbLButton->setGeometry(QRect(780, 108, 58, 19));
  QGraphicsProxyWidget *reverbLBtnProxy = addWidget(_reverbLButton);

  _reverbRButton = new SynthButton(SynthButton::Right);
  _reverbRButton->setGeometry(QRect(840, 108, 58, 19));
  QGraphicsProxyWidget *reverbRBtnProxy = addWidget(_reverbRButton);

  // Add chorus L/R buttons
  _chorusLButton = new SynthButton(SynthButton::Left);
  _chorusLButton->setGeometry(QRect(935, 108, 58, 19));
  QGraphicsProxyWidget *chorusLBtnProxy = addWidget(_chorusLButton);

  _chorusRButton = new SynthButton(SynthButton::Right);
  _chorusRButton->setGeometry(QRect(995, 108, 58, 19));
  QGraphicsProxyWidget *chorusRBtnProxy = addWidget(_chorusRButton);

  // Add keyshift L/R buttons
  _keyshiftLButton = new SynthButton(SynthButton::Left);
  _keyshiftLButton->setGeometry(QRect(780, 163, 58, 19));
  QGraphicsProxyWidget *keyshiftLBtnProxy = addWidget(_keyshiftLButton);

  _keyshiftRButton = new SynthButton(SynthButton::Right);
  _keyshiftRButton->setGeometry(QRect(840, 163, 58, 19));
  QGraphicsProxyWidget *keyshiftRBtnProxy = addWidget(_keyshiftRButton);

  // Add midich L/R buttons
  _midichLButton = new SynthButton(SynthButton::Left);
  _midichLButton->setGeometry(QRect(935, 163, 58, 19));
  QGraphicsProxyWidget *midichLBtnProxy = addWidget(_midichLButton);

  _midichRButton = new SynthButton(SynthButton::Right);
  _midichRButton->setGeometry(QRect(995, 163, 58, 19));
  QGraphicsProxyWidget *midichRBtnProxy = addWidget(_midichRButton);

  QGraphicsSvgItem *allBtnText = new QGraphicsSvgItem(":/text/all.svg");
  allBtnText->setScale(1.22);
  allBtnText->setPos(661, 3);
  allBtnText->setOpacity(0.66);
  addItem(allBtnText);

  QGraphicsSvgItem *muteBtnText = new QGraphicsSvgItem(":/text/mute.svg");
  muteBtnText->setScale(1.22);
  muteBtnText->setPos(647, 53);
  muteBtnText->setOpacity(0.66);
  addItem(muteBtnText);

  QGraphicsSvgItem *partBtnText = new QGraphicsSvgItem(":/text/part.svg");
  partBtnText->setScale(1.22);
  partBtnText->setPos(820, -25);
  partBtnText->setOpacity(0.66);
  addItem(partBtnText);

  QGraphicsSvgItem *instrumentBtnText = new QGraphicsSvgItem(":/text/instrument.svg");
  instrumentBtnText->setScale(1.22);
  instrumentBtnText->setPos(942, -25);
  instrumentBtnText->setOpacity(0.66);
  addItem(instrumentBtnText);

  QGraphicsSvgItem *levelBtnText = new QGraphicsSvgItem(":/text/level.svg");
  levelBtnText->setScale(1.22);
  levelBtnText->setPos(815, 30);
  levelBtnText->setOpacity(0.66);
  addItem(levelBtnText);

  QGraphicsSvgItem *panBtnText = new QGraphicsSvgItem(":/text/pan.svg");
  panBtnText->setScale(1.22);
  panBtnText->setPos(980, 30);
  panBtnText->setOpacity(0.66);
  addItem(panBtnText);

  QGraphicsSvgItem *reverbBtnText = new QGraphicsSvgItem(":/text/reverb.svg");
  reverbBtnText->setScale(1.22);
  reverbBtnText->setPos(810, 85);
  reverbBtnText->setOpacity(0.66);
  addItem(reverbBtnText);

  QGraphicsSvgItem *chorusBtnText = new QGraphicsSvgItem(":/text/chorus.svg");
  chorusBtnText->setScale(1.22);
  chorusBtnText->setPos(959, 85);
  chorusBtnText->setOpacity(0.66);
  addItem(chorusBtnText);

  QGraphicsSvgItem *keyShiftBtnText = new QGraphicsSvgItem(":/text/key_shift.svg");
  keyShiftBtnText->setScale(1.22);
  keyShiftBtnText->setPos(795, 140);
  keyShiftBtnText->setOpacity(0.66);
  addItem(keyShiftBtnText);

  QGraphicsSvgItem *midiChBtnText = new QGraphicsSvgItem(":/text/midi_ch.svg");
  midiChBtnText->setScale(1.22);
  midiChBtnText->setPos(960, 140);
  midiChBtnText->setOpacity(0.66);
  addItem(midiChBtnText);

  // TODO: Show GM / GS logo based on ROM version
  QGraphicsSvgItem *gmLogo = new QGraphicsSvgItem(":/images/gm_logo.svg");
  gmLogo->setScale(0.56);
  gmLogo->setPos(645, 170);
  addItem(gmLogo);

  QGraphicsSvgItem *gsLogo = new QGraphicsSvgItem(":/images/gs_logo.svg");
  gsLogo->setScale(0.024);
  gsLogo->setPos(695, 177);
  addItem(gsLogo);

  // Add LED button
  _ledOnGradient = new QRadialGradient(35, 190, 20);
  _ledOnGradient->setColorAt(.0, 0x2fafff);
  _ledOnGradient->setColorAt(.6, 0x0000ff);

  _ledOffGradient = new QRadialGradient(35, 190, 20);
  _ledOffGradient->setColorAt(.0, 0x000090);
  _ledOffGradient->setColorAt(.9, Qt::black);

  _midiActLed = new QGraphicsRectItem(25, 190, 20, 8);
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
  connect(_allButton, SIGNAL(clicked(bool)), this, SIGNAL(all_button_clicked(bool)));
  connect(_muteButton, SIGNAL(toggled(bool)), this, SIGNAL(mute_button_clicked(bool)));

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

  // Turn on backround of all non-static LCD text
  _lcdPartTextBkg->show();
  _lcdInstrumentTextBkg->show();
  _lcdLevelTextBkg->show();
  _lcdPanTextBkg->show();
  _lcdReverbTextBkg->show();
  _lcdChorusTextBkg->show();
  _lcdKshiftTextBkg->show();
  _lcdMidichTextBkg->show();

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

  // Turn off backround of all non-static LCD text
  _lcdPartTextBkg->hide();
  _lcdInstrumentTextBkg->hide();
  _lcdLevelTextBkg->hide();
  _lcdPanTextBkg->hide();
  _lcdReverbTextBkg->hide();
  _lcdChorusTextBkg->hide();
  _lcdKshiftTextBkg->hide();
  _lcdMidichTextBkg->hide();

  // Turn off all non-static LCD text
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

  // And turn off check buttons
  _allButton->setChecked(false);
  _muteButton->setChecked(false);
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

void Scene::set_lcd_bkg_on_color(QColor color, bool update)
{
  _lcdOnBackgroundColor = color;
  if (!update)
    return;

  _lcdBackground->setBrush(color);
  _lcdBackground->update();
}


void Scene::set_lcd_active_on_color(QColor color, bool update)
{
  _lcdOnActiveColor = color;
  if (!update)
    return;

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

  _lcdInstrumentTextBkg->setDefaultTextColor(color);
  _lcdPartTextBkg->setDefaultTextColor(color);
  _lcdLevelTextBkg->setDefaultTextColor(color);
  _lcdPanTextBkg->setDefaultTextColor(color);
  _lcdReverbTextBkg->setDefaultTextColor(color);
  _lcdChorusTextBkg->setDefaultTextColor(color);
  _lcdKshiftTextBkg->setDefaultTextColor(color);
  _lcdMidichTextBkg->setDefaultTextColor(color);
}


// TODO: Rename to set_model_layout() and add proper logo placements as well?
void Scene::set_model_name(QString name, QString version)
{
  if (name == "SC-55mkII")
    _sc55mk2Text->show();
  else
    _sc55mk2Text->hide();
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

  _init_lcd_text(&_lcdPartTextBkg, retroSynth, _lcdOnInactiveColor,
                 QPointF(110, 2), "‑‑‑");
  _init_lcd_text(&_lcdInstrumentTextBkg, retroSynth, _lcdOnInactiveColor,
                 QPointF(192, 2), "‑‑‑‑‑‑‑‑‑‑‑‑‑‑‑‑");
  _init_lcd_text(&_lcdLevelTextBkg, retroSynth, _lcdOnInactiveColor,
                 QPointF(110, 46), "‑‑‑");
  _init_lcd_text(&_lcdPanTextBkg, retroSynth, _lcdOnInactiveColor,
                 QPointF(192, 46), "‑‑‑");
  _init_lcd_text(&_lcdReverbTextBkg, retroSynth, _lcdOnInactiveColor,
                 QPointF(110, 90), "‑‑‑");
  _init_lcd_text(&_lcdChorusTextBkg, retroSynth, _lcdOnInactiveColor,
                 QPointF(192, 90), "‑‑‑");
  _init_lcd_text(&_lcdKshiftTextBkg, retroSynth, _lcdOnInactiveColor,
                 QPointF(110, 132), "‑‑‑");
  _init_lcd_text(&_lcdMidichTextBkg, retroSynth, _lcdOnInactiveColor,
                 QPointF(192, 132), "‑‑‑");

  // Background text is hidden until LCD is turned on
  _lcdPartTextBkg->hide();
  _lcdInstrumentTextBkg->hide();
  _lcdLevelTextBkg->hide();
  _lcdPanTextBkg->hide();
  _lcdReverbTextBkg->hide();
  _lcdChorusTextBkg->hide();
  _lcdKshiftTextBkg->hide();
  _lcdMidichTextBkg->hide();

  _init_lcd_text(&_lcdPartText, retroSynth, _lcdOnActiveColor,
                 QPointF(110, 2));
  _init_lcd_text(&_lcdInstrumentText, retroSynth, _lcdOnActiveColor,
                 QPointF(192, 2));
  _init_lcd_text(&_lcdLevelText, retroSynth, _lcdOnActiveColor,
                 QPointF(110, 46));
  _init_lcd_text(&_lcdPanText, retroSynth, _lcdOnActiveColor,
                 QPointF(192, 46));
  _init_lcd_text(&_lcdReverbText, retroSynth, _lcdOnActiveColor,
                 QPointF(110, 90));
  _init_lcd_text(&_lcdChorusText, retroSynth, _lcdOnActiveColor,
                 QPointF(192, 90));
  _init_lcd_text(&_lcdKshiftText, retroSynth, _lcdOnActiveColor,
                 QPointF(110, 132));
  _init_lcd_text(&_lcdMidichText, retroSynth, _lcdOnActiveColor,
                 QPointF(192, 132));

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


void Scene::_init_lcd_text(QGraphicsTextItem **item, QFont &font, QColor &color,
                           QPointF pos, QString text)
{
  (*item) = new QGraphicsTextItem;
  (*item)->setFont(font);
  (*item)->setDefaultTextColor(color);
  (*item)->setPos(pos);

  if (!text.isEmpty())
    (*item)->setHtml(_generate_retro_text_html(text));

  addItem(*item);
}


SynthButton::SynthButton(enum Direction direction, enum Shape shape,
                         QWidget *parent)
  : QPushButton(parent),
    _direction(direction),
    _shape(shape),
    _hOffset(2)
{
  setAttribute(Qt::WA_TranslucentBackground);
  setAutoRepeat(true);
  setAutoRepeatDelay(500);
  setAutoRepeatInterval(120);
}


void SynthButton::mousePressEvent(QMouseEvent *event)
{
  if(event->button() == Qt::RightButton)
    emit rightClicked();
  else
    QPushButton::mousePressEvent(event);
}


void SynthButton::paintEvent(QPaintEvent* e)
{
  QPushButton::paintEvent(e);            // Draw the normal button first

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(0x81, 0x81, 0x81));

  const QRectF r = rect();
  const qreal cy = r.center().y();
  qreal cx = r.center().x();
  cx += (_direction == Right) ? _hOffset : -_hOffset;

  // Triangle size
  const qreal h = r.height() * 0.45;
  const qreal w = h * 1.1;

  QPointF apex, top, bot;
  if (_direction == Right) {
    apex = { cx + w/2, cy };
    top  = { cx - w/2, cy - h/2 };
    bot  = { cx - w/2, cy + h/2 };
  } else {
    apex = { cx - w/2, cy };
    top  = { cx + w/2, cy - h/2 };
    bot  = { cx + w/2, cy + h/2 };
  }
  QPolygonF tri;
  tri << apex << top << bot;
  p.drawPolygon(tri);
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
{
  _knob = new QSvgRenderer{QStringLiteral(":/images/volume_knob.svg")};
}


void VolumeDial::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Max / min dots
  painter.setBrush(QColor(0xbb, 0xbb, 0xbb));
  painter.setPen(QColor(0, 0, 0, 0));
  painter.drawEllipse(15, 79, 5, 5);
  painter.drawEllipse(60, 79, 5, 5);

  // Black outer background circle
  painter.setBrush(QColor(0, 0, 0));
  painter.drawEllipse(0, 0, 80, 80);

  // Grey middle circle
  painter.setBrush(QColor(0x3b, 0x3b, 0x3b));
  painter.drawEllipse(2, 2, 76, 76);

  // Black inner circle
  painter.setBrush(QColor(0, 0, 0));
  painter.drawEllipse(5, 5, 70, 70);

  const QPointF center(width() / 2.0, height() / 2.0 - 2);
  const qreal knobSize   = 65.0;
  const qreal knobRadius = knobSize * 0.454;
  const QRectF target(-knobSize / 2.0, -knobSize / 2.0, knobSize, knobSize);

  // Rotating knob
  painter.save();
  painter.translate(center);
  painter.rotate(-150 + 3 * value());
  _knob->render(&painter, target);
  painter.restore();

  // Light effect on rotating knob
  painter.save();
  QPainterPath clip;
  clip.addEllipse(center, knobRadius, knobRadius);
  painter.setClipPath(clip);

  // Bright at top-left, fading out. Plain source-over so it actually lightens
  // the black body
  QRadialGradient hi(center.x() - knobRadius * 0.45,
                     center.y() - knobRadius * 0.45,
                     knobRadius * 1.5);
  hi.setColorAt(0.0, QColor(255, 255, 255, 90));
  hi.setColorAt(0.6, QColor(255, 255, 255, 0));
  painter.fillPath(clip, hi);

  // Shadow toward bottom-right
  QRadialGradient sh(center.x() + knobRadius * 0.5,
                     center.y() + knobRadius * 0.5,
                     knobRadius * 1.5);
  sh.setColorAt(0.0, QColor(0, 0, 0, 90));
  sh.setColorAt(0.7, QColor(0, 0, 0, 0));
  painter.fillPath(clip, sh);

  painter.restore();
}
