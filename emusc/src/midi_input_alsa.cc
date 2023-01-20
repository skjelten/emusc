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


#ifdef __ALSA_MIDI__


#include "midi_input_alsa.h"

#include "emusc/synth.h"

#include <QString>

#include <iostream>


//void MidiInputAlsa::MidiInputAlse(void(EmuSC::Synth::*send_midi)(EmuSC::Synth::MidiEvent)) //EmuSC::Synth *synth)
MidiInputAlsa::MidiInputAlsa()
  : _stop(false),
    _eventInputThread(NULL)
{
}


MidiInputAlsa::~MidiInputAlsa()
{
  stop();
  
  snd_seq_delete_simple_port(_seqHandle, _seqPort);
  snd_seq_close(_seqHandle);
}


void MidiInputAlsa::start(EmuSC::Synth *synth, QString device)
{
  _synth = synth;
  
  if (snd_seq_open(&_seqHandle, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
    throw(QString("Error opening ALSA sequencer"));

  snd_seq_set_client_name(_seqHandle, "EmuSC");

  _seqPort = snd_seq_create_simple_port(_seqHandle, "EmuSC",
					SND_SEQ_PORT_CAP_SUBS_WRITE |
					SND_SEQ_PORT_CAP_WRITE,
					SND_SEQ_PORT_TYPE_MIDI_GS |
					SND_SEQ_PORT_TYPE_SYNTHESIZER);
  if (_seqPort < 0)
    throw(QString("Error creating ALSA sequencer port"));

  std::cout << "EmuSC: MIDI sequencer [ALSA] client started at address "
        << snd_seq_client_id(_seqHandle) << std::endl;

  // Start separate thread for polling ALSA for new MIDI events
  _eventInputThread = new std::thread(&MidiInputAlsa::run, this);
}


// TODO: Find a more elegant way to resume from blocking
void MidiInputAlsa::stop(void)
{
  _stop = true;

  // Push a signal to the MIDI port so that the thread can exit
  snd_seq_t *handle;
  snd_seq_open(&handle, "default", SND_SEQ_OPEN_OUTPUT, 0);
  int port = snd_seq_create_simple_port(handle, "temp",
					SND_SEQ_PORT_CAP_WRITE,
					SND_SEQ_PORT_TYPE_SYNTH);
  snd_seq_connect_to(handle, port, snd_seq_client_id(_seqHandle), _seqPort);
  snd_seq_delete_simple_port(handle, port);
  snd_seq_close(handle);

  // Wait for poll to return
  if (_eventInputThread) {
    _eventInputThread->join();
    delete (_eventInputThread), _eventInputThread = NULL;
  }
}


void MidiInputAlsa::run()
{
  snd_seq_event_t *seqEv = NULL;
  int numOfClients = 0;
  
  while (!_stop) {		
    if (snd_seq_event_input(_seqHandle, &seqEv) < 0 || _stop)
      continue;

    switch (seqEv->type)
      {
      case SND_SEQ_EVENT_NOTEON:
	send_midi_event(0x90 | seqEv->data.control.channel,
			seqEv->data.note.note, seqEv->data.note.velocity);
	break;

      case SND_SEQ_EVENT_NOTEOFF:
	send_midi_event(0x80 | seqEv->data.control.channel,
			seqEv->data.note.note, seqEv->data.note.velocity);
	break;

      case SND_SEQ_EVENT_KEYPRESS:
	send_midi_event(0xa0 | seqEv->data.control.channel,
			seqEv->data.note.note, seqEv->data.note.velocity);
	break;

      case SND_SEQ_EVENT_CONTROLLER:
	send_midi_event(0xb0 | seqEv->data.control.channel,
			seqEv->data.control.param,
			seqEv->data.control.value);	
	break;

      case SND_SEQ_EVENT_PGMCHANGE:
	send_midi_event(0xc0 | seqEv->data.control.channel,
			seqEv->data.control.value, 0x00);
	break;

      case SND_SEQ_EVENT_CHANPRESS:
	send_midi_event(0xd0 | seqEv->data.control.channel,
			seqEv->data.control.value, 0x00);
	break;

      case SND_SEQ_EVENT_PITCHBEND:
	{
	  int properPitchBendValue = seqEv->data.control.value + 8192;	
	  if (properPitchBendValue > 0x3fff)
	    properPitchBendValue = 0x3fff;

	  send_midi_event(0xe0 | seqEv->data.control.channel,
			  (properPitchBendValue) & 0x7f,
			  (properPitchBendValue >> 7) & 0x7f);
	}
	break;

      case SND_SEQ_EVENT_SYSEX:
	send_midi_event_sysex((uint8_t *) seqEv->data.ext.ptr,
			      seqEv->data.ext.len);
	break;

      case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
	std::cout << "EmuSC: MIDI input event [Port unsubscribed]" << std::endl;
	numOfClients--;
	if (numOfClients == 0) {
	  send_midi_event(0xb0, 123, 0);
	}
	break;
      case SND_SEQ_EVENT_PORT_SUBSCRIBED:
	std::cout << "EmuSC: MIDI input event [Port subscribed]" << std::endl;
	numOfClients++;
	break;
      default:
	std::cerr << "EmuSC: ALSA sequencer received an unknown MIDI event"
		  << std::endl;
	break;
      }
  }
}


QStringList MidiInputAlsa::get_available_devices(void)
{
  QStringList deviceList;

  // We only support the sequencer interface on ALSA
  deviceList << "Sequencer";
  
  return deviceList;
}


#endif  // __ALSA_MIDI__
