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


#ifndef PART_LIST_DIALOG_H
#define PART_LIST_DIALOG_H


#include "emulator.h"
#include "scene.h"

#include <QDialog>
#include <QCheckBox>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QString>


class PartListDialog : public QDialog
{
  Q_OBJECT

public:
  PartListDialog(Emulator *emulator, Scene *scene, QWidget *parent = nullptr);
  virtual ~PartListDialog();

private:
  Emulator *_emulator;
  Scene *_scene;

  QLineEdit *_instNameQLE[16];

  QCheckBox *_muteCB[16];

  QSpinBox *_volumeSB[16];
  QSpinBox *_panSB[16];
  QSpinBox *_reverbSB[16];
  QSpinBox *_chorusSB[16];

  QSlider *_volumeS[16];
  QSlider *_panS[16];
  QSlider *_reverbS[16];
  QSlider *_chorusS[16];

  void _set_level(int partId, int value);
  void _set_pan(int partId, int value);
  void _set_reverb(int partId, int value);
  void _set_chorus(int partId, int value);

public slots:
  void accept(void);

  void update_part(int partId);
  
};




#endif // PART_LIST_DIALOG_H
