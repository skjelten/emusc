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


#include "audio_output_alsa.h"
#include "audio_output_core.h"
#include "audio_output_pulse.h"
#include "audio_output_qt.h"
#include "audio_output_win32.h"
#include "midi_input.h"
#include "midi_input_alsa.h"
#include "midi_input_core.h"
#include "midi_input_win32.h"
#include "preferences_dialog.h"

#include <iostream>

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSettings>
#include <QGroupBox>
#include <QFileDialog>
#include <QStringList>
#include <QColorDialog>
#include <QCryptographicHash>
#include <QPainter>
#include <QStyleHints>


PreferencesDialog::PreferencesDialog(Emulator *emulator, Scene *scene, MainWindow *mWindow, QWidget *parent)
  : QDialog{parent},
    _emulator(emulator)
{
  _generalSettings = new GeneralSettings(mWindow, emulator, scene);
  _audioSettings = new AudioSettings(emulator);
  _midiSettings = new MidiSettings(emulator, scene);
  _romSettings = new RomSettings(emulator);

  _stack = new QStackedWidget();
  _stack->addWidget(_generalSettings);
  _stack->addWidget(_audioSettings);
  _stack->addWidget(_midiSettings);
  _stack->addWidget(_romSettings);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  QHBoxLayout *settingsLayout = new QHBoxLayout;
  
  _menuList = new QListWidget();
  _menuList->setMinimumHeight(100);
  
  _generalLW = new QListWidgetItem(tr("General"), _menuList);
  _audioLW = new QListWidgetItem(tr("Audio"), _menuList);
  _midiLW = new QListWidgetItem(tr("MIDI"), _menuList);
  _romLW = new QListWidgetItem(tr("ROM"), _menuList);

  QPixmap generalPM(":/images/gear.png");
  QPixmap audioPM(":/images/speaker.png");
  QPixmap midiPM(":/images/midi.png");
  QPixmap romPM(":/images/rom.png");

  // Invert icon colors in case of dark theme
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  if (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
    generalPM = _invert_pixmap_color(generalPM);
    audioPM = _invert_pixmap_color(audioPM);
    midiPM = _invert_pixmap_color(midiPM);
    romPM = _invert_pixmap_color(romPM);
  }
#endif

  _generalLW->setIcon(generalPM);
  _audioLW->setIcon(audioPM);
  _midiLW->setIcon(midiPM);
  _romLW->setIcon(romPM);

  _menuList->addItem(_generalLW);
  _menuList->addItem(_audioLW);
  _menuList->addItem(_midiLW);
  _menuList->addItem(_romLW);

  _menuList->setFixedWidth(_menuList->sizeHintForColumn(0) + 10 +
			 _menuList->frameWidth() * 2);
  settingsLayout->addWidget(_menuList, 0);
  settingsLayout->addWidget(_stack, 1);

  connect(_menuList, SIGNAL(currentRowChanged(int)),
	  _stack, SLOT(setCurrentIndex(int)));
 
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Reset |
						     QDialogButtonBox::Ok);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox->button(QDialogButtonBox::Reset), SIGNAL(clicked()),
	  this, SLOT(reset()));
  
  mainLayout->addLayout(settingsLayout);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  setWindowTitle(tr("Preferences"));
  setModal(true);
  resize(600, 500);

  // Increase height for _menuList items
  for(int i = 0; i < _menuList->count(); i++)
    _menuList->item(i)->setSizeHint(QSize(0, 35));
}


PreferencesDialog::~PreferencesDialog()
{
}


void PreferencesDialog::accept()
{
  delete this;
}


void PreferencesDialog::reset()
{
  if (_menuList->currentItem() == _generalLW)
    _generalSettings->reset();
  else if (_menuList->currentItem() == _audioLW)
    _audioSettings->reset();
  else if (_menuList->currentItem() == _midiLW)
    _midiSettings->reset();
  else if (_menuList->currentItem() == _romLW)
    _romSettings->reset();
  else
    std::cerr << "EmuSC: Internal error, reset unkown menu widget" << std::endl;
}


QPixmap PreferencesDialog::_invert_pixmap_color(const QPixmap &pixmap)
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


