
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

#include <QFont>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLegendMarker>
#include <QPalette>
#include <QVBoxLayout>
#include <QStyleHints>

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
    _sinePM(":/images/wf_sine.png"),
    _squarePM(":/images/wf_square.png"),
    _trianglePM(":/images/wf_triangle.png"),
    _sawtoothPM(":/images/wf_sawtooth.png"),
    _sampleHoldPM(":/images/wf_samplehold.png"),
    _randomPM(":/images/wf_random.png"),
    _timePeriod(3),
    _selectedPart(0)
{
  // Convert black waveform icons to white if we are using dark mode
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  if (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
    _sinePM = _invert_pixmap_color(_sinePM);
    _squarePM = _invert_pixmap_color(_squarePM);
    _trianglePM = _invert_pixmap_color(_trianglePM);
    _sawtoothPM = _invert_pixmap_color(_sawtoothPM);
    _sampleHoldPM = _invert_pixmap_color(_sampleHoldPM);
    _randomPM = _invert_pixmap_color(_randomPM);
  }
#endif

  QObject::connect(&_chartTimer, &QTimer::timeout,
                   this, &LFODialog::chart_timeout);
  _chartTimer.setInterval(25);
  _chartTimer.setTimerType(Qt::PreciseTimer);

  _chart = new QChart();

  QFont titleFont = _chart->titleFont();
  titleFont.setBold(true);
  _chart->setTitleFont(titleFont);

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
  _xAxis->setReverse(true);
  _yAxis->setRange(-1, 1);

  titleFont.setBold(false);
  _xAxis->setTitleFont(titleFont);
  _xAxis->setTitleText("Seconds");

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

  _chart->setAnimationOptions(QChart::NoAnimation);
  _chart->legend()->hide();

  QChartView *chartView = new QChartView(_chart);
  chartView->setRenderHint(QPainter::Antialiasing);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  _legendL[0] = new QLabel("LFO1");
  _legendL[1] = new QLabel("LFO2P1");
  _legendL[2] = new QLabel("LFO2P2");

  QGridLayout *gridLayout = new QGridLayout();
  for (int i = 0; i < 3; i++) {
    _waveformPML[i] = new QLabel();
    gridLayout->addWidget(_waveformPML[i], 0, i * 2, Qt::AlignRight | Qt::AlignVCenter);
    _legendBoxL[i] = new QLabel();
    gridLayout->addWidget(_legendBoxL[i], 0, i * 2 + 1, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(_legendL[i], 0, i * 2 + 2, Qt::AlignLeft | Qt::AlignVCenter);

    gridLayout->addWidget(new QLabel("Waveform:"), 1, 2 * i, 1, 2, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(new QLabel("Rate:"), 2, 2 * i, 1, 2, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(new QLabel("Delay:"), 3, 2 * i, 1, 2,  Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(new QLabel("Fade:"), 4, 2 * i, 1, 2,  Qt::AlignRight | Qt::AlignVCenter);

    gridLayout->setColumnStretch(i * 2 + 2, 1);

    _waveformL[i] = new QLabel();
    _rateL[i] = new QLabel();
    _delayL[i] = new QLabel();
    _fadeL[i] = new QLabel();
    _waveformNameL[i] = new QLabel();
    _rateValueL[i] = new QLabel();
    _delayValueL[i] = new QLabel();
    _fadeValueL[i] = new QLabel();

    gridLayout->addWidget(_waveformL[i], 1, 2 * i + 1);
    gridLayout->addWidget(_rateL[i], 2, 2 * i + 1);
    gridLayout->addWidget(_delayL[i], 3, 2 * i + 1);
    gridLayout->addWidget(_fadeL[i], 4, 2 * i + 1);
    gridLayout->addWidget(_waveformNameL[i], 1, 2 * i + 2);
    gridLayout->addWidget(_rateValueL[i], 2, 2 * i + 2);
    gridLayout->addWidget(_delayValueL[i], 3, 2 * i + 2);
    gridLayout->addWidget(_fadeValueL[i], 4, 2 * i + 2);

    _rateL[i]->setEnabled(false);
  }

  QHBoxLayout *hboxLayout = new QHBoxLayout;
  _partCB = new QComboBox();
  for (int i = 1; i <= 16; i++) {
    _partCB->addItem(QString::number(i));
  }
  _partCB->setEditable(false);

  _timeCB = new QComboBox();
  _timeCB->addItem("3s");
  _timeCB->addItem("5s");
  _timeCB->addItem("10s");
  _timeCB->setEditable(false);

  hboxLayout->addWidget(new QLabel("Part:"));
  hboxLayout->addWidget(_partCB);
  hboxLayout->addSpacing(15);
  hboxLayout->addWidget(new QLabel("Time period:"));
  hboxLayout->addWidget(_timeCB);
  hboxLayout->addStretch(1);

  connect(_partCB, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_partCB_changed(int)));
  connect(_timeCB, SIGNAL(currentTextChanged(QString)),
	  this, SLOT(_timeCB_changed(QString)));
  connect(_emulator, SIGNAL(part_changed(int)),
	  this, SLOT(_part_changed(int)));
  connect(this, SIGNAL(lfo_data_received(int, int, int)),
          this, SLOT(_update_lfo_series(int, int, int)));

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(chartView, 1);
  mainLayout->addLayout(gridLayout);
  mainLayout->addSpacing(15);
  mainLayout->addLayout(hboxLayout);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  setWindowTitle(tr("LFOs monitor dialog"));
  setModal(false);

  _update_instrument_info();

  resize(700, 600);
  show();

  // Prepare scope buffers
  int lfoBufferSize = 125 * _timePeriod;
  _lfo1Buf.reserve(lfoBufferSize);
  _lfo2p1Buf.reserve(lfoBufferSize);
  _lfo2p2Buf.reserve(lfoBufferSize);

  _chartTimer.start();

  _emulator->set_lfo_callback(_selectedPart, this);
}


LFODialog::~LFODialog()
{
  _emulator->clear_lfo_callback(_selectedPart);
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


// QTimer interval: 25ms => 40Hz
void LFODialog::chart_timeout(void)
{
  _LFO1Series->replace(_lfo1Buf);
  _LFO2P1Series->replace(_lfo2p1Buf);
  _LFO2P2Series->replace(_lfo2p2Buf);
}


// Called from emulator thread
void LFODialog::lfo_callback(const int lfo1, const int lfo2p1, const int lfo2p2)
{
  emit lfo_data_received(lfo1, lfo2p1, lfo2p2);
}


void LFODialog::_update_lfo_series(const int lfo1,
                                   const int lfo2p1, const int lfo2p2)
{
  // Remove oldest recording if it is outside scope
  int bufSize = 125 * _timePeriod;
  if (_lfo1Buf.size() >= bufSize)
    _lfo1Buf.removeLast();
  if (_lfo2p1Buf.size() >= bufSize)
    _lfo2p1Buf.removeLast();
  if (_lfo2p2Buf.size() >= bufSize)
    _lfo2p2Buf.removeLast();

  // Move all existing points back
  for (QPointF &point : _lfo1Buf)
    point.setX((float) point.x() + (1.0 / 125.0));
  for (QPointF &point : _lfo2p1Buf)
    point.setX((float) point.x() + (1.0 / 125.0));
  for (QPointF &point : _lfo2p2Buf)
    point.setX((float) point.x() + (1.0 / 125.0));

  // Add new measurement for active LFOs
  if (_activeLFO[0])
    _lfo1Buf.prepend(QPointF(0, (float) lfo1 / 32767.0));
  if (_activeLFO[1])
    _lfo2p1Buf.prepend(QPointF(0, (float) lfo2p1 / 32767.0));
  if (_activeLFO[2])
    _lfo2p2Buf.prepend(QPointF(0, (float) lfo2p2 / 32767.0));
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
  _emulator->clear_lfo_callback(_selectedPart);

  _selectedPart = value;

  _partCB->setCurrentIndex(_selectedPart);
  _emulator->set_lfo_callback(_selectedPart, this);

  _update_instrument_info();
}


void LFODialog::_timeCB_changed(QString string)
{
  if (string == "3s")
    _timePeriod = 3;
  else if (string == "5s")
    _timePeriod = 5;
  else
    _timePeriod = 10;

  _xAxis->setRange(0, _timePeriod);
}


void LFODialog::_update_instrument_info(void)
{
  QList<QLegendMarker *> legendList = _chart->legend()->markers();

  QString waveform[3];
  QString delay[3];
  QString fade[3];
  
  uint8_t rhythm =
    _emulator->get_param(EmuSC::PatchParam::UseForRhythm, _selectedPart);
  if (!rhythm) {
    uint8_t *tone =
      _emulator->get_param_ptr(EmuSC::PatchParam::ToneNumber, _selectedPart);
    EmuSC::ControlRom::Instrument &iRom =
      _emulator->get_instrument_rom(tone[0], tone[1]);

    _chart->setTitle(QString::fromStdString(iRom.name));

    // LFO1
    if (iRom.partialsUsed & 0x1) {
      if (iRom.LFO1Rate) {
        _set_waveform_image(iRom.LFO1Waveform & 0x0f, _waveformPML[0]);
        _waveformPML[0]->setPixmap(_sinePM.scaled(25, 25,
                                                  Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation));
        _waveformNameL[0]->setText(_get_qstring_from_waveform(iRom.LFO1Waveform & 0x0f));
        _rateValueL[0]->setText(QString::number((_emulator->get_lfo_rate_LUT(iRom.LFO1Rate)) / 512.0, 'f', 1) + " Hz");
        _delayValueL[0]->setText(QString::number(512.0 / (_emulator->get_lfo_delay_fade_LUT(iRom.LFO1Delay)), 'f', 1) + " s");
        _fadeValueL[0]->setText(QString::number(512.0 / (_emulator->get_lfo_delay_fade_LUT(iRom.LFO1Fade)), 'f', 1) + " s");
        _enable_lfo_column(true, 0);
        _activeLFO[0] = true;
        QPixmap p(15, 15);
        p.fill(legendList.at(0)->brush().color());
        _legendBoxL[0]->setPixmap(p);

      } else {
        _waveformPML[0]->clear();
        _waveformNameL[0]->setText("Not in use");
        _rateValueL[0]->setText("-");
        _delayValueL[0]->setText("-");
        _fadeValueL[0]->setText("-");
        _enable_lfo_column(false, 0);
        _activeLFO[0] = false;
        _legendBoxL[0]->clear();
      }

      // LFO2P1
      if (iRom.partials[0].LFO2Rate) {
        _set_waveform_image(iRom.partials[0].LFO2Waveform & 0x0f,
                            _waveformPML[1]);
        _waveformNameL[1]->setText(_get_qstring_from_waveform(iRom.partials[0].LFO2Waveform & 0x0f));
        _rateValueL[1]->setText(QString::number((_emulator->get_lfo_rate_LUT(iRom.partials[0].LFO2Rate)) / 512.0, 'f', 1) + " Hz");
        _delayValueL[1]->setText(QString::number(512.0 / (_emulator->get_lfo_delay_fade_LUT(iRom.partials[0].LFO2Delay)), 'f', 1) + " s");
        _fadeValueL[1]->setText(QString::number(512.0 / (_emulator->get_lfo_delay_fade_LUT(iRom.partials[0].LFO2Fade)), 'f', 1) + " s");
        _enable_lfo_column(true, 1);
        _activeLFO[1] = true;
        QPixmap p(15, 15);
        p.fill(legendList.at(1)->brush().color());
        _legendBoxL[1]->setPixmap(p);

      } else {
        _waveformPML[1]->clear();
        _waveformNameL[1]->setText("Not in use");
        _rateValueL[1]->setText("-");
        _delayValueL[1]->setText("-");
        _fadeValueL[1]->setText("-");
        _enable_lfo_column(false, 1);
        _activeLFO[1] = false;
        _legendBoxL[1]->clear();
      }
    }

    // LFO2P2
    if (iRom.partialsUsed & 0x2 && iRom.partials[1].LFO2Rate) {
      _set_waveform_image(iRom.partials[1].LFO2Waveform & 0x0f,
                          _waveformPML[2]);
      _waveformNameL[2]->setText(_get_qstring_from_waveform(iRom.partials[1].LFO2Waveform & 0x0f));
      _rateValueL[2]->setText(QString::number((_emulator->get_lfo_rate_LUT(iRom.partials[1].LFO2Rate)) / 512.0, 'f', 1) + " Hz");
        _delayValueL[2]->setText(QString::number(512.0 / (_emulator->get_lfo_delay_fade_LUT(iRom.partials[1].LFO2Delay)), 'f', 1) + " s");
        _fadeValueL[2]->setText(QString::number(512.0 / (_emulator->get_lfo_delay_fade_LUT(iRom.partials[1].LFO2Fade)), 'f', 1) + " s");
      _enable_lfo_column(true, 2);
      _activeLFO[2] = true;
      QPixmap p(15, 15);
      p.fill(legendList.at(2)->brush().color());
      _legendBoxL[2]->setPixmap(p);

    } else {
      _waveformPML[2]->clear();
      _waveformNameL[2]->setText("Not in use");
      _rateValueL[2]->setText("-");
      _delayValueL[2]->setText("-");
      _fadeValueL[2]->setText("-");
      _enable_lfo_column(false, 2);
      _activeLFO[2] = false;
      _legendBoxL[2]->clear();
    }

  } else {  // Drumset
    std::string name((char *) _emulator->get_param_ptr(EmuSC::DrumParam::DrumsMapName,
                                                       rhythm - 1), 12);

    QString title("Drumset: " + QString(name.c_str()));
    _chart->setTitle(title);

    // TODO: Update title and active LFOs whenever a new drum note is played
    //       This is the only way to give a good & correct view of drum LFOs
  }
}


void LFODialog::_enable_lfo_column(bool enable, int lfo)
{
  if (lfo < 0 || lfo > 3)
    return;

  _legendL[lfo]->setEnabled(enable);

  _waveformL[1]->setEnabled(enable);
  _rateL[lfo]->setEnabled(enable);
  _delayL[lfo]->setEnabled(enable);
  _fadeL[lfo]->setEnabled(enable);

  _waveformNameL[lfo]->setEnabled(enable);
  _rateValueL[lfo]->setEnabled(enable);
  _delayValueL[lfo]->setEnabled(enable);
  _fadeValueL[lfo]->setEnabled(enable);
}


QString LFODialog::_get_qstring_from_waveform(uint8_t waveform)
{
  switch(waveform)
    {
    case 0: return QString("Sine");
    case 1: return QString("Square");
    case 2: return QString("Sawtooth");
    case 3: return QString("Triangle");
    case 8: return QString("Sample & Hold");
    case 9: return QString("Random");
    }

  return QString("Unknown waveform");
}


void LFODialog::_part_changed(int partId)
{
  if (partId == _selectedPart)
    _update_instrument_info();
}


void LFODialog::_set_waveform_image(int waveform, QLabel *label)
{
  switch (waveform)
    {
    case 0:
      label->setPixmap(_sinePM.scaled(25, 25,
                                      Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation));
      break;
    case 1:
      label->setPixmap(_squarePM.scaled(25, 25,
                                        Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation));
      break;

    case 2:
      label->setPixmap(_sawtoothPM.scaled(25, 25,
                                          Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation));
      break;

    case 3:
      label->setPixmap(_trianglePM.scaled(25, 25,
                                          Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation));
      break;

    case 8:
      label->setPixmap(_sampleHoldPM.scaled(25, 25,
                                            Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation));
      break;

    case 9:
      label->setPixmap(_randomPM.scaled(25, 25,
                                        Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation));
      break;

    default:
      label->clear();
    }
}


QPixmap LFODialog::_invert_pixmap_color(const QPixmap &pixmap)
{
  QPixmap newPixmap(pixmap.size());
  newPixmap.fill(Qt::transparent);

  QPainter painter(&newPixmap);
  painter.drawPixmap(0, 0, pixmap);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.fillRect(pixmap.rect(), Qt::white);
  painter.end();

  return newPixmap;
  }


#endif  // __USE_QTCHARTS__
