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


#ifndef SCENE_H
#define SCENE_H


#include <QBrush>
#include <QColor>
#include <QDial>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPushButton>
#include <QRadialGradient>
#include <QTimer>
#include <QVector>


class VolumeDial;
class SynthButton;


class Scene : public QGraphicsScene
{
  Q_OBJECT

private:
  QGraphicsRectItem* _lcdBackground;
  QPushButton *_powerButton;
  VolumeDial *_volumeDial;
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
  SynthButton *_instrumentLButton;
  SynthButton *_instrumentRButton;
  SynthButton *_panRButton;
  SynthButton *_panLButton;
  SynthButton *_chorusRButton;
  SynthButton *_chorusLButton;
  SynthButton *_midichRButton;
  SynthButton *_midichLButton;
  SynthButton *_levelRButton;
  SynthButton *_levelLButton;
  SynthButton *_reverbRButton;
  SynthButton *_reverbLButton;
  SynthButton *_keyshiftRButton;
  SynthButton *_keyshiftLButton;

  QGraphicsRectItem *_midiActLed;
  QTimer *_midiActTimer;
  QRadialGradient *_ledOnGradient;
  QRadialGradient *_ledOffGradient;

  QColor _lcdOnBackgroundColor;
  QColor _lcdOffBackgroundColor;
  QColor _lcdOnActiveColor;
  QColor _lcdOnInactiveColor;
  QColor _lcdOffFontColor;

  QColor _lcdOnBackgroundColorReset;
  QColor _lcdOnActiveColorReset;
  QColor _lcdOnInactiveColorReset;

  QColor _backgroundColor;
  QBrush _backgroundBrush;

  bool _midiKbdInput;
  int _keyNoteOctave;

  void _add_lcd_display_items(void);

  QString _generate_sans_text_html(QString text, float size);
  QString _generate_retro_text_html(QString text);

  void _connect_signals(void);


protected:
  void drawBackground(QPainter *painter, const QRectF &rect);

  void keyPressEvent(QKeyEvent *keyEvent);
  void keyReleaseEvent(QKeyEvent *keyEvent);

  void mousePressEvent(QGraphicsSceneMouseEvent *event);

public:
  Scene(QWidget *parent = nullptr);
  virtual ~Scene();

  QColor get_lcd_bkg_on_color(void) { return _lcdOnBackgroundColor; }
  QColor get_lcd_active_on_color(void) { return _lcdOnActiveColor; }
  QColor get_lcd_inactive_on_color(void) { return _lcdOnInactiveColor; }

  QColor get_lcd_bkg_on_color_reset(void) {return _lcdOnBackgroundColorReset; }
  QColor get_lcd_active_on_color_reset(void) { return _lcdOnActiveColorReset; }
  QColor get_lcd_inactive_on_color_reset(void) { return _lcdOnInactiveColorReset; }

  void set_lcd_bkg_on_color(QColor color);
  void set_lcd_active_on_color(QColor color);
  void set_lcd_inactive_on_color(QColor color);

  void set_midi_kbd_enable(bool state) { _midiKbdInput = state; }

public slots:
  void display_on(void);
  void display_off(void);

  void update_lcd_bar_display(QVector<bool> *partAmp);

  void update_all_button(bool status);
  void update_mute_button(bool status);

  void update_midi_activity_led(bool sysex, int length);
  void update_midi_activity_timeout(void);

  void update_lcd_instrument_text(QString text);
  void update_lcd_part_text(QString text);
  void update_lcd_level_text(QString text);
  void update_lcd_pan_text(QString text);
  void update_lcd_reverb_text(QString text);
  void update_lcd_chorus_text(QString text);
  void update_lcd_kshift_text(QString text);
  void update_lcd_midich_text(QString text);

signals:
  void volume_changed(int percent);
  void play_note(uint8_t key, uint8_t velocity);
  void lcd_display_mouse_press_event(Qt::MouseButton button,const QPointF &pos);

  void all_button_clicked(void);
  void mute_button_clicked(void);
  void partL_button_clicked(void);
  void partR_button_clicked(void);
  void instrumentL_button_clicked(void);
  void instrumentR_button_clicked(void);
  void instrumentL_button_rightClicked(void);
  void instrumentR_button_rightClicked(void);
  void panL_button_clicked(void);
  void panR_button_clicked(void);
  void chorusL_button_clicked(void);
  void chorusR_button_clicked(void);
  void midichL_button_clicked(void);
  void midichR_button_clicked(void);
  void levelL_button_clicked(void);
  void levelR_button_clicked(void);
  void reverbL_button_clicked(void);
  void reverbR_button_clicked(void);
  void keyshiftL_button_clicked(void);
  void keyshiftR_button_clicked(void);

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


class GrooveRect : public QGraphicsRectItem
{
public:
  explicit GrooveRect(qreal x, qreal y, qreal w, qreal h,
		      QGraphicsItem *parent = nullptr);

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
	     QWidget *widget);
};


class VolumeDial : public QDial
{
  Q_OBJECT

public:
  VolumeDial(QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event);
};


#endif // SCENE_H
