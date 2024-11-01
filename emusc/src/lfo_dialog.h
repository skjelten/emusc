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


class LFODialog : public QDialog
{
  Q_OBJECT

public:
  LFODialog(Emulator *emulator, Scene *scene, QWidget *parent = nullptr);
  virtual ~LFODialog();

  void lfo_callback(const float lfo1, const float lfo2);

public slots:
  void chart_timeout(void);

  void pause(void);
  void done(int res);
  void reject(void);

private:
  Emulator *_emulator;
  Scene *_scene;

  QTimer _chartTimer;
  QMutex _dataMutex;

  QVector<float> _lfoData1;
  QVector<float> _lfoData2;

  QChart *_chart;
  QValueAxis *_xAxis;
  QValueAxis *_yAxis;

  QLineSeries *_LFO1Series;
  QLineSeries *_LFO2Series;

  QComboBox *_partCB;
  QPushButton *_pausePB;

  int _partId;
  
  int _timePeriod;
  unsigned int _iteration;
  float _xPos;

  void keyPressEvent(QKeyEvent *keyEvent);
  void keyReleaseEvent(QKeyEvent *keyEvent);

private slots:
  void _partCB_changed(int value);

};


#endif // LFO_DIALOG_H


#endif // __USE_QTCHARTS__
