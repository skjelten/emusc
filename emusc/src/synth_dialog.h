/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2023  Håkon Skjelten
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


#ifndef SYNTH_DIALOG_H
#define SYNTH_DIALOG_H


#include "emulator.h"
#include "scene.h"

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QString>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QKeyEvent>


class SynthDialog : public QDialog
{
  Q_OBJECT

private:
  class MasterSettings *_masterSettings;
  class ReverbSettings *_reverbSettings;
  class ChorusSettings *_chorusSettings;
  class PartMainSettings *_partMainSettings;
  class PartRxModeSettings *_partRxModeSettings;
  class PartToneSettings *_partToneSettings;
  class PartScaleSettings *_partScaleSettings;
  class PartControllerSettings *_partControllerSettings;
  class DrumSettings *_drumSettings;

  QStackedWidget *_stack;
  QListWidget *_menuList;
  
  Emulator *_emulator;
  Scene *_scene;
  int8_t _partId;

  void keyPressEvent(QKeyEvent *keyEvent);
  void keyReleaseEvent(QKeyEvent *keyEvent);

public:
  explicit SynthDialog(Emulator *emulator, Scene *s, QWidget *parent = nullptr);
  ~SynthDialog();

private slots:
  void accept(void);
  void _display_help(void);
  void _new_stack_item_focus(int index);
};


class MasterSettings : public QWidget
{
  Q_OBJECT

private:
  QSlider *_volumeS;
  QSlider *_panS;
  QSlider *_keyShiftS;
  QSlider *_tuneS;

  QLabel *_volumeL;
  QLabel *_panL;
  QLabel *_keyShiftL;
  QLabel *_tuneL;
  QLabel *_tuneHzL;

  QComboBox *_deviceIdC;

  QCheckBox *_rxSysExCh;
  QCheckBox *_rxGMOnCh;
  QCheckBox *_rxGSResetCh;
  QCheckBox *_rxInstChgCh;
  QCheckBox *_rxFuncCtrlCh;
  
  Emulator *_emulator;
  
public:
  explicit MasterSettings(Emulator *emulator, QWidget *parent = nullptr);

  void calculate_tune_value(uint32_t settings);
					      
private slots:
  void _volume_changed(int value);
  void _pan_changed(int value);
  void _keyShift_changed(int value);
  void _tune_changed(int value);
  void _device_id_changed(int value);
  void _rxSysEx_changed(int value);
  void _rxGMOn_changed(int value);
  void _rxGSReset_changed(int value);
  void _rxInstChg_changed(int value);
  void _rxFuncCtrl_changed(int value);
};


class ReverbSettings : public QWidget
{
  Q_OBJECT

private:
  QComboBox *_presetC;
  QComboBox *_characterC;

  QSlider *_levelS;
  QSlider *_filterS;
  QSlider *_timeS;
  QSlider *_feedbackS;
//  QSlider *_delayS;
  QSlider *_sendChoS;

  QLabel *_levelL;
  QLabel *_filterL;
  QLabel *_timeL;
  QLabel *_feedbackL;
//  QLabel *_delayL;
  QLabel *_sendChoL;

  Emulator *_emulator;

public:
  explicit ReverbSettings(Emulator *emulator, QWidget *parent = nullptr);

private slots:
  void _preset_changed(int value);
  void _character_changed(int value);

  void _level_changed(int value);
  void _filter_changed(int value);
  void _time_changed(int value);
  void _feedback_changed(int value);
  void _sendCho_changed(int value);
};


class ChorusSettings : public QWidget
{
  Q_OBJECT

private:
  QComboBox *_presetC;

  QSlider *_levelS;
  QSlider *_filterS;
  QSlider *_feedbackS;
  QSlider *_delayS;
  QSlider *_rateS;
  QSlider *_depthS;
  QSlider *_sendRevS;
//  QSlider *_sendDlyS;  // Note: SC-88 +

  QLabel *_levelL;
  QLabel *_filterL;
  QLabel *_feedbackL;
  QLabel *_delayL;
  QLabel *_rateL;
  QLabel *_depthL;
  QLabel *_sendRevL;
//  QSlider *_sendDlyL;  // Note: SC-88 +

  Emulator *_emulator;

public:
  explicit ChorusSettings(Emulator *emulator, QWidget *parent = nullptr);

private slots:
  void _preset_changed(int value);

  void _level_changed(int value);
  void _filter_changed(int value);
  void _feedback_changed(int value);
  void _delay_changed(int value);
  void _rate_changed(int value);
  void _depth_changed(int value);
  void _sendRev_changed(int value);
//  void _sendDly_changed(int value);  // Note: SC-88 +
};


class PartMainSettings : public QWidget
{
  Q_OBJECT

private:
  QComboBox *_partC;
  QComboBox *_midiChC;
  QComboBox *_instModeC;