GeneralSettings::GeneralSettings(MainWindow *mainWindow, Emulator *emulator,
                                 Scene *scene, QWidget *parent)
  : _mainWindow(mainWindow),
    _emulator(emulator),
    _scene(scene)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();
  QLabel *headerL = new QLabel("General Settings");
  QFont f("Arial", 12, QFont::Bold);
  headerL->setFont(f);
  vboxLayout->addWidget(headerL);

  _autoPowerOnCB = new QCheckBox("Power on at startup", this);
  _rememberLayoutCB = new QCheckBox("Remember window layout", this);
  _enableKbdMidi = new QCheckBox("Enable keyboard MIDI input", this);

  QGroupBox *animGroupBox = new QGroupBox("LCD animations on startup");
  _emuscAnim = new QRadioButton("Show EmuSC and model animations", this);
  _romAnim = new QRadioButton("Show only animations from ROM", this);
  _noAnim = new QRadioButton("Do not show animations", this);

  QVBoxLayout *animVbox = new QVBoxLayout();
  animVbox->addWidget(_emuscAnim);
  animVbox->addWidget(_romAnim);
  animVbox->addWidget(_noAnim);
  animGroupBox->setLayout(animVbox);

  QGroupBox *colorGroupBox = new QGroupBox("LCD colors");
  _lcdBkgColorPickB = new QPushButton();
  _lcdActiveColorPickB = new QPushButton();
  _lcdInactiveColorPickB = new QPushButton();

  _set_button_color(_lcdBkgColorPickB, _scene->get_lcd_bkg_on_color());
  _set_button_color(_lcdActiveColorPickB, _scene->get_lcd_active_on_color());
  _set_button_color(_lcdInactiveColorPickB,_scene->get_lcd_inactive_on_color());

  QGridLayout *lcdGridLayout = new QGridLayout();
  lcdGridLayout->addWidget(new QLabel("Background "), 0, 0);
  lcdGridLayout->addWidget(_lcdBkgColorPickB, 0, 1);
  lcdGridLayout->addWidget(new QLabel("Active"), 1, 0);
  lcdGridLayout->addWidget(_lcdActiveColorPickB, 1, 1);
  lcdGridLayout->addWidget(new QLabel("Inactive"), 2, 0);
  lcdGridLayout->addWidget(_lcdInactiveColorPickB, 2, 1);
  lcdGridLayout->setColumnStretch(3, 1);
  colorGroupBox->setLayout(lcdGridLayout);

  // Set correct states
  QSettings settings;
  if (!settings.contains("Synth/auto_power_on")) {
    settings.setValue("Synth/auto_power_on", true);
    settings.setValue("Synth/show_statusbar", false);
    settings.setValue("Synth/startup_animations", "all");

    QColor bkgColor = _scene->get_lcd_bkg_on_color_reset();
    QColor activeColor = _scene->get_lcd_active_on_color_reset();
    QColor inactiveColor = _scene->get_lcd_inactive_on_color_reset();
    settings.setValue("Synth/lcd_bkg_color", bkgColor.name());
    settings.setValue("Synth/lcd_active_color", activeColor.name());
    settings.setValue("Synth/lcd_inactive_color", inactiveColor.name());  
  }

  _autoPowerOnCB->setChecked(settings.value("Synth/auto_power_on").toBool());
  _rememberLayoutCB->setChecked(settings.value("remember_layout").toBool());
  _enableKbdMidi->setChecked(settings.value("kbd_midi_input").toBool());

  QString animSetting = settings.value("Synth/startup_animations").toString();
  if (animSetting == "only_rom")
    _romAnim->setChecked(true);
  else if (animSetting == "none")
    _noAnim->setChecked(true);
  else
    _emuscAnim->setChecked(true);
  
  connect(_autoPowerOnCB, SIGNAL(toggled(bool)),
	  this, SLOT(_autoPowerOn_toggled(bool)));
  connect(_rememberLayoutCB, SIGNAL(toggled(bool)),
	  this, SLOT(_remember_layout_toggled(bool)));
  connect(_enableKbdMidi, SIGNAL(toggled(bool)),
	  this, SLOT(_enableKbdMidi_toggled(bool)));
  connect(_lcdBkgColorPickB, SIGNAL(clicked(void)),
	  this, SLOT(_lcd_bkg_colorpick_clicked(void)));
  connect(_lcdActiveColorPickB, SIGNAL(clicked(void)),
	  this, SLOT(_lcd_active_colorpick_clicked(void)));
  connect(_lcdInactiveColorPickB, SIGNAL(clicked(void)),
	  this, SLOT(_lcd_inactive_colorpick_clicked(void)));
  connect(_emuscAnim, SIGNAL(toggled(bool)),
	  this, SLOT(_emuscAnim_toggled(bool)));
  connect(_romAnim, SIGNAL(toggled(bool)),
	  this, SLOT(_romAnim_toggled(bool)));
  connect(_noAnim, SIGNAL(toggled(bool)),
	  this, SLOT(_noAnim_toggled(bool)));

  vboxLayout->addWidget(_autoPowerOnCB);
  vboxLayout->addWidget(_rememberLayoutCB);
  vboxLayout->addWidget(_enableKbdMidi);
  vboxLayout->addSpacing(15);
  vboxLayout->addWidget(animGroupBox);
  vboxLayout->addSpacing(15);
  vboxLayout->addWidget(colorGroupBox);
  vboxLayout->insertSpacing(1, 15);
  vboxLayout->addStretch(0);
 
  setLayout(vboxLayout);
}


void GeneralSettings::reset(void)
{
  _autoPowerOnCB->setChecked(true);
  _rememberLayoutCB->setChecked(false);
  _emuscAnim->setChecked(true);

  QColor bkgColor = _scene->get_lcd_bkg_on_color_reset();
  QColor activeColor = _scene->get_lcd_active_on_color_reset();
  QColor inactiveColor = _scene->get_lcd_inactive_on_color_reset();

  _scene->set_lcd_bkg_on_color(bkgColor, _emulator->running());
  _scene->set_lcd_active_on_color(activeColor, _emulator->running());
  _scene->set_lcd_inactive_on_color(inactiveColor);

  _set_button_color(_lcdBkgColorPickB, bkgColor);
  _set_button_color(_lcdActiveColorPickB, activeColor);
  _set_button_color(_lcdInactiveColorPickB, inactiveColor);

  QSettings settings;
  settings.setValue("Synth/auto_power_on", true);
  settings.setValue("Synth/show_statusbar", false);
  settings.setValue("Synth/startup_animations", "all");
  settings.setValue("Synth/lcd_bkg_color", bkgColor.name());
  settings.setValue("Synth/lcd_active_color", activeColor.name());
  settings.setValue("Synth/lcd_inactive_color", inactiveColor.name());  
}


void GeneralSettings::_autoPowerOn_toggled(bool checked)
{
  QSettings settings;
  settings.setValue("Synth/auto_power_on", checked);
}


void GeneralSettings::_remember_layout_toggled(bool checked)
{
  QSettings settings;
  settings.setValue("remember_layout", checked);
}


void GeneralSettings::_enableKbdMidi_toggled(bool checked)
{
  QSettings settings;
  settings.setValue("kbd_midi_input", checked);
  _scene->set_midi_kbd_enable(checked);
}


void GeneralSettings::_lcd_bkg_colorpick_clicked(void)
{
  const QColor color = QColorDialog::getColor(_scene->get_lcd_bkg_on_color(),
                                              this,
					      "Select background color");

  if (color.isValid()) {
    _set_button_color(_lcdBkgColorPickB, color);
    _scene->set_lcd_bkg_on_color(color, _emulator->running());

    QSettings settings;
    settings.setValue("Synth/lcd_bkg_color", color.name());
  }
}


