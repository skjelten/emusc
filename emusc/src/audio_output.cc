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


#include "audio_output.h"


AudioOutput::AudioOutput(EmuSC::Synth *synth)
  : _quit(false),
    _synth(synth),
    _volume(1.0f),
    _accLeft(0),
    _accRight(0),
    _accPeakLeft(0),
    _accPeakRight(0),
    _accNum(0)
{}


AudioOutput::~AudioOutput()
{}


void AudioOutput::_publish_levels(void)
{
  if (_meter && _accNum > 0)
    _meter->publish(_accLeft / _accNum, _accRight / _accNum,
                    _accPeakLeft, _accPeakRight,
                    _synth->get_num_clipped_samples() > 0,
                    _accNum);

  _accLeft = _accRight = 0;
  _accPeakLeft = _accPeakRight = 0;
  _accNum = 0;
}