  QSlider *_levelS;
  QSlider *_panS;
  QSlider *_keyShiftS;
  QSlider *_tuneS;
  QSlider *_reverbS;
  QSlider *_chorusS;
  QSlider *_fineTuneS;
  QSlider *_coarseTuneS;
  QSlider *_velDepthS;
  QSlider *_velOffsetS;
  QSlider *_keyRangeLS;
  QSlider *_keyRangeHS;

  QLabel *_levelL;
  QLabel *_panL;
  QLabel *_keyShiftL;
  QLabel *_tuneL;
  QLabel *_reverbL;
  QLabel *_chorusL;
  QLabel *_fineTuneL;
  QLabel *_coarseTuneL;
  QLabel *_velDepthL;
  QLabel *_velOffsetL;
  QLabel *_keyRangeLL;
  QLabel *_keyRangeHL;
  
  Emulator *_emulator;
  int8_t &_partId;

public:
  explicit PartMainSettings(Emulator *emulator, int8_t &partId,
			    QWidget *parent = nullptr);

  void update_all_widgets(void);

private slots:
  void _partC_changed(int value);

  void _midiCh_changed(int value);
  void _instMode_changed(int value);

  void _level_changed(int value);
  void _pan_changed(int value); 
  void _keyShift_changed(int value);
  void _tune_changed(int value);
  void _reverb_changed(int value);
  void _chorus_changed(int value);
  void _fineTune_changed(int value);
  void _coarseTune_changed(int value);
  void _velDepth_changed(int value);
  void _velOffset_changed(int value); 
  void _keyRangeL_changed(int value);
  void _keyRangeH_changed(int value); 
};


class PartRxModeSettings : public QWidget
{
  Q_OBJECT

private:
  QComboBox *_partC;
  QComboBox *_polyModeC;
  QComboBox *_assignModeC;

  QCheckBox *_rxVolumeCh;
  QCheckBox *_rxPanCh;
  QCheckBox *_rxNoteCh;
  QCheckBox *_rxProgramChangeCh;
  QCheckBox *_rxControlChangeCh;
  QCheckBox *_rxPitchBendCh;
  QCheckBox *_rxChAftertouchCh;
  QCheckBox *_rxPolyAftertouchCh;
  QCheckBox *_rxRPNCh;
  QCheckBox *_rxNRPNCh;
  QCheckBox *_rxModulationCh;
  QCheckBox *_rxHold1Ch;
  QCheckBox *_rxPortamentoCh;
  QCheckBox *_rxSostenutoCh;
  QCheckBox *_rxSoftCh;
  QCheckBox *_rxExpressionCh;

  // SC-55mkII+
  QCheckBox *_rxBankSelectCh;

  // SC-88+
//  QCheckBox *_rxMapSelectCh;

  int8_t &_partId;
  Emulator *_emulator;

public:
  explicit PartRxModeSettings(Emulator *emulator, int8_t &partId,
			      QWidget *parent = nullptr);
  void update_all_widgets(void);

private slots:
  void _partC_changed(int value);

  void _polyModeC_changed(int value);
  void _assignModeC_changed(int value);

  void _rxVolume_changed(int state);
  void _rxPan_changed(int state);
  void _rxNote_changed(int state);
  void _rxProgramChange_changed(int state);
  void _rxControlChange_changed(int state);
  void _rxPitchBend_changed(int state);
  void _rxChAftertouch_changed(int state);
  void _rxPolyAftertouch_changed(int state);
  void _rxRPN_changed(int state);
  void _rxNRPN_changed(int state);
  void _rxModulation_changed(int state);
  void _rxHold1_changed(int state);
  void _rxPortamento_changed(int state);
  void _rxSostenuto_changed(int state);
  void _rxSoft_changed(int state);
  void _rxExpression_changed(int state);

  // SC-55mkII+
  void _rxBankSelect_changed(int state);

  // SC-88+
//  void _rxMapSelect_changed(int state);
};


class PartToneSettings : public QWidget
{
  Q_OBJECT

private:
  QComboBox *_partC;

  QSlider *_vibratoRateS;
  QSlider *_vibratoDepthS;
  QSlider *_vibratoDelayS;
  QSlider *_TVFCutoffFreqS;
  QSlider *_TVFResonanceS;
  QSlider *_TVFAEnvAttackS;
  QSlider *_TVFAEnvDecayS;
  QSlider *_TVFAEnvReleaseS;

  QLabel *_vibratoRateL;
  QLabel *_vibratoDepthL;
  QLabel *_vibratoDelayL;
  QLabel *_TVFCutoffFreqL;
  QLabel *_TVFResonanceL;
  QLabel *_TVFAEnvAttackL;
  QLabel *_TVFAEnvDecayL;
  QLabel *_TVFAEnvReleaseL;

  Emulator *_emulator;
  int8_t &_partId;

public:
  explicit PartToneSettings(Emulator *emulator, int8_t &partId,
			    QWidget *parent = nullptr);

  void update_all_widgets(void);

private slots:
  void _partC_changed(int value);