void GeneralSettings::_lcd_active_colorpick_clicked(void)
{
  const QColor color = QColorDialog::getColor(_scene->get_lcd_active_on_color_reset(),
                                              this,
					      "Select active color");

  if (color.isValid()) {
    _set_button_color(_lcdActiveColorPickB, color);
    _scene->set_lcd_active_on_color(color, _emulator->running());

    QSettings settings;
    settings.setValue("Synth/lcd_active_color", color.name());
  }
}


void GeneralSettings::_lcd_inactive_colorpick_clicked(void)
{
  const QColor color = QColorDialog::getColor(_scene->get_lcd_inactive_on_color_reset(),
                                              this,
					      "Select inactive color");

  if (color.isValid()) {
    _set_button_color(_lcdInactiveColorPickB, color);
    _scene->set_lcd_inactive_on_color(color);

    QSettings settings;
    settings.setValue("Synth/lcd_inactive_color", color.name());
  }
}


void GeneralSettings::_emuscAnim_toggled(bool checked)
{
  if (checked) {
    QSettings settings;
    settings.setValue("Synth/startup_animations", "all");
  }
}


void GeneralSettings::_romAnim_toggled(bool checked)
{
  if (checked) {
    QSettings settings;
    settings.setValue("Synth/startup_animations", "only_rom");
  }
}


void GeneralSettings::_noAnim_toggled(bool checked)
{
  if (checked) {
    QSettings settings;
    settings.setValue("Synth/startup_animations", "none");
  }
}


void GeneralSettings::_set_button_color(QPushButton *button, QColor color)
{
  if (!button)
    return;

  button->setStyleSheet(QString("background-color: %1; "
				"border: 1px; "
				"border-radius: 3px; "
				"border-color: #ababab; "
				"border-style: solid;").arg(color.name()));
}


AudioSettings::AudioSettings(Emulator *emulator, QWidget *parent)
  : _emulator(emulator)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("Audio Settings");
  QFont f("Arial", 12, QFont::Bold);
  headerL->setFont( f);
  vboxLayout->addWidget(headerL);

  QGridLayout *gridLayout = new QGridLayout();

  gridLayout->addWidget(new QLabel("Audio system"), 0, 0);
  _systemBox = new QComboBox();
  QHBoxLayout *systemLayout = new QHBoxLayout();
  systemLayout->addWidget(_systemBox);
  systemLayout->addStretch();
  gridLayout->addLayout(systemLayout, 0, 1);

#ifdef __ALSA_AUDIO__
  _systemBox->addItem("ALSA");
#endif
#ifdef __JACK_AUDIO__
  _systemBox->addItem("JACK");
#endif
#ifdef __PULSE_AUDIO__
  _systemBox->addItem("Pulse");
#endif
#ifdef __QT_AUDIO__
  _systemBox->addItem("Qt");
#endif
#ifdef __WAV_AUDIO__
  _systemBox->addItem("WAV");
#endif
#ifdef __WIN32_AUDIO__
  _systemBox->addItem("Win32");
#endif
#ifdef __CORE_AUDIO__
  _systemBox->addItem("Core Audio");
#endif
  _systemBox->addItem("Null");

  _deviceLabel = new QLabel("Audio device");
  gridLayout->addWidget(_deviceLabel, 1, 0);
  _deviceBox = new QComboBox();
  _deviceBox->setMinimumContentsLength(20);
  _deviceBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  gridLayout->addWidget(_deviceBox, 1, 1, 1, 3);

  gridLayout->setRowMinimumHeight(2, 15);
  
  _bufferTimeLabel = new QLabel("Buffer time (µs)");
  gridLayout->addWidget(_bufferTimeLabel, 3, 0);
  QHBoxLayout *bufferTimeLayout = new QHBoxLayout();
  _bufferTimeSB = new QSpinBox();
  _bufferTimeSB->setRange(0, 1000000);
  _bufferTimeSB->setSingleStep(100);
  bufferTimeLayout->addWidget(_bufferTimeSB);
  bufferTimeLayout->addStretch();
  gridLayout->addLayout(bufferTimeLayout, 3, 1);

  _periodTimeLabel = new QLabel("Period time (µs)");
  gridLayout->addWidget(_periodTimeLabel, 4, 0);
  QHBoxLayout *periodTimeLayout = new QHBoxLayout();
  _periodTimeSB = new QSpinBox();
  _periodTimeSB->setRange(0, 1000000);
  _periodTimeSB->setSingleStep(100);
  periodTimeLayout->addWidget(_periodTimeSB);
  periodTimeLayout->addStretch();
  gridLayout->addLayout(periodTimeLayout, 4, 1);

  _sampleRateLabel = new QLabel("Sample rate (Hz)");
  gridLayout->addWidget(_sampleRateLabel, 5, 0);
  QHBoxLayout *sampleRateLayout = new QHBoxLayout();
  _sampleRateSB = new QSpinBox();
  _sampleRateSB->setRange(0, 96000);
  _sampleRateSB->setSingleStep(100);
  sampleRateLayout->addWidget(_sampleRateSB);
  sampleRateLayout->addStretch();
  gridLayout->addLayout(sampleRateLayout, 5, 1);

  _channelsLabel = new QLabel("Channels");
  gridLayout->addWidget(_channelsLabel, 6, 0);
  QHBoxLayout *channelsLayout = new QHBoxLayout();
  _channelsCB = new QComboBox();
//  _channelsCB->addItem("Mono");
  _channelsCB->addItem("Stereo");
  _channelsLabel->setEnabled(false); // FIXME
  _channelsCB->setEnabled(false);    // TODO: Fix support for selecting channels
  channelsLayout->addWidget(_channelsCB);
  channelsLayout->addStretch();
  gridLayout->addLayout(channelsLayout, 6, 1);

  gridLayout->setRowMinimumHeight(7, 15);

  _filePathLabel = new QLabel("File path");
  gridLayout->addWidget(_filePathLabel, 8, 0);

  _filePathLE = new QLineEdit(this);
