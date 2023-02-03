/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2023-2023  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libEmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libEmuSC. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __PARAMS_H__
#define __PARAMS_H__


namespace EmuSC {

  
// All variables for master settings as defined by the Sound Canvas lineup
enum class SystemParam : int {

  // Part 1: Settings defined by SysEx chart
  // These settings can be both requested and modified by SysEx messages
  Tune                = 0x0000,    // 4B: [0x0014 - 0x07e8 : 0x0400]-100.0-100.0
  Tune1               = 0x0001,
  Tune2               = 0x0002,
  Tune3               = 0x0003,
  Volume              = 0x0004,    // [0x00 - 0x7f : 0x7f]
  KeyShift            = 0x0005,    // [0x28 - 0x58 : 0x40]
  Pan                 = 0x0006,    // [0x01 - 0x7f : 0x40]

  ResetGSstandardMode = 0x007f,    // No data stored, but triggers a reset event

  // Part 2: Settings outside SysEx chart
  SampleRate          = 0x0080,    // 4B: [32000 - 96000 : 44100]
  Channels            = 0x0084,    // [1 - 2 : 2]

  RxSysEx             = 0x0090,    // [0 - 1 : 1]
  RxGMOn              = 0x0091,    // [0 - 1 : 1]
  RxGSReset           = 0x0092,    // [0 - 1 : 1]
  RxInstrumentChange  = 0x0093,    // [0 - 1 : 1]
  RxFunctionControl   = 0x0094,    // [0 - 1 : 1]
  DeviceID            = 0x0095,    // [1 - 32 : 17]

};


enum class PatchParam : int {

  // Part 1: Settings defined by SysEx chart
  // These settings can be both requested and modified by SysEx messages

  PatchName           = 0x0100,
  PartialReserve      = 0x0110,

  ReverbMacro         = 0x0130,    // [0x00 - 0x07 : 0x04]
  ReverbCharacter     = 0x0131,    // [0x00 - 0x07 : 0x04]
  ReverbPreLPF        = 0x0132,    // [0x00 - 0x7f : 0x00]
  ReverbLevel         = 0x0133,    // [0x00 - 0x7f : 0x40]
  ReverbTime          = 0x0134,    // [0x00 - 0x7f : 0x40]
  ReverbDelayFeedback = 0x0135,    // [0x00 - 0x7f : 0x00]
  ReverbSendToChorus  = 0x0136,    // [0x00 - 0x7f : 0x00]
  ReverbPreDelayTime  = 0x0137,    // SC-88
  
  ChorusMacro         = 0x0138,    // [0x00 - 0x07 : 0x02]
  ChorusPreLPF        = 0x0139,    // [0x00 - 0x07 : 0x00]
  ChorusLevel         = 0x013a,    // [0x00 - 0x7f : 0x40]
  ChorusFeedback      = 0x013b,    // [0x00 - 0x7f : 0x08]
  ChorusDelay         = 0x013c,    // [0x00 - 0x7f : 0x50]
  ChorusRate          = 0x013d,    // [0x00 - 0x7f : 0x03]
  ChorusDepth         = 0x013e,    // [0x00 - 0x7f : 0x13]
  ChorusSendToReverb  = 0x013f,    // [0x00 - 0x7f : 0x00]

  // The remaining patch paramters have separate values for each part
  // All addresses are either 0x1PXX or 0x2PXX, where P = part number
  
