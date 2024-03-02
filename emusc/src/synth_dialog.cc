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


#include "synth_dialog.h"

#include "emusc/control_rom.h"

#include <cmath>
#include <iostream>

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QApplication>
#include <QRegularExpressionValidator>


SynthDialog::SynthDialog(Emulator *emulator, Scene *scene, QWidget *parent)
  : QDialog{parent},
    _partId(0),
    _emulator(emulator),
    _scene(scene)
{  
  _masterSettings = new MasterSettings(emulator);
  _reverbSettings = new ReverbSettings(emulator);
  _chorusSettings = new ChorusSettings(emulator);
  _partMainSettings = new PartMainSettings(emulator, _partId);
  _partRxModeSettings = new PartRxModeSettings(emulator, _partId);
  _partToneSettings = new PartToneSettings(emulator, _partId);
  _partScaleSettings = new PartScaleSettings(emulator, _partId);
  _partControllerSettings = new PartControllerSettings(emulator, _partId);
  _drumSettings = new DrumSettings(emulator);
  _displaySettings = new DisplaySettings(emulator);

  _stack = new QStackedWidget();
  _stack->addWidget(_masterSettings);
  _stack->addWidget(_reverbSettings);
  _stack->addWidget(_chorusSettings);
  _stack->addWidget(_partMainSettings);
  _stack->addWidget(_partRxModeSettings);
  _stack->addWidget(_partToneSettings);
  _stack->addWidget(_partScaleSettings);
  _stack->addWidget(_partControllerSettings);
  _stack->addWidget(_drumSettings);
  _stack->addWidget(_displaySettings);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  QHBoxLayout *settingsLayout = new QHBoxLayout;

  _menuList = new QListWidget();
  _menuList->setMinimumHeight(100);

  _masterLW = new QListWidgetItem(QIcon(":/images/master.png"),
				  tr("Master"), _menuList);
  _menuList->addItem(_masterLW);
  _reverbLW = new QListWidgetItem(QIcon(":/images/reverb.png"),
				  tr("Reverb"), _menuList);
  _menuList->addItem(_reverbLW);
  _chorusLW = new QListWidgetItem(QIcon(":/images/chorus.png"),
				  tr("Chorus"), _menuList);
  _menuList->addItem(_chorusLW);
  _partMainLW = new QListWidgetItem(QIcon(":/images/part.png"),
				    tr("Part: Main"), _menuList);
  _menuList->addItem(_partMainLW);
  _partRxLW = new QListWidgetItem(QIcon(":/images/rx.png"),
				  tr("Part: Rx & Mode"), _menuList);
  _menuList->addItem(_partRxLW);
  _partTonesLW = new QListWidgetItem(QIcon(":/images/tone.png"),
				     tr("Part: Tones"), _menuList);
  _menuList->addItem(_partTonesLW);
  _partScaleLW = new QListWidgetItem(QIcon(":/images/scale.png"),
				     tr("Part: Scale Tuning"), _menuList);
  _menuList->addItem(_partScaleLW);
  _partCtrlLW = new QListWidgetItem(QIcon(":/images/controller.png"),
				    tr("Part: Controllers"), _menuList);
  _menuList->addItem(_partCtrlLW);
  _drumsLW = new QListWidgetItem(QIcon(":/images/drum.png"),
				 tr("Drums"), _menuList);
  _menuList->addItem(_drumsLW);
  _displayLW =new QListWidgetItem(QIcon(":/images/display.png"),
				  tr("Display"), _menuList);
  _menuList->addItem(_displayLW);

  _menuList->setFixedWidth(_menuList->sizeHintForColumn(0) + 10 +
			 _menuList->frameWidth() * 2);
  settingsLayout->addWidget(_menuList, 0);
  settingsLayout->addWidget(_stack, 1);

  connect(_menuList, SIGNAL(currentRowChanged(int)),
	  _stack, SLOT(setCurrentIndex(int)));
  connect(_stack, SIGNAL(currentChanged(int)),
	  this, SLOT(_new_stack_item_focus(int)));

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Help  |
						     QDialogButtonBox::Reset |
						     QDialogButtonBox::Ok);
  connect(buttonBox, &QDialogButtonBox::helpRequested,
	  this, &SynthDialog::_display_help);
  connect(buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()),
	  this, SLOT(_reset()));
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

  mainLayout->addLayout(settingsLayout);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  setWindowTitle(tr("Synth settings"));
  setModal(true);
  resize(600, 500);

  // Increase height for _menuList items
  for(int i = 0; i < _menuList->count(); i++)
    _menuList->item(i)->setSizeHint(QSize(0, 35));
}


SynthDialog::~SynthDialog()
{
}


void SynthDialog::_reset(void)
{
  if (_menuList->currentItem() == _masterLW)
    _masterSettings->reset();
  else if (_menuList->currentItem() == _reverbLW)
    _reverbSettings->reset();
  else if (_menuList->currentItem() == _chorusLW)
    _chorusSettings->reset();
  else if (_menuList->currentItem() == _partMainLW)
    _partMainSettings->reset();
  else if (_menuList->currentItem() == _partRxLW)
    _partRxModeSettings->reset();
  else if (_menuList->currentItem() == _partTonesLW)
    _partToneSettings->reset();
  else if (_menuList->currentItem() == _partScaleLW)
    _partScaleSettings->reset();
  else if (_menuList->currentItem() == _partCtrlLW)
    _partControllerSettings->reset();
  else if (_menuList->currentItem() == _drumsLW)
    _drumSettings->reset();
  else if (_menuList->currentItem() == _displayLW)
    _displaySettings->reset();
  else
    std::cerr << "EmuSC: Internal error, reset unkown menu widget" << std::endl;
}


void SynthDialog::keyPressEvent(QKeyEvent *keyEvent)
{
  if (keyEvent->key() != Qt::Key_Space)
    QApplication::sendEvent(_scene, keyEvent);
}


void SynthDialog::keyReleaseEvent(QKeyEvent *keyEvent)
{
  if (keyEvent->key() != Qt::Key_Space)
    QApplication::sendEvent(_scene, keyEvent);
}


// Update all stacked "part" items when they get focus to use same part id
// TODO: There has to be a better way to do this
void SynthDialog::_new_stack_item_focus(int index)
{
  if (index == 3) {
    ((PartMainSettings *) _stack->currentWidget())->update_all_widgets();
  } else if (index == 4) {
    ((PartRxModeSettings *) _stack->currentWidget())->update_all_widgets();
  } else if (index == 5) {
    ((PartToneSettings *) _stack->currentWidget())->update_all_widgets();
  } else if (index == 6) {
    ((PartScaleSettings *) _stack->currentWidget())->update_all_widgets();
  } else if (index == 7) {
    ((PartControllerSettings *) _stack->currentWidget())->update_all_widgets();
  }
}


void SynthDialog::accept()
{
  delete this;
}


void SynthDialog::_display_help()
{
  QMessageBox helpBox;

  switch(_menuList->currentRow())
    {
    case 0: // Master settings
      helpBox.setText("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:14pt; font-weight:bold;\">Help: Master settings</font>"
		  "<p>"
		  "  <b>Volume</b> Master volume attenuation that is added to "
		  "all parts in adittion to part volume attenuation (0 - 127)"
		  "</p><p>"
		  "  <b>Pan</b> Pan, also called panpot, is stereo positioning "
		  "of sounds from left to right. This setting is added "
		  "to all instruments and all parts in adittion to instruments'"
		  " predefined pan and part's own pan settings. (-64 - 64)"
		  "</p><p>"
		  "  <b>Key shift</b> Shift all notes a whole number of keys "
		  "up or down for all parts. Typically used if you want to "
		  "use multiple synths at different pitch settings (-24 - 24)"
		  "</p><p>"
		  "  <b>Tune</b> Slight shift in pitch that affects all notes "
		  "in all parts. (-100 - 100 cents)"
		  "</p><p>"
		  "  <b>Device ID</b> This ID is used when receiving SysEx "
		  "messages separate multiple synths that are connected to the "
		  "same sequencer. (0 - 31)"
		  "</p><p>"
		  "  <b>Rx SysEx</b> If checked the synth will accept 'SysEx' "
		  "MIDI messages."
		  "</p><p>"
		  "  <b>Rx GM on</b> If checked the synth will accept 'GM on' "
		  "MIDI messages. This message sets certain default values for "
		  "the synth. See the owner's manual for more information."
		  "</p><p>"
		  "  <b>Rx GS reset</b> If checked the synth will accept 'GS "
		  "reset' MIDI messages. This will reset all settings to "
		  "default settings"
		  "</p><p>"
		  "  <b>Rx Instrument Change</b> If checked the synth will accept "
		  "MIDI messages for chaning the instrument used for selected "
		  "parts"
		  "</p><p>"
		  "  <b>Rx Function Control</b>"
		  "</p></body></html>");
      break;

    case 1: // Reverb settings
      helpBox.setText("<html><head><body style=\" white-space: pre-wrap; font-family:Sans Serif; font-style:normal; text-decoration:none;\"><font style=\"font-size:14pt; font-weight:bold;\">Help: Reverb settings</font>"
		  "<p>"
		  "  <b>Preset</b> Specifies which of the 8 avialble reverb "
		  "types to use.<ul>"
		      " <li><i>Room 1 - 3</i>: Simulates reverbation of a room</li>"
		      " <li><i>Hall 1 - 2</i>: Simulates reverbation of a concert hall</li>"
		      " <li><i>Plate</i>: Simulates metal plate reverb</li>"
		      " <li><i>Delay</i>: Produces echo effects</li>"
		      " <li><i>Panning Delay</i>: Produces echo with stereo positioning effect</li></ul>"
		      "Changing reverb type will alter the other paramters below to default values for each type. These can be altered afterwards to tune the reverb effect."
		      "</p><p>"
		      "<b>Character</b> Specifies the reverb type (same as Type)"
		      "</p><p>"
		      "<b>Pre-reverb LP filter</b> Low pass filter applied before reverb function. Higher value means lower cut off frequnecy."
		      "</p><p>"
		      "<b>Level</b> Amount of reverb"
		      "</p><p>"
		      "<b>Time</b> Time duration for reverbation"
		      "</p><p>"
		      "<b>Pre-delay time</b>"
		      ""
		      "</p></body></html>");
      break;
//    case 2: // Chorus settings
//      break;
      
    default:
      helpBox.setText("No help written yet for this section. "
		      "Feel free to write & submit!");
      break;
    }
  
  helpBox.exec();
}


MasterSettings::MasterSettings(Emulator *emulator, QWidget *parent)
  : _emulator(emulator)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("Master Settings");
  QFont f( "Arial", 12, QFont::Bold);
  headerL->setFont( f);
  vboxLayout->addWidget(headerL);
  
  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(new QLabel("Volume"), 0, 0);
  gridLayout->addWidget(new QLabel("Pan"), 1, 0);
  gridLayout->addWidget(new QLabel("Key Shift"), 2, 0);
  gridLayout->addWidget(new QLabel("Tune"), 3, 0);

  _volumeS = new QSlider(Qt::Horizontal);
  _volumeS->setRange(0, 127);
  _volumeS->setTickPosition(QSlider::TicksBelow);
  _volumeS->setTickInterval(64);
  _volumeS->setValue(emulator->get_param(EmuSC::SystemParam::Volume));
  _volumeS->setToolTip("Master Volume [0-127]");

  _panS = new QSlider(Qt::Horizontal);
  _panS->setRange(-64, 63);
  _panS->setTickPosition(QSlider::TicksBelow);
  _panS->setTickInterval(64);
  _panS->setValue(emulator->get_param(EmuSC::SystemParam::Pan) - 0x40);
  _panS->setToolTip("Master Pan: Rnd, -63 - 64");

  _keyShiftS = new QSlider(Qt::Horizontal);
  _keyShiftS->setRange(-24, 24);
  _keyShiftS->setTickPosition(QSlider::TicksBelow);
  _keyShiftS->setTickInterval(1);
  _keyShiftS->setValue(emulator->get_param(EmuSC::SystemParam::KeyShift) -0x40);
  _keyShiftS->setToolTip("Master Key Shift: -24 - 24 [semitones]");

  _tuneS = new QSlider(Qt::Horizontal);
  _tuneS->setRange(-1000, 1000);
  _tuneS->setTickPosition(QSlider::TicksBelow);
  _tuneS->setTickInterval(1000);
  _tuneS->setValue(emulator->get_param_32nib(EmuSC::SystemParam::Tune)- 0x400);
  _tuneS->setToolTip("Master Tune: -100 - 100 [cent] / 415.3 - 466.2 [Hz]");

  _volumeL = new QLabel();
  _panL = new QLabel();
  _keyShiftL = new QLabel();
  _tuneL = new QLabel();
  _tuneHzL = new QLabel();

  _volumeL->setText(": " + QString::number(_volumeS->value()));
  if (_panS->value() < 0)
    _panL->setText(": L" + QString::number(std::abs(_panS->value())));
  else if (_panS->value() > 0)
    _panL->setText(": R" + QString::number(std::abs(_panS->value())));
  else
    _panL->setText(": 0");
  _keyShiftL->setText(": " + QString::number(_keyShiftS->value()));
  _tuneL->setText(": " + QString::number((float) (_tuneS->value() / 10.0),
					 'f', 1));
  float tuneHz = 440.0 * exp(log(2) * _tuneS->value() / 12000);
  _tuneHzL->setText(": " + QString::number((float) tuneHz, 'f', 1));

  connect(_volumeS, SIGNAL(valueChanged(int)), this,SLOT(_volume_changed(int)));
  connect(_panS, SIGNAL(valueChanged(int)), this, SLOT(_pan_changed(int)));
  connect(_keyShiftS, SIGNAL(valueChanged(int)),
	  this, SLOT(_keyShift_changed(int)));
  connect(_tuneS, SIGNAL(valueChanged(int)), this, SLOT(_tune_changed(int)));

  // Set static width for slider value labels
  QFontMetrics fontMetrics(_tuneL->font());
  int labelWidth = fontMetrics.horizontalAdvance(": -888.8");
  _tuneL->setFixedWidth(labelWidth);

  gridLayout->addWidget(_volumeL,   0, 1);
  gridLayout->addWidget(_panL,      1, 1);
  gridLayout->addWidget(_keyShiftL, 2, 1);
  gridLayout->addWidget(_tuneL,     3, 1);
  gridLayout->addWidget(_tuneHzL,     4, 1);
  gridLayout->addWidget(_volumeS,   0, 2);
  gridLayout->addWidget(_panS,      1, 2);
  gridLayout->addWidget(_keyShiftS, 2, 2);
  gridLayout->addWidget(_tuneS,     3, 2);

  QGridLayout *gridLayout2 = new QGridLayout();
  gridLayout2->addWidget(new QLabel("Device ID"), 0, 0);

  _deviceIdC = new QComboBox();
  for (int i = 1; i < 33; i++)
    _deviceIdC->addItem(QString::number(i));

  _deviceIdC->setCurrentIndex(emulator->get_param(EmuSC::SystemParam::DeviceID)
			      - 1);
  _deviceIdC->setEditable(false);
  _deviceIdC->setToolTip("SysEx Device ID");

  gridLayout2->addWidget(_deviceIdC, 0, 1);
  gridLayout2->setColumnStretch(2, 1);

  connect(_deviceIdC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_device_id_changed(int)));  

  _rxSysExCh = new QCheckBox("Rx SysEx");
  _rxGSResetCh = new QCheckBox("Rx GS reset");
  _rxInstChgCh = new QCheckBox("Rx Instrument change");
  _rxFuncCtrlCh = new QCheckBox("Rx Function Control");

  _rxSysExCh->setChecked(emulator->get_param(EmuSC::SystemParam::RxSysEx));
  _rxGSResetCh->setChecked(emulator->get_param(EmuSC::SystemParam::RxGSReset));
  _rxInstChgCh->setChecked(emulator->get_param(EmuSC::SystemParam::RxInstrumentChange));
  _rxFuncCtrlCh->setChecked(emulator->get_param(EmuSC::SystemParam::RxFunctionControl));

  connect(_rxSysExCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxSysEx_changed(int)));
  connect(_rxGSResetCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxGSReset_changed(int)));
  connect(_rxInstChgCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxInstChg_changed(int)));
  connect(_rxFuncCtrlCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxFuncCtrl_changed(int)));

  QGridLayout *gridLayout3 = new QGridLayout();
  gridLayout3->addWidget(_rxSysExCh,    7, 0);
  gridLayout3->addWidget(_rxGSResetCh,  8, 0);
  gridLayout3->addWidget(_rxInstChgCh,  7, 1);
  gridLayout3->addWidget(_rxFuncCtrlCh, 8, 1);

  gridLayout3->setHorizontalSpacing(50);
  gridLayout3->setColumnStretch(2, 1);

  // Additional widgets for later generations
  if (_emulator->get_synth_generation() >=EmuSC::ControlRom::SynthGen::SC55mk2){
    _rxGMOnCh = new QCheckBox("Rx GM on");
    _rxGMOnCh->setChecked(emulator->get_param(EmuSC::SystemParam::RxGMOn));
    connect(_rxGMOnCh, SIGNAL(stateChanged(int)),
	    this, SLOT(_rxGMOn_changed(int)));
    gridLayout3->addWidget(_rxGMOnCh,     9, 0);
  }

  vboxLayout->addLayout(gridLayout);
  vboxLayout->addLayout(gridLayout2);
  vboxLayout->addLayout(gridLayout3);
  vboxLayout->insertSpacing(1, 15);
  vboxLayout->insertSpacing(3, 15);
  vboxLayout->insertSpacing(5, 15);
  vboxLayout->addStretch(0);
  setLayout(vboxLayout);
}