//  _filePathLE->setFixedWidth(440); // set anything > 0
  gridLayout->addWidget(_filePathLE, 8, 1, 1, 3);

  _fileDialogTB = new QToolButton();
  _fileDialogTB->setToolButtonStyle(Qt::ToolButtonTextOnly);
  _fileDialogTB->setText("...");
  gridLayout->addWidget(_fileDialogTB, 8, 4);

  gridLayout->setRowMinimumHeight(9, 15);

  gridLayout->addWidget(new QLabel("Interpolation"), 10, 0);
  _interpolBox = new QComboBox();
  _interpolBox->addItem("Nearest", 0);
  _interpolBox->addItem("Linear", 1);
  _interpolBox->addItem("Cubic", 2);

  QHBoxLayout *interpolLayout = new QHBoxLayout();
  interpolLayout->addWidget(_interpolBox);
  interpolLayout->addWidget(new QLabel("Default: Cubic"));
  interpolLayout->addStretch();
  gridLayout->addLayout(interpolLayout, 10, 1);

  vboxLayout->addLayout(gridLayout);
  vboxLayout->addSpacing(15);

  _reverseStereo = new QCheckBox("Reverse Stereo");
  _reverseStereo->setEnabled(false);             // TODO: Not implemented yet
  vboxLayout->addWidget(_reverseStereo);

  vboxLayout->addStretch(0);

  if (_emulator->running()) {
    QHBoxLayout *restartInfoLayout = new QHBoxLayout();
    QLabel *infoTextL = new QLabel("Changes will take effect next time the "
				   "synth is started");
    QLabel *infoIconL = new QLabel();
    QIcon icon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
    QPixmap pixmap = icon.pixmap(QSize(32, 32));
    infoIconL->setPixmap(pixmap);

    restartInfoLayout->addStretch(0);
    restartInfoLayout->addWidget(infoIconL);
    restartInfoLayout->addWidget(infoTextL);
    restartInfoLayout->addStretch(0);

    vboxLayout->addLayout(restartInfoLayout);
    vboxLayout->addSpacing(15);
  }

  // Finally read & update settings from config file
  QSettings settings;
  if (!settings.contains("Audio/system"))
    reset();

  // Set best audio system for each system if no audio config exist
  if (!settings.contains("Audio/system")) {
#ifdef __ALSA_AUDIO__
    _systemBox->setCurrentText("ALSA");
#elif __CORE_AUDIO__
    _systemBox->setCurrentText("Core Audio");
#elif __WIN32_AUDIO__
    _systemBox->setCurrentText("Win32");
#endif
  } else {
    _systemBox->setCurrentText(settings.value("Audio/system").toString());
  }

  _deviceBox->setCurrentText(settings.value("Audio/device").toString());
  _system_box_changed(0);

  int bufferTime = settings.value("Audio/buffer_time").toInt();
  if (!bufferTime) bufferTime = _defaultBufferTime;
  int periodTime = settings.value("Audio/period_time").toInt();
  if (!periodTime) periodTime = _defaultPeriodTime;
  int sampleRate = settings.value("Audio/sample_rate").toInt();
  if (!sampleRate) sampleRate = _defaultSampleRate;

  bool stereo = settings.value("Audio/stereo").toBool();

  _bufferTimeSB->setValue(bufferTime);
  _periodTimeSB->setValue(periodTime);
  _sampleRateSB->setValue(sampleRate);
//  _channelsCB->setCurrentIndex((int) stereo);
  _filePathLE->setText(settings.value("Audio/wav_file_path").toString());
  _interpolBox->setCurrentText(settings.value("Audio/interpolation").toString());

  connect(_fileDialogTB, SIGNAL(clicked()),
	  this, SLOT(_open_file_path_dialog()));
  connect(_systemBox, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_system_box_changed(int)));
  connect(_deviceBox, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_device_box_changed(int)));
  connect(_bufferTimeSB, SIGNAL(valueChanged(int)),
	  this, SLOT(_bufferTimeSB_changed(int)));
  connect(_periodTimeSB, SIGNAL(valueChanged(int)),
	  this, SLOT(_periodTimeSB_changed(int)));
  connect(_sampleRateSB, SIGNAL(valueChanged(int)),
	  this, SLOT(_sampleRateSB_changed(int)));
  connect(_filePathLE, SIGNAL(editingFinished()),
	  this, SLOT(_filePathLE_changed()));
//  connect(_channelsCB, SIGNAL(currentIndexChanged(int)),
//	  this, SLOT(_channels_box_changed(int)));
  connect(_interpolBox, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_interpolation_box_changed(int)));

  vboxLayout->addStretch(0);
  vboxLayout->insertSpacing(1, 15);
  vboxLayout->insertSpacing(6, 85);
  setLayout(vboxLayout);  
  }


void AudioSettings::reset(void)
{
  _bufferTimeSB->setValue(_defaultBufferTime);
  _periodTimeSB->setValue(_defaultPeriodTime);
  _sampleRateSB->setValue(_defaultSampleRate);
//  _channelsCB->setCurrentIndex(1);                              // Stereo
  _interpolBox->setCurrentIndex(2);                              // Cubic

  QSettings settings;
  settings.setValue("Audio/buffer_time", _defaultBufferTime);
  settings.setValue("Audio/period_time", _defaultPeriodTime);
  settings.setValue("Audio/sample_rate", _defaultSampleRate);
  settings.setValue("Audio/interpolation", "Cubic");
}


