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

  _seqPort = snd_seq_create_simple_port(_seqHandle, "EmuSC Midi Input",
					SND_SEQ_PORT_CAP_SUBS_WRITE |
					SND_SEQ_PORT_CAP_WRITE,
					SND_SEQ_PORT_TYPE_MIDI_GENERIC |
					SND_SEQ_PORT_TYPE_MIDI_GS |
					SND_SEQ_PORT_TYPE_SOFTWARE |
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


QStringList MidiInputAlsa::get_available_ports(void)
{
  QStringList portList;
  snd_seq_t *seqHandle;

  if (snd_seq_open(&seqHandle, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
    throw(QString("Error opening ALSA sequencer"));

  snd_seq_client_info_t *clientInfo;
  snd_seq_port_info_t *portInfo;

  snd_seq_client_info_alloca(&clientInfo);
  snd_seq_port_info_alloca(&portInfo);
  snd_seq_client_info_set_client(clientInfo, -1);
  while (snd_seq_query_next_client(seqHandle, clientInfo) >= 0) {
    snd_seq_port_info_set_client(portInfo,
				 snd_seq_client_info_get_client(clientInfo));
    snd_seq_port_info_set_port(portInfo, -1);

    while (snd_seq_query_next_port(seqHandle, portInfo) >= 0) {
      int cap = (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ);
      if ((snd_seq_port_info_get_capability(portInfo) & cap) == cap &&
	  !(snd_seq_port_info_get_capability(portInfo) &
	    SND_SEQ_PORT_CAP_NO_EXPORT) &&
	  snd_seq_port_info_get_type(portInfo) & SND_SEQ_PORT_TYPE_MIDI_GENERIC) {
	int client = snd_seq_client_info_get_client(clientInfo);
	int port = snd_seq_port_info_get_port(portInfo);
	QString name = snd_seq_client_info_get_name(clientInfo);
	QString fullName = QString("%1:%2  ").arg(client, 3).arg(port) + name;
	portList << fullName;
      }
    }
  }

  snd_seq_close(seqHandle);

  return portList;
}


QStringList MidiInputAlsa::list_subscribers(void)
{
  QStringList list;

  snd_seq_client_info_t *clientInfo;
  snd_seq_port_info_t *portInfo;

  snd_seq_client_info_alloca(&clientInfo);
  snd_seq_port_info_alloca(&portInfo);

  snd_seq_client_info_set_client(clientInfo, snd_seq_client_id(_seqHandle));
  snd_seq_port_info_set_client(portInfo,
			       snd_seq_client_info_get_client(clientInfo));
  snd_seq_port_info_set_port(portInfo, 0);

  snd_seq_query_subscribe_t *subs;

  snd_seq_query_subscribe_alloca(&subs);
  snd_seq_query_subscribe_set_type(subs, SND_SEQ_QUERY_SUBS_WRITE);
  snd_seq_query_subscribe_set_index(subs, 0);
  snd_seq_query_subscribe_set_client(subs, snd_seq_client_id(_seqHandle));
  snd_seq_query_subscribe_set_port(subs, 0);

  while (snd_seq_query_port_subscribers(_seqHandle, subs) >= 0) {
    const snd_seq_addr_t *a = snd_seq_query_subscribe_get_addr(subs);
    QString portNumber =
      QString::number(a->client) + QString(":") + QString::number(a->port);
    list << portNumber;

    snd_seq_query_subscribe_set_index(subs, snd_seq_query_subscribe_get_index(subs) + 1);
  }

  return list;
}


bool MidiInputAlsa::connect_port(QString portName, bool status)
{
  QString srcPortString = portName.left(portName.indexOf(":"));
  QString destPortString = QString::number(snd_seq_client_id(_seqHandle));

  int queue = 0, convertTime = 0, convertReal = 0, exclusive = 0;
  snd_seq_t *seq;
  snd_seq_port_subscribe_t *subs;
  snd_seq_addr_t sender, dest;

  if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0)
    throw (QString("Cannot open ALSA sequencer"));

  if (snd_seq_set_client_name(seq, "ALSA Connector") < 0) {
    snd_seq_close(seq);
    throw (QString("Cannot set client informantion"));
  }

  if (snd_seq_parse_address(seq, &sender, srcPortString.toStdString().c_str()) < 0) {
    snd_seq_close(seq);
    throw (QString("Invalid sender address: " + srcPortString));
  }

  if (snd_seq_parse_address(seq, &dest, destPortString.toStdString().c_str()) < 0) {
    snd_seq_close(seq);
    throw (QString("Invalid destination address: " + destPortString));
  }

  snd_seq_port_subscribe_alloca(&subs);
  snd_seq_port_subscribe_set_sender(subs, &sender);
  snd_seq_port_subscribe_set_dest(subs, &dest);
  snd_seq_port_subscribe_set_queue(subs, queue);
  snd_seq_port_subscribe_set_exclusive(subs, exclusive);
  snd_seq_port_subscribe_set_time_update(subs, convertTime);
  snd_seq_port_subscribe_set_time_real(subs, convertReal);

  if (status) {                                             // Connect
    if (snd_seq_get_port_subscription(seq, subs) == 0) {
      snd_seq_close(seq);
      throw (QString("Connection is already subscribed"));
    }
    if (snd_seq_subscribe_port(seq, subs) < 0) {
      snd_seq_close(seq);
      throw (QString("Connection failed: " + QString(snd_strerror(errno))));
    }
  } else {                                                  // Disconnect
    if (snd_seq_get_port_subscription(seq, subs) < 0) {
      snd_seq_close(seq);
      throw (QString("No subscription found"));
    }
    if (snd_seq_unsubscribe_port(seq, subs) < 0) {
      snd_seq_close(seq);
      throw (QString("Disconnection failed: " + QString(snd_strerror(errno))));
    }
  }

  snd_seq_close(seq);

  return 0;
}


#endif  // __ALSA_MIDI__