  void _vibratoRate_changed(int value);
  void _vibratoDepth_changed(int value);
  void _vibratoDelay_changed(int value);
  void _TVFCutoffFreq_changed(int value);
  void _TVFResonance_changed(int value);
  void _TVFAEnvAttack_changed(int value);
  void _TVFAEnvDecay_changed(int value);
  void _TVFAEnvRelease_changed(int value);
};


class PartScaleSettings : public QWidget
{
  Q_OBJECT

private:
  QComboBox *_partC;
  
  QSlider *_noteS[12];
  QLabel *_valueL[12];

  Emulator *_emulator;
  int8_t &_partId;
  
public:
  explicit PartScaleSettings(Emulator *emulator, int8_t &partId,
			     QWidget *parent = nullptr);

  void update_all_widgets(bool blockSignals = true);

private slots:
  void _partC_changed(int value);
  void _noteC_changed(int value);
  void _noteCh_changed(int value);
  void _noteD_changed(int value);
  void _noteDh_changed(int value);
  void _noteE_changed(int value);
  void _noteF_changed(int value);
  void _noteFh_changed(int value);
  void _noteG_changed(int value);
  void _noteGh_changed(int value);
  void _noteA_changed(int value);
  void _noteAh_changed(int value);
  void _noteB_changed(int value);
};


class PartControllerSettings : public QWidget
{
  Q_OBJECT

private:
  QComboBox *_partC;
  QComboBox *_controllerC;

  QSlider *_cc1S;
  QSlider *_cc2S;

  QSlider *_pitchCtrlS;
  QSlider *_tvfCutoffS;
  QSlider *_amplitudeS;
  QSlider *_lfo1RateS;
  QSlider *_lfo1PitchDepthS;
  QSlider *_lfo1TVFDepthS;
  QSlider *_lfo1TVADepthS;
  QSlider *_lfo2RateS;
  QSlider *_lfo2PitchDepthS;
  QSlider *_lfo2TVFDepthS;
  QSlider *_lfo2TVADepthS;

  QLabel *_cc1L;
  QLabel *_cc2L;

  QLabel *_pitchCtrlL;
  QLabel *_tvfCutoffL;
  QLabel *_amplitudeL;
  QLabel *_lfo1RateL;
  QLabel *_lfo1PitchDepthL;
  QLabel *_lfo1TVFDepthL;
  QLabel *_lfo1TVADepthL;
  QLabel *_lfo2RateL;
  QLabel *_lfo2PitchDepthL;
  QLabel *_lfo2TVFDepthL;
  QLabel *_lfo2TVADepthL;

  Emulator *_emulator;
  int8_t &_partId;
  int _controllerId;
  
public:
  explicit PartControllerSettings(Emulator *emulator, int8_t &partId,
				  QWidget *parent = nullptr);
  void update_all_widgets(void);

private slots:
  void _partC_changed(int value);

  void _cc1_changed(int value);
  void _cc2_changed(int value);

  void _controller_changed(int value);
  
  void _pitchCtrl_changed(int value);
  void _tvfCutoff_changed(int value);
  void _amplitude_changed(int value);
  void _lfo1Rate_changed(int value);
  void _lfo1PitchDepth_changed(int value);
  void _lfo1TVFDepth_changed(int value);
  void _lfo1TVADepth_changed(int value);
  void _lfo2Rate_changed(int value);
  void _lfo2PitchDepth_changed(int value);
  void _lfo2TVFDepth_changed(int value);
  void _lfo2TVADepth_changed(int value);
};


class DrumSettings : public QWidget
{
  Q_OBJECT

private:
  QComboBox *_mapC;
  QLineEdit *_nameLE;

  QComboBox *_instrumentC;

  QSlider *_volumeS;
  QSlider *_pitchS;
  QSlider *_panS;
  QSlider *_reverbS;
  QSlider *_chorusS;
//  QSlider *_delayS;       // TODO: SC-88
  QSlider *_exlGroupS;

  QLabel *_volumeL;
  QLabel *_pitchL;
  QLabel *_panL;
  QLabel *_reverbL;
  QLabel *_chorusL;
//  QLabel *_delayL;       // TODO: SC-88
  QLabel *_exlGroupL;

  QCheckBox *_rxNoteOn;
  QCheckBox *_rxNoteOff;
  
  Emulator *_emulator;

  int _map;
  int _instrument;
  
public:
  explicit DrumSettings(Emulator *emulator, QWidget *parent = nullptr);

  void update_all_widgets(void);

private slots:
  void _map_changed(int value);
  void _name_changed(const QString &name);
  void _instrument_changed(int value);

  void _volume_changed(int value);
  void _pitch_changed(int value);
  void _pan_changed(int value);
  void _reverb_changed(int value);
  void _chorus_changed(int value);
  void _exlGroup_changed(int value);

  void _rxNoteOn_changed(int value);
  void _rxNoteOff_changed(int value);
};


#endif // SYNTH_DIALOG_H