void AudioSettings::_system_box_changed(int index)
{
  _deviceBox->clear();

  if (!_systemBox->currentText().compare("alsa", Qt::CaseInsensitive)) {
#ifdef __ALSA_AUDIO__
    QStringList devices = AudioOutputAlsa::get_available_devices();
    for (auto d : devices)
      _deviceBox->addItem(d);
#endif

    _deviceLabel->setEnabled(true);
    _deviceBox->setEnabled(true);
    _bufferTimeLabel->setEnabled(true);
    _periodTimeLabel->setEnabled(true);
    _sampleRateLabel->setEnabled(true);
    _sampleRateSB->setEnabled(true);
    _bufferTimeSB->setEnabled(true);
    _periodTimeSB->setEnabled(true);
//    _channelsLabel->setEnabled(true);
//    _channelsCB->setEnabled(true);
    _filePathLabel->setEnabled(false);
    _filePathLE->setEnabled(false);
    _fileDialogTB->setEnabled(false);

  } else if (!_systemBox->currentText().compare("Qt", Qt::CaseInsensitive)) {
#ifdef __QT_AUDIO__
    QStringList devices = AudioOutputQt::get_available_devices();
    for (auto d : devices)
      _deviceBox->addItem(d);
#endif

    _deviceLabel->setEnabled(true);
    _deviceBox->setEnabled(true);
    _bufferTimeLabel->setEnabled(true);
    _periodTimeLabel->setEnabled(false);
    _sampleRateLabel->setEnabled(true);
    _sampleRateSB->setEnabled(true);
    _bufferTimeSB->setEnabled(true);
    _periodTimeSB->setEnabled(false);
//    _channelsLabel->setEnabled(true);
//    _channelsCB->setEnabled(true);
    _filePathLabel->setEnabled(false);
    _filePathLE->setEnabled(false);
    _fileDialogTB->setEnabled(false);

  } else if (!_systemBox->currentText().compare("jack", Qt::CaseInsensitive)) {
    _deviceLabel->setEnabled(false);
    _deviceBox->setEnabled(false);
    _bufferTimeLabel->setEnabled(false);
    _periodTimeLabel->setEnabled(false);
    _sampleRateLabel->setEnabled(false);
    _sampleRateSB->setEnabled(false);
    _bufferTimeSB->setEnabled(false);
    _periodTimeSB->setEnabled(false);
//    _channelsLabel->setEnabled(false);
//    _channelsCB->setEnabled(false);
    _filePathLabel->setEnabled(false);
    _filePathLE->setEnabled(false);
    _fileDialogTB->setEnabled(false);

  } else if (!_systemBox->currentText().compare("pulse", Qt::CaseInsensitive)) {
    _deviceLabel->setEnabled(false);
    _deviceBox->setEnabled(false);
    _bufferTimeLabel->setEnabled(false);
    _periodTimeLabel->setEnabled(false);
    _sampleRateLabel->setEnabled(false);
    _sampleRateSB->setEnabled(false);
    _bufferTimeSB->setEnabled(false);
    _periodTimeSB->setEnabled(false);
//    _channelsLabel->setEnabled(false);
//    _channelsCB->setEnabled(false);
    _filePathLabel->setEnabled(false);
    _filePathLE->setEnabled(false);
    _fileDialogTB->setEnabled(false);

  } else if (!_systemBox->currentText().compare("win32", Qt::CaseInsensitive)) {

#ifdef __WAV_AUDIO__
  } else if (!_systemBox->currentText().compare("wav", Qt::CaseInsensitive)) {
    _deviceBox->addItem("File Writer");

    _deviceLabel->setEnabled(true);
    _deviceBox->setEnabled(true);
    _bufferTimeLabel->setEnabled(false);
    _periodTimeLabel->setEnabled(false);
    _sampleRateLabel->setEnabled(true);
    _sampleRateSB->setValue(44100);
    _sampleRateSB->setEnabled(true);
    _bufferTimeSB->setEnabled(false);
    _periodTimeSB->setEnabled(false);
//    _channelsLabel->setEnabled(false);
//    _channelsCB->setEnabled(false);
    _filePathLabel->setEnabled(true);
    _filePathLE->setEnabled(true);
    _fileDialogTB->setEnabled(true);
#endif

#ifdef __WIN32_AUDIO__
    QStringList devices = AudioOutputWin32::get_available_devices();
    for (auto d : devices)
      _deviceBox->addItem(d);
#endif

  } else if (!_systemBox->currentText().compare("core audio",
						Qt::CaseInsensitive)) {

#ifdef __CORE_AUDIO__
    QStringList devices = AudioOutputCore::get_available_devices();
    for (auto d : devices)
      _deviceBox->addItem(d);
#endif

  } else if (!_systemBox->currentText().compare("null", Qt::CaseInsensitive)) {
    _deviceBox->setEnabled(false);
    _deviceLabel->setEnabled(false);
    _bufferTimeLabel->setEnabled(false);
    _periodTimeLabel->setEnabled(false);
    _sampleRateLabel->setEnabled(false);
    _sampleRateSB->setEnabled(false);
    _bufferTimeSB->setEnabled(false);
    _periodTimeSB->setEnabled(false);
//    _channelsLabel->setEnabled(false);
//    _channelsCB->setEnabled(false);
    _filePathLabel->setEnabled(false);
    _filePathLE->setEnabled(false);
    _fileDialogTB->setEnabled(false);
  }

  QSettings settings;
  settings.setValue("Audio/system", _systemBox->currentText());
  _deviceBox->setCurrentText(settings.value("Audio/device").toString());
  adjustSize();
}


void AudioSettings::_device_box_changed(int index)
{
  QSettings settings;
  settings.setValue("Audio/device", _deviceBox->currentText());
}


void AudioSettings::_channels_box_changed(int index)
{
  if (index)
    _reverseStereo->setEnabled(true);
  else
    _reverseStereo->setEnabled(false);
}


void AudioSettings::_bufferTimeSB_changed(int value)
{
  QSettings settings;
  settings.setValue("Audio/buffer_time", value);
}


void AudioSettings::_periodTimeSB_changed(int value)
{
  QSettings settings;
  settings.setValue("Audio/period_time", value);
}


