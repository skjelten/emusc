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


#include "envelope_dialog.h"

#include <iostream>
#include <vector>

#include <QApplication>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QtCharts/QChartView>


EnvelopeDialog::EnvelopeDialog(Emulator *emulator, Scene *scene, QWidget *parent)
  : QDialog{parent},
    _emulator(emulator),
    _scene(scene),
    _timePeriod(10),
    _partId(0),
    _callbackReceived(0),
    _reset(0)
{
  QObject::connect(&_chartTimer, &QTimer::timeout,
                   this, &EnvelopeDialog::chart_timeout);
  _chartTimer.setInterval(100);
  _chartTimer.setTimerType(Qt::PreciseTimer);

  _tvpChart = new QChart();
  _tvfChart = new QChart();
  _tvaChart = new QChart();

  _tvpP1Series = new QLineSeries(this);
  _tvpP2Series = new QLineSeries(this);
  _tvfP1Series = new QLineSeries(this);
  _tvfP2Series = new QLineSeries(this);
  _tvaP1Series = new QLineSeries(this);
  _tvaP2Series = new QLineSeries(this);

  _tvpChart->addSeries(_tvpP1Series);
  _tvpChart->addSeries(_tvpP2Series);
  _tvfChart->addSeries(_tvfP1Series);
  _tvfChart->addSeries(_tvfP2Series);
  _tvaChart->addSeries(_tvaP1Series);
  _tvaChart->addSeries(_tvaP2Series);

  _tvpXAxis = new QValueAxis();
  _tvfXAxis = new QValueAxis();
  _tvaXAxis = new QValueAxis();
  _tvpXAxis->setTickCount(6);
  _tvpXAxis->setRange(0, _timePeriod);
  _tvfXAxis->setTickCount(6);
  _tvfXAxis->setRange(0, _timePeriod);
  _tvaXAxis->setTickCount(6);
  _tvaXAxis->setRange(0, _timePeriod);

  _tvpYAxis = new QValueAxis();
  _tvpYAxis->setTickCount(5);
  _tvpYAxis->setRange(-64, 64);
  _tvfYAxis = new QValueAxis();
  _tvfYAxis->setTickCount(5);
  _tvfYAxis->setRange(0, 128);
  _tvaYAxis = new QValueAxis();
  _tvaYAxis->setTickCount(5);
  _tvaYAxis->setRange(0, 1);

  _tvpChart->addAxis(_tvpXAxis, Qt::AlignBottom);
  _tvfChart->addAxis(_tvfXAxis, Qt::AlignBottom);
  _tvaChart->addAxis(_tvaXAxis, Qt::AlignBottom);
  _tvpChart->addAxis(_tvpYAxis, Qt::AlignLeft);
  _tvfChart->addAxis(_tvfYAxis, Qt::AlignLeft);
  _tvaChart->addAxis(_tvaYAxis, Qt::AlignLeft);

  _tvpP1Series->attachAxis(_tvpXAxis);
  _tvpP1Series->attachAxis(_tvpYAxis);  
  _tvpP2Series->attachAxis(_tvpXAxis);
  _tvpP2Series->attachAxis(_tvpYAxis);

  _tvfP1Series->attachAxis(_tvfXAxis);
  _tvfP1Series->attachAxis(_tvfYAxis);
  _tvfP2Series->attachAxis(_tvfXAxis);
  _tvfP2Series->attachAxis(_tvfYAxis);

  _tvaP1Series->attachAxis(_tvaXAxis);
  _tvaP1Series->attachAxis(_tvaYAxis);
  _tvaP2Series->attachAxis(_tvaXAxis);
  _tvaP2Series->attachAxis(_tvaYAxis);

  _tvpP1Series->setName("Pitch P1");
  _tvpP2Series->setName("Pitch P2");
  _tvfP1Series->setName("TVF P1");
  _tvfP2Series->setName("TVF P2");
  _tvaP1Series->setName("TVA P1");
  _tvaP2Series->setName("TVA P2");

  QChartView *tvpChartView = new QChartView(_tvpChart);
  QChartView *tvfChartView = new QChartView(_tvfChart);
  QChartView *tvaChartView = new QChartView(_tvaChart);
  tvpChartView->setRenderHint(QPainter::Antialiasing);
  tvfChartView->setRenderHint(QPainter::Antialiasing);
  tvaChartView->setRenderHint(QPainter::Antialiasing);

  tvpChartView->setRubberBand(QChartView::HorizontalRubberBand);
  tvfChartView->setRubberBand(QChartView::HorizontalRubberBand);
  tvaChartView->setRubberBand(QChartView::HorizontalRubberBand);
  
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Reset |
                                                     QDialogButtonBox::Close);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()),
	  this, SLOT(_reset_view()));

  
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
  mainLayout->addWidget(tvpChartView);
  mainLayout->addWidget(tvfChartView);
  mainLayout->addWidget(tvaChartView);
  mainLayout->addLayout(hboxLayout);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  setWindowTitle(tr("Envelopes monitor dialog"));
  setModal(false);

  resize(600, 750);
  show();

  _chartTimer.start();

  _emulator->set_envelope_callback(_partId, this);
}


