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


#ifdef __WIN32_MIDI__


#include "midi_input_win32.h"

#include <algorithm>
#include <iostream>


MidiInputWin32::MidiInputWin32()
  : _sysExDataLength(0)
{
}


MidiInputWin32::~MidiInputWin32()
{
  stop();

  midiInUnprepareHeader(_handle, &_header, sizeof(MIDIHDR));

  MMRESULT res = midiInClose(_handle);
  if (res != MMSYSERR_NOERROR) {
    std::cerr << "EmuSC: Failed to close win32 MIDI input!" << std::endl;
    if (res == MIDIERR_STILLPLAYING)
      std::cerr << "EmuSC:   -> Buffers are still in the queue" << std::endl;
    else if (res == MMSYSERR_INVALHANDLE)
      std::cerr << "EmuSC:   -> Invalid device handle" << std::endl;
    else if (res == MMSYSERR_NOMEM)
      std::cerr << "EmuSC:   -> Unable to allocate or lock memory" << std::endl;
  }
}


void MidiInputWin32::_midi_callback(HMIDIIN handle, UINT msg,
				    DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
  switch(msg)
    {
    // Regular MIDI message (3 bytes)
    case MIM_DATA:
      {
	// dwParam2 = timestamp
	send_midi_event((dwParam1 >>  0) & 0x000000FF,
			(dwParam1 >>  8) & 0x000000FF,
			(dwParam1 >> 16) & 0x000000FF);
	break;
      }

    // SysEx MIDI message (-> 1024 bytes)
    case MIM_LONGDATA:
      {
	LPMIDIHDR mh = (LPMIDIHDR) dwParam1;

	// Ignore empty messages
	if (mh->dwBytesRecorded == 0)
	  break;

	// New and complete SysEx message
	if (_sysExDataLength == 0 && (mh->lpData[0] & 0xff) == 0xf0 &&
	    (mh->lpData[mh->dwBytesRecorded - 1] & 0xff) == 0xf7) {
	  send_midi_event_sysex((uint8_t *)mh->lpData, mh->dwBytesRecorded);

        // Starting or ongoing SysEx message
	} else {
	  // SysEx too large
	  if (_sysExDataLength + mh->dwBytesRecorded > _sysExData.size()) {
	    _sysExDataLength = 0;
	    break;
	  }

	  std::copy(mh->lpData, mh->lpData + mh->dwBytesRecorded,
		    &_sysExData[_sysExDataLength]);
	  _sysExDataLength += mh->dwBytesRecorded;

	  // SysEx message is complete
	  if (_sysExData[_sysExDataLength - 1] == 0xf7) {
	    send_midi_event_sysex(_sysExData.data(), _sysExDataLength);
	    _sysExDataLength = 0;
	  }
	}

	midiInAddBuffer(handle, mh, sizeof(MIDIHDR));
	break;
      }

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


void MidiInputWin32::start(EmuSC::Synth *synth, QString device)
{
  _synth = synth;

  UINT n = midiInGetNumDevs();
  int deviceID = -1;
  for (int i = 0; i < n; i++) {
    MIDIINCAPS caps;
    MMRESULT res = midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS));
    if (res == MMSYSERR_NOERROR && !device.compare(caps.szPname)) {
      deviceID = i;
      break;
    }
  }

  if (deviceID < 0)
    throw(QString("MIDI device '" + device + "' not found!"));

  // Open selected MIDI In device
  MMRESULT res = midiInOpen(&_handle, deviceID, (DWORD_PTR) midi_callback,
		   (DWORD_PTR) this, CALLBACK_FUNCTION);
  if (res != MMSYSERR_NOERROR)
    throw (QString("Failed to open win32 MIDI input device"));

  _header.lpData = &_data[0];
  _header.dwBufferLength = sizeof(&_data[0]);
  _header.dwFlags = 0;
  res = midiInPrepareHeader(_handle, &_header, sizeof(MIDIHDR));
  if (res != MMSYSERR_NOERROR)
    throw (QString("Failed to prepare sysex buffer for win32 MIDI input"));

  res = midiInAddBuffer(_handle, &_header, sizeof(MIDIHDR));
  if (res != MMSYSERR_NOERROR)
    throw (QString("Failed to add sysex buffer to handle for win32 MIDI input"));

  res = midiInStart(_handle);
  if (res != MMSYSERR_NOERROR)
    throw (QString("EmuSC: Failed to start win32 MIDI input!"));
}


void MidiInputWin32::stop(void)
{
  MMRESULT res = midiInStop(_handle);
  if (res != MMSYSERR_NOERROR)
    std::cerr << "EmuSC: Failed to stop win32 MIDI input!" << std::endl;

  res = midiInReset(_handle);
  if (res != MMSYSERR_NOERROR)
    std::cerr << "EmuSC: Failed to reset win32 MIDI input!" << std::endl;

  _sysExDataLength = 0;
}


void MidiInputWin32::midi_callback(HMIDIIN handle, UINT uMsg, DWORD_PTR dwInstance,
				   DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
  MidiInputWin32 *miw = (MidiInputWin32 *) dwInstance;
  miw->_midi_callback(handle, uMsg, dwParam1, dwParam2);
}


QStringList MidiInputWin32::get_available_devices(void)
{
  QStringList deviceList;
  UINT n = midiInGetNumDevs();

  for (int i = 0; i < n; ++i) {
    MIDIINCAPS caps;
    MMRESULT res = midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS));
    if (res == MMSYSERR_NOERROR)
      deviceList << QString::fromWCharArray(caps.szPname);
  }

  return deviceList;
}


#endif  // __WIN32_MIDI__