  ToneNumber          = 0x1000,    // 2B: Byte 1 = CtrlChg, Byte 2 = ProgChg
  ToneNumber2         = 0x1001,    // Only helper for easy access 2nd. byte
  RxChannel           = 0x1002,    // [1 - 17] X = Part number, 17 = Off
  RxPitchBend         = 0x1003,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxChPressure        = 0x1004,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxProgramChange     = 0x1005,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxControlChange     = 0x1006,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxPolyPressure      = 0x1007,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxNoteMessage       = 0x1008,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxRPN               = 0x1009,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxNRPN              = 0x100a,    // [0 - 1 : 1] 0 = Off, 1 = On
                                   // Note: Set to Off if mode is set to 'GM'
  RxModulation        = 0x100b,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxVolume            = 0x100c,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxPanpot            = 0x100d,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxExpression        = 0x100e,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxHold1             = 0x100f,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxPortamento        = 0x1010,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxSostenuto         = 0x1011,    // [0 - 1 : 1] 0 = Off, 1 = On
  RxSoft              = 0x1012,    // [0 - 1 : 1] 0 = Off, 1 = On
  PolyMode            = 0x1013,    // [0 (Mono) | 1 (Poly) : 1]
  AssignMode          = 0x1014,    // [0 - 2] 0 = Single, 1 = Limited-Multi,
                                   //         2 = Full-Multi
  UseForRhythm        = 0x1015,    // [0 - 3] 0 = Off, 1 = Map1, 2 = Map2
  PitchKeyShift       = 0x1016,    // [0x28 - 0x58 : 0x40] -24 - 24 semitone
  PitchOffsetFine     = 0x1017,    // 2B [0x08 - 0xf8 : 0x0800] -12.0 - 12.0 Hz
  PitchOffsetFine2    = 0x1018,
  PartLevel           = 0x1019,    // [0x00 - 0x7f : 0x64]
  VelocitySenseDepth  = 0x101a,    // [0x00 - 0x7f : 0x40]
  VelocitySenseOffset = 0x101b,    // [0x00 - 0x7f : 0x40]
  PartPanpot          = 0x101c,    // [0x00 - 0x7f : 0x40] 0 = rnd, left > right
  KeyRangeLow         = 0x101d,    // [0x00 - 0x7f : 0x00]
  KeyRangeHigh        = 0x101e,    // [0x00 - 0x7f : 0x7f]
  CC1ControllerNumber = 0x101f,    // [0x00 - 0x5f : 0x40]
  CC2ControllerNumber = 0x1020,    // [0x00 - 0x5f : 0x40]
  ChorusSendLevel     = 0x1021,    // [0x00 - 0x7f : 0x00] (CC# 93)
  ReverbSendLevel     = 0x1022,    // [0x00 - 0x7f : 0x28] (CC# 91)
  RxBankSelect        = 0x1023,    // [0 - 1 : 1]  Note: Missing from SC-55 OM,
                                   // but present on SC-55mkII and onwards
  RxBankSelectLSB     = 0x1024,    // (SC88+)
  PitchFineTune       = 0x102a,    // [SC88+][0x0000 - 0x7f7f : 0x400] (14 bit)
  PitchFineTune2      = 0x102b,    // Also RPN #1 so used for that on SC-55
  DelaySendLevel      = 0x102c,    // (SC88+)

  // Tone modify
  // Note: SC-55 has [0x0e - 0x72 : 0x40] for all tone modify parameters
  //       SC-88 has [0x00 - 0x7f : 0x40] for all tone modify parameters
  VibratoRate         = 0x1030,    // NRPN#   8
  VibratoDepth        = 0x1031,    // NRPN#   9
  TVFCutoffFreq       = 0x1032,    // NRPN#  32
  TVFResonance        = 0x1033,    // NRPN#  33
  TVFAEnvAttack       = 0x1034,    // NRPN#  99
  TVFAEnvDecay        = 0x1035,    // NRPN# 100
  TVFAEnvRelease      = 0x1036,    // NRPN# 102
  VibratoDelay        = 0x1037,    // NRPN#  10

  // Scale tuning
  ScaleTuningC        = 0x1040,    // [0x00 - 0x7f : 0x40], -63 - 63 cent
  ScaleTuningC_       = 0x1041,    // [0x00 - 0x7f : 0x40], -63 - 63 cent
  ScaleTuningD        = 0x1042,    // [0x00 - 0x7f : 0x40], -63 - 63 cent
  ScaleTuningD_       = 0x1043,    // [0x00 - 0x7f : 0x40], -63 - 63 cent
  ScaleTuningE        = 0x1044,    // [0x00 - 0x7f : 0x40], -63 - 63 cent
  ScaleTuningF        = 0x1045,    // [0x00 - 0x7f : 0x40], -63 - 63 cent
  ScaleTuningF_       = 0x1046,    // [0x00 - 0x7f : 0x40], -63 - 63 cent
  ScaleTuningG        = 0x1047,    // [0x00 - 0x7f : 0x40], -63 - 63 cent
  ScaleTuningG_       = 0x1048,    // [0x00 - 0x7f : 0x40], -63 - 63 cent
  ScaleTuningA        = 0x1049,    // [0x00 - 0x7f : 0x40], -63 - 63 cent
  ScaleTuningA_       = 0x104a,    // [0x00 - 0x7f : 0x40], -63 - 63 cent
  ScaleTuningB        = 0x104b,    // [0x00 - 0x7f : 0x40], -63 - 63 cent