void MasterSettings::reset(void)
{
  std::cout << "RESET" << std::endl;
}


void MasterSettings::_volume_changed(int value)
{
  _volumeL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::SystemParam::Volume, (uint8_t) value);
  _emulator->update_LCD_display();
}


void MasterSettings::_pan_changed(int value)
{
  QString labelText(": ");

  if (value < 0)
    labelText.append("L" + QString::number(std::abs(value)));
  else if (value > 0)
    labelText.append("R" + QString::number(std::abs(value)));
  else
    labelText.append("0");

  _panL->setText(labelText);
  _emulator->set_param(EmuSC::SystemParam::Pan, (uint8_t) (value + 0x40));
  _emulator->update_LCD_display();
}


void MasterSettings::_keyShift_changed(int value)
{
  _keyShiftL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::SystemParam::KeyShift, (uint8_t) (value + 0x40));
  _emulator->update_LCD_display();
}


void MasterSettings::_tune_changed(int value)
{
  float tuneHz = 440.0 * exp(log(2) * value / 12000);
  _tuneL->setText(": " + QString::number((float) (value / 10.0), 'f', 1));
  _tuneHzL->setText(": " + QString::number((float) tuneHz, 'f', 1));
  _emulator->set_param_32nib(EmuSC::SystemParam::Tune, (uint16_t) value + 1024);
}

void MasterSettings::_rxSysEx_changed(int value)
{
  _emulator->set_param(EmuSC::SystemParam::RxSysEx, (uint8_t) value);
}


void MasterSettings::_device_id_changed(int value)
{
  _emulator->set_param(EmuSC::SystemParam::DeviceID, (uint8_t) (value + 1));
}


void MasterSettings::_rxGMOn_changed(int value)
{
  _emulator->set_param(EmuSC::SystemParam::RxGMOn, (uint8_t) value);
}


void MasterSettings::_rxGSReset_changed(int value)
{
  _emulator->set_param(EmuSC::SystemParam::RxGSReset, (uint8_t) value);
}


void MasterSettings::_rxInstChg_changed(int value)
{
  _emulator->set_param(EmuSC::SystemParam::RxInstrumentChange, (uint8_t) value);
}


void MasterSettings::_rxFuncCtrl_changed(int value)
{
  _emulator->set_param(EmuSC::SystemParam::RxFunctionControl, (uint8_t) value);
}


ReverbSettings::ReverbSettings(Emulator *emulator, QWidget *parent)
  : _emulator(emulator)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("Reverb Settings");
  QFont f( "Arial", 12, QFont::Bold);
  headerL->setFont( f);
  vboxLayout->addWidget(headerL);

  QGridLayout *gridLayout1 = new QGridLayout();
  gridLayout1->addWidget(new QLabel("Preset"), 0, 0);
  gridLayout1->addWidget(new QLabel("Character"), 1, 0);
  _presetC = new QComboBox();
  _presetC->addItems({ "Room 1", "Room 2", "Room 3", "Hall 1", "Hall 2",
                       "Plate", "Delay", "Panning Delay" });
  gridLayout1->addWidget(_presetC, 0, 1);
  _characterC = new QComboBox();
  _characterC->addItems({ "Room 1", "Room 2", "Room 3", "Hall 1", "Hall 2",
                          "Plate", "Delay", "Panning Delay" });
  gridLayout1->addWidget(_characterC, 1, 1);
  gridLayout1->addWidget(new QLabel(""), 0, 2);
  gridLayout1->setColumnStretch(2, 1);

  QLabel *spacerL = new QLabel();
  spacerL->setFixedHeight(8);

  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(new QLabel("Level"),             0, 0);
  gridLayout->addWidget(spacerL,                         1, 0);
  gridLayout->addWidget(new QLabel("Pre-LP filter"),     2, 0);
  gridLayout->addWidget(new QLabel("Reverb Time"),       3, 0);
  gridLayout->addWidget(new QLabel("Delay Feedback"),    4, 0);
//  gridLayout->addWidget(new QLabel("Pre-delay time"),       6, 0);

  _levelL = new QLabel(": ");
  _filterL = new QLabel(": ");
  _timeL = new QLabel(": ");
  _feedbackL = new QLabel(": ");
//  _delayL = new QLabel(": ");

  gridLayout->addWidget(_levelL,    0, 1);
  gridLayout->addWidget(_filterL,   2, 1);
  gridLayout->addWidget(_timeL,     3, 1);
  gridLayout->addWidget(_feedbackL, 4, 1);
//  gridLayout->addWidget(_delayL,    6, 1);
  
  // Set static width for slider value labels
  QFontMetrics fontMetrics(_levelL->font());
  int labelWidth = fontMetrics.horizontalAdvance(": 888");
  _levelL->setFixedWidth(labelWidth);

  _levelS = new QSlider(Qt::Horizontal);
  _levelS->setRange(0, 127);
  _levelS->setTickPosition(QSlider::TicksBelow);
  _levelS->setTickInterval(64);

  _filterS = new QSlider(Qt::Horizontal);
  _filterS->setRange(0, 7);
  _filterS->setTickPosition(QSlider::TicksBelow);
  _filterS->setTickInterval(1);

  _timeS = new QSlider(Qt::Horizontal);
  _timeS->setRange(0, 127);
  _timeS->setTickPosition(QSlider::TicksBelow);
  _timeS->setTickInterval(64);

  _feedbackS = new QSlider(Qt::Horizontal);
  _feedbackS->setRange(0, 127);
  _feedbackS->setTickPosition(QSlider::TicksBelow);
  _feedbackS->setTickInterval(64);

// SC-88
//  _delayS = new QSlider(Qt::Horizontal);
//  _delayS->setRange(0, 127);
//  _delayS->setTickPosition(QSlider::TicksBelow);
//  _delayS->setTickInterval(1);

  gridLayout->addWidget(_levelS,    0, 2);
  gridLayout->addWidget(_filterS,   2, 2);
  gridLayout->addWidget(_timeS,     3, 2);
  gridLayout->addWidget(_feedbackS, 4, 2);
//  gridLayout->addWidget(_delayS,    6, 2);

  update_all_widgets();

  connect(_presetC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_preset_changed(int)));
  connect(_characterC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_character_changed(int)));
  connect(_levelS, SIGNAL(valueChanged(int)),
	  this, SLOT(_level_changed(int)));
  connect(_filterS, SIGNAL(valueChanged(int)),
	  this, SLOT(_filter_changed(int)));
  connect(_timeS, SIGNAL(valueChanged(int)),
	  this, SLOT(_time_changed(int)));
  connect(_feedbackS, SIGNAL(valueChanged(int)),
	  this, SLOT(_feedback_changed(int)));
// connect(_delayS, SIGNAL(valueChanged(int)), this, SLOT(_delay_changed(int)));

  vboxLayout->addLayout(gridLayout1);
  vboxLayout->addLayout(gridLayout);
  vboxLayout->addStretch(0);
  vboxLayout->insertSpacing(1, 15);
  vboxLayout->insertSpacing(3, 15);

  setLayout(vboxLayout);  
}


void ReverbSettings::reset(void)
{
  _preset_changed(4);                   // Default: Hall 2
}


void ReverbSettings::update_all_widgets(void)
{
  _presetC->setCurrentIndex(_emulator->get_param(EmuSC::PatchParam::ReverbMacro));
  _characterC->setCurrentIndex(_emulator->get_param(EmuSC::PatchParam::ReverbCharacter));
  _levelS->setValue(_emulator->get_param(EmuSC::PatchParam::ReverbLevel));
  _filterS->setValue(_emulator->get_param(EmuSC::PatchParam::ReverbPreLPF));
  _timeS->setValue(_emulator->get_param(EmuSC::PatchParam::ReverbTime));
  _feedbackS->setValue(_emulator->get_param(EmuSC::PatchParam::ReverbDelayFeedback));

  _levelL->setText(": " + QString::number(_levelS->value()));
  _filterL->setText(": " + QString::number(_filterS->value()));
  _timeL->setText(": " + QString::number(_timeS->value()));
  _feedbackL->setText(": " + QString::number(_feedbackS->value()));
}


void ReverbSettings::_preset_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::ReverbMacro, (uint8_t) value);
  update_all_widgets();
}


void ReverbSettings::_character_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::ReverbCharacter, (uint8_t) value);
}


void ReverbSettings::_level_changed(int value)
{
  _levelL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ReverbLevel, (uint8_t) value);
  _emulator->update_LCD_display();
}


void ReverbSettings::_filter_changed(int value)
{
  _filterL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ReverbPreLPF, (uint8_t) value);
}


void ReverbSettings::_time_changed(int value)
{
  _timeL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ReverbTime, (uint8_t) value);
}


void ReverbSettings::_feedback_changed(int value)
{
  _feedbackL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ReverbDelayFeedback, (uint8_t) value);
}


// SC-88
//void ReverbSettings::_delay_changed(int value)
//{
//  _delayL->setText(": " + QString::number(value));
//  _emulator->set_param(EmuSC::PatchParam::ReverbPreDelayTime, (uint8_t)value);
//}


ChorusSettings::ChorusSettings(Emulator *emulator, QWidget *parent)
  : _emulator(emulator)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("Chorus Settings");
  QFont f("Arial", 12, QFont::Bold);
  headerL->setFont(f);
  vboxLayout->addWidget(headerL);

  QGridLayout *gridLayout1 = new QGridLayout();
  gridLayout1->addWidget(new QLabel("Preset"), 0, 0);
  _presetC = new QComboBox();
  _presetC->addItems({ "Chorus 1", "Chorus 2", "Chorus 3", "Chorus 4",
                       "Feedback Chorus", "Flanger", "Short Delay",
                       "Short Delay (FB)" });
  gridLayout1->addWidget(_presetC, 0, 1);
  gridLayout1->setColumnStretch(2, 1);

  QLabel *spacerL = new QLabel();
  spacerL->setFixedHeight(8);

  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(new QLabel("Level"),          0, 0);
  gridLayout->addWidget(spacerL,                      1, 0);
  gridLayout->addWidget(new QLabel("Pre-LP filter"),  2, 0);
  gridLayout->addWidget(new QLabel("Feedback"),       3, 0);
  gridLayout->addWidget(new QLabel("Delay"),          4, 0);
  gridLayout->addWidget(new QLabel("Rate"),           5, 0);
  gridLayout->addWidget(new QLabel("Depth"),          6, 0);
  gridLayout->addWidget(new QLabel("Send to reverb"), 7, 0);
//  gridLayout->addWidget(new QLabel("Send to delay"), 8, 0);    // SC-88+

  _levelL = new QLabel(": ");
  _filterL = new QLabel(": ");
  _feedbackL = new QLabel(": ");
  _delayL = new QLabel(": ");
  _rateL = new QLabel(": ");
  _depthL = new QLabel(": ");
  _sendRevL = new QLabel(": ");
