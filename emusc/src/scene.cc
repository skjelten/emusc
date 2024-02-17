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


Scene::Scene(Emulator *emulator, QWidget *parent)
  : _emulator(emulator),
    _lcdOnBackgroundColorReset(225, 145, 15),
    _lcdOnActiveColorReset(94, 37, 28),
    _lcdOnInactiveColorReset(215, 135, 10),
    _lcdOffBackgroundColor(140, 160, 140),
    _lcdOffFontColor(80, 80, 80),
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

  // Connect all signals for the emulator
  connect(_emulator, SIGNAL(emulator_started()), this, SLOT(display_on()));
  connect(_emulator, SIGNAL(emulator_stopped()), this, SLOT(display_off()));
  connect(_emulator, SIGNAL(all_button_changed(bool)),
	  this, SLOT(update_all_button(bool)));
  connect(_emulator, SIGNAL(mute_button_changed(bool)),
	  this, SLOT(update_mute_button(bool)));

  // Set background color to grey (use bitmap for better look?)
  setBackgroundBrush(QBrush(QColor(60, 60, 60),QPixmap(":/images/synth_bkg.png")));

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

  // Set LCD display background
  _lcdBackground = new QGraphicsRectItem(0, 0, 494, 175);
  _lcdBackground->setBrush(_lcdOffBackgroundColor);
  _lcdBackground->setPen(QColor(0, 0, 0, 0));
  _lcdBackground->setPos(QPointF(102, 0));
  addItem(_lcdBackground);

  // Set black vertical bar background
  QGraphicsRectItem *blackBackground = new QGraphicsRectItem(0, 0, 110, 300);
  blackBackground->setBrush(QColor(0, 0, 0));
  blackBackground->setPen(QColor(0, 0, 0, 0));
  blackBackground->setPos(QPointF(637, -50));
  addItem(blackBackground);