void AudioSettings::_sampleRateSB_changed(int value)
{
  QSettings settings;
  settings.setValue("Audio/sample_rate", value);
}


void AudioSettings::_filePathLE_changed(void)
{
  QSettings settings;
  settings.setValue("Audio/wav_file_path", _filePathLE->text());
}


void AudioSettings::_open_file_path_dialog(void)
{
  QFileDialog dialog(this, "Select file name and location for WAV recording");
  dialog.setFileMode(QFileDialog::AnyFile);

  QStringList fileNames;
  if (dialog.exec()) {
    fileNames = dialog.selectedFiles();
    _filePathLE->setText(fileNames[0]);
  }
}


void AudioSettings::_interpolation_box_changed(int index)
{
  QSettings settings;
  settings.setValue("Audio/interpolation", _interpolBox->currentText());
  _emulator->set_interpolation_mode(index);
}


MidiSettings::MidiSettings(Emulator *emulator, Scene *scene, QWidget *parent)
  : _emulator(emulator)
{
  QSettings settings;
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("MIDI Settings");
  QFont f("Arial", 12, QFont::Bold);
  headerL->setFont(f);
  vboxLayout->addWidget(headerL);


  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->addWidget(new QLabel("MIDI System"), 0, 0);
  gridLayout->addWidget(new QLabel("MIDI Device"), 1, 0);

  _systemCB = new QComboBox();
  gridLayout->addWidget(_systemCB, 0, 1);
  _deviceCB = new QComboBox();
  gridLayout->addWidget(_deviceCB, 1, 1);

  _portsListLW = new QListWidget();
  _update_ports_list();
  QVBoxLayout *portsLayout = new QVBoxLayout();
  portsLayout->addWidget(_portsListLW);
  QGroupBox *portsGroupBox = new QGroupBox("List of connected MIDI output ports");
  portsGroupBox->setLayout(portsLayout);

  if (!emulator->running())
    portsGroupBox->setEnabled(false);

  connect(_systemCB, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_systemCB_changed(int)));
  connect(_deviceCB, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(_deviceCB_changed(int)));
  connect(_portsListLW, SIGNAL(itemChanged(QListWidgetItem*)),
	  this, SLOT(_ports_list_changed(QListWidgetItem*)));

#ifdef __ALSA_MIDI__
  _systemCB->addItem("ALSA");
#endif
#ifdef __WIN32_MIDI__
  _systemCB->addItem("Win32");
#endif
#ifdef __CORE_MIDI__
  _systemCB->addItem("Core MIDI");
#endif

  if (settings.contains("Midi/system"))
    settings.setValue("Midi/system", _systemCB->itemText(0));
  if (settings.contains("Midi/device"))
    settings.setValue("Midi/device", _deviceCB->itemText(0));

  QString systemStr = settings.value("Midi/system").toString();
  int systemIndex = _systemCB->findText(systemStr);
  if (systemIndex < 0)
    std::cerr << "EmuSC: Illegal configuration for MIDI system" << std::endl
	      << systemStr.toStdString() << std::endl;
  else
    _systemCB->setCurrentIndex(systemIndex);

  QString deviceStr = settings.value("Midi/device").toString();
  int deviceIndex = _deviceCB->findText(deviceStr);
  if (deviceIndex < 0)
    std::cerr << "EmuSC: Illegal confguration for MIDI device" << std::endl;
  else
    _deviceCB->setCurrentIndex(deviceIndex);

  gridLayout->setColumnStretch(2, 1);
  vboxLayout->addLayout(gridLayout);
  vboxLayout->addSpacing(15);
  vboxLayout->addWidget(portsGroupBox);
  vboxLayout->addStretch(0);
  vboxLayout->insertSpacing(1, 15);
  setLayout(vboxLayout);
}


void MidiSettings::reset(void)
{
}


void MidiSettings::_update_ports_list(void)
{
  // 1. Get a list of all ports from the active System / Device
#ifdef __ALSA_MIDI__
  QStringList ports = MidiInputAlsa::get_available_ports();
  QStringList connections;

  // Update port list with current connections if emulator is running
  if (_emulator->running() && _emulator->get_midi_driver())
    connections = _emulator->get_midi_driver()->list_subscribers();

  for (const auto &p : ports) {
    QListWidgetItem *item = new QListWidgetItem(p);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);
    _portsListLW->addItem(item);

    if (!connections.isEmpty()) {
      QString portNum = p.left(p.indexOf(":") + 2);

      for (const auto &c : connections) {
	if (portNum.trimmed() == c)
	  item->setCheckState(Qt::Checked);
      }
    }
  }
#endif
}


void MidiSettings::_ports_list_changed(QListWidgetItem *item)
{
  if (!_emulator->running() || !_emulator->get_midi_driver())
    return;

  try {
  // if ALSA etc
    _emulator->get_midi_driver()->connect_port(item->text(),
					       item->checkState());
  } catch(QString errorMsg) {
    QMessageBox::critical(this,
			  "Connection failure",
			  QString("Failed to connect or disconnect MIDI port.\n"
				  "Error message: ") + errorMsg,
			  QMessageBox::Close);
  }
}


void MidiSettings::_systemCB_changed(int index)
{
  _deviceCB->clear();

  if (!_systemCB->currentText().compare("Core MIDI", Qt::CaseInsensitive)) {
    // List all available MIDI devices - or show a warning if none    
#ifdef __CORE_MIDI__
    QStringList devices = MidiInputCore::get_available_devices();

    for (auto d : devices)
      _deviceCB->addItem(d);
#endif

  } else if (!_systemCB->currentText().compare("alsa", Qt::CaseInsensitive)) {
#ifdef __ALSA_MIDI__
    QStringList devices = MidiInputAlsa::get_available_devices();

    for (auto d : devices)
      _deviceCB->addItem(d);
#endif

  } else if (!_systemCB->currentText().compare("win32", Qt::CaseInsensitive)) {
#ifdef __WIN32_MIDI__
    QStringList devices = MidiInputWin32::get_available_devices();

    for (auto d : devices)
      _deviceCB->addItem(d);
#endif
  }

  QSettings settings;
  settings.setValue("Midi/system", _systemCB->currentText());
}


