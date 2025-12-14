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


#ifdef __USE_QTCHARTS__


#ifndef LFO_DIALOG_H
#define LFO_DIALOG_H


#include "emulator.h"
#include "scene.h"

#include <QPushButton>
#include <QComboBox>
#include <QDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMutex>
#include <QString>
#include <QTimer>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
using namespace QtCharts;
#endif


class LFODialog : public QDialog
{
  Q_OBJECT

public:
  LFODialog(Emulator *emulator, Scene *scene, QWidget *parent = nullptr);
  virtual ~LFODialog();

  void lfo_callback(const int lfo1, const int lfo2p1, const int lfo2p2);

public slots:
  void chart_timeout(void);

  void done(int res);
  void reject(void);

private:
  Emulator *_emulator;
  Scene *_scene;

  QTimer _chartTimer;
  QMutex _dataMutex;

  QList<QPointF> _lfo1Buf;
  QList<QPointF> _lfo2p1Buf;
  QList<QPointF> _lfo2p2Buf;

  QChart *_chart;
  QValueAxis *_xAxis;
  QValueAxis *_yAxis;

  QLineSeries *_LFO1Series;
  QLineSeries *_LFO2P1Series;
  QLineSeries *_LFO2P2Series;

  QComboBox *_partCB;
  QComboBox *_timeCB;

  QLabel *_legendL[3];
  QLabel *_legendBoxL[3];
  QLabel *_waveformPML[3];
  QLabel *_waveformL[3];
  QLabel *_rateL[3];
  QLabel *_delayL[3];
  QLabel *_fadeL[3];

  QLabel *_waveformNameL[3];
  QLabel *_rateValueL[3];
  QLabel *_delayValueL[3];
  QLabel *_fadeValueL[3];

  QPixmap _sinePM;
  QPixmap _squarePM;
  QPixmap _trianglePM;
  QPixmap _sawtoothPM;
  QPixmap _sampleHoldPM;
  QPixmap _randomPM;

  bool _activeLFO[3];

  int _selectedPart;

  int _timePeriod;

  void keyPressEvent(QKeyEvent *keyEvent);
  void keyReleaseEvent(QKeyEvent *keyEvent);

  void _update_instrument_info(void);
  void _enable_lfo_column(bool enable, int column);
  QString _get_qstring_from_waveform(uint8_t waveform);
  void _show_legend(bool status, int lfo);
  void _set_waveform_image(int waveform, QLabel *label);
  QPixmap _invert_pixmap_color(const QPixmap &pixmap);

signals:
  void lfo_data_received(const int lfo1, const int lfo2p1, const int lfo2p2);

private slots:
  void _partCB_changed(int value);
  void _timeCB_changed(QString string);
  void _part_changed(int partId);
  void _update_lfo_series(const int lfo1, const int lfo2p1, const int lfo2p2);

};


#endif // LFO_DIALOG_H


#endif // __USE_QTCHARTS__
