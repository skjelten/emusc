/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022  Håkon Skjelten
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


#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H


class AudioOutput
{
public:
  AudioOutput();
  virtual ~AudioOutput() = 0;

  virtual void start(void) = 0;
  virtual void stop(void) = 0;

  float volume(void) { return _volume; }
  void set_volume(float value) { _volume = value; }

protected:
  bool _quit;
  float _volume;              // [0 - 1] Default 1

private:

};


#endif  // AUDIO_OUTPUT_H
