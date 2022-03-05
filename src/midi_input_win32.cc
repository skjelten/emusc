/* 
 *  EmuSC - Sound Canvas emulator
 *  Copyright (C) 2022  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifdef __WIN32__


#include "ex.h"
#include "midi_input_win32.h"

#include <iostream>


MidiInputWin32::MidiInputWin32(Config *config, Synth *synth)
  : _synth(synth)
{
  MMRESULT res;

  int deviceID = 0;
  if (!config->get("input_device").empty())
    deviceID = stoi(config->get("input_device"));

  // Open default MIDI In device
  res = midiInOpen(&_handle, deviceID, (DWORD_PTR) midi_callback,
		   (DWORD_PTR) this, CALLBACK_FUNCTION);
  if (res != MMSYSERR_NOERROR)
    throw (Ex(-1, "Failed to open win32 MIDI input device"));

  _header.lpData = &_data[0];
  _header.dwBufferLength = sizeof(&_data[0]);
  _header.dwFlags = 0;
  res = midiInPrepareHeader(_handle, &_header, sizeof(MIDIHDR));
  if (res != MMSYSERR_NOERROR)
    throw (Ex(-1, "Failed to prepare sysex buffer for win32 MIDI input"));

  res = midiInAddBuffer(_handle, &_header, sizeof(MIDIHDR));
  if (res != MMSYSERR_NOERROR)
    throw (Ex(-1, "Failed to add sysex buffer to handle for win32 MIDI input"));

  std::cout << "EmuSC: Win32 MIDI input initialized" << std::endl;

  MIDIINCAPS caps;
  res = midiInGetDevCaps((UINT) _handle, &caps, sizeof(MIDIINCAPS));
  if (res == MMSYSERR_NOERROR)
    std::cout << " -> device=" << deviceID << " name=" << caps.szPname
	      << std::endl;
}


MidiInputWin32::~MidiInputWin32()
{
  midiInUnprepareHeader(_handle, &_header, sizeof(MIDIHDR));

  MMRESULT res = midiInClose(_handle);
  if (res != MMSYSERR_NOERROR)
    std::cerr << "EmuSC: Failed to close win32 MIDI input!" << std::endl;
}


void MidiInputWin32::_midi_callback(HMIDIIN handle, UINT msg,
				    DWORD dwParam1, DWORD dwParam2)
{
  switch(msg)
    {
    // Regular MIDI message (-> 3 byte)
    case MIM_DATA:
      {
	// dwParam2 = timestamp
	send_raw_std_msg(_synth,
			 (dwParam1 >>  0) & 0x000000FF,
			 (dwParam1 >>  8) & 0x000000FF,
			 (dwParam1 >> 16) & 0x000000FF);
	break;
      }

    // SysEx MIDI message : TODO
    case MIM_LONGDATA:
      break;


    // Ignore open & close
    case MIM_OPEN:
    case MIM_CLOSE:
      break;

    case MIM_ERROR:
    case MIM_LONGERROR:
      std::cerr << "EmuSC: Error while receiving MIDI data" << std::endl;
      break;

    case MIM_MOREDATA:
      break;
    }
}


void MidiInputWin32::stop(void)
{
  MMRESULT res = midiInStop(_handle);
  if (res != MMSYSERR_NOERROR)
    std::cerr << "EmuSC: Failed to stop win32 MIDI input!" << std::endl;

  res = midiInReset(_handle);
  if (res != MMSYSERR_NOERROR)
    std::cerr << "EmuSC: Failed to reset win32 MIDI input!" << std::endl;
}


void MidiInputWin32::run(Synth *synth)
{
  MMRESULT res = midiInStart(_handle);
  if (res != MMSYSERR_NOERROR)
    std::cerr << "EmuSC: Failed to start win32 MIDI input!" << std::endl;
}


void MidiInputWin32::midi_callback(HMIDIIN handle, UINT uMsg, DWORD dwInstance,
				   DWORD dwParam1, DWORD dwParam2)
{
  MidiInputWin32 *miw = (MidiInputWin32 *) dwInstance;
  miw->_midi_callback(handle, uMsg, dwParam1, dwParam2);
}


#endif  // __WIN32__