//  _sendDlyL = new QLabel(": ");

  gridLayout->addWidget(_levelL,    0, 1);
  gridLayout->addWidget(_filterL,   2, 1);
  gridLayout->addWidget(_feedbackL, 3, 1);
  gridLayout->addWidget(_delayL,    4, 1);
  gridLayout->addWidget(_rateL,     5, 1);
  gridLayout->addWidget(_depthL,    6, 1);
  gridLayout->addWidget(_sendRevL,  7, 1);
//  gridLayout->addWidget(_sendDlyL,  8, 1);

  // Set static width for slider value labels
  QFontMetrics fontMetrics(_levelL->font());
  int labelWidth = fontMetrics.horizontalAdvance(": 888");
  _levelL->setFixedWidth(labelWidth);

  _levelS = new QSlider(Qt::Horizontal);
  _levelS->setRange(0, 127);
  _levelS->setTickPosition(QSlider::TicksBelow);
  _levelS->setTickInterval(64);

  _filterS = new QSlider(Qt::Horizontal);
  _filterS->setRange(0, 7);
  _filterS->setTickPosition(QSlider::TicksBelow);
  _filterS->setTickInterval(1);

  _feedbackS = new QSlider(Qt::Horizontal);
  _feedbackS->setRange(0, 127);
  _feedbackS->setTickPosition(QSlider::TicksBelow);
  _feedbackS->setTickInterval(64);

  _delayS = new QSlider(Qt::Horizontal);
  _delayS->setRange(0, 127);
  _delayS->setTickPosition(QSlider::TicksBelow);
  _delayS->setTickInterval(64);

  _rateS = new QSlider(Qt::Horizontal);
  _rateS->setRange(0, 127);
  _rateS->setTickPosition(QSlider::TicksBelow);
  _rateS->setTickInterval(64);

  _depthS = new QSlider(Qt::Horizontal);
  _depthS->setRange(0, 127);
  _depthS->setTickPosition(QSlider::TicksBelow);
  _depthS->setTickInterval(64);

  _sendRevS = new QSlider(Qt::Horizontal);
  _sendRevS->setRange(0, 127);
  _sendRevS->setTickPosition(QSlider::TicksBelow);
  _sendRevS->setTickInterval(64);

// TODO: Only for SC-88+
//  _sendDlyS = new QSlider(Qt::Horizontal);
//  _sendDlyS->setRange(0, 127);
//  _sendDlyS->setTickPosition(QSlider::TicksBelow);
//  _sendDlyS->setTickInterval(1);

  gridLayout->addWidget(_levelS,    0, 2);
  gridLayout->addWidget(_filterS,   2, 2);
  gridLayout->addWidget(_feedbackS, 3, 2);
  gridLayout->addWidget(_delayS,    4, 2);
  gridLayout->addWidget(_rateS,     5, 2);
  gridLayout->addWidget(_depthS,    6, 2);
  gridLayout->addWidget(_sendRevS,  7, 2);
//  gridLayout->addWidget(_sendDlyS, 8, 2);

  update_all_widgets();

  connect(_presetC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_preset_changed(int)));

  connect(_levelS, SIGNAL(valueChanged(int)),
	  this, SLOT(_level_changed(int)));
  connect(_filterS, SIGNAL(valueChanged(int)),
	  this, SLOT(_filter_changed(int)));
  connect(_feedbackS, SIGNAL(valueChanged(int)),
	  this, SLOT(_feedback_changed(int)));
  connect(_delayS, SIGNAL(valueChanged(int)),
	  this, SLOT(_delay_changed(int)));
  connect(_rateS, SIGNAL(valueChanged(int)),
	  this, SLOT(_rate_changed(int)));
  connect(_depthS, SIGNAL(valueChanged(int)),
	  this, SLOT(_depth_changed(int)));
  connect(_sendRevS, SIGNAL(valueChanged(int)),
	  this, SLOT(_sendRev_changed(int)));

  vboxLayout->addLayout(gridLayout1);
  vboxLayout->addLayout(gridLayout);
  vboxLayout->addStretch(0);
  vboxLayout->insertSpacing(1, 15);
  vboxLayout->insertSpacing(3, 15);

  setLayout(vboxLayout);
}


void ChorusSettings::reset(void)
{
  _preset_changed(2);                   // Default: Chorus 3
}


void ChorusSettings::update_all_widgets(void)
{
  _presetC->setCurrentIndex(_emulator->get_param(EmuSC::PatchParam::ChorusMacro));

  _levelS->setValue(_emulator->get_param(EmuSC::PatchParam::ChorusLevel));
  _filterS->setValue(_emulator->get_param(EmuSC::PatchParam::ChorusPreLPF));
  _feedbackS->setValue(_emulator->get_param(EmuSC::PatchParam::ChorusFeedback));
  _delayS->setValue(_emulator->get_param(EmuSC::PatchParam::ChorusDelay));
  _rateS->setValue(_emulator->get_param(EmuSC::PatchParam::ChorusRate));
  _depthS->setValue(_emulator->get_param(EmuSC::PatchParam::ChorusDepth));
  _sendRevS->setValue(_emulator->get_param(EmuSC::PatchParam::ChorusSendToReverb));

  _levelL->setText(": " + QString::number(_levelS->value()));
  _filterL->setText(": " + QString::number(_filterS->value()));
  _feedbackL->setText(": " + QString::number(_feedbackS->value()));
  _delayL->setText(": " + QString::number(_delayS->value()));
  _rateL->setText(": " + QString::number(_rateS->value()));
  _depthL->setText(": " + QString::number(_depthS->value()));
  _sendRevL->setText(": " + QString::number(_sendRevS->value()));
}


void ChorusSettings::_preset_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::ChorusMacro, (uint8_t) value);
  update_all_widgets();
}


void ChorusSettings::_level_changed(int value)
{
  _levelL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ChorusLevel, (uint8_t) value);
  _emulator->update_LCD_display();
}


void ChorusSettings::_filter_changed(int value)
{
  _filterL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ChorusPreLPF, (uint8_t) value);
}


void ChorusSettings::_feedback_changed(int value)
{
  _feedbackL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ChorusFeedback, (uint8_t) value);
}


void ChorusSettings::_delay_changed(int value)
{
  _delayL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ChorusDelay, (uint8_t) value);
}

void ChorusSettings::_rate_changed(int value)
{
  _rateL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ChorusRate, (uint8_t) value);
}

void ChorusSettings::_depth_changed(int value)
{
  _depthL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ChorusDepth, (uint8_t) value);
}


void ChorusSettings::_sendRev_changed(int value)
{
  _sendRevL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ChorusSendToReverb, (uint8_t) value);
}


PartMainSettings::PartMainSettings(Emulator *emulator, int8_t &partId,
				   QWidget *parent)
  : _emulator(emulator),
    _partId(partId)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("Part Settings: Main");
  QFont f( "Arial", 12, QFont::Bold);
  headerL->setFont( f);
  vboxLayout->addWidget(headerL);

  QHBoxLayout *hboxLayout = new QHBoxLayout();
  hboxLayout->addWidget(new QLabel("Part:"));
  _partC = new QComboBox();
  for (int i = 1; i <= 16; i++)                 // TODO: SC-88 => A1-16 + B1-16
    _partC->addItem(QString::number(i));
  _partC->setEditable(false);  
  hboxLayout->addWidget(_partC);
  hboxLayout->addStretch(1);
  vboxLayout->addLayout(hboxLayout);

  QFrame *line;
  line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  vboxLayout->addWidget(line);

  QHBoxLayout *hboxLayout2 = new QHBoxLayout();
  hboxLayout2->addWidget(new QLabel("MIDI Channel"));

  _midiChC = new QComboBox();
  for (int i = 1; i <= 16; i++)
    _midiChC->addItem(QString::number(i));
  _midiChC->addItem("Off");
  _midiChC->setEditable(false);
  hboxLayout2->addWidget(_midiChC);

  hboxLayout2->addSpacing(50);

  hboxLayout2->addWidget(new QLabel("Instrument Mode"));

  _instModeC = new QComboBox();
  _instModeC->addItems({ "Normal", "Drum1", "Drum2" });
  _instModeC->setEditable(false);
  hboxLayout2->addWidget(_instModeC);
  hboxLayout2->addStretch(1);

  QLabel *spacerL = new QLabel();
  spacerL->setFixedHeight(5);
  
  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(new QLabel("Volume"),          0, 0);
  gridLayout->addWidget(new QLabel("Pan"),             1, 0);
  gridLayout->addWidget(new QLabel("Key Shift"),       2, 0);
  gridLayout->addWidget(new QLabel("Pitch Offset"),    3, 0);
  gridLayout->addWidget(new QLabel("Reverb"),          4, 0);
  gridLayout->addWidget(new QLabel("Chorus"),          5, 0);
//  gridLayout->addWidget(new QLabel("Delay"), 9, 0);  // TODO: SC-88
  gridLayout->addWidget(spacerL,                       6, 0);
  gridLayout->addWidget(new QLabel("Fine Tune"),       7, 0);
  gridLayout->addWidget(new QLabel("Coarse Tune"),     8, 0);
  gridLayout->addWidget(spacerL,                       9, 0);
  gridLayout->addWidget(new QLabel("Velocity Depth"), 10, 0);
  gridLayout->addWidget(new QLabel("Velocity Offset"),11, 0);
  gridLayout->addWidget(spacerL,                      12, 0);
  gridLayout->addWidget(new QLabel("Key Range Low"),  13, 0);
  gridLayout->addWidget(new QLabel("Key Range High"), 14, 0);

  _levelL = new QLabel(": ");
  _panL = new QLabel(": ");
  _keyShiftL = new QLabel(": ");
  _tuneL = new QLabel(": ");
  _reverbL = new QLabel(": ");
  _chorusL = new QLabel(": ");
//  _delayL = new QLabel(": ");
  _fineTuneL = new QLabel(": ");
  _coarseTuneL = new QLabel(": ");
  _velDepthL = new QLabel(": ");
  _velOffsetL = new QLabel(": ");
  _keyRangeLL = new QLabel(": ");
  _keyRangeHL = new QLabel(": ");

  gridLayout->addWidget(_levelL,      0, 1);
  gridLayout->addWidget(_panL,        1, 1);
  gridLayout->addWidget(_keyShiftL,   2, 1);
  gridLayout->addWidget(_tuneL,       3, 1);
  gridLayout->addWidget(_reverbL,     4, 1);
  gridLayout->addWidget(_chorusL,     5, 1);
//  gridLayout->addWidget(_delayL,  9, 1);
  gridLayout->addWidget(_fineTuneL,   7, 1);
  gridLayout->addWidget(_coarseTuneL, 8, 1);
  gridLayout->addWidget(_velDepthL,  10, 1);
  gridLayout->addWidget(_velOffsetL, 11, 1);
  gridLayout->addWidget(_keyRangeLL, 13, 1);
  gridLayout->addWidget(_keyRangeHL, 14, 1);

  // Set static width for slider value labels
  QFontMetrics fontMetrics(_levelL->font());
  int labelWidth = fontMetrics.horizontalAdvance(":188888");
  _levelL->setFixedWidth(labelWidth);

  _levelS = new QSlider(Qt::Horizontal);
  _levelS->setRange(0, 127);
  _levelS->setTickPosition(QSlider::TicksBelow);
  _levelS->setTickInterval(64);
  _levelS->setToolTip("Part Volume: 0 - 127");

  _panS = new QSlider(Qt::Horizontal);
  _panS->setRange(-64, 64);
  _panS->setTickPosition(QSlider::TicksBelow);
  _panS->setTickInterval(64);
  _panS->setToolTip("Part Pan: RND, -63 - 63");

  _keyShiftS = new QSlider(Qt::Horizontal);
  _keyShiftS->setRange(-24, 24);
  _keyShiftS->setTickPosition(QSlider::TicksBelow);
  _keyShiftS->setTickInterval(1);
  _keyShiftS->setToolTip("Part Key Shift: -24 - 24 [semitones]");


  _tuneS = new QSlider(Qt::Horizontal);
  _tuneS->setRange(8, 248);
  _tuneS->setTickPosition(QSlider::TicksBelow);
  _tuneS->setTickInterval(120);
  _tuneS->setToolTip("Pitch Offset Fine: -12 - 12 [Hz]");


  _reverbS = new QSlider(Qt::Horizontal);
  _reverbS->setRange(0, 127);
  _reverbS->setTickPosition(QSlider::TicksBelow);
  _reverbS->setTickInterval(64);
  _reverbS->setToolTip("Reverb Level [0 - 127");

  _chorusS = new QSlider(Qt::Horizontal);
  _chorusS->setRange(0, 127);
  _chorusS->setTickPosition(QSlider::TicksBelow);
  _chorusS->setTickInterval(64);
  _chorusS->setToolTip("Chorus Level [0 - 127");

  _fineTuneS = new QSlider(Qt::Horizontal);
  _fineTuneS->setRange(0, 16383);
  _fineTuneS->setTickPosition(QSlider::TicksBelow);
  _fineTuneS->setTickInterval(8192);
  _fineTuneS->setToolTip("Master Fine Tuning (RPN#1): -100 - 100 [cent]");

  _coarseTuneS = new QSlider(Qt::Horizontal);
  _coarseTuneS->setRange(40, 88);
  _coarseTuneS->setTickPosition(QSlider::TicksBelow);
  _coarseTuneS->setTickInterval(24);
  _coarseTuneS->setToolTip("Master Coarse Tuning (RPN#2): -24 - 24 [semitones]");

  _velDepthS = new QSlider(Qt::Horizontal);
  _velDepthS->setRange(0, 127);
  _velDepthS->setTickPosition(QSlider::TicksBelow);
  _velDepthS->setTickInterval(64);
  _velDepthS->setToolTip("Velocity Depth: 0 - 127");

  _velOffsetS = new QSlider(Qt::Horizontal);
  _velOffsetS->setRange(0, 127);
  _velOffsetS->setTickPosition(QSlider::TicksBelow);
  _velOffsetS->setTickInterval(64);
  _velOffsetS->setToolTip("Velocity Offset: 0 - 127");

  _keyRangeLS = new QSlider(Qt::Horizontal);
  _keyRangeLS->setRange(0, 127);
  _keyRangeLS->setTickPosition(QSlider::TicksBelow);
  _keyRangeLS->setTickInterval(64);
  _keyRangeLS->setToolTip("Keyboard Range Low: 0 (C1) - 127 (G9)");

  _keyRangeHS = new QSlider(Qt::Horizontal);
  _keyRangeHS->setRange(0, 127);
  _keyRangeHS->setTickPosition(QSlider::TicksBelow);
  _keyRangeHS->setTickInterval(64);
  _keyRangeHS->setToolTip("Keyboard Range High: 0 (C1) - 127 (G9)");

  gridLayout->addWidget(_levelS,      0, 2);
  gridLayout->addWidget(_panS,        1, 2);
  gridLayout->addWidget(_keyShiftS,   2, 2);
  gridLayout->addWidget(_tuneS,       3, 2);
  gridLayout->addWidget(_reverbS,     4, 2);
  gridLayout->addWidget(_chorusS,     5, 2);
  gridLayout->addWidget(_fineTuneS,   7, 2);
  gridLayout->addWidget(_coarseTuneS, 8, 2);
  gridLayout->addWidget(_velDepthS,  10, 2);
  gridLayout->addWidget(_velOffsetS, 11, 2);
  gridLayout->addWidget(_keyRangeLS, 13, 2);
  gridLayout->addWidget(_keyRangeHS, 14, 2);

  vboxLayout->addLayout(hboxLayout2);
  vboxLayout->addSpacing(10);
  vboxLayout->addLayout(gridLayout);
  vboxLayout->addStretch(0);
  vboxLayout->insertSpacing(1, 15);
  vboxLayout->insertSpacing(3, 10);
  vboxLayout->insertSpacing(5, 10);

  connect(_partC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_partC_changed(int)));

  connect(_midiChC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_midiCh_changed(int)));
  connect(_instModeC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_instMode_changed(int)));

  connect(_levelS, SIGNAL(valueChanged(int)),
	  this, SLOT(_level_changed(int)));
  connect(_panS, SIGNAL(valueChanged(int)),
	  this, SLOT(_pan_changed(int)));
  connect(_keyShiftS, SIGNAL(valueChanged(int)),
	  this, SLOT(_keyShift_changed(int)));
  connect(_tuneS, SIGNAL(valueChanged(int)),
	  this, SLOT(_tune_changed(int)));
  connect(_reverbS, SIGNAL(valueChanged(int)),
	  this,SLOT(_reverb_changed(int)));
  connect(_chorusS, SIGNAL(valueChanged(int)),
	  this,SLOT(_chorus_changed(int)));
  connect(_fineTuneS, SIGNAL(valueChanged(int)),
	  this, SLOT(_fineTune_changed(int)));
  connect(_coarseTuneS, SIGNAL(valueChanged(int)),
	  this, SLOT(_coarseTune_changed(int)));
  connect(_velDepthS, SIGNAL(valueChanged(int)),
	  this, SLOT(_velDepth_changed(int)));
  connect(_velOffsetS, SIGNAL(valueChanged(int)),
	  this, SLOT(_velOffset_changed(int)));
  connect(_keyRangeLS, SIGNAL(valueChanged(int)),
	  this, SLOT(_keyRangeL_changed(int)));
  connect(_keyRangeHS, SIGNAL(valueChanged(int)),
	  this, SLOT(_keyRangeH_changed(int)));

  update_all_widgets();
  
  setLayout(vboxLayout);
}


