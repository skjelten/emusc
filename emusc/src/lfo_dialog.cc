
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


#include "lfo_dialog.h"

#include <iostream>
#include <vector>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

#include <QApplication>
#include <QMainWindow>
#include <QtCharts/QChartView>


LFODialog::LFODialog(Emulator *emulator, Scene *scene, QWidget *parent)
  : QDialog{parent},
    _emulator(emulator),
    _scene(scene),
    _LFO1Series(0),
    _LFO2P1Series(0),
    _LFO2P2Series(0),
    _timePeriod(5),
    _partId(0),
    _xPos(0),
    _iteration(1)
{
  QObject::connect(&_chartTimer, &QTimer::timeout,
                   this, &LFODialog::chart_timeout);
  _chartTimer.setInterval(100);
  _chartTimer.setTimerType(Qt::PreciseTimer);

  _chart = new QChart();

  _LFO1Series = new QLineSeries(this);
  _LFO2P1Series = new QLineSeries(this);
  _LFO2P2Series = new QLineSeries(this);
  _chart->addSeries(_LFO1Series);
  _chart->addSeries(_LFO2P1Series);
  _chart->addSeries(_LFO2P2Series);

  _xAxis = new QValueAxis();
  _yAxis = new QValueAxis();

  _xAxis->setTickCount(6);
  _yAxis->setTickCount(5);
  _xAxis->setRange(0, _timePeriod);
  _yAxis->setRange(-1, 1);

  _chart->addAxis(_xAxis, Qt::AlignBottom);
  _chart->addAxis(_yAxis, Qt::AlignLeft);

  _LFO1Series->attachAxis(_xAxis);
  _LFO1Series->attachAxis(_yAxis);
  _LFO2P1Series->attachAxis(_xAxis);
  _LFO2P1Series->attachAxis(_yAxis);
  _LFO2P2Series->attachAxis(_xAxis);
  _LFO2P2Series->attachAxis(_yAxis);

  _LFO1Series->setName("LFO1");
  _LFO2P1Series->setName("LFO2 P1");
  _LFO2P2Series->setName("LFO2 P2");

  _chart->setAnimationOptions(QChart::GridAxisAnimations);

  QChartView *chartView = new QChartView(_chart);
  chartView->setRenderHint(QPainter::Antialiasing);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  _pausePB = buttonBox->addButton("Pause", QDialogButtonBox::ActionRole);
  _pausePB->setIcon(style()->standardIcon(QStyle::SP_MediaPause));

  connect(_pausePB, &QAbstractButton::clicked, this, &LFODialog::pause);  

  QHBoxLayout *hboxLayout = new QHBoxLayout;
  hboxLayout->addWidget(new QLabel("Part:"));
  _partCB = new QComboBox();
  for (int i = 1; i <= 16; i++) {               // TODO: SC-88 => A1-16 + B1-16
    _partCB->addItem(QString::number(i));
  }
  _partCB->setEditable(false);
  hboxLayout->addWidget(_partCB);
  hboxLayout->addStretch(1);

  connect(_partCB, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_partCB_changed(int)));

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(chartView);
  mainLayout->addLayout(hboxLayout);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  setWindowTitle(tr("LFOs monitor dialog"));
  setModal(false);

  resize(600, 400);
  show();

  _chartTimer.start();

  _emulator->set_lfo_callback(_partId, this);
}


LFODialog::~LFODialog()
{
  _emulator->clear_lfo_callback(_partId);
}


void LFODialog::pause(void)
{
  if (_chartTimer.isActive()) {
    _chartTimer.stop();
    _pausePB->setText("Start");
    _pausePB->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

  } else {
    _chartTimer.start();
    _pausePB->setText("Pause");
    _pausePB->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
  }
}


void LFODialog::reject()
{
  _chartTimer.stop();
  delete this;
}


void LFODialog::done(int res)
{
  _chartTimer.stop();
  delete this;
}


// Interval = 100ms => 10Hz
void LFODialog::chart_timeout(void)
{
  if (_iteration % (_timePeriod * 10) == 0) {
    _LFO1Series->clear();
    _LFO2P1Series->clear();
    _LFO2P2Series->clear();
    _chart->scroll(_chart->plotArea().width(), 0);
  }

  _dataMutex.lock();

  std::vector<float> lfo1Data(_lfo1Data.constBegin(), _lfo1Data.constEnd());
  std::vector<float> lfo2p1Data(_lfo2p1Data.constBegin(), _lfo2p1Data.constEnd());
  std::vector<float> lfo2p2Data(_lfo2p2Data.constBegin(), _lfo2p2Data.constEnd());

  _lfo1Data.clear();
  _lfo2p1Data.clear();
  _lfo2p2Data.clear();

  _dataMutex.unlock();

  if (lfo1Data.empty()) {
    _LFO1Series->append(_xPos, 0);
  } else {  
    float dX = 0.1 / lfo1Data.size();
    int i = 0;
    for (auto &value : lfo1Data)
      if (value == value)
        _LFO1Series->append(_xPos + (float) i++ * dX, value);
  }

  if (lfo2p1Data.empty()) {
    _LFO2P1Series->append(_xPos, 0);
  } else {
    float dX = 0.1 / lfo2p1Data.size();
    int i = 0;
    for (auto &value : lfo2p1Data)
      if (value == value)
        _LFO2P1Series->append(_xPos + (float) i++ * dX, value);
  }

  if (lfo2p2Data.empty()) {
    _LFO2P2Series->append(_xPos, 0);
  } else {
    float dX = 0.1 / lfo2p2Data.size();
    int i = 0;
    for (auto &value : lfo2p2Data)
      if (value == value)
        _LFO2P2Series->append(_xPos + (float) i++ * dX, value);
  }

  _xPos += 0.1;
  _iteration++;
}


void LFODialog::lfo_callback(const float lfo1,
                             const float lfo2p1, const float lfo2p2)
{
  _dataMutex.lock();

  _lfo1Data.push_back(lfo1);
  _lfo2p1Data.push_back(lfo2p1);
  _lfo2p2Data.push_back(lfo2p2);

  _dataMutex.unlock();
}


void LFODialog::keyPressEvent(QKeyEvent *keyEvent)
{
  if (keyEvent->key() != Qt::Key_Space)
    QApplication::sendEvent(_scene, keyEvent);
}


void LFODialog::keyReleaseEvent(QKeyEvent *keyEvent)
{
  if (keyEvent->key() != Qt::Key_Space)
    QApplication::sendEvent(_scene, keyEvent);
}


void LFODialog::_partCB_changed(int value)
{
  _emulator->clear_lfo_callback(_partId);

  _partId = value;
  
  _partCB->setCurrentIndex(_partId);
  _emulator->set_lfo_callback(_partId, this);
}


#endif  // __USE_QTCHARTS__