//    QGraphicsTextItem *logoText = new QGraphicsTextItem;
//    logoText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:18pt; font-weight:bold; color: #bbbbbb\">EmuSC</font>");
//    logoText->setPos(QPointF(500, -35));
//    addItem(logoText);

  QGraphicsTextItem *partHeaderText = new QGraphicsTextItem;
  partHeaderText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10pt; font-weight:normal; color: #bbbbbb\">PART</font>");
  partHeaderText->setPos(QPointF(110, -30));
  addItem(partHeaderText);

  QGraphicsTextItem *instrumentHeaderText = new QGraphicsTextItem;
  instrumentHeaderText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10pt; font-weight:normal; color: #bbbbbb\">INSTRUMENT</font>");
  instrumentHeaderText->setPos(QPointF(192, -30));
  addItem(instrumentHeaderText);

  QGraphicsTextItem *partBottomText = new QGraphicsTextItem;
  partBottomText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10pt; font-weight:normal; color: #bbbbbb\">PART</font>");
  partBottomText->setPos(QPointF(385, 180));
  addItem(partBottomText);

  QGraphicsTextItem *powerText = new QGraphicsTextItem;
  powerText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:12pt; font-weight:normal; color: #bbbbbb\">POWER</font>");
  powerText->setPos(QPointF(0, -20));
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
  _powerButton->setGeometry(QRect(-5, 13, 78, 25));
  _powerButton->setStyleSheet("background-color: #111111; \
                             border-style: outset;	    \
                             border-width: 2px;		    \
                             border-radius: 5px;	    \
                             border-color: #333333;");
  _powerButton->setAttribute(Qt::WA_TranslucentBackground);
  connect(_powerButton, SIGNAL(clicked()), parent, SLOT(power_switch()));
  QGraphicsProxyWidget *pwrProxy = addWidget(_powerButton);

  _volumeDial = new VolumeDial();
  _volumeDial->setGeometry(QRect(-2, 73, 75, 75));
  _volumeDial->setStyleSheet("background-color: #00000000;");
  _volumeDial->setRange(0,100);

  // Default value
  _volumeDial->setValue(settings.value("Audio/volume", 80).toInt());

  connect(_volumeDial, SIGNAL(valueChanged(int)), emulator, SLOT(change_volume(int)));

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
  connect(_allButton, SIGNAL(clicked()), emulator, SLOT(select_all()));
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
  connect(_muteButton, SIGNAL(clicked()), emulator, SLOT(select_mute()));
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

  connect(_partLButton, SIGNAL(clicked()), emulator, SLOT(select_prev_part()));
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

  connect(_partRButton, SIGNAL(clicked()), emulator, SLOT(select_next_part()));
  QGraphicsProxyWidget *partRBtnProxy = addWidget(_partRButton);

  // Add instrument L/R buttons
  _instrumentLButton = new SynthButton();
  _instrumentLButton->setGeometry(QRect(945, -5, 70, 25));
  _instrumentLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  connect(_instrumentLButton, SIGNAL(clicked()),
	  emulator, SLOT(select_prev_instrument()));
  connect(_instrumentLButton, SIGNAL(rightClicked()),
	  emulator, SLOT(select_prev_instrument_variant()));
  QGraphicsProxyWidget *instLBtnProxy = addWidget(_instrumentLButton);

  _instrumentRButton = new SynthButton();
  _instrumentRButton->setGeometry(QRect(1018, -5, 70, 25));
  _instrumentRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  connect(_instrumentRButton, SIGNAL(clicked()),
	  emulator, SLOT(select_next_instrument()));
  connect(_instrumentRButton, SIGNAL(rightClicked()),
	  emulator, SLOT(select_next_instrument_variant()));
  QGraphicsProxyWidget *instRBtnProxy = addWidget(_instrumentRButton);

  // Add pan L/R buttons
  _panLButton = new SynthButton();
  _panLButton->setGeometry(QRect(945, 50, 70, 25));
  _panLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  connect(_panLButton, SIGNAL(clicked()), emulator, SLOT(select_prev_pan()));
  QGraphicsProxyWidget *panLBtnProxy = addWidget(_panLButton);

  _panRButton = new SynthButton();
  _panRButton->setGeometry(QRect(1018, 50, 70, 25));
  _panRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  connect(_panRButton, SIGNAL(clicked()), emulator, SLOT(select_next_pan()));
  QGraphicsProxyWidget *panRBtnProxy = addWidget(_panRButton);

  // Add chorus L/R buttons
  _chorusLButton = new SynthButton();
  _chorusLButton->setGeometry(QRect(945, 105, 70, 25));
  _chorusLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  connect(_chorusLButton, SIGNAL(clicked()), emulator, SLOT(select_prev_chorus()));
  QGraphicsProxyWidget *chorusLBtnProxy = addWidget(_chorusLButton);

  _chorusRButton = new SynthButton();
  _chorusRButton->setGeometry(QRect(1018, 105, 70, 25));
  _chorusRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  connect(_chorusRButton, SIGNAL(clicked()), emulator, SLOT(select_next_chorus()));
  QGraphicsProxyWidget *chorusRBtnProxy = addWidget(_chorusRButton);

  // Add midich L/R buttons
  _midichLButton = new SynthButton();
  _midichLButton->setGeometry(QRect(945, 160, 70, 25));
  _midichLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  connect(_midichLButton, SIGNAL(clicked()), emulator, SLOT(select_prev_midi_channel()));
  QGraphicsProxyWidget *midichLBtnProxy = addWidget(_midichLButton);

  _midichRButton = new SynthButton();
  _midichRButton->setGeometry(QRect(1018, 160, 70, 25));
  _midichRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  connect(_midichRButton, SIGNAL(clicked()), emulator, SLOT(select_next_midi_channel()));
  QGraphicsProxyWidget *midichRBtnProxy = addWidget(_midichRButton);

  // Add level L/R buttons
  _levelLButton = new SynthButton();
  _levelLButton->setGeometry(QRect(780, 50, 70, 25));
  _levelLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  connect(_levelLButton, SIGNAL(clicked()), emulator,SLOT(select_prev_level()));
  QGraphicsProxyWidget *levelLBtnProxy = addWidget(_levelLButton);

  _levelRButton = new SynthButton();
  _levelRButton->setGeometry(QRect(853, 50, 70, 25));
  _levelRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  connect(_levelRButton, SIGNAL(clicked()), emulator,SLOT(select_next_level()));
  QGraphicsProxyWidget *levelRBtnProxy = addWidget(_levelRButton);

  // Add reverb L/R buttons
  _reverbLButton = new SynthButton();
  _reverbLButton->setGeometry(QRect(780, 105, 70, 25));
  _reverbLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  connect(_reverbLButton, SIGNAL(clicked()), emulator, SLOT(select_prev_reverb()));
  QGraphicsProxyWidget *reverbLBtnProxy = addWidget(_reverbLButton);

  _reverbRButton = new SynthButton();
  _reverbRButton->setGeometry(QRect(853, 105, 70, 25));
  _reverbRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  connect(_reverbRButton, SIGNAL(clicked()), emulator, SLOT(select_next_reverb()));
  QGraphicsProxyWidget *reverbRBtnProxy = addWidget(_reverbRButton);

  // Add keyshift L/R buttons
  _keyshiftLButton = new SynthButton();
  _keyshiftLButton->setGeometry(QRect(780, 160, 70, 25));
  _keyshiftLButton->setIcon(QPixmap(":/images/left_arrow.png"));
  connect(_keyshiftLButton, SIGNAL(clicked()), emulator, SLOT(select_prev_key_shift()));
  QGraphicsProxyWidget *keyshiftLBtnProxy = addWidget(_keyshiftLButton);

  _keyshiftRButton = new SynthButton();
  _keyshiftRButton->setGeometry(QRect(853, 160, 70, 25));
  _keyshiftRButton->setIcon(QPixmap(":/images/right_arrow.png"));
  connect(_keyshiftRButton, SIGNAL(clicked()), emulator, SLOT(select_next_key_shift()));
  QGraphicsProxyWidget *keyshiftRBtnProxy = addWidget(_keyshiftRButton);

  QGraphicsTextItem *allBtnText = new QGraphicsTextItem;
  allBtnText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10.5pt; font-weight: normal; color: #bbbbbb\">ALL</font>");
  allBtnText->setPos(QPointF(690 - allBtnText->boundingRect().right(), 3));
  addItem(allBtnText);

  QGraphicsTextItem *muteBtnText = new QGraphicsTextItem;
  muteBtnText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10.5pt; font-weight: normal; color: #bbbbbb\">MUTE</font>");
  muteBtnText->setPos(QPointF(690 - muteBtnText->boundingRect().right(), 56));
  addItem(muteBtnText);


  QGraphicsTextItem *partBtnText = new QGraphicsTextItem;
  partBtnText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10.5pt; font-weight: normal; color: #bbbbbb\">PART</font>");
  partBtnText->setPos(QPointF(855 - partBtnText->boundingRect().center().x(), -33));
  addItem(partBtnText);

  QGraphicsTextItem *instrumentBtnText = new QGraphicsTextItem;
  instrumentBtnText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10.5pt; font-weight: normal; color: #bbbbbb\">INSTRUMENT</font>");
  instrumentBtnText->setPos(QPointF(1015 - instrumentBtnText->boundingRect().center().x(), -33));
  addItem(instrumentBtnText);

  QGraphicsTextItem *levelBtnText = new QGraphicsTextItem;
  levelBtnText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10.5pt; font-weight: normal; color: #bbbbbb\">LEVEL</font>");
  levelBtnText->setPos(QPointF(855 - levelBtnText->boundingRect().center().x(), 22));
  addItem(levelBtnText);

  QGraphicsTextItem *panBtnText = new QGraphicsTextItem;
  panBtnText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10.5pt; font-weight: normal; color: #bbbbbb\">PAN</font>");
  panBtnText->setPos(QPointF(1015 - panBtnText->boundingRect().center().x(), 22));
  addItem(panBtnText);

  QGraphicsTextItem *reverbBtnText = new QGraphicsTextItem;
  reverbBtnText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10.5pt; font-weight: normal; color: #bbbbbb\">REVERB</font>");
  reverbBtnText->setPos(QPointF(855 - reverbBtnText->boundingRect().center().x(), 77));
  addItem(reverbBtnText);

  QGraphicsTextItem *chorusBtnText = new QGraphicsTextItem;
  chorusBtnText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10.5pt; font-weight: normal; color: #bbbbbb\">CHORUS</font>");
  chorusBtnText->setPos(QPointF(1015 - chorusBtnText->boundingRect().center().x(), 77));
  addItem(chorusBtnText);

    QGraphicsTextItem *keyshiftBtnText = new QGraphicsTextItem;
  keyshiftBtnText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10.5pt; font-weight: normal; color: #bbbbbb\">KEY SHIFT</font>");
  keyshiftBtnText->setPos(QPointF(855 - keyshiftBtnText->boundingRect().center().x(), 132));
  addItem(keyshiftBtnText);

  QGraphicsTextItem *midichBtnText = new QGraphicsTextItem;
  midichBtnText->setHtml("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:10.5pt; font-weight: normal; color: #bbbbbb\">MIDI CH</font>");
  midichBtnText->setPos(QPointF(1015 - midichBtnText->boundingRect().center().x(), 132));
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

  // Connect emulator's signals to our display's slots
  connect(emulator, SIGNAL(new_bar_display(QVector<bool>*)),
	  this, SLOT(update_lcd_bar_display(QVector<bool>*)));
  connect(emulator, SIGNAL(display_part_updated(QString)),
	  this, SLOT(update_lcd_part_text(QString)));
  connect(emulator, SIGNAL(display_instrument_updated(QString)),
	  this, SLOT(update_lcd_instrument_text(QString)));
  connect(emulator, SIGNAL(display_level_updated(QString)),
	  this, SLOT(update_lcd_level_text(QString)));
  connect(emulator, SIGNAL(display_pan_updated(QString)),
	  this, SLOT(update_lcd_pan_text(QString)));
  connect(emulator, SIGNAL(display_reverb_updated(QString)),
	  this, SLOT(update_lcd_reverb_text(QString)));
  connect(emulator, SIGNAL(display_chorus_updated(QString)),
	  this, SLOT(update_lcd_chorus_text(QString)));
  connect(emulator, SIGNAL(display_key_shift_updated(QString)),
	  this, SLOT(update_lcd_kshift_text(QString)));
  connect(emulator, SIGNAL(display_midi_channel_updated(QString)),
	  this, SLOT(update_lcd_midich_text(QString)));
  connect(_midiActTimer, SIGNAL(timeout()),
	  this, SLOT(update_midi_activity_timeout()));
}