void PartMainSettings::reset(void)
{
  std::cout << "RESET" << std::endl;
}

void PartMainSettings::update_all_widgets(void)
{
  _partC->setCurrentIndex(_partId);

  _midiChC->setCurrentIndex(_emulator->get_param(EmuSC::PatchParam::RxChannel,
						_partId));
  _instModeC->setCurrentIndex(_emulator->get_param(EmuSC::PatchParam::UseForRhythm,
						_partId));

  _levelS->setValue(_emulator->get_param(EmuSC::PatchParam::PartLevel,_partId));
  _panS->setValue(_emulator->get_param(EmuSC::PatchParam::PartPanpot, _partId) - 0x40);
  _keyShiftS->setValue(_emulator->get_param(EmuSC::PatchParam::PitchKeyShift, _partId) - 0x40);
  _tuneS->setValue(_emulator->get_param_nib16(EmuSC::PatchParam::PitchOffsetFine, _partId));
  _reverbS->setValue(_emulator->get_param(EmuSC::PatchParam::ReverbSendLevel, _partId));
  _chorusS->setValue(_emulator->get_param(EmuSC::PatchParam::ChorusSendLevel, _partId));
  _fineTuneS->setValue(_emulator->get_param_uint14(EmuSC::PatchParam::PitchFineTune, _partId));
  _coarseTuneS->setValue(_emulator->get_param(EmuSC::PatchParam::PitchCoarseTune, _partId));
  _velDepthS->setValue(_emulator->get_param(EmuSC::PatchParam::VelocitySenseDepth, _partId));
  _velOffsetS->setValue(_emulator->get_param(EmuSC::PatchParam::VelocitySenseOffset, _partId));
  _keyRangeLS->setValue(_emulator->get_param(EmuSC::PatchParam::KeyRangeLow, _partId));
  _keyRangeHS->setValue(_emulator->get_param(EmuSC::PatchParam::KeyRangeHigh, _partId));

  _levelL->setText(": " + QString::number(_levelS->value()));
  _panL->setText(": " + QString::number(_panS->value()));
  _keyShiftL->setText(": " + QString::number(_keyShiftS->value()));
  _tuneL->setText(": " + QString::number((float) (_tuneS->value() - 128) / 10, 'f', 1));
  _reverbL->setText(": " + QString::number(_reverbS->value()));
  _chorusL->setText(": " + QString::number(_chorusS->value()));
  _fineTuneL->setText(": " + QString::number((float)(_fineTuneS->value() - 8192) * 100 / 8192, 'f', 2));
  _coarseTuneL->setText(": " + QString::number(_coarseTuneS->value() - 0x40));
  _velDepthL->setText(": " + QString::number(_velDepthS->value()));
  _velOffsetL->setText(": " + QString::number(_velOffsetS->value()));
  _keyRangeLL->setText(": " + QString::number(_keyRangeLS->value()));
  _keyRangeHL->setText(": " + QString::number(_keyRangeHS->value()));
}


void PartMainSettings::_partC_changed(int value)
{
  _partId = (int8_t) value;
  update_all_widgets();
}


void PartMainSettings::_midiCh_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxChannel, (uint8_t) value, _partId);
  _emulator->update_LCD_display(_partId);
}


void PartMainSettings::_instMode_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::UseForRhythm, (uint8_t) value,
		       _partId);
  _emulator->update_LCD_display(_partId);
}


void PartMainSettings::_level_changed(int value)
{
  _levelL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::PartLevel, (uint8_t) value, _partId);
  _emulator->update_LCD_display(_partId);
}


void PartMainSettings::_pan_changed(int value)
{
  if (value == -64)
    _panL->setText(": RND");
  else if (value < 0)
    _panL->setText(": L" + QString::number(std::abs(value)));
  else if (value > 0)
    _panL->setText(": R" + QString::number(value));
  else
    _panL->setText(": " + QString::number(value));

  _emulator->set_param(EmuSC::PatchParam::PartPanpot, (uint8_t) value + 0x40,
		       _partId);
  _emulator->update_LCD_display(_partId);
}

void PartMainSettings::_keyShift_changed(int value)
{
  _keyShiftL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::PitchKeyShift, (uint8_t) value + 0x40,
		       _partId);
  _emulator->update_LCD_display(_partId);
}

void PartMainSettings::_tune_changed(int value)
{
  _tuneL->setText(": " + QString::number((float) (value - 128) / 10, 'f', 1));
  _emulator->set_param_nib16(EmuSC::PatchParam::PitchOffsetFine,
			     (uint8_t) value, _partId);
}

void PartMainSettings::_reverb_changed(int value)
{
  _reverbL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ReverbSendLevel, (uint8_t) value,
		       _partId);

  _emulator->update_LCD_display(_partId);
}

void PartMainSettings::_chorus_changed(int value)
{
  _chorusL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ChorusSendLevel, (uint8_t) value,
		       _partId);
  _emulator->update_LCD_display(_partId);
}

void PartMainSettings::_fineTune_changed(int value)
{
  _fineTuneL->setText(": " + QString::number((float)(value - 8192) * 100 / 8192,
					     'f', 2));
  _emulator->set_param_uint14(EmuSC::PatchParam::PitchFineTune,
			      (uint16_t) value, _partId);
}

void PartMainSettings::_coarseTune_changed(int value)
{
  _coarseTuneL->setText(": " + QString::number(value - 0x40));
  _emulator->set_param(EmuSC::PatchParam::PitchCoarseTune, (uint8_t) value,
		       _partId);
}

void PartMainSettings::_velDepth_changed(int value)
{
  _velDepthL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::VelocitySenseDepth, (uint8_t) value,
		       _partId);
}

void PartMainSettings::_velOffset_changed(int value)
{
  _velOffsetL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::VelocitySenseOffset, (uint8_t) value,
		       _partId);
}


void PartMainSettings::_keyRangeL_changed(int value)
{
  _keyRangeLL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::KeyRangeLow, (uint8_t) value,_partId);
}


void PartMainSettings::_keyRangeH_changed(int value)
{
  _keyRangeHL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::KeyRangeHigh, (uint8_t)value,_partId);
}


PartRxModeSettings::PartRxModeSettings(Emulator *emulator, int8_t &partId,
				       QWidget *parent)
  : _emulator(emulator),
    _partId(partId)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("Part Settings: Rx & Mode");
  QFont f( "Arial", 12, QFont::Bold);
  headerL->setFont( f);
  vboxLayout->addWidget(headerL);

  QHBoxLayout *hboxLayout = new QHBoxLayout();
  hboxLayout->addWidget(new QLabel("Part:"));
  _partC = new QComboBox();
  for (int i = 1; i <= 16; i++)                 // TODO: SC-88 => A1-16 + B1-16
    _partC->addItem(QString::number(i));
  _partId = _partC->currentIndex();
  _partC->setEditable(false);  
  hboxLayout->addWidget(_partC);
  hboxLayout->addStretch(1);
  vboxLayout->addLayout(hboxLayout);

  QFrame *line;
  line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  vboxLayout->addWidget(line);

  _rxVolumeCh = new QCheckBox("Rx Volume");
  _rxPanCh = new QCheckBox("Rx Pan");
  _rxNoteCh = new QCheckBox("Rx Note");
  _rxProgramChangeCh = new QCheckBox("Rx Program Change");
  _rxControlChangeCh = new QCheckBox("Rx Control Change");
  _rxPitchBendCh = new QCheckBox("Rx Pitch Bend");
  _rxChAftertouchCh = new QCheckBox("Rx Channel Aftertouch");
  _rxPolyAftertouchCh = new QCheckBox("Rx Polyphonic Aftertouch");
  _rxRPNCh = new QCheckBox("Rx RPN");
  _rxNRPNCh = new QCheckBox("Rx NRPN");
  _rxModulationCh = new QCheckBox("Rx Modulation");
  _rxHold1Ch = new QCheckBox("Rx Hold 1");
  _rxPortamentoCh = new QCheckBox("Rx Portamento");
  _rxSostenutoCh = new QCheckBox("Rx Sostenuto");
  _rxSoftCh = new QCheckBox("Rx Soft");
  _rxExpressionCh = new QCheckBox("Rx Expression");

  //  _rxMapSelectCh = new QCheckBox("Rx Map Select");
 
  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(_rxVolumeCh,         0, 0);
  gridLayout->addWidget(_rxPanCh,            1, 0);
  gridLayout->addWidget(_rxNoteCh,           2, 0);
  gridLayout->addWidget(_rxProgramChangeCh,  3, 0);
  gridLayout->addWidget(_rxControlChangeCh,  4, 0);
  gridLayout->addWidget(_rxPitchBendCh,      5, 0);
  gridLayout->addWidget(_rxChAftertouchCh,   6, 0);
  gridLayout->addWidget(_rxPolyAftertouchCh, 7, 0);
  gridLayout->addWidget(_rxRPNCh,            0, 1);
  gridLayout->addWidget(_rxNRPNCh,           1, 1);
  gridLayout->addWidget(_rxModulationCh,     2, 1);
  gridLayout->addWidget(_rxHold1Ch,          3, 1);
  gridLayout->addWidget(_rxPortamentoCh,     4, 1);
  gridLayout->addWidget(_rxSostenutoCh,      5, 1);
  gridLayout->addWidget(_rxSoftCh,           6, 1);
  gridLayout->addWidget(_rxExpressionCh,     7, 1);

// TODO: SC-88
//  gridLayout->addWidget(_rxBankSelectCh,     3, 0);
//  gridLayout->addWidget(_rxMapSelectCh,      0, 1);
    
  gridLayout->setHorizontalSpacing(50);
  gridLayout->setColumnStretch(2, 1);

  vboxLayout->addLayout(gridLayout);

  QGridLayout *gridLayout1 = new QGridLayout();
  gridLayout1->addWidget(new QLabel("Mono / Poly Mode"), 0, 0);
  _polyModeC = new QComboBox();
  _polyModeC->addItems({ "Monophonic", "Polyphonic" });
  gridLayout1->addWidget(_polyModeC, 0, 1);

  gridLayout1->addWidget(new QLabel("Assign Mode"), 1, 0);
  _assignModeC = new QComboBox();
  _assignModeC->addItems({ "Single", "Limited-Multi", "Full-Multi" });
  gridLayout1->addWidget(_assignModeC, 1, 1);
  gridLayout1->setColumnStretch(2, 1);

  vboxLayout->addLayout(gridLayout1);
  vboxLayout->addStretch(0);
  vboxLayout->insertSpacing(1, 15);
  vboxLayout->insertSpacing(3, 10);
  vboxLayout->insertSpacing(5, 10);
  vboxLayout->insertSpacing(7, 15);

  connect(_partC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_partC_changed(int)));

  connect(_polyModeC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_polyModeC_changed(int)));
  connect(_assignModeC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_assignModeC_changed(int)));

  connect(_rxVolumeCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxVolume_changed(int)));
  connect(_rxPanCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxPan_changed(int)));
  connect(_rxNoteCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxNote_changed(int)));
  connect(_rxProgramChangeCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxProgramChange_changed(int)));
  connect(_rxControlChangeCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxControlChange_changed(int)));
  connect(_rxPitchBendCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxPitchBend_changed(int)));
  connect(_rxChAftertouchCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxChAftertouch_changed(int)));
  connect(_rxPolyAftertouchCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxPolyAftertouch_changed(int)));
  connect(_rxRPNCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxRPN_changed(int)));
  connect(_rxNRPNCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxNRPN_changed(int)));
  connect(_rxModulationCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxModulation_changed(int)));
  connect(_rxHold1Ch, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxHold1_changed(int)));
  connect(_rxPortamentoCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxPortamento_changed(int)));
  connect(_rxSostenutoCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxSostenuto_changed(int)));
  connect(_rxSoftCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxSoft_changed(int)));
  connect(_rxExpressionCh, SIGNAL(stateChanged(int)),
	  this, SLOT(_rxExpression_changed(int)));

  // Additional widgets for later models:
  // SC-55mkII+
  if (_emulator->get_synth_generation() >=EmuSC::ControlRom::SynthGen::SC55mk2){
    _rxBankSelectCh = new QCheckBox("Rx Bank Select");
    gridLayout->addWidget(_rxBankSelectCh,     8, 0);
    connect(_rxBankSelectCh, SIGNAL(stateChanged(int)),
	    this, SLOT(_rxBankSelect_changed(int)));
  }

