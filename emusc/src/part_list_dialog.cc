/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2026  Håkon Skjelten
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


#include "part_list_dialog.h"

#include "emusc/control_rom.h"

#include <iostream>

#include <QFont>
#include <QFontDatabase>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>


PartListDialog::PartListDialog(Emulator *emulator, Scene *scene, QWidget *parent)
  : QDialog{parent},
    _emulator(emulator),
    _scene(scene)
{
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  QGridLayout *partLayout = new QGridLayout;

  partLayout->addWidget(new QLabel("Part"), 0, 0, Qt::AlignCenter);
  partLayout->addWidget(new QLabel("Instrument"), 0, 1, Qt::AlignCenter);
  partLayout->addWidget(new QLabel("Level"), 0, 2, Qt::AlignCenter);
  partLayout->addWidget(new QLabel("Pan"), 0, 3, Qt::AlignCenter);
  partLayout->addWidget(new QLabel("Reverb"), 0, 4, Qt::AlignCenter);
  partLayout->addWidget(new QLabel("Chorus"), 0, 5, Qt::AlignCenter);

  QFrame* line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  line->setLineWidth(1);
  partLayout->addWidget(line, 1, 0, 1, 6);
//  partLayout->setRowStretch(1, 0);

  for (int i = 0; i < 16; i++) {
    partLayout->addWidget(new QLabel(QString::number(i + 1)), i + 2, 0, Qt::AlignCenter);

    _instNameQLE[i] = new QLineEdit("");
    _instNameQLE[i]->setMaxLength(12);
 
//    QFontMetrics fm(_instNameQLE[i]->font());
//    int width = 12 * fm.horizontalAdvance('x') +
//              _instNameQLE[i]->frameSize().width() - _instNameQLE[i]->width();
//    _instNameQLE[i]->setFixedWidth(width);
    partLayout->addWidget(_instNameQLE[i], i + 2, 1);

    // Volume
    _muteCB[i] = new QCheckBox();
    _volumeSB[i] = new QSpinBox();
    _volumeSB[i]->setRange(0, 127);
    _volumeSB[i]->setButtonSymbols(QAbstractSpinBox::NoButtons);
    _volumeS[i] = new QSlider(Qt::Horizontal);
    _volumeS[i]->setRange(0, 127);
    QHBoxLayout *volLayout = new QHBoxLayout;
//    volLayout->addWidget(_muteCB[i]);
    volLayout->addWidget(_volumeSB[i]);
    volLayout->addWidget(_volumeS[i]);
    partLayout->addLayout(volLayout, i + 2, 2);
    connect(_volumeS[i], &QSlider::valueChanged, _volumeSB[i], &QSpinBox::setValue);
    connect(_volumeSB[i], QOverload<int>::of(&QSpinBox::valueChanged), _volumeS[i], &QSlider::setValue);
    connect(_volumeS[i], &QSlider::valueChanged, [=](int value)
    { _set_level(i, value); });

    // Pan
    _panSB[i] = new QSpinBox();
    _panSB[i]->setRange(-64, 63);
    _panSB[i]->setButtonSymbols(QAbstractSpinBox::NoButtons);
    _panS[i] = new QSlider(Qt::Horizontal);
    _panS[i]->setRange(-64, 63);
    QHBoxLayout *panLayout = new QHBoxLayout;
    panLayout->addWidget(_panSB[i]);
    panLayout->addWidget(_panS[i]);
    partLayout->addLayout(panLayout, i + 2, 3);
    connect(_panS[i], &QSlider::valueChanged, _panSB[i], &QSpinBox::setValue);
    connect(_panSB[i], QOverload<int>::of(&QSpinBox::valueChanged), _panS[i], &QSlider::setValue);
    connect(_panS[i], &QSlider::valueChanged, [=](int value)
    { _set_pan(i, value); });

    // Reverb
    _reverbSB[i] = new QSpinBox();
    _reverbSB[i]->setRange(0, 127);
    _reverbSB[i]->setButtonSymbols(QAbstractSpinBox::NoButtons);
    _reverbS[i] = new QSlider(Qt::Horizontal);
    _reverbS[i]->setRange(0, 127);
    QHBoxLayout *revLayout = new QHBoxLayout;
    revLayout->addWidget(_reverbSB[i]);
    revLayout->addWidget(_reverbS[i]);
    partLayout->addLayout(revLayout, i + 2, 4);
    connect(_reverbS[i], &QSlider::valueChanged, _reverbSB[i], &QSpinBox::setValue);
    connect(_reverbSB[i], QOverload<int>::of(&QSpinBox::valueChanged), _reverbS[i], &QSlider::setValue);
    connect(_reverbS[i], &QSlider::valueChanged, [=](int value)
    { _set_reverb(i, value); });

    //Chorus
    _chorusSB[i] = new QSpinBox();
    _chorusSB[i]->setRange(0, 127);
    _chorusSB[i]->setButtonSymbols(QAbstractSpinBox::NoButtons);
    _chorusS[i] = new QSlider(Qt::Horizontal);
    _chorusS[i]->setRange(0, 127);
    QHBoxLayout *choLayout = new QHBoxLayout;
    choLayout->addWidget(_chorusSB[i]);
    choLayout->addWidget(_chorusS[i]);
    partLayout->addLayout(choLayout, i + 2, 5);
    connect(_chorusS[i], &QSlider::valueChanged, _chorusSB[i], &QSpinBox::setValue);
    connect(_chorusSB[i], QOverload<int>::of(&QSpinBox::valueChanged), _chorusS[i], &QSlider::setValue);
    connect(_chorusS[i], &QSlider::valueChanged, [=](int value)
    { _set_chorus(i, value); });

    update_part(i);
  }

  mainLayout->addLayout(partLayout);
  mainLayout->addWidget(buttonBox);

  setLayout(mainLayout);

  setWindowTitle(tr("Part List Dialog"));
  setModal(false);
//  resize(500, 600);

  connect(_emulator, &Emulator::part_changed, this, &PartListDialog::update_part);
  show();
}


