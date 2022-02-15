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


#ifdef __ALSA__


#include "midi_input_alsa.h"
#include "ex.h"

#include <iostream>


MidiInputAlsa::MidiInputAlsa()
{
  if (snd_seq_open(&_seqHandle, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
    throw(Ex(-1, "Error opening ALSA sequencer"));

  snd_seq_set_client_name(_seqHandle, "EmuSC");

  _seqPort = snd_seq_create_simple_port(_seqHandle, "EmuSC",
					SND_SEQ_PORT_CAP_SUBS_WRITE |
					SND_SEQ_PORT_CAP_WRITE,
					SND_SEQ_PORT_TYPE_MIDI_GS |
					SND_SEQ_PORT_TYPE_SYNTHESIZER);
  if (_seqPort < 0) {
    throw(Ex(-1, "Error creating ALSA sequencer port"));
    return ;
  }	

  std::cout << "EmuSC: MIDI sequencer [ALSA] client started at address "
	    << snd_seq_client_id(_seqHandle) << std::endl;
}


MidiInputAlsa::~MidiInputAlsa()
{
  snd_seq_delete_simple_port(_seqHandle, _seqPort);
  snd_seq_close(_seqHandle);
}


// TODO: Find a more elegant way to resume from blocking
void MidiInputAlsa::stop(void)
{
  MidiInput::stop();

  // Push a signal to the MIDI input thread
  snd_seq_t *handle;
  snd_seq_open(&handle, "default", SND_SEQ_OPEN_OUTPUT, 0);
  int port = snd_seq_create_simple_port(handle, "temp",
					SND_SEQ_PORT_CAP_WRITE,
					SND_SEQ_PORT_TYPE_SYNTH);
  snd_seq_connect_to(handle, port, snd_seq_client_id(_seqHandle), _seqPort);
  snd_seq_delete_simple_port(handle, port);
  snd_seq_close(handle);
}


void MidiInputAlsa::run(Synth *synth)
{
  snd_seq_event_t *seqEv = NULL;
  MidiEvent midiEvent;
  int numOfClients = 0;
  
  while(!_quit) {		
    if (snd_seq_event_input(_seqHandle, &seqEv) < 0 || _quit)
      continue;

    switch (seqEv->type)
      {
      case SND_SEQ_EVENT_PGMCHANGE:
	midiEvent.type = se_PrgChange;
	midiEvent.channel = seqEv->data.control.channel;
	midiEvent.data1 = seqEv->data.control.value;
	midiEvent.status = 0xff;
	midiEvent.data2 = 0xff;
	synth->midi_input(&midiEvent);
	break;
      case SND_SEQ_EVENT_CONTROLLER:
	midiEvent.type = se_CtrlChange;
	midiEvent.channel = seqEv->data.control.channel;
	midiEvent.status = seqEv->data.control.param;
	midiEvent.data1 = seqEv->data.control.value;
	midiEvent.data2 = 0xff;
	synth->midi_input(&midiEvent);
	break;
      case SND_SEQ_EVENT_PITCHBEND:
	midiEvent.type = se_PitchBend;
	midiEvent.channel = seqEv->data.control.channel;
	midiEvent.data = seqEv->data.control.value;
	midiEvent.status = 0xff;
	midiEvent.data1 = 0xff;
	midiEvent.data2 = 0xff;
	synth->midi_input(&midiEvent);
	break;
      case SND_SEQ_EVENT_CHANPRESS:
	midiEvent.type = se_ChPressure;
	midiEvent.channel = seqEv->data.control.channel;
	midiEvent.data1 = seqEv->data.control.value;
	midiEvent.status = 0xff;
	midiEvent.data2 = 0xff;
	synth->midi_input(&midiEvent);
	break;
      case SND_SEQ_EVENT_KEYPRESS:
	midiEvent.type = se_KeyPressure;
	midiEvent.channel = seqEv->data.control.channel;
	midiEvent.data1 = seqEv->data.note.note;
	midiEvent.data2 = seqEv->data.control.value;
	midiEvent.status = 0xff;
	synth->midi_input(&midiEvent);
	break;
      case SND_SEQ_EVENT_NOTEON:
	midiEvent.type = se_NoteOn;
	midiEvent.channel = seqEv->data.control.channel;
	midiEvent.data1 = seqEv->data.note.note;
	midiEvent.data2 = seqEv->data.note.velocity;
	midiEvent.status = 0xff;
	synth->midi_input(&midiEvent);
	break;        
      case SND_SEQ_EVENT_NOTEOFF:
	midiEvent.type = se_NoteOff;
	midiEvent.channel = seqEv->data.control.channel;
	midiEvent.data1 = seqEv->data.note.note;
	midiEvent.data2 = seqEv->data.note.off_velocity;
	midiEvent.status = 0xff;
	synth->midi_input(&midiEvent);
	break;
      case SND_SEQ_EVENT_SYSEX:
	midiEvent.type = se_Sysex;
	midiEvent.data1 = seqEv->data.ext.len;
	midiEvent.ptr = (uint8_t *) seqEv->data.ext.ptr;
	midiEvent.status = 0xff;
	midiEvent.channel = 0xff;
	synth->midi_input(&midiEvent);
	break;
      case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
	std::cout << "EmuSC: MIDI input event [Port unsubscribed]" << std::endl;
	numOfClients--;
	if (numOfClients == 0) {
	  midiEvent.type = se_Cancel;
	  synth->midi_input(&midiEvent);
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


#endif  // __USE_ALSA__