// TODO: SC-88
//  connect(_rxMapSelectCh, SIGNAL(stateChanged(int)),
//          this, SLOT(rxMapSelect_changed(int)));

  update_all_widgets();

  setLayout(vboxLayout);
}


void PartRxModeSettings::reset(void)
{
  std::cout << "RESET" << std::endl;
}


void PartRxModeSettings::_partC_changed(int value)
{
  _partId = (int8_t) value;
  update_all_widgets();
}


void PartRxModeSettings::_polyModeC_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::PolyMode, (uint8_t) value, _partId);
}


void PartRxModeSettings::_assignModeC_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::AssignMode, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxVolume_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxVolume, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxPan_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxPanpot, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxNote_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxNoteMessage, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxProgramChange_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxProgramChange, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxControlChange_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxControlChange, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxPitchBend_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxPitchBend, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxChAftertouch_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxChPressure, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxPolyAftertouch_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxPolyPressure, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxRPN_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxRPN, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxNRPN_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxNRPN, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxModulation_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxModulation, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxHold1_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxHold1, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxPortamento_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxPortamento, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxSostenuto_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxSostenuto, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxSoft_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxSoft, (uint8_t) value, _partId);
}


void PartRxModeSettings::_rxExpression_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxExpression, (uint8_t) value, _partId);
}


// SC-55mkII+
void PartRxModeSettings::_rxBankSelect_changed(int value)
{
  _emulator->set_param(EmuSC::PatchParam::RxBankSelect, (uint8_t) value, _partId);
}


// SC-88
//void PartRxModeSettings::_rxMapSelect_changed(int value)
//{  _emulator->set_param(EmuSC::PatchParam::RxMapSelect, (uint8_t) value, _partId); }

void PartRxModeSettings::update_all_widgets(void)
{
  _partC->setCurrentIndex(_partId);
  
  _polyModeC->setCurrentIndex(_emulator->get_param(EmuSC::PatchParam::PolyMode, _partId));
  _assignModeC->setCurrentIndex(_emulator->get_param(EmuSC::PatchParam::AssignMode, _partId));

  _rxVolumeCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxVolume, _partId));
  _rxPanCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxPanpot, _partId));
  _rxNoteCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxNoteMessage, _partId));
  _rxProgramChangeCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxProgramChange, _partId));
  _rxControlChangeCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxControlChange, _partId));
  _rxPitchBendCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxPitchBend, _partId));
  _rxChAftertouchCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxChPressure, _partId));
  _rxPolyAftertouchCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxPolyPressure, _partId));
  _rxRPNCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxRPN, _partId));
  _rxNRPNCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxNRPN, _partId));
  _rxModulationCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxModulation, _partId));
  _rxHold1Ch->setChecked(_emulator->get_param(EmuSC::PatchParam::RxHold1, _partId));
  _rxPortamentoCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxPortamento, _partId));
  _rxSostenutoCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxSostenuto, _partId));
  _rxSoftCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxSoft, _partId));
  _rxExpressionCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxExpression, _partId));

  if (_emulator->get_synth_generation() >= EmuSC::ControlRom::SynthGen::SC55mk2)
    _rxBankSelectCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxBankSelect,
						     _partId));

//  SC-88
//  _rxMapSelectCh->setChecked(_emulator->get_param(EmuSC::PatchParam::RxMapSelect, _partId));
}


PartToneSettings::PartToneSettings(Emulator *emulator, int8_t &partId,
				   QWidget *parent)
  : _emulator(emulator),
    _partId(partId)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("Part Settings: Tone Modifiers");
  QFont f( "Arial", 12, QFont::Bold);
  headerL->setFont( f);
  vboxLayout->addWidget(headerL);

  QHBoxLayout *hboxLayout = new QHBoxLayout();
  hboxLayout->addWidget(new QLabel("Part:"));
  _partC = new QComboBox();
  for (int i = 1; i <= 16; i++)                 // TODO: SC-88 => A1-16 + B1-16
    _partC->addItem(QString::number(i));
  _partC->setEditable(false);  
  hboxLayout->addWidget(_partC);
  hboxLayout->addStretch(1);
  vboxLayout->addLayout(hboxLayout);

  QFrame *line;
  line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  vboxLayout->addWidget(line);

  QLabel *spacerL = new QLabel();
  spacerL->setFixedHeight(8);
  
  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(new QLabel("Vibrato Rate"),     0, 0);
  gridLayout->addWidget(new QLabel("Vibrato Depth"),    1, 0);
  gridLayout->addWidget(new QLabel("Vibrato Delay"),    2, 0);
  gridLayout->addWidget(spacerL,                        3, 0);
  gridLayout->addWidget(new QLabel("TVF Cutoff freq."), 4, 0);
  gridLayout->addWidget(new QLabel("TVF Resonance"),    5, 0);
  gridLayout->addWidget(spacerL,                        6, 0);
  gridLayout->addWidget(new QLabel("TVF/A Env. Att."),  7, 0);
  gridLayout->addWidget(new QLabel("TVF/A Env. Dec."),  8, 0);
  gridLayout->addWidget(new QLabel("TVF/A Env. Rel."),  9, 0);

  _vibratoRateL    = new QLabel(": ");
  _vibratoDepthL   = new QLabel(": ");
  _vibratoDelayL   = new QLabel(": ");
  _TVFCutoffFreqL  = new QLabel(": ");
  _TVFResonanceL   = new QLabel(": ");
  _TVFAEnvAttackL  = new QLabel(": ");
  _TVFAEnvDecayL   = new QLabel(": ");
  _TVFAEnvReleaseL = new QLabel(": ");

  /*
  _vibratoRateL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  _vibratoDepthL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  _vibratoDelayL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  _TVFCutoffFreqL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  _TVFResonanceL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  _TVFAEnvAttackL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  _TVFAEnvDecayL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  _TVFAEnvReleaseL->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  */
 
  // Set static width for slider value labels
  QFontMetrics fontMetrics(_vibratoRateL->font());
  int labelWidth = fontMetrics.horizontalAdvance("-888");
  _vibratoRateL->setFixedWidth(labelWidth);
 
  gridLayout->addWidget(_vibratoRateL,    0, 1);
  gridLayout->addWidget(_vibratoDepthL,   1, 1);
  gridLayout->addWidget(_vibratoDelayL,   2, 1);
  gridLayout->addWidget(_TVFCutoffFreqL,  4, 1);
  gridLayout->addWidget(_TVFResonanceL,   5, 1);
  gridLayout->addWidget(_TVFAEnvAttackL,  7, 1);
  gridLayout->addWidget(_TVFAEnvDecayL,   8, 1);
  gridLayout->addWidget(_TVFAEnvReleaseL, 9, 1);

  _vibratoRateS = new QSlider(Qt::Horizontal);
  _vibratoRateS->setRange(-50, 50);
  _vibratoRateS->setTickPosition(QSlider::TicksBelow);
  _vibratoRateS->setTickInterval(50);
  gridLayout->addWidget(_vibratoRateS, 0, 2);

  _vibratoDepthS = new QSlider(Qt::Horizontal);
  _vibratoDepthS->setRange(-50, 50);
  _vibratoDepthS->setTickPosition(QSlider::TicksBelow);
  _vibratoDepthS->setTickInterval(50);

  _vibratoDelayS = new QSlider(Qt::Horizontal);
  _vibratoDelayS->setRange(-50, 50);
  _vibratoDelayS->setTickPosition(QSlider::TicksBelow);
  _vibratoDelayS->setTickInterval(50);

  _TVFCutoffFreqS = new QSlider(Qt::Horizontal);
  _TVFCutoffFreqS->setRange(-50, 50);
  _TVFCutoffFreqS->setTickPosition(QSlider::TicksBelow);
  _TVFCutoffFreqS->setTickInterval(50);

  _TVFResonanceS = new QSlider(Qt::Horizontal);
  _TVFResonanceS->setRange(-50, 50);
  _TVFResonanceS->setTickPosition(QSlider::TicksBelow);
  _TVFResonanceS->setTickInterval(50);

  _TVFAEnvAttackS = new QSlider(Qt::Horizontal);
  _TVFAEnvAttackS->setRange(-50, 50);
  _TVFAEnvAttackS->setTickPosition(QSlider::TicksBelow);
  _TVFAEnvAttackS->setTickInterval(50);

  _TVFAEnvDecayS = new QSlider(Qt::Horizontal);
  _TVFAEnvDecayS->setRange(-50, 50);
  _TVFAEnvDecayS->setTickPosition(QSlider::TicksBelow);
  _TVFAEnvDecayS->setTickInterval(50);

  _TVFAEnvReleaseS = new QSlider(Qt::Horizontal);
  _TVFAEnvReleaseS->setRange(-50, 50);
  _TVFAEnvReleaseS->setTickPosition(QSlider::TicksBelow);
  _TVFAEnvReleaseS->setTickInterval(50);

  gridLayout->addWidget(_vibratoRateS,    0, 2);
  gridLayout->addWidget(_vibratoDepthS,   1, 2);
  gridLayout->addWidget(_vibratoDelayS,   2, 2);
  gridLayout->addWidget(_TVFCutoffFreqS,  4, 2);
  gridLayout->addWidget(_TVFResonanceS,   5, 2);
  gridLayout->addWidget(_TVFAEnvAttackS,  7, 2);
  gridLayout->addWidget(_TVFAEnvDecayS,   8, 2);
  gridLayout->addWidget(_TVFAEnvReleaseS, 9, 2);

  connect(_partC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_partC_changed(int)));

  connect(_vibratoRateS, SIGNAL(valueChanged(int)),
	  this, SLOT(_vibratoRate_changed(int)));
  connect(_vibratoDepthS, SIGNAL(valueChanged(int)),
	  this, SLOT(_vibratoDepth_changed(int)));
  connect(_vibratoDelayS, SIGNAL(valueChanged(int)),
	  this, SLOT(_vibratoDelay_changed(int)));
  connect(_TVFCutoffFreqS, SIGNAL(valueChanged(int)),
	  this, SLOT(_TVFCutoffFreq_changed(int)));
  connect(_TVFResonanceS, SIGNAL(valueChanged(int)),
	  this, SLOT(_TVFResonance_changed(int)));
  connect(_TVFAEnvAttackS, SIGNAL(valueChanged(int)),
	  this, SLOT(_TVFAEnvAttack_changed(int)));
  connect(_TVFAEnvDecayS, SIGNAL(valueChanged(int)),
	  this, SLOT(_TVFAEnvDecay_changed(int)));
  connect(_TVFAEnvReleaseS, SIGNAL(valueChanged(int)),
	  this, SLOT(_TVFAEnvRelease_changed(int)));

  update_all_widgets();
  
  vboxLayout->addLayout(gridLayout);
  vboxLayout->addStretch(0);
  vboxLayout->insertSpacing(1, 15);
  vboxLayout->insertSpacing(3, 10);
  vboxLayout->insertSpacing(5, 10);

  setLayout(vboxLayout);
}


void PartToneSettings::reset(void)
{
  std::cout << "RESET" << std::endl;
}


void PartToneSettings::update_all_widgets(void)
{
  _partC->setCurrentIndex(_partId);

  _vibratoRateS->setValue(_emulator->get_param(EmuSC::PatchParam::VibratoRate,
					       _partId) - 0x40);
  _vibratoDepthS->setValue(_emulator->get_param(EmuSC::PatchParam::VibratoDepth,
					       _partId) - 0x40);
  _vibratoDelayS->setValue(_emulator->get_param(EmuSC::PatchParam::VibratoDelay,
					       _partId) - 0x40);
  _TVFCutoffFreqS->setValue(_emulator->get_param(EmuSC::PatchParam::TVFCutoffFreq,
					       _partId) - 0x40);
  _TVFResonanceS->setValue(_emulator->get_param(EmuSC::PatchParam::TVFResonance,
					       _partId) - 0x40);
  _TVFAEnvAttackS->setValue(_emulator->get_param(EmuSC::PatchParam::TVFAEnvAttack,
					       _partId) - 0x40);
  _TVFAEnvDecayS->setValue(_emulator->get_param(EmuSC::PatchParam::TVFAEnvDecay,
					       _partId) - 0x40);
  _TVFAEnvReleaseS->setValue(_emulator->get_param(EmuSC::PatchParam::TVFAEnvRelease,
					       _partId) - 0x40);

  _vibratoRateL->setText(": " + QString::number(_vibratoRateS->value()));
  _vibratoDepthL->setText(": " + QString::number(_vibratoDepthS->value()));
  _vibratoDelayL->setText(": " + QString::number(_vibratoDelayS->value()));
  _TVFCutoffFreqL->setText(": " + QString::number(_TVFCutoffFreqS->value()));
  _TVFResonanceL->setText(": " + QString::number(_TVFResonanceS->value()));
  _TVFAEnvAttackL->setText(": " + QString::number(_TVFAEnvAttackS->value()));
  _TVFAEnvDecayL->setText(": " + QString::number(_TVFAEnvDecayS->value()));
  _TVFAEnvReleaseL->setText(": " + QString::number(_TVFAEnvReleaseS->value()));
  }