  // Controller setttings (0x41 0x2P 0xXX)
  // Modulator
  MOD_PitchControl    = 0x2000,    // [0x20 - 0x58 : 0x40], -24 - +24 semitones
  MOD_TVFCutoffControl= 0x2001,    // [0x00 - 0x7f : 0x40], -9600 - 9600 cent
  MOD_AmplitudeControl= 0x2002,    // [0x00 - 0x7f : 0x40], -100.0 - 100.0 %
  MOD_LFO1RateControl = 0x2003,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  MOD_LFO1PitchDepth  = 0x2004,    // [0x00 - 0x7f : 0x0a], 0 - 600 cent
  MOD_LFO1TVFDepth    = 0x2005,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  MOD_LFO1TVADepth    = 0x2006,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %
  MOD_LFO2RateControl = 0x2007,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  MOD_LFO2PitchDepth  = 0x2008,    // [0x00 - 0x7f : 0x00], 0 - 600 cent
  MOD_LFO2TVFDepth    = 0x2009,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  MOD_LFO2TVADepth    = 0x200a,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %

  // Pitch Bend
  PB_PitchControl     = 0x2010,    // [0x40 - 0x58 : 0x40], 0 - +24 semitones
  PB_TVFCutoffControl = 0x2011,    // [0x00 - 0x7f : 0x40], -9600 - 9600 cent
  PB_AmplitudeControl = 0x2012,    // [0x00 - 0x7f : 0x40], -100.0 - 100.0 %
  PB_LFO1RateControl  = 0x2013,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  PB_LFO1PitchDepth   = 0x2014,    // [0x00 - 0x7f : 0x0a], 0 - 600 cent
  PB_LFO1TVFDepth     = 0x2015,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  PB_LFO1TVADepth     = 0x2016,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %
  PB_LFO2RateControl  = 0x2017,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  PB_LFO2PitchDepth   = 0x2018,    // [0x00 - 0x7f : 0x00], 0 - 600 cent
  PB_LFO2TVFDepth     = 0x2019,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  PB_LFO2TVADepth     = 0x201a,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %

  // Channel Aftertouch
  CAf_PitchControl    = 0x2020,    // [0x20 - 0x58 : 0x40], -24 - +24 semitones
  CAf_TVFCutoffControl= 0x2021,    // [0x00 - 0x7f : 0x40], -9600 - 9600 cent
  CAf_AmplitudeControl= 0x2022,    // [0x00 - 0x7f : 0x40], -100.0 - 100.0 %
  CAf_LFO1RateControl = 0x2023,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  CAf_LFO1PitchDepth  = 0x2024,    // [0x00 - 0x7f : 0x0a], 0 - 600 cent
  CAf_LFO1TVFDepth    = 0x2025,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  CAf_LFO1TVADepth    = 0x2026,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %
  CAf_LFO2RateControl = 0x2027,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  CAf_LFO2PitchDepth  = 0x2028,    // [0x00 - 0x7f : 0x00], 0 - 600 cent
  CAf_LFO2TVFDepth    = 0x2029,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  CAf_LFO2TVADepth    = 0x202a,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %

  // Poly Aftertouch
  PAf_PitchControl    = 0x2030,    // [0x20 - 0x58 : 0x40], -24 - +24 semitones
  PAf_TVFCutoffControl= 0x2031,    // [0x00 - 0x7f : 0x40], -9600 - 9600 cent
  PAf_AmplitudeControl= 0x2032,    // [0x00 - 0x7f : 0x40], -100.0 - 100.0 %
  PAf_LFO1RateControl = 0x2033,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  PAf_LFO1PitchDepth  = 0x2034,    // [0x00 - 0x7f : 0x0a], 0 - 600 cent
  PAf_LFO1TVFDepth    = 0x2035,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  PAf_LFO1TVADepth    = 0x2036,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %
  PAf_LFO2RateControl = 0x2037,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  PAf_LFO2PitchDepth  = 0x2038,    // [0x00 - 0x7f : 0x00], 0 - 600 cent
  PAf_LFO2TVFDepth    = 0x2039,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  PAf_LFO2TVADepth    = 0x203a,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %

  // CC#1
  CC1_PitchControl    = 0x2040,    // [0x20 - 0x58 : 0x40], -24 - +24 semitones
  CC1_TVFCutoffControl= 0x2041,    // [0x00 - 0x7f : 0x40], -9600 - 9600 cent
  CC1_AmplitudeControl= 0x2042,    // [0x00 - 0x7f : 0x40], -100.0 - 100.0 %
  CC1_LFO1RateControl = 0x2043,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  CC1_LFO1PitchDepth  = 0x2044,    // [0x00 - 0x7f : 0x0a], 0 - 600 cent
  CC1_LFO1TVFDepth    = 0x2045,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  CC1_LFO1TVADepth    = 0x2046,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %
  CC1_LFO2RateControl = 0x2047,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  CC1_LFO2PitchDepth  = 0x2048,    // [0x00 - 0x7f : 0x00], 0 - 600 cent
  CC1_LFO2TVFDepth    = 0x2049,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  CC1_LFO2TVADepth    = 0x204a,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %

  // CC#2
  CC2_PitchControl    = 0x2050,    // [0x20 - 0x58 : 0x40], -24 - 24 semitones
  CC2_TVFCutoffControl= 0x2051,    // [0x00 - 0x7f : 0x40], -9600 - 9600 cent
  CC2_AmplitudeControl= 0x2052,    // [0x00 - 0x7f : 0x40], -100.0 - 100.0 %
  CC2_LFO1RateControl = 0x2053,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  CC2_LFO1PitchDepth  = 0x2054,    // [0x00 - 0x7f : 0x0a], 0 - 600 cent
  CC2_LFO1TVFDepth    = 0x2055,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  CC2_LFO1TVADepth    = 0x2056,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %
  CC2_LFO2RateControl = 0x2057,    // [0x00 - 0x7f : 0x40], -10.0 - 10.0 Hz
  CC2_LFO2PitchDepth  = 0x2058,    // [0x00 - 0x7f : 0x00], 0 - 600 cent
  CC2_LFO2TVFDepth    = 0x2059,    // [0x00 - 0x7f : 0x00], 0 - 2400 cent
  CC2_LFO2TVADepth    = 0x205a,    // [0x00 - 0x7f : 0x00], 0 - 100.0 %

  // SC-88 only additions TODO: FIX
//  ToneMapNumber       = 0x4000,     // [SC-88Pro]
//  ToneMap0Number      = 0x4001,     // [SC-88Pro]
//  EqOnOff             = 0x4020,     // [SC-88]
//  OutputAssign        = 0x4021,     // [SC-88Pro]
//  PartEfxAssign       = 0x4022,     // [SC-88Pro]

  // Part 2: Settings outside SysEx chart

  // Status controller inputs
  PitchBend           = 0x1080,    // 2B [0x00 - 0x4000 : 0x2000]
  Modulation          = 0x1082,    // [0x00 - 0x7f]
  CC1Controller       = 0x1083,    // [0x00 - 0x7f : 0]
  CC2Controller       = 0x1084,    // [0x00 - 0x7f : 0]
  ChannelPressure     = 0x1085,    // [0x00 - 0x7f : 0]
  PolyKeyPressure     = 0x1086,    // [0x00 - 0x7f : 0] pr. key. Need an array?
  Hold1               = 0x1087,    // [0x00 - 0x7f : 0] 0-63: OFF, 64-127: ON
  Sostenuto           = 0x1088,    // [0x00 - 0x7f : 0] 0-63: OFF, 64-127: ON
  Soft                = 0x1089,    // [0x00 - 0x7f : 0] 0-63: OFF, 64-127: ON
  Expression          = 0x108a,    // [0x00 - 0x7f : 0xff]
  Portamento          = 0x108b,    // [0 - 1 : 0]
  PortamentoTime      = 0x108c,    // [0x00 - 0x7f : 0]

  // Current RPN and NRPN
  RPN_LSB             = 0x1090,    // [0x00 - 0x7f : 0]
  RPN_MSB             = 0x1091,    // [0x00 - 0x7f : 0]
  NRPN_LSB            = 0x1092,    // [0x00 - 0x7f : 0]
  NRPN_MSB            = 0x1093,    // [0x00 - 0x7f : 0]

  // Other patch parameters from CC / RPN / NRPN / menu
  PitchCoarseTune     = 0x1094     // [0x28 - 0x58 : 0x40] -24 - 24 semit. RPN#2
};

// All variables for individual parts as defined by the Sound Canvas lineup
enum class DrumParam : int {
  DrumsMapName        = 0x0000,    // 12B ASCII
  PlayKeyNumber       = 0x0100,    // [0x00 - 0x7f] semitones
  Level               = 0x0200,    // [0x00 - 0x7f]
  AssignGroupNumber   = 0x0300,    // [0x00 - 0x7f]
  Panpot              = 0x0400,    // [0x00 - 0x7f]
  ReverbDepth         = 0x0500,    // [0x00 - 0x7f]
  ChorusDepth         = 0x0600,    // [0x00 - 0x7f]
  RxNoteOff           = 0x0700,    // [0 - 1]
  RxNoteOn            = 0x0800     // [0 - 1]
};

}

#endif  // __PARAMS_H__