EnvelopeDialog::~EnvelopeDialog()
{
  _emulator->clear_envelope_callback(_partId);
}


void EnvelopeDialog::_clear_series(void)
{
  _tvpP1Series->clear();
  _tvpP2Series->clear();
  _tvfP1Series->clear();
  _tvfP2Series->clear();
  _tvaP1Series->clear();
  _tvaP2Series->clear();
}


void EnvelopeDialog::reject()
{
  _chartTimer.stop();
  delete this;
}


void EnvelopeDialog::done(int res)
{
  _chartTimer.stop();
  delete this;
}


// Update all charts at 10Hz
void EnvelopeDialog::chart_timeout(void)
{
  if (!_callbackReceived) {
    _reset = true;
    return;

  } else {
    if (_reset) {
      _clear_series();
      _reset = false;
      _timeStart = QDateTime::currentMSecsSinceEpoch();
    }
  }

  // Copy all the data to a temporary buffer to let the audio thread run
  _dataMutex.lock();

  std::vector<std::pair<float, qint64>> tvpData1(_tvpData1.constBegin(),
                                                 _tvpData1.constEnd());
  std::vector<std::pair<float, qint64>> tvpData2(_tvpData2.constBegin(),
                                                 _tvpData2.constEnd());
  std::vector<std::pair<float, qint64>> tvfData1(_tvfData1.constBegin(),
                                                 _tvfData1.constEnd());
  std::vector<std::pair<float, qint64>> tvfData2(_tvfData2.constBegin(),
                                                 _tvfData2.constEnd());
  std::vector<std::pair<float, qint64>> tvaData1(_tvaData1.constBegin(),
                                                 _tvaData1.constEnd());
  std::vector<std::pair<float, qint64>> tvaData2(_tvaData2.constBegin(),
                                                 _tvaData2.constEnd());

  _tvpData1.clear();
  _tvpData2.clear();
  _tvfData1.clear();
  _tvfData2.clear();
  _tvaData1.clear();
  _tvaData2.clear();

  _callbackReceived = false;

  _dataMutex.unlock();

  for (auto &value : tvpData1)
    _tvpP1Series->append((value.second - _timeStart) / 1000.0, value.first);
  for (auto &value : tvpData2)
    _tvpP2Series->append((value.second - _timeStart) / 1000.0, value.first);

  for (auto &value : tvfData1)
    _tvfP1Series->append((value.second - _timeStart) / 1000.0, value.first);
  for (auto &value : tvfData2)
    _tvfP2Series->append((value.second - _timeStart) / 1000.0, value.first);

  for (auto &value : tvaData1)
    _tvaP1Series->append((value.second - _timeStart) / 1000.0, value.first);
  for (auto &value : tvaData2)
    _tvaP2Series->append((value.second - _timeStart) / 1000.0, value.first);
}


void EnvelopeDialog::envelope_callback(const float tvp1, const float tvp2,
                                       const float tvf1, const float tvf2,
                                       const float tva1, const float tva2)
{
  qint64 time = QDateTime::currentMSecsSinceEpoch();

  _dataMutex.lock();

  _tvpData1.push_back(std::pair(tvp1, time));
  _tvpData2.push_back(std::pair(tvp2, time));
  _tvfData1.push_back(std::pair(tvf1, time));
  _tvfData2.push_back(std::pair(tvf2, time));
  _tvaData1.push_back(std::pair(tva1, time));
  _tvaData2.push_back(std::pair(tva2, time));

  _dataMutex.unlock();

  _callbackReceived = true;
}


void EnvelopeDialog::keyPressEvent(QKeyEvent *keyEvent)
{
  if (keyEvent->key() != Qt::Key_Space)
    QApplication::sendEvent(_scene, keyEvent);
}


void EnvelopeDialog::keyReleaseEvent(QKeyEvent *keyEvent)
{
  if (keyEvent->key() != Qt::Key_Space)
    QApplication::sendEvent(_scene, keyEvent);
}


void EnvelopeDialog::_partCB_changed(int value)
{
  _emulator->clear_envelope_callback(_partId);

  _partId = value;

  _partCB->setCurrentIndex(_partId);
  _emulator->set_envelope_callback(_partId, this);
}


void EnvelopeDialog::_reset_view(void)
{
  _tvpChart->zoomReset();
  _tvfChart->zoomReset();
  _tvaChart->zoomReset();
}

#endif  // __USE_QTCHARTS__