void PartToneSettings::_partC_changed(int value)
{
  _partId = (int8_t) value;
  update_all_widgets();
}


void PartToneSettings::_vibratoRate_changed(int value)
{
  _vibratoRateL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::VibratoRate,
		       (uint8_t) (value + 0x40), _partId);
}


void PartToneSettings::_vibratoDepth_changed(int value)
{
  _vibratoDepthL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::VibratoDepth,
		       (uint8_t) (value + 0x40), _partId);
}


void PartToneSettings::_vibratoDelay_changed(int value)
{
  _vibratoDelayL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::VibratoDelay,
		       (uint8_t) (value + 0x40), _partId);
}


void PartToneSettings::_TVFCutoffFreq_changed(int value)
{
  _TVFCutoffFreqL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::TVFCutoffFreq,
		       (uint8_t) (value + 0x40), _partId);
}


void PartToneSettings::_TVFResonance_changed(int value)
{
  _TVFResonanceL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::TVFResonance,
		       (uint8_t) (value + 0x40), _partId);
}


void PartToneSettings::_TVFAEnvAttack_changed(int value)
{
  _TVFAEnvAttackL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::TVFAEnvAttack,
		       (uint8_t) (value + 0x40), _partId);
}


void PartToneSettings::_TVFAEnvDecay_changed(int value)
{
  _TVFAEnvDecayL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::TVFAEnvDecay,
		       (uint8_t) (value + 0x40), _partId);
}


void PartToneSettings::_TVFAEnvRelease_changed(int value)
{
  _TVFAEnvReleaseL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::TVFAEnvRelease,
		       (uint8_t) (value + 0x40), _partId);
}
  

PartScaleSettings::PartScaleSettings(Emulator *emulator, int8_t &partId,
				     QWidget *parent)
  : _emulator(emulator),
    _partId(partId)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("Part Settings: Scale Tuning");
  QFont f( "Arial", 12, QFont::Bold);
  headerL->setFont( f);
  vboxLayout->addWidget(headerL);

  QHBoxLayout *hboxLayout = new QHBoxLayout();
  hboxLayout->addWidget(new QLabel("Part:"));
  _partC = new QComboBox();
  for (int i = 1; i <= 16; i++)                 // TODO: SC-88 => A1-16 + B1-16
    _partC->addItem(QString::number(i));
  _partC->setEditable(false);  
  hboxLayout->addWidget(_partC);
  hboxLayout->addStretch(1);
  vboxLayout->addLayout(hboxLayout);

  QFrame *line;
  line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  vboxLayout->addWidget(line);

  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(new QLabel("C"),   0, 0);
  gridLayout->addWidget(new QLabel("C#"),  1, 0);
  gridLayout->addWidget(new QLabel("D"),   2, 0);
  gridLayout->addWidget(new QLabel("D#"),  3, 0);
  gridLayout->addWidget(new QLabel("E"),   4, 0);
  gridLayout->addWidget(new QLabel("F"),   5, 0);
  gridLayout->addWidget(new QLabel("F#"),  6, 0);
  gridLayout->addWidget(new QLabel("G"),   7, 0);
  gridLayout->addWidget(new QLabel("G#"),  8, 0);
  gridLayout->addWidget(new QLabel("A"),   9, 0);
  gridLayout->addWidget(new QLabel("A#"), 10, 0);
  gridLayout->addWidget(new QLabel("B"),  11, 0);

  for (int i = 0; i < 12; i ++) {
    _valueL[i]  = new QLabel(": ");
    gridLayout->addWidget(_valueL[i],   i, 1);

    _noteS[i] = new QSlider(Qt::Horizontal);
    _noteS[i]->setRange(-64, 63);
    _noteS[i]->setTickPosition(QSlider::TicksBelow);
    _noteS[i]->setTickInterval(64);
    gridLayout->addWidget(_noteS[i], i, 2);
  }

  // Set static width for slider value labels
  QFontMetrics fontMetrics(_valueL[0]->font());
  int labelWidth = fontMetrics.horizontalAdvance(": 888");
  _valueL[0]->setFixedWidth(labelWidth);

  update_all_widgets(false);

  connect(_partC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_partC_changed(int)));

  connect(_noteS[0], SIGNAL(valueChanged(int)), this,SLOT(_noteC_changed(int)));
  connect(_noteS[1], SIGNAL(valueChanged(int)),this,SLOT(_noteCh_changed(int)));
  connect(_noteS[2], SIGNAL(valueChanged(int)), this,SLOT(_noteD_changed(int)));
  connect(_noteS[3], SIGNAL(valueChanged(int)),this,SLOT(_noteDh_changed(int)));
  connect(_noteS[4], SIGNAL(valueChanged(int)), this,SLOT(_noteE_changed(int)));
  connect(_noteS[5], SIGNAL(valueChanged(int)), this,SLOT(_noteF_changed(int)));
  connect(_noteS[6], SIGNAL(valueChanged(int)),this,SLOT(_noteFh_changed(int)));
  connect(_noteS[7], SIGNAL(valueChanged(int)), this,SLOT(_noteG_changed(int)));
  connect(_noteS[8], SIGNAL(valueChanged(int)),this,SLOT(_noteGh_changed(int)));
  connect(_noteS[9], SIGNAL(valueChanged(int)), this,SLOT(_noteA_changed(int)));
  connect(_noteS[10],SIGNAL(valueChanged(int)),this,SLOT(_noteAh_changed(int)));
  connect(_noteS[11], SIGNAL(valueChanged(int)),this,SLOT(_noteB_changed(int)));

  vboxLayout->addLayout(gridLayout);
  vboxLayout->insertSpacing(1, 15);
  vboxLayout->insertSpacing(3, 10);
  vboxLayout->insertSpacing(5, 10);
  vboxLayout->addStretch(0);
  setLayout(vboxLayout);
}


void PartScaleSettings::reset(void)
{
  std::cout << "RESET" << std::endl;
}


void PartScaleSettings::update_all_widgets(bool blockSignals)
{
  _partC->setCurrentIndex(_partId);

  bool wasBlocked[12];

  if (blockSignals)
    for (int i = 0; i < 12; i ++)
      wasBlocked[i]  = _noteS[i]->blockSignals(true);

  _noteS[0]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningC,
					   _partId) - 0x40);
  _noteS[1]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningC_,
					   _partId) - 0x40);  
  _noteS[2]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningD,
					   _partId) - 0x40);  
  _noteS[3]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningD_,
					   _partId) - 0x40);  
  _noteS[4]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningE,
					   _partId) - 0x40);  
  _noteS[5]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningF,
					   _partId) - 0x40);  
  _noteS[6]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningF_,
					   _partId) - 0x40);  
  _noteS[7]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningG,
					   _partId) - 0x40);  
  _noteS[8]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningG_,
					   _partId) - 0x40);  
  _noteS[9]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningA,
					   _partId) - 0x40);  
  _noteS[10]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningA_,
					    _partId) - 0x40);  
  _noteS[11]->setValue(_emulator->get_param(EmuSC::PatchParam::ScaleTuningB,
					    _partId) - 0x40);  

  for (int i = 0; i < 12; i ++)
    _valueL[i]->setText(": " + QString::number(_noteS[i]->value()));

  if (blockSignals)
    for (int i = 0; i < 12; i ++)
      _noteS[i]->blockSignals(wasBlocked[i]);

}


void PartScaleSettings::_partC_changed(int value)
{
  _partId = (int8_t) value;
  update_all_widgets();
}


void PartScaleSettings::_noteC_changed(int value)
{
  _valueL[0]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningC,
		       (uint8_t) (value + 0x40), _partId);
}


void PartScaleSettings::_noteCh_changed(int value)
{
  _valueL[1]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningC_,
		       (uint8_t) (value + 0x40), _partId);
}


void PartScaleSettings::_noteD_changed(int value)
{
  _valueL[2]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningD,
		       (uint8_t) (value + 0x40), _partId);
}


void PartScaleSettings::_noteDh_changed(int value)
{
  _valueL[3]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningD_,
		       (uint8_t) (value + 0x40), _partId);
}

void PartScaleSettings::_noteE_changed(int value)
{
  _valueL[4]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningE,
		       (uint8_t) (value + 0x40), _partId);
}


void PartScaleSettings::_noteF_changed(int value)
{
  _valueL[5]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningF,
		       (uint8_t) (value + 0x40), _partId);
}


void PartScaleSettings::_noteFh_changed(int value)
{
  _valueL[6]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningF_,
		       (uint8_t) (value + 0x40), _partId);
}


void PartScaleSettings::_noteG_changed(int value)
{
  _valueL[7]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningG,
		       (uint8_t) (value + 0x40), _partId);
}


void PartScaleSettings::_noteGh_changed(int value)
{
  _valueL[8]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningG_,
		       (uint8_t) (value + 0x40), _partId);
}


void PartScaleSettings::_noteA_changed(int value)
{
  _valueL[9]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningA,
		       (uint8_t) (value + 0x40), _partId);
}


void PartScaleSettings::_noteAh_changed(int value)
{
  _valueL[10]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningA_,
		       (uint8_t) (value + 0x40), _partId);
}


void PartScaleSettings::_noteB_changed(int value)
{
  _valueL[11]->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::ScaleTuningB,
		       (uint8_t) (value + 0x40), _partId);
}


