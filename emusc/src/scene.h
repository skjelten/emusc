/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2023  Håkon Skjelten
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

  QPushButton *_allButton;
  QPushButton *_muteButton;

  QPushButton *_partLButton;
  QPushButton *_partRButton;
  class SynthButton *_instrumentLButton;
  QPushButton *_instrumentRButton;
  QPushButton *_panRButton;
  QPushButton *_panLButton;
  QPushButton *_chorusRButton;
  QPushButton *_chorusLButton;
  QPushButton *_midichRButton;
  QPushButton *_midichLButton;
  QPushButton *_levelRButton;
  QPushButton *_levelLButton;
  QPushButton *_reverbRButton;
  QPushButton *_reverbLButton;
  QPushButton *_keyshiftRButton;
  QPushButton *_keyshiftLButton;

  QColor _lcdBackgroundOnColor;
  QColor _lcdBackgroundOffColor;
  QColor _lcdOnActiveColor;
  QColor _lcdOnInactiveColor;
  QColor _lcdOffFontColor;
 
  void keyPressEvent(QKeyEvent *keyEvent);
  void keyReleaseEvent(QKeyEvent *keyEvent);

public:
  Scene(Emulator *emulator, QWidget *parent = nullptr);
  virtual ~Scene();

public slots:
  void display_on(void);
  void display_off(void);

  void update_lcd_instrument_text(QString text);
  void update_lcd_part_text(QString text);
  void update_lcd_level_text(QString text);
  void update_lcd_pan_text(QString text);
  void update_lcd_reverb_text(QString text);
  void update_lcd_chorus_text(QString text);
  void update_lcd_kshift_text(QString text);
  void update_lcd_midich_text(QString text);
  void update_lcd_bar_display(QVector<bool> *partAmp);

  void update_all_button(bool status);
  void update_mute_button(bool status);
};


class SynthButton : public QPushButton
{
  Q_OBJECT

public:
  SynthButton(QWidget *parent = nullptr);
  virtual ~SynthButton();

  protected:
    void mousePressEvent(QMouseEvent *event) override;

  signals:
    void rightClicked();
};


#endif // SCENE_H
