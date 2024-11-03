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


#ifndef ENVELOPE_DIALOG_H
#define ENVELOPE_DIALOG_H


#include "emulator.h"
#include "scene.h"

#include <QVector>
#include <QPushButton>
#include <QComboBox>
#include <QDialog>
#include <QTimer>
#include <QMutex>
#include <QKeyEvent>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
using namespace QtCharts;
#endif


class EnvelopeDialog : public QDialog
{
  Q_OBJECT

public:
  EnvelopeDialog(Emulator *emulator, Scene *scene, QWidget *parent = nullptr);
  virtual ~EnvelopeDialog();

  void envelope_callback(const float tvp1, const float tvp2,
                         const float tvf1, const float tvf2,
                         const float tva1, const float tva2);

public slots:
  void chart_timeout(void);

  void done(int res);
  void reject(void);

private:
  Emulator *_emulator;
  Scene *_scene;

  QTimer _chartTimer;
  QMutex _dataMutex;

  QVector<float> _tvpData1;
  QVector<float> _tvpData2;
  QVector<float> _tvfData1;
  QVector<float> _tvfData2;
  QVector<float> _tvaData1;
  QVector<float> _tvaData2;
  
  QChart *_tvpChart;
  QChart *_tvfChart;
  QChart *_tvaChart;

  QValueAxis *_tvpXAxis;
  QValueAxis *_tvfXAxis;
  QValueAxis *_tvaXAxis;
  QValueAxis *_tvpYAxis;
  QValueAxis *_tvfYAxis;
  QValueAxis *_tvaYAxis;
  
  QLineSeries *_tvpP1Series;
  QLineSeries *_tvpP2Series;
  QLineSeries *_tvfP1Series;
  QLineSeries *_tvfP2Series;
  QLineSeries *_tvaP1Series;
  QLineSeries *_tvaP2Series;

  QComboBox *_partCB;

  int _partId;

  int _timePeriod;
  unsigned int _iteration;
  float _xPos;

  bool _callbackReceived;
  bool _reset;

  void keyPressEvent(QKeyEvent *keyEvent);
  void keyReleaseEvent(QKeyEvent *keyEvent);

  void _clear_series(void);

private slots:
  void _partCB_changed(int value);
  void _reset_view(void);
};


#endif // ENVELOPE_DIALOG_H


#endif // __USE_QTCHARTS__