void MidiSettings::_deviceCB_changed(int index)
{
  QSettings settings;
  settings.setValue("Midi/device", _deviceCB->currentText());
}


RomSettings::RomSettings(Emulator *emulator, QWidget *parent)
  : _emulator(emulator)
{
  QVBoxLayout *vboxLayout = new QVBoxLayout();

  QLabel *headerL = new QLabel("ROM Settings");
  QFont f( "Arial", 12, QFont::Bold);
  headerL->setFont( f);
  vboxLayout->addWidget(headerL);

  vboxLayout->addSpacing(15);
  vboxLayout->addWidget(new QLabel("Selected ROM files:"));

  _romTableView = new QTableView();
  _romModel = new QStandardItemModel(0, 5);

  QStringList romHeaders;
  romHeaders << " Select " << "Status" << " Model " << "Version" << "Date"
	     << "GS Version" << "Index" << "File";
  _romModel->setHorizontalHeaderLabels(romHeaders);
  _romTableView->setModel(_romModel);
  _romTableView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
  _romTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  vboxLayout->addWidget(_romTableView);

  // Add all types as default table data
  QStringList itemNames;
  itemNames << "Prog" << "CPU" << "Wave1" << "Wave2" << "Wave3";
  for (int i = 0; i < 5; i++) {
    QStandardItem *item = new QStandardItem(itemNames[i]);
    item->setBackground(QColor(235, 235, 235));
    _romModel->setItem(i, 0, item);
  }
  connect(_romTableView, SIGNAL(clicked(QModelIndex)),
          this, SLOT(_table_view_clicked(QModelIndex)));

  QSettings settings;
  _update_rom_table_progrom(settings.value("Rom/prog").toString());
  _update_rom_table_cpurom(settings.value("Rom/cpu").toString());
  _update_rom_table_waveroms(settings.value("Rom/wave1").toString(), 0);
  _update_rom_table_waveroms(settings.value("Rom/wave2").toString(), 1);
  _update_rom_table_waveroms(settings.value("Rom/wave3").toString(), 2);

  QLabel *info = new QLabel;
  info->setTextFormat(Qt::RichText);
  info->setText("EmuSC needs 3 type of ROM files:<ul>"
                "<li>Prog: An external program EPROM</li>"
                "<li>CPU: An integrated EPROM on the CPU</li>"
                "<li>Wave 1-3: ROM files containing PCM audio</li></ul>"
                "All ROM files must belong to the same ROM set (green status) "
                "as different versions might be incompatible.");
  info->setWordWrap(true);
  vboxLayout->addSpacing(15);
  vboxLayout->addWidget(info);
  vboxLayout->addStretch(0);
  setLayout(vboxLayout);
}


void RomSettings::reset(void)
{
}


void RomSettings::_table_view_clicked(const QModelIndex &index)
{
  if (index.column() != 0)
    return;

  QSettings settings;

  if (index.row() == 0) {
    QFileInfo fi(settings.value("Rom/prog").toString());
    QString path = fi.absolutePath();
    if (path.isEmpty())
      path = QDir::homePath();

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select program ROM file"),
                                                    path,
                                                    tr("ROMs (*.bin *.rom)"));

    if (!fileName.isEmpty()) {
      settings.setValue("Rom/prog", fileName);
      _update_rom_table_progrom(fileName);
    }

  } else if (index.row() == 1) {
    QString file(settings.value("Rom/cpu").toString());
    if (file.isEmpty())
      file = settings.value("Rom/prog").toString();

    QFileInfo fi(file);
    QString path = fi.absolutePath();
    if (path.isEmpty())
      path = QDir::homePath();

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select CPU ROM file"),
                                                    path,
                                                    tr("ROMs (*.bin *.rom)"));

    if (!fileName.isEmpty()) {
      settings.setValue("Rom/cpu", fileName);
      _update_rom_table_cpurom(fileName);
    }

  } else if (index.row() == 2) {
    QString file(settings.value("Rom/wave1").toString());
    if (file.isEmpty())
      file = settings.value("Rom/prog").toString();

    QFileInfo fi(file);
    QString path = fi.absolutePath();
    if (path.isEmpty())
      path = QDir::homePath();

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select 1st wave ROM file"),
                                                    path,
                                                    tr("ROMs (*.bin *.rom)"));

    if (!fileName.isEmpty())
      settings.setValue("Rom/wave1", fileName);

    _update_rom_table_waveroms(fileName, 0);

  } else if (index.row() == 3) {
    QString file(settings.value("Rom/wave2").toString());
    if (file.isEmpty())
      file = settings.value("Rom/wave1").toString();
    if (file.isEmpty())
      file = settings.value("Rom/prog").toString();

    QFileInfo fi(file);
    QString path = fi.absolutePath();
    if (path.isEmpty())
      path = QDir::homePath();

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select 2nd wave ROM file"),
                                                    path,
                                                    tr("ROMs (*.bin *.rom)"));

    if (!fileName.isEmpty())
      settings.setValue("Rom/wave2", fileName);

    _update_rom_table_waveroms(fileName, 1);

  } else if (index.row() == 4) {
    QString file(settings.value("Rom/wave3").toString());
    if (file.isEmpty())
      file = settings.value("Rom/wave2").toString();
    if (file.isEmpty())
      file = settings.value("Rom/wave1").toString();
    if (file.isEmpty())
      file = settings.value("Rom/prog").toString();

    QFileInfo fi(file);
    QString path = fi.absolutePath();
    if (path.isEmpty())
      path = QDir::homePath();

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select 3rd wave ROM file"),
                                                    path,
                                                    tr("ROMs (*.bin *.rom)"));

    if (!fileName.isEmpty())
      settings.setValue("Rom/wave3", fileName);

    _update_rom_table_waveroms(fileName, 2);
  }
}