PartControllerSettings::PartControllerSettings(Emulator *emulator,
						 int8_t &partId,
						 QWidget *parent)
  : _emulator(emulator),
    _partId(partId)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("Part Settings: Controllers");
  QFont f( "Arial", 12, QFont::Bold);
  headerL->setFont( f);
  vboxLayout->addWidget(headerL);

  QHBoxLayout *hboxLayout = new QHBoxLayout();
  hboxLayout->addWidget(new QLabel("Part:"));
  _partC = new QComboBox();
  for (int i = 1; i <= 16; i++)                 // TODO: SC-88 => A1-16 + B1-16
    _partC->addItem(QString::number(i));
  _partC->setEditable(false);  
  hboxLayout->addWidget(_partC);
  hboxLayout->addStretch(1);
  vboxLayout->addLayout(hboxLayout);

  QFrame *line;
  line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  vboxLayout->addWidget(line);

  QGridLayout *gridLayout0 = new QGridLayout();
  gridLayout0->addWidget(new QLabel("CC #1"), 0, 0);
  gridLayout0->addWidget(new QLabel("CC #2"), 1, 0);

  _cc1L = new QLabel(": ");
  _cc2L = new QLabel(": ");

  // Set static width for slider value labels
  QFontMetrics fontMetrics(_cc1L->font());
  int labelWidth = fontMetrics.horizontalAdvance(": 1888");
  _cc1L->setFixedWidth(labelWidth);

  gridLayout0->addWidget(_cc1L, 0, 1);
  gridLayout0->addWidget(_cc2L, 1, 1);

  _cc1S = new QSlider(Qt::Horizontal);
  _cc1S->setRange(0, 127);
  _cc1S->setTickPosition(QSlider::TicksBelow);
  _cc1S->setTickInterval(32);

  _cc2S = new QSlider(Qt::Horizontal);
  _cc2S->setRange(0, 127);
  _cc2S->setTickPosition(QSlider::TicksBelow);
  _cc2S->setTickInterval(32);

  gridLayout0->addWidget(_cc1S, 0, 2);
  gridLayout0->addWidget(_cc2S, 1, 2);
  vboxLayout->addLayout(gridLayout0);
  
  QHBoxLayout *hboxLayout1 = new QHBoxLayout();
  hboxLayout1->addWidget(new QLabel("Controller:"));
  _controllerC = new QComboBox();
  _controllerC->addItems({ "Modulation Controller #1",
                           "Pitch Bend",
			   "Channel Aftertouch",
			   "Polyphinc Aftertouch",
			   "CC1 Controller Variable",
			   "CC2 Controller Variable" });
  _controllerC->setEditable(false);  
  _controllerId = _controllerC->currentIndex();
  hboxLayout1->addWidget(_controllerC);
  hboxLayout1->addStretch(1);
  vboxLayout->addLayout(hboxLayout1);

  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(new QLabel("Pitch Control"), 0, 0);
  gridLayout->addWidget(new QLabel("TVF Cutoff"), 1, 0);
  gridLayout->addWidget(new QLabel("Amplitude"), 2, 0);
  gridLayout->addWidget(new QLabel("LFO 1 Rate"), 3, 0);
  gridLayout->addWidget(new QLabel("LFO 1 Pitch Depth"), 4, 0);
  gridLayout->addWidget(new QLabel("LFO 1 TVF Depth"), 5, 0);
  gridLayout->addWidget(new QLabel("LFO 1 TVA Depth"), 6, 0);
  gridLayout->addWidget(new QLabel("LFO 2 Rate"), 7, 0);
  gridLayout->addWidget(new QLabel("LFO 2 Pitch Depth"), 8, 0);
  gridLayout->addWidget(new QLabel("LFO 2 TVF Depth"), 9, 0);
  gridLayout->addWidget(new QLabel("LFO 2 TVA Depth"), 10, 0);

  _pitchCtrlL = new QLabel(": ");
  _tvfCutoffL = new QLabel(": ");
  _amplitudeL = new QLabel(": ");
  _lfo1RateL = new QLabel(": ");
  _lfo1PitchDepthL = new QLabel(": ");
  _lfo1TVFDepthL = new QLabel(": ");
  _lfo1TVADepthL = new QLabel(": ");
  _lfo2RateL = new QLabel(": ");
  _lfo2PitchDepthL = new QLabel(": ");
  _lfo2TVFDepthL = new QLabel(": ");
  _lfo2TVADepthL = new QLabel(": ");
 
  // Set static width for slider value labels
  int label2Width = fontMetrics.horizontalAdvance(":-18888");
  _pitchCtrlL->setFixedWidth(label2Width);
 
  gridLayout->addWidget(_pitchCtrlL,      0, 1);
  gridLayout->addWidget(_tvfCutoffL,      1, 1);
  gridLayout->addWidget(_amplitudeL,      2, 1);
  gridLayout->addWidget(_lfo1RateL,       3, 1);
  gridLayout->addWidget(_lfo1PitchDepthL, 4, 1);
  gridLayout->addWidget(_lfo1TVFDepthL,   5, 1);
  gridLayout->addWidget(_lfo1TVADepthL,   6, 1);
  gridLayout->addWidget(_lfo2RateL,       7, 1);
  gridLayout->addWidget(_lfo2PitchDepthL, 8, 1);
  gridLayout->addWidget(_lfo2TVFDepthL,   9, 1);
  gridLayout->addWidget(_lfo2TVADepthL,  10, 1);
  
  _pitchCtrlS = new QSlider(Qt::Horizontal);
  _pitchCtrlS->setRange(40, 88);
  _pitchCtrlS->setTickPosition(QSlider::TicksBelow);
  _pitchCtrlS->setTickInterval(1);

  _tvfCutoffS = new QSlider(Qt::Horizontal);
  _tvfCutoffS->setRange(0, 127);
  _tvfCutoffS->setTickPosition(QSlider::TicksBelow);
  _tvfCutoffS->setTickInterval(2400);

  _amplitudeS = new QSlider(Qt::Horizontal);
  _amplitudeS->setRange(0, 127);
  _amplitudeS->setTickPosition(QSlider::TicksBelow);
  _amplitudeS->setTickInterval(25);

  _lfo1RateS = new QSlider(Qt::Horizontal);
  _lfo1RateS->setRange(0, 127);
  _lfo1RateS->setTickPosition(QSlider::TicksBelow);
  _lfo1RateS->setTickInterval(100);

  _lfo1PitchDepthS = new QSlider(Qt::Horizontal);
  _lfo1PitchDepthS->setRange(0, 127);
  _lfo1PitchDepthS->setTickPosition(QSlider::TicksBelow);
  _lfo1PitchDepthS->setTickInterval(150);

  _lfo1TVFDepthS = new QSlider(Qt::Horizontal);
  _lfo1TVFDepthS->setRange(0, 127);
  _lfo1TVFDepthS->setTickPosition(QSlider::TicksBelow);
  _lfo1TVFDepthS->setTickInterval(600);

  _lfo1TVADepthS = new QSlider(Qt::Horizontal);
  _lfo1TVADepthS->setRange(0, 127);
  _lfo1TVADepthS->setTickPosition(QSlider::TicksBelow);
  _lfo1TVADepthS->setTickInterval(25);

  _lfo2RateS = new QSlider(Qt::Horizontal);
  _lfo2RateS->setRange(0, 127);
  _lfo2RateS->setTickPosition(QSlider::TicksBelow);
  _lfo2RateS->setTickInterval(100);

  _lfo2PitchDepthS = new QSlider(Qt::Horizontal);
  _lfo2PitchDepthS->setRange(0, 127);
  _lfo2PitchDepthS->setTickPosition(QSlider::TicksBelow);
  _lfo2PitchDepthS->setTickInterval(150);

  _lfo2TVFDepthS = new QSlider(Qt::Horizontal);
  _lfo2TVFDepthS->setRange(0, 127);
  _lfo2TVFDepthS->setTickPosition(QSlider::TicksBelow);
  _lfo2TVFDepthS->setTickInterval(32);

  _lfo2TVADepthS = new QSlider(Qt::Horizontal);
  _lfo2TVADepthS->setRange(0, 127);
  _lfo2TVADepthS->setTickPosition(QSlider::TicksBelow);
  _lfo2TVADepthS->setTickInterval(25);

  gridLayout->addWidget(_pitchCtrlS,      0, 2);
  gridLayout->addWidget(_tvfCutoffS,      1, 2);
  gridLayout->addWidget(_amplitudeS,      2, 2);
  gridLayout->addWidget(_lfo1RateS,       3, 2);
  gridLayout->addWidget(_lfo1PitchDepthS, 4, 2);
  gridLayout->addWidget(_lfo1TVFDepthS,   5, 2);
  gridLayout->addWidget(_lfo1TVADepthS,   6, 2);
  gridLayout->addWidget(_lfo2RateS,       7, 2);
  gridLayout->addWidget(_lfo2PitchDepthS, 8, 2);
  gridLayout->addWidget(_lfo2TVFDepthS,   9, 2);
  gridLayout->addWidget(_lfo2TVADepthS,  10, 2);

  vboxLayout->addLayout(gridLayout);
  vboxLayout->addStretch(0);
  vboxLayout->insertSpacing(1, 15);
  vboxLayout->insertSpacing(3, 10);
  vboxLayout->insertSpacing(5, 10);
  vboxLayout->insertSpacing(7, 10);
  vboxLayout->insertSpacing(9, 5);

  update_all_widgets();
  
  connect(_partC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_partC_changed(int)));

  connect(_cc1S, SIGNAL(valueChanged(int)), this, SLOT(_cc1_changed(int)));
  connect(_cc2S, SIGNAL(valueChanged(int)), this, SLOT(_cc2_changed(int)));

  connect(_controllerC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_controller_changed(int)));

  connect(_pitchCtrlS, SIGNAL(valueChanged(int)),
	  this, SLOT(_pitchCtrl_changed(int)));
  connect(_tvfCutoffS, SIGNAL(valueChanged(int)),
	  this, SLOT(_tvfCutoff_changed(int)));
  connect(_amplitudeS, SIGNAL(valueChanged(int)),
	  this, SLOT(_amplitude_changed(int)));
  connect(_lfo1RateS, SIGNAL(valueChanged(int)),
	  this, SLOT(_lfo1Rate_changed(int)));
  connect(_lfo1RateS, SIGNAL(valueChanged(int)),
	  this, SLOT(_lfo1Rate_changed(int)));
  connect(_lfo1PitchDepthS, SIGNAL(valueChanged(int)),
	  this, SLOT(_lfo1PitchDepth_changed(int)));
  connect(_lfo1TVFDepthS, SIGNAL(valueChanged(int)),
	  this, SLOT(_lfo1TVFDepth_changed(int)));
  connect(_lfo1TVADepthS, SIGNAL(valueChanged(int)),
	  this, SLOT(_lfo1TVADepth_changed(int)));
  connect(_lfo2RateS, SIGNAL(valueChanged(int)),
	  this, SLOT(_lfo2Rate_changed(int)));
  connect(_lfo2PitchDepthS, SIGNAL(valueChanged(int)),
	  this, SLOT(_lfo2PitchDepth_changed(int)));
  connect(_lfo2TVFDepthS, SIGNAL(valueChanged(int)),
	  this, SLOT(_lfo2TVFDepth_changed(int)));
  connect(_lfo2TVADepthS, SIGNAL(valueChanged(int)),
	  this, SLOT(_lfo2TVADepth_changed(int)));

  setLayout(vboxLayout);
}


void PartControllerSettings::reset(void)
{
  std::cout << "RESET" << std::endl;
}


void PartControllerSettings::update_all_widgets(void)
{
  _partC->setCurrentIndex(_partId);

  _cc1S->setValue(_emulator->get_param(EmuSC::PatchParam::CC1ControllerNumber,
				       _partId));
  _cc2S->setValue(_emulator->get_param(EmuSC::PatchParam::CC2ControllerNumber,
				       _partId));
  
  _cc1L->setText(": " + QString::number(_cc1S->value()));
  _cc2L->setText(": " + QString::number(_cc2S->value()));

  
  if (_controllerId == 1) {
    _pitchCtrlS->setRange(64, 88);
    _pitchCtrlS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_PitchControl + 0x10 * _controllerId, _partId));
  } else {
    _pitchCtrlS->setRange(40, 88);
    _pitchCtrlS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_PitchControl + 0x10 * _controllerId, _partId));
  }
  
  _tvfCutoffS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_TVFCutoffControl + 0x10 * _controllerId, _partId));
  _amplitudeS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_AmplitudeControl + 0x10 * _controllerId, _partId));
  _lfo1RateS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO1RateControl + 0x10 * _controllerId, _partId));
  _lfo1PitchDepthS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO1PitchDepth + 0x10 * _controllerId, _partId));
  _lfo1TVFDepthS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO1TVFDepth + 0x10 * _controllerId, _partId));
  _lfo1TVADepthS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO1TVADepth + 0x10 * _controllerId, _partId));
  _lfo2RateS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO2RateControl + 0x10 * _controllerId, _partId));
  _lfo2PitchDepthS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO2PitchDepth + 0x10 * _controllerId, _partId));
  _lfo2TVFDepthS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO2TVFDepth + 0x10 * _controllerId, _partId));
  _lfo2TVADepthS->setValue(_emulator->get_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO2TVADepth + 0x10 * _controllerId, _partId));

  _pitchCtrlL->setText(": " + QString::number(_pitchCtrlS->value() - 0x40));

  if (_tvfCutoffS->value() == 0x40)       // Fix rounding error for 0 (center)
    _tvfCutoffL->setText(": 0");
  else
    _tvfCutoffL->setText(": " + QString::number(_tvfCutoffS->value() * 19200 / 127 - 9600));

  _amplitudeL->setText(": " + QString::number(_amplitudeS->value() * 200 / 127 - 100));

  if (_lfo1RateS->value() == 0x40)       // Fix rounding error for 0 (center)
    _tvfCutoffL->setText(": 0");
  else
    _lfo1RateL->setText(": " + QString::number(_lfo1RateS->value() * 20 / 127.0 - 10, 'f', 1));

  _lfo1PitchDepthL->setText(": " + QString::number(_lfo1PitchDepthS->value() * 600 / 127));
  _lfo1TVFDepthL->setText(": " + QString::number(_lfo1TVFDepthS->value() * 2400 / 127));
  _lfo1TVADepthL->setText(": " + QString::number((float) _lfo1TVADepthS->value() * 100 / 127.0, 'f', 1));

  if (_lfo2RateS->value() == 0x40)       // Fix rounding error for 0 (center)
    _tvfCutoffL->setText(": 0");
  else
    _lfo2RateL->setText(": " + QString::number((float) _lfo2RateS->value() * 20 / 127.0 - 10, 'f', 1));
  
  _lfo2PitchDepthL->setText(": " + QString::number(_lfo2PitchDepthS->value() * 600 / 127));
  _lfo2TVFDepthL->setText(": " + QString::number(_lfo2TVFDepthS->value() * 2400 / 127));
  _lfo2TVADepthL->setText(": " + QString::number((float) _lfo2TVADepthS->value() * 100 / 127.0, 'f', 1));
}


void PartControllerSettings::_partC_changed(int value)
{
  _partId = (int8_t) value;
  update_all_widgets();
}


void PartControllerSettings::_cc1_changed(int value)
{
  _cc1L->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::CC1ControllerNumber, (uint8_t) value,
		       _partId);  
}


void PartControllerSettings::_cc2_changed(int value)
{
  _cc2L->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::PatchParam::CC2ControllerNumber, (uint8_t) value,
		       _partId);  
}

void PartControllerSettings::_controller_changed(int value)
{
  _controllerId = value;
  update_all_widgets();
}


void PartControllerSettings::_pitchCtrl_changed(int value)
{
  _pitchCtrlL->setText(": " + QString::number(value - 0x40));
  _emulator->set_patch_param((uint16_t) EmuSC::PatchParam::MOD_PitchControl +
			     0x10 * _controllerId, (uint8_t) value, _partId);
}


void PartControllerSettings::_tvfCutoff_changed(int value)
{
  _tvfCutoffL->setText(": " + QString::number(value * 19200 / 127 - 9600));
  _emulator->set_patch_param((uint16_t) EmuSC::PatchParam::MOD_TVFCutoffControl+
			     0x10 * _controllerId, (uint8_t) value, _partId);
}


void PartControllerSettings::_amplitude_changed(int value)
{
  _amplitudeL->setText(": " + QString::number((float) value * 200 / 127.0 - 100, 'f', 1));
  _emulator->set_patch_param((uint16_t) EmuSC::PatchParam::MOD_AmplitudeControl+
			     0x10 * _controllerId, (uint8_t) value, _partId);
}


void PartControllerSettings::_lfo1Rate_changed(int value)
{
  _lfo1RateL->setText(": " + QString::number((float) (value * 20 / 127.0 - 10), 'f', 1));
  _emulator->set_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO1RateControl +
			     0x10 * _controllerId, (uint8_t) value, _partId);
}


void PartControllerSettings::_lfo1PitchDepth_changed(int value)
{
  _lfo1PitchDepthL->setText(": " + QString::number(value * 600 / 127));
  _emulator->set_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO1PitchDepth +
			     0x10 * _controllerId, (uint8_t) value, _partId);
}


void PartControllerSettings::_lfo1TVFDepth_changed(int value)
{
  _lfo1TVFDepthL->setText(": " + QString::number(value * 2400 / 127));
  _emulator->set_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO1TVFDepth +
			     0x10 * _controllerId, (uint8_t) value, _partId);
}


void PartControllerSettings::_lfo1TVADepth_changed(int value)
{
  _lfo1TVADepthL->setText(": " + QString::number((float) value * 100 / 127.0, 'f', 1));
  _emulator->set_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO1TVADepth +
			     0x10 * _controllerId, (uint8_t) value, _partId);
}


void PartControllerSettings::_lfo2Rate_changed(int value)
{
  _lfo2RateL->setText(": " + QString::number((float) (value * 20 / 127.0 - 10), 'f', 1));
  _emulator->set_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO2RateControl +
			     0x10 * _controllerId, (uint8_t) value, _partId);
}


void PartControllerSettings::_lfo2PitchDepth_changed(int value)
{
  _lfo2PitchDepthL->setText(": " + QString::number(value * 600 / 127));
  _emulator->set_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO2PitchDepth +
			     0x10 * _controllerId, (uint8_t) value, _partId);
}


void PartControllerSettings::_lfo2TVFDepth_changed(int value)
{
  _lfo2TVFDepthL->setText(": " + QString::number(value * 2400 / 127));
  _emulator->set_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO2TVFDepth +
			     0x10 * _controllerId, (uint8_t) value, _partId);
}


void PartControllerSettings::_lfo2TVADepth_changed(int value)
{
  _lfo2TVADepthL->setText(": " + QString::number((float) value * 100 / 127.0, 'f', 1));
  _emulator->set_patch_param((uint16_t) EmuSC::PatchParam::MOD_LFO2TVADepth +
			     0x10 * _controllerId, (uint8_t) value, _partId);
}