Scene::~Scene()
{
  QSettings settings;
  settings.setValue("Audio/volume", _volumeDial->value());
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

  // Hack to update volume when power on
  _emulator->change_volume(_volumeDial->value());
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


void Scene::keyPressEvent(QKeyEvent *keyEvent)
{
  // Ignore repeating key events generated from keys being held down
  if (keyEvent->isAutoRepeat())
    return;

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
    _emulator->play_note(_keyNoteOctave * 12 + 0, 120);
  } else if (keyEvent->key() == Qt::Key_S) {
    _emulator->play_note(_keyNoteOctave * 12 + 1, 120);
  } else if (keyEvent->key() == Qt::Key_X) {
    _emulator->play_note(_keyNoteOctave * 12 + 2, 120);
  } else if (keyEvent->key() == Qt::Key_D) {
    _emulator->play_note(_keyNoteOctave * 12 + 3, 120);
  } else if (keyEvent->key() == Qt::Key_C) {
    _emulator->play_note(_keyNoteOctave * 12 + 4, 120);
  } else if (keyEvent->key() == Qt::Key_V) {
    _emulator->play_note(_keyNoteOctave * 12 + 5, 120);
  } else if (keyEvent->key() == Qt::Key_G) {
    _emulator->play_note(_keyNoteOctave * 12 + 6, 120);
  } else if (keyEvent->key() == Qt::Key_B) {
    _emulator->play_note(_keyNoteOctave * 12 + 7, 120);
  } else if (keyEvent->key() == Qt::Key_H) {
    _emulator->play_note(_keyNoteOctave * 12 + 8, 120);
  } else if (keyEvent->key() == Qt::Key_N) {
    _emulator->play_note(_keyNoteOctave * 12 + 9, 120);
  } else if (keyEvent->key() == Qt::Key_J) {
    _emulator->play_note(_keyNoteOctave * 12 + 10, 120);
  } else if (keyEvent->key() == Qt::Key_M) {
    _emulator->play_note(_keyNoteOctave * 12 + 11, 120);
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
    _emulator->play_note(_keyNoteOctave * 12 + 0, 0);
  } else if (keyEvent->key() == Qt::Key_S) {
    _emulator->play_note(_keyNoteOctave * 12 + 1, 0);
  } else if (keyEvent->key() == Qt::Key_X) {
    _emulator->play_note(_keyNoteOctave * 12 + 2, 0);
  } else if (keyEvent->key() == Qt::Key_D) {
    _emulator->play_note(_keyNoteOctave * 12 + 3, 0);
  } else if (keyEvent->key() == Qt::Key_C) {
    _emulator->play_note(_keyNoteOctave * 12 + 4, 0);
  } else if (keyEvent->key() == Qt::Key_V) {
    _emulator->play_note(_keyNoteOctave * 12 + 5, 0);
  } else if (keyEvent->key() == Qt::Key_G) {
    _emulator->play_note(_keyNoteOctave * 12 + 6, 0);
  } else if (keyEvent->key() == Qt::Key_B) {
    _emulator->play_note(_keyNoteOctave * 12 + 7, 0);
  } else if (keyEvent->key() == Qt::Key_H) {
    _emulator->play_note(_keyNoteOctave * 12 + 8, 0);
  } else if (keyEvent->key() == Qt::Key_N) {
    _emulator->play_note(_keyNoteOctave * 12 + 9, 0);
  } else if (keyEvent->key() == Qt::Key_J) {
    _emulator->play_note(_keyNoteOctave * 12 + 10, 0);
  } else if (keyEvent->key() == Qt::Key_M) {
    _emulator->play_note(_keyNoteOctave * 12 + 11, 0);
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