QString RomSettings::_get_file_sha256(QString filePath)
{
  QCryptographicHash hash(QCryptographicHash::Sha256);

  QFile f(filePath);
  if (f.open(QFile::ReadOnly)) {
    if (!hash.addData(&f)) {
      QMessageBox::critical(this, tr("EmuSC"),
                            tr("Unable to read ROM files, do you have read "
                               "access?"), QMessageBox::Close);
      return QString("");
    }
  }
  f.close();

  return QString(hash.result().toHex().data());
}


void RomSettings::_add_table_row(int row, QStringList text, QColor c)
{
  QStandardItem *status = new QStandardItem(text[0]);
  status->setTextAlignment(Qt::AlignCenter);
  if (c != QColor(0,0,0))
    status->setBackground(c);
  _romModel->setItem(row, 1, status);

  for (int i = 1; i <= 6; i++) {
    if (text.size() <= i)
      break;

    QStandardItem *item = new QStandardItem(text[i]);
    _romModel->setItem(row, i + 1, item);

    if (i == 6)
      item->setTextAlignment(Qt::AlignLeft);
    else
      item->setTextAlignment(Qt::AlignCenter);
  }
}


void RomSettings::_update_rom_table_progrom(QString fileName)
{
  QString sha256 = _get_file_sha256(fileName);
  struct ROMInfo::ROMSetInfo *romSetInfo;
  romSetInfo = _romInfo.get_rom_set_info_from_prog(sha256.toStdString());

  QStringList textList;
  if (fileName.isEmpty()) {
    textList << "Missing" << "" << "" << "" << "" << "" << fileName;
    _add_table_row(0, textList, QColor(255, 122, 122));
  } else  if (romSetInfo == nullptr) {
    textList << "Incompatible" << "" << "" << "" << "" << "" << fileName;
    _add_table_row(0, textList, QColor(255, 122, 122));
  } else {
    textList << "OK"
             << QString(romSetInfo->controlROMs.model)
             << QString(romSetInfo->controlROMs.version)
             << QString(romSetInfo->controlROMs.date)
             << QString(romSetInfo->controlROMs.gsVersion)
             << "1/1"
             << fileName;
    _add_table_row(0, textList, QColor(122, 255, 122));
  }

  _romTableView->resizeColumnsToContents();
}


void RomSettings::_update_rom_table_cpurom(QString fileName)
{
  QString sha256 = _get_file_sha256(fileName);
  struct ROMInfo::ROMSetInfo *romSetInfo;
  romSetInfo = _romInfo.get_rom_set_info_from_cpu(sha256.toStdString());

  QStringList textList;
  if (fileName.isEmpty()) {
    textList << "Missing" << "" << "" << "" << "" << "" << fileName;
    _add_table_row(1, textList, QColor(255, 122, 122));
  } else  if (romSetInfo == nullptr) {
    textList << "Incompatible" << "" << "" << "" << "" << "" << fileName;
    _add_table_row(1, textList, QColor(255, 122, 122));
  } else {
    textList << "OK"
             << QString(romSetInfo->controlROMs.model)
             << QString(romSetInfo->controlROMs.version)
             << QString(romSetInfo->controlROMs.date)
             << QString(romSetInfo->controlROMs.gsVersion)
             << "1/1"
             << fileName;
    _add_table_row(1, textList, QColor(122, 255, 122));
  }

  _romTableView->resizeColumnsToContents();
}

void RomSettings::_update_rom_table_waveroms(QString fileName, int index)
{
  QString sha256 = _get_file_sha256(fileName);
  struct ROMInfo::ROMSetInfo *romSetInfo;
  int pos;
  romSetInfo = _romInfo.get_rom_set_info_from_wave(sha256.toStdString(), &pos);

  QStringList textList;
  if (index == 2 && fileName.isEmpty()
      && _romModel->index(3,6).data().toString() == "2/2") {
    textList << "N/A";
    _add_table_row(index + 2, textList, QColor(122, 255, 122));
  } else if (fileName.isEmpty()) {
    textList << "Missing" << "" << "" << "" << "" << "" << fileName;
    _add_table_row(index + 2, textList, QColor(255, 122, 122));
  } else  if (romSetInfo == nullptr) {
    textList << "Incompatible" << "" << "" << "" << "" << "" << fileName;
    _add_table_row(index + 2, textList, QColor(255, 122, 122));
  } else  if (pos != index) {
    textList << "Wrong index" << "" << "" << "" << "" << "" << fileName;
    _add_table_row(index + 2, textList, QColor(255, 122, 122));
  } else {
    textList << "OK"
             << QString(romSetInfo->controlROMs.model)
             << QString(romSetInfo->controlROMs.version)
             << QString(romSetInfo->controlROMs.date)
             << QString(romSetInfo->controlROMs.gsVersion)
             << QString::number(pos+1) + QString("/") + QString::number(romSetInfo->waveROMs.numROMs)
             << fileName;
    _add_table_row(index + 2, textList, QColor(122, 255, 122));
  }

  // Remove any selected wave 3 ROM if we only need 2 wave ROM files
  if (index == 1 && pos == 1 && romSetInfo->waveROMs.numROMs == 2) {
    QSettings settings;
    settings.setValue("Rom/wave3", "");

    textList.clear();
    textList << "N/A";
    _add_table_row(4, textList, QColor(122, 255, 122));

  // Add missing flag if wave3 is empty & "N/A" while wave 2 change -> 3 part
  } else if (index == 1 && pos == 1 && romSetInfo->waveROMs.numROMs != 2
             && _romModel->index(4,1).data().toString() == "N/A") {
    textList.clear();
    textList << "Missing";
    _add_table_row(4, textList, QColor(255, 122, 122));
  }

  _romTableView->resizeColumnsToContents();
}
