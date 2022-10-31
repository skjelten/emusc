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


#ifndef SCENE_H
#define SCENE_H


#include "emulator.h"

#include <QGraphicsScene>

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QPushButton>
#include <QDial>
#include <QVector>
#include <QColor>
#include <QBrush>
#include <QTimer>
#include <QKeyEvent>


class Scene : public QGraphicsScene
{
  Q_OBJECT

private:
  Emulator *_emulator;

  QGraphicsRectItem* _lcdBackground;
  QPushButton *_powerButton;
  QDial *_volumeDial;
  QVector<QGraphicsRectItem*> _volumeBars;
  QVector<QGraphicsEllipseItem*> _volumeCircles;
  QVector<QGraphicsTextItem*> _partNumText;

  QGraphicsTextItem *_lcdLevelHeaderText;
  QGraphicsTextItem *_lcdPanHeaderText;
  QGraphicsTextItem *_lcdReverbHeaderText;
  QGraphicsTextItem *_lcdChorusHeaderText;
  QGraphicsTextItem *_lcdKshiftHeaderText;
  QGraphicsTextItem *_lcdMidichHeaderText;

  QGraphicsTextItem *_lcdInstrumentText;
  QGraphicsTextItem *_lcdPartText;
  QGraphicsTextItem *_lcdLevelText;
  QGraphicsTextItem *_lcdPanText;
  QGraphicsTextItem *_lcdReverbText;
  QGraphicsTextItem *_lcdChorusText;
  QGraphicsTextItem *_lcdKshiftText;
  QGraphicsTextItem *_lcdMidichText;

  QColor _lcdBackgroundOnColor;
  QColor _lcdBackgroundOffColor;
  QColor _lcdOnActiveColor;
  QColor _lcdOnInactiveColor;
  QColor _lcdOffFontColor;

  QTimer *_lcdDisplayTimer;

  int _introFrameIndex;
  QVector<uint8_t> _introAnimData;
  
  struct lcdBarDisplayPartHist {
    bool falling;
    int value;
    int time;
  }; 
  QVector<struct lcdBarDisplayPartHist> _lcdBarDisplayHistVect;
 
  void _init_lcd_display(void);

  void keyPressEvent(QKeyEvent *keyEvent);

public:
  Scene(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~Scene();

  void display_on(void);
  void display_off(void);

private slots:
  void update_lcd_instrument_text(QString text);
  void update_lcd_part_text(QString text);
  void update_lcd_level_text(QString text);
  void update_lcd_pan_text(QString text);
  void update_lcd_reverb_text(QString text);
  void update_lcd_chorus_text(QString text);
  void update_lcd_kshift_text(QString text);
  void update_lcd_midich_text(QString text);

  void update_intro_anim(void);
  void update_bar_display(void);
};


#endif // SCENE_H