DrumSettings::DrumSettings(Emulator *emulator, QWidget *parent)
  : _emulator(emulator)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("Drum Settings");
  QFont f("Arial", 12, QFont::Bold);
  headerL->setFont( f);
  vboxLayout->addWidget(headerL);

  QHBoxLayout *hboxLayout = new QHBoxLayout();
  hboxLayout->addWidget(new QLabel("Map:"));

  _mapC = new QComboBox();
  _mapC->addItems({ "Drum 1", "Drum 2" });
  _mapC->setEditable(false);  
  hboxLayout->addWidget(_mapC);
  hboxLayout->addSpacing(50);
  hboxLayout->addWidget(new QLabel("Name:"));
  _nameLE = new QLineEdit();
  _nameLE->setMaxLength(12);
  _nameLE->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9 ]{0,12}$"), this));
  hboxLayout->addWidget(_nameLE);
  hboxLayout->addStretch(1);
  vboxLayout->addLayout(hboxLayout);

  QFrame *line;
  line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  vboxLayout->addWidget(line);

  QHBoxLayout *hboxLayout2 = new QHBoxLayout();
  hboxLayout2->addWidget(new QLabel("Drum instrument:"));
  _instrumentC = new QComboBox();
  for (int i = 0; i < 128; i++)
    _instrumentC->addItem(QString::number(i) + QString(" Drum"));
  _instrumentC->setEditable(false);  
  hboxLayout2->addWidget(_instrumentC);
  hboxLayout2->addStretch(1);  
  vboxLayout->addLayout(hboxLayout2);

  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(new QLabel("Volume"),       0, 0);
  gridLayout->addWidget(new QLabel("Coarse tune"),  1, 0);
  gridLayout->addWidget(new QLabel("Panpot"),       2, 0);
  gridLayout->addWidget(new QLabel("Reverb Depth"), 3, 0);
  gridLayout->addWidget(new QLabel("Chorus Depth"), 4, 0);
  gridLayout->addWidget(new QLabel("Assign group"), 5, 0);
//  gridLayout->addWidget(new QLabel("Delay"),       5, 0);   // TODO: SC-88

  _volumeL = new QLabel(": ");
  _pitchL = new QLabel(": ");
  _panL = new QLabel(": ");
  _reverbL = new QLabel(": ");
  _chorusL = new QLabel(": ");
//  _delayL = new QLabel(": ");
  _exlGroupL = new QLabel(": ");

  // Set static width for slider value labels
  QFontMetrics fontMetrics(_volumeL->font());
  int labelWidth = fontMetrics.horizontalAdvance(": 18888");
  _volumeL->setFixedWidth(labelWidth);

  gridLayout->addWidget(_volumeL,   0, 1);
  gridLayout->addWidget(_pitchL,    1, 1);
  gridLayout->addWidget(_panL,      2, 1);
  gridLayout->addWidget(_reverbL,   3, 1);
  gridLayout->addWidget(_chorusL,   4, 1);
  gridLayout->addWidget(_exlGroupL, 5, 1);
//  gridLayout->addWidget(_delayL,  4, 1);

  _volumeS = new QSlider(Qt::Horizontal);
  _volumeS->setRange(0, 127);
  _volumeS->setTickPosition(QSlider::TicksBelow);
  _volumeS->setTickInterval(64);

  _pitchS = new QSlider(Qt::Horizontal);
  _pitchS->setRange(0, 127);
  _pitchS->setTickPosition(QSlider::TicksBelow);
  _pitchS->setTickInterval(64);

  _panS = new QSlider(Qt::Horizontal);
  _panS->setRange(0, 127);
  _panS->setTickPosition(QSlider::TicksBelow);
  _panS->setTickInterval(64);

  _reverbS = new QSlider(Qt::Horizontal);
  _reverbS->setRange(0, 127);
  _reverbS->setTickPosition(QSlider::TicksBelow);
  _reverbS->setTickInterval(64);

  _chorusS = new QSlider(Qt::Horizontal);
  _chorusS->setRange(0, 127);
  _chorusS->setTickPosition(QSlider::TicksBelow);
  _chorusS->setTickInterval(64);

  _exlGroupS = new QSlider(Qt::Horizontal);
  _exlGroupS->setRange(0, 127);
  _exlGroupS->setTickPosition(QSlider::TicksBelow);
  _exlGroupS->setTickInterval(64);

// TODO: SC-88
//  _delayS = new QSlider(Qt::Horizontal);
//  _delayS->setRange(0, 127);
//  _delayS->setTickPosition(QSlider::TicksBelow);
//  _delayS->setTickInterval(64);

  gridLayout->addWidget(_volumeS,   0, 2);
  gridLayout->addWidget(_pitchS,    1, 2);
  gridLayout->addWidget(_panS,      2, 2);
  gridLayout->addWidget(_reverbS,   3, 2);
  gridLayout->addWidget(_chorusS,   4, 2);
  gridLayout->addWidget(_exlGroupS, 5, 2);
//  gridLayout->addWidget(_delayS,    4, 2);

  vboxLayout->addLayout(gridLayout);

  _rxNoteOn = new QCheckBox("Rx Note On", this);
  _rxNoteOff = new QCheckBox("Rx Note Off", this);

  QGridLayout *gridLayout3 = new QGridLayout();
  gridLayout3->addWidget(_rxNoteOn, 0, 0);
  gridLayout3->addWidget(_rxNoteOff, 1, 0);

  vboxLayout->addLayout(gridLayout3);
  vboxLayout->insertSpacing(1, 15);
  vboxLayout->insertSpacing(3, 10);
  vboxLayout->insertSpacing(5, 10);
  vboxLayout->insertSpacing(7, 15);
  vboxLayout->insertSpacing(9, 15);
  vboxLayout->addStretch(0);

  update_all_widgets();

  connect(_mapC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_map_changed(int)));
  connect(_nameLE, SIGNAL(textChanged(const QString)),
	  this, SLOT(_name_changed(const QString)));
  connect(_instrumentC, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_instrument_changed(int)));

  connect(_volumeS, SIGNAL(valueChanged(int)), this,SLOT(_volume_changed(int)));
  connect(_pitchS, SIGNAL(valueChanged(int)), this, SLOT(_pitch_changed(int)));
  connect(_panS, SIGNAL(valueChanged(int)), this, SLOT(_pan_changed(int)));
  connect(_reverbS, SIGNAL(valueChanged(int)), this,SLOT(_reverb_changed(int)));
  connect(_chorusS, SIGNAL(valueChanged(int)), this,SLOT(_chorus_changed(int)));
  connect(_exlGroupS, SIGNAL(valueChanged(int)), this, SLOT(_exlGroup_changed(int)));

  connect(_rxNoteOn, SIGNAL(stateChanged(int)),
          this, SLOT(_rxNoteOn_changed(int)));
  connect(_rxNoteOff, SIGNAL(stateChanged(int)),
          this, SLOT(_rxNoteOff_changed(int)));

  setLayout(vboxLayout);
}


void DrumSettings::reset(void)
{
  std::cout << "RESET" << std::endl;
}


void DrumSettings::update_all_widgets(void)
{
  _instrument = _instrumentC->currentIndex();
  _map = _mapC->currentIndex();
  
  std::string name((const char *) _emulator->get_param_ptr(EmuSC::DrumParam::DrumsMapName, _map), 12);
  _nameLE->setText(QString::fromStdString(name));

  _volumeS->setValue(_emulator->get_param(EmuSC::DrumParam::Level,
					  _map, _instrument));
  _pitchS->setValue(_emulator->get_param(EmuSC::DrumParam::PlayKeyNumber,
					 _map, _instrument));
  _panS->setValue(_emulator->get_param(EmuSC::DrumParam::Panpot,
				       _map, _instrument));
  _reverbS->setValue(_emulator->get_param(EmuSC::DrumParam::ReverbDepth,
					  _map, _instrument));
  _chorusS->setValue(_emulator->get_param(EmuSC::DrumParam::ChorusDepth,
					  _map, _instrument));
  _exlGroupS->setValue(_emulator->get_param(EmuSC::DrumParam::AssignGroupNumber,
					    _map, _instrument));
  //  _delayS->setValue(_drumSets[_drumSet].volume[_instrument]);

  _volumeL->setText(": " + QString::number(_volumeS->value()));
  _pitchL->setText(": " + QString::number(_pitchS->value() - 0x40));

  if (_panS->value() == 0)
    _panL->setText(": RND");
  else if (_panS->value() < 0x40)
    _panL->setText(": L" + QString::number(std::abs(_panS->value() - 0x40)));
  else if (_panS->value() > 0x40)
    _panL->setText(": R" + QString::number(_panS->value() - 0x40));
  else
    _panL->setText(": 0");

  if (_reverbS->value() < 127)
    _reverbL->setText(": " + QString::number((float) _reverbS->value() *
					     100 / 127.0, 'f', 1) + "%");
  else
    _reverbL->setText(": 100%");

  if (_chorusS->value() < 127)
    _chorusL->setText(": " + QString::number((float) _chorusS->value() *
					     100 / 127.0, 'f', 1) + "%");
  else
    _chorusL->setText(": 100%");

  if (_exlGroupS->value() == 0)
    _exlGroupL->setText(": Off");
  else
    _exlGroupL->setText(": " + QString::number(_exlGroupS->value()));
//  _delayL->setText(": " + QString::number(_delayS->value()));

  _rxNoteOn->setChecked(_emulator->get_param(EmuSC::DrumParam::RxNoteOn,
					     _map, _instrument));
  _rxNoteOff->setChecked(_emulator->get_param(EmuSC::DrumParam::RxNoteOff,
					      _map, _instrument));
}


void DrumSettings::_map_changed(int value)
{
  _map = (int8_t) value;
  update_all_widgets();
}


void DrumSettings::_name_changed(const QString &name)
{
  _emulator->set_param(EmuSC::DrumParam::DrumsMapName, _map,
		       (uint8_t*) name.leftJustified(12, ' ').toStdString().c_str(), 12);
}


void DrumSettings::_instrument_changed(int value)
{
  _instrument = (int8_t) value;
  update_all_widgets();
}


void DrumSettings::_volume_changed(int value)
{
  _volumeL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::DrumParam::Level, _map, _instrument, value);
}


void DrumSettings::_pitch_changed(int value)
{
  _pitchL->setText(": " + QString::number(value));
  _emulator->set_param(EmuSC::DrumParam::PlayKeyNumber, _map,_instrument,value);
}


void DrumSettings::_pan_changed(int value)
{
  if (value == 0)
    _panL->setText(": RND");
  else if (value < 0x40)
    _panL->setText(": L" + QString::number(std::abs(value - 0x40)));
  else if (value > 0x40)
    _panL->setText(": R" + QString::number(value - 0x40));
  else
    _panL->setText(": 0");

  _emulator->set_param(EmuSC::DrumParam::Panpot, _map, _instrument, value);
}


void DrumSettings::_reverb_changed(int value)
{
  if (value < 127)
    _reverbL->setText(": " + QString::number((float) value * 100 / 127.0,
					     'f', 1) + "%");
  else
    _reverbL->setText(": 100%");

  _emulator->set_param(EmuSC::DrumParam::ReverbDepth, _map, _instrument, value);
}


void DrumSettings::_chorus_changed(int value)
{
  if (value < 127)
    _chorusL->setText(": " + QString::number((float) value * 100 / 127.0,
					     'f', 1) + "%");
  else
    _chorusL->setText(": 100%");

  _emulator->set_param(EmuSC::DrumParam::ChorusDepth, _map, _instrument, value);
}


void DrumSettings::_exlGroup_changed(int value)
{
  if (value == 0)
    _exlGroupL->setText(": Off");
  else
    _exlGroupL->setText(": " + QString::number(value));

  _emulator->set_param(EmuSC::DrumParam::AssignGroupNumber, _map, _instrument,
		       value);
}


void DrumSettings::_rxNoteOn_changed(int value)
{
  _emulator->set_param(EmuSC::DrumParam::RxNoteOn, _map, _instrument, value);
}


void DrumSettings::_rxNoteOff_changed(int value)
{
  _emulator->set_param(EmuSC::DrumParam::RxNoteOff, _map, _instrument, value);
}


DisplaySettings::DisplaySettings(Emulator *emulator, QWidget *parent)
  : _emulator(emulator)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("Display Settings");
  QFont f("Arial", 12, QFont::Bold);
  headerL->setFont( f);
  vboxLayout->addWidget(headerL);

  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(new QLabel("Bar display:"), 0, 0);
  gridLayout->addWidget(new QLabel("Peak hold:"), 1, 0);

  _barDisplayCB = new QComboBox();
  _barDisplayCB->addItems({"Type 1", "Type 2", "Type 3", "Type 4",
                           "Type 5", "Type 6", "Type 7", "Type 8"});
  _barDisplayCB->setEditable(false);
  gridLayout->addWidget(_barDisplayCB, 0, 1);

  _peakHoldCB = new QComboBox();
  _peakHoldCB->addItems({"Off", "Type 1", "Type 2", "Type 3"});
  _peakHoldCB->setEditable(false);
  gridLayout->addWidget(_peakHoldCB, 1, 1);

  update_all_widgets();

  connect(_barDisplayCB, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_barDisplay_changed(int)));
  connect(_peakHoldCB, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_peakHold_changed(int)));

  gridLayout->setColumnStretch(2, 1);

  vboxLayout->insertSpacing(1, 15);
  vboxLayout->addLayout(gridLayout);
  vboxLayout->addStretch(0);
  setLayout(vboxLayout);
}


void DisplaySettings::reset(void)
{
  _emulator->set_bar_display_type(1);
  _emulator->set_bar_display_peak_hold(1);

  update_all_widgets();
}


void DisplaySettings::update_all_widgets(void)
{
  _barDisplayCB->setCurrentIndex(_emulator->get_bar_display_type() - 1);
  _peakHoldCB->setCurrentIndex(_emulator->get_bar_display_peak_hold());
}


void DisplaySettings::_barDisplay_changed(int value)
{
  _emulator->set_bar_display_type(value + 1);
}


void DisplaySettings::_peakHold_changed(int value)
{
  _emulator->set_bar_display_peak_hold(value);
}