PartListDialog::~PartListDialog()
{}


void PartListDialog::accept()
{
  delete this;
}


void PartListDialog::update_part(int partId)
{
  if (!_emulator || partId < 0 || partId >= 16)
    return;

  // Find instrument names
  uint8_t rhythm = _emulator->get_param(EmuSC::PatchParam::UseForRhythm,partId);
  if (!rhythm) {
    uint8_t *tone = _emulator->get_param_ptr(EmuSC::PatchParam::ToneNumber, partId);
    EmuSC::ControlRom::Instrument &iRom = _emulator->get_instrument_rom(tone[0], tone[1]);
    _instNameQLE[partId]->setText(iRom.name.c_str());

  } else {
    std::string name((char *) _emulator->get_param_ptr(EmuSC::DrumParam::DrumsMapName, rhythm - 1), 12);
    _instNameQLE[partId]->setText(name.c_str());
  }
  
  _volumeS[partId]->setValue(_emulator->get_param(EmuSC::PatchParam::PartLevel, partId));
  _panS[partId]->setValue(_emulator->get_param(EmuSC::PatchParam::PartPanpot, partId) - 0x40);
  _reverbS[partId]->setValue(_emulator->get_param(EmuSC::PatchParam::ReverbSendLevel, partId));
  _chorusS[partId]->setValue(_emulator->get_param(EmuSC::PatchParam::ChorusSendLevel, partId));
}


void PartListDialog::_set_level(int partId, int value)
{
  _emulator->set_param(EmuSC::PatchParam::PartLevel, (uint8_t) value, partId);
}

void PartListDialog::_set_pan(int partId, int value)
{
  _emulator->set_param(EmuSC::PatchParam::PartPanpot, (uint8_t) value + 0x40,
		       partId);
}

void PartListDialog::_set_reverb(int partId, int value)
{
  _emulator->set_param(EmuSC::PatchParam::ReverbSendLevel, (uint8_t) value,
		       partId);
}

void PartListDialog::_set_chorus(int partId, int value)
{
  _emulator->set_param(EmuSC::PatchParam::ChorusSendLevel, (uint8_t) value,
		       partId);
}
