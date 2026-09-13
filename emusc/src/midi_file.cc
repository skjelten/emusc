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


#include "midi_file.h"

#include <algorithm>
#include <cstdio>
#include <cstring>


namespace {


inline uint32_t be32(const uint8_t *p)
{ return ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

inline uint16_t be16(const uint8_t *p)
{ return (uint16_t)((p[0] << 8) | p[1]); }


// Read a variable length quantity. Returns false on end of data.
bool read_VLQ(const uint8_t *d, size_t end, size_t &pos, uint32_t &value)
{
  value = 0;
  for (int i = 0; i < 4; i++) {
    if (pos >= end)
      return false;
    uint8_t b = d[pos++];
    value = (value << 7) | (b & 0x7f);
    if (!(b & 0x80))
      return true;
  }

  return false;                       // > 4 bytes: malformed
}


void write_VLQ(std::vector<uint8_t> &out, uint32_t v)
{
  uint8_t buf[4];
  int n = 0;
  do {
    buf[n++] = v & 0x7f;
    v >>= 7;
  } while (v && n < 4);
  while (n--)
    out.push_back((uint8_t)(buf[n] | (n ? 0x80 : 0x00)));
}


struct TruncatedInput {};             // Thrown when input ends mid-event


inline uint8_t need(const uint8_t *d, size_t end, size_t &pos)
{
  if (pos >= end)
    throw TruncatedInput();
  return d[pos++];
}

} // namespace


bool MidiFile::load(const uint8_t *data, size_t size,
                    Variant variant, std::string *error)
{
  _events.clear();
  _truncated = false;

  if (variant != Variant::MkIIRomDemo) {
    std::string err;
    if (_parse(data, size, false, &err)) {
      _detected = Variant::Standard;
      return true;
    }
    if (variant == Variant::Standard) {
      if (error) *error = err;
      return false;
    }
  }

  if (_parse(data, size, true, error)) {
    _detected = Variant::MkIIRomDemo;
    return true;
  }

  return false;
}


bool MidiFile::load_file(const std::string &path,
			 Variant variant, std::string *error)
{
  FILE *f = std::fopen(path.c_str(), "rb");
  if (!f) {
    if (error) *error = "Cannot open " + path;
    return false;
  }
  std::fseek(f, 0, SEEK_END);
  long len = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  std::vector<uint8_t> buf(len > 0 ? (size_t)len : 0);
  size_t got = buf.empty() ? 0 : std::fread(buf.data(), 1, buf.size(), f);
  std::fclose(f);
  
  return load(buf.data(), got, variant, error);
}


bool MidiFile::_parse(const uint8_t *d, size_t size, bool shortNoteOff,
                      std::string *error)
{
  _events.clear();
  _truncated = false;

  if (size < 14 || std::memcmp(d, "MThd", 4) || be32(d + 4) != 6) {
    if (error) *error = "Not a standard MIDI file (bad MThd)";
    return false;
  }
  uint16_t format = be16(d + 8);
  uint16_t ntrks  = be16(d + 10);
  uint16_t div    = be16(d + 12);

  if (format > 1) {
    if (error) *error = "Unsupported SMF format " + std::to_string(format);
    return false;
  }
  if (div & 0x8000) {
    if (error) *error = "SMPTE time division not supported";
    return false;
  }
  if (div == 0 || ntrks == 0) {
    if (error) *error = "Invalid division or track count";
    return false;
  }
  _division = div;

  size_t pos = 14;
  std::vector<std::vector<MidiEvent> > tracks;

  for (uint16_t t = 0; t < ntrks && pos < size; t++) {
    if (pos + 8 > size || std::memcmp(d + pos, "MTrk", 4)) {
      if (error) *error = "Bad track chunk header";
      return false;
    }
    uint32_t trkLen = be32(d + pos + 4);
    pos += 8;
    // Tolerate a declared length running past the buffer (truncated dump)
    size_t trkEnd = pos + trkLen;
    if (trkEnd > size) {
      trkEnd = size;
      _truncated = true;
    }
    tracks.push_back(std::vector<MidiEvent>());
    if (!_parse_track(d, size, pos, trkEnd, shortNoteOff, tracks.back(), error))
      return false;
    pos = trkEnd;
  }

  // Merge tracks
  size_t total = 0;
  for (size_t i = 0; i < tracks.size(); i++)
    total += tracks[i].size();
  _events.reserve(total);
  for (size_t i = 0; i < tracks.size(); i++)
    _events.insert(_events.end(), tracks[i].begin(), tracks[i].end());
  std::stable_sort(_events.begin(), _events.end(),
                   [](const MidiEvent &a, const MidiEvent &b)
                   { return a.tick < b.tick; });

  return true;
}


bool MidiFile::_parse_track(const uint8_t *d, size_t /*size*/, size_t &pos,
                            size_t end, bool shortNoteOff,
                            std::vector<MidiEvent> &out, std::string *error)
{
  uint32_t tick = 0;
  uint8_t running = 0;

  try {
    while (pos < end) {
      uint32_t delta;
      if (!read_VLQ(d, end, pos, delta))
        throw TruncatedInput();
      tick += delta;

      uint8_t b = need(d, end, pos);
      uint8_t status;
      if (b & 0x80) {
        status = b;
        if (status < 0xf0)
          running = status;
      } else {
        if (!running) {
          if (error) *error = "Data byte with no running status";
          return false;
        }
        status = running;
        pos--;                        // Byte belongs to the event data
      }

      MidiEvent ev;
      ev.tick = tick;
      ev.status = status;
      ev.meta = 0;
      ev.data[0] = ev.data[1] = 0;

      if (status == 0xff) {                            // Meta event
        ev.meta = need(d, end, pos);
        uint32_t len;
        if (!read_VLQ(d, end, pos, len) || pos + len > end)
          throw TruncatedInput();
        ev.blob.assign(d + pos, d + pos + len);
        pos += len;
        out.push_back(ev);
        if (ev.meta == 0x2f)                           // End of track
          return true;
      } else if (status == 0xf0 || status == 0xf7) {   // SysEx
        uint32_t len;
        if (!read_VLQ(d, end, pos, len) || pos + len > end)
          throw TruncatedInput();
        if (status == 0xf0)
          ev.blob.push_back(0xf0);    // SMF omits the leading F0; restore it
        ev.blob.insert(ev.blob.end(), d + pos, d + pos + len);
        pos += len;
        out.push_back(ev);
      } else if (status >= 0xf1) {
        if (error) *error = "Unexpected system message in track";
        return false;
      } else {                                         // Channel message
        uint8_t type = status & 0xf0;
        int n = 2;
        if (type == 0xc0 || type == 0xd0)
          n = 1;
        else if (type == 0x80 && shortNoteOff)
          n = 1;                      // mkII ROM variant: no release velocity
        for (int i = 0; i < n; i++) {
          uint8_t v = need(d, end, pos);
          if (v & 0x80) {
            if (error) *error = "Invalid data byte (bit 7 set)";
            return false;             // The Auto-detection discriminator
          }
          ev.data[i] = v;
        }
        if (type == 0x80 && shortNoteOff)
          ev.data[1] = 0x40;          // Normalize: default release velocity
        out.push_back(ev);
      }
    }
  } catch (TruncatedInput &) {
    _truncated = true;                // Keep everything parsed so far
  }

  return true;
}


uint32_t MidiFile::duration_ticks() const
{
  return _events.empty() ? 0 : _events.back().tick;
}


double MidiFile::duration_seconds() const
{
  double seconds = 0.0;
  double tempoUs = 500000.0;         // SMF default until first FF 51
  uint32_t prevTick = 0;

  for (size_t i = 0; i < _events.size(); i++) {
    const MidiEvent &ev = _events[i];
    seconds += (double) (ev.tick - prevTick) * tempoUs
               / (1.0e6 * _division);
    prevTick = ev.tick;
    if (ev.status == 0xff && ev.meta == 0x51 && ev.blob.size() == 3) {
      uint32_t t = ((uint32_t) ev.blob[0] << 16) |
                   ((uint32_t) ev.blob[1] << 8) | ev.blob[2];
      if (t > 0)
        tempoUs = (double) t;
    }
  }

  return seconds;
}


std::string MidiFile::meta_text(uint8_t metaType) const
{
  for (size_t i = 0; i < _events.size(); i++)
    if (_events[i].status == 0xff && _events[i].meta == metaType)
      return std::string(_events[i].blob.begin(), _events[i].blob.end());

  return std::string();
}


std::string MidiFile::song_name() const
{
  std::string name = meta_text(0x03);
  if (name.empty())
    name = meta_text(0x01);
  // Trim any trailing NUL/space padding
  while (!name.empty() && (name.back() == '\0' || name.back() == ' '))
    name.pop_back();

  return name;
}


std::vector<uint8_t> MidiFile::to_standard_midi(const std::string &name) const
{
  std::vector<uint8_t> track;
  uint32_t lastTick = 0;
  uint8_t running = 0;
  bool haveEOT = false;

  // Optional title, only if the song does not already carry one
  if (!name.empty() && meta_text(0x03).empty()) {
    write_VLQ(track, 0);                         // At tick 0
    track.push_back(0xff);
    track.push_back(0x03);                       // Sequence/Track Name
    std::string n = name.substr(0, 127);         // Keep the length VLQ short
    write_VLQ(track, (uint32_t)n.size());
    track.insert(track.end(), n.begin(), n.end());
  }

  for (size_t i = 0; i < _events.size(); i++) {
    const MidiEvent &ev = _events[i];
    write_VLQ(track, ev.tick - lastTick);
    lastTick = ev.tick;

    if (ev.status == 0xff) {
      track.push_back(0xff);
      track.push_back(ev.meta);
      write_VLQ(track, (uint32_t)ev.blob.size());
      track.insert(track.end(), ev.blob.begin(), ev.blob.end());
      running = 0;
      if (ev.meta == 0x2f)
        haveEOT = true;
    } else if (ev.status == 0xf0 || ev.status == 0xf7) {
      // Blob holds the wire message (F0 ... F7); SMF stores payload after
      // the F0 status byte
      const uint8_t *p = ev.blob.data();
      size_t n = ev.blob.size();
      if (ev.status == 0xf0 && n && p[0] == 0xf0) { p++; n--; }
      track.push_back(ev.status);
      write_VLQ(track, (uint32_t)n);
      track.insert(track.end(), p, p + n);
      running = 0;
    } else {
      if (ev.status != running) {
        track.push_back(ev.status);
        running = ev.status;
      }
      uint8_t type = ev.status & 0xf0;
      track.push_back(ev.data[0]);
      if (type != 0xc0 && type != 0xd0)
        track.push_back(ev.data[1]);    // Add velocity byte to note-offs
    }
  }

  if (!haveEOT) {                       // E.g. source dump was truncated
    write_VLQ(track, 0);
    track.push_back(0xff);
    track.push_back(0x2f);
    track.push_back(0x00);
  }

  std::vector<uint8_t> out;
  const uint8_t hdr[8] = { 'M','T','h','d', 0,0,0,6 };
  out.insert(out.end(), hdr, hdr + 8);
  out.push_back(0); out.push_back(0);                       // Format 0
  out.push_back(0); out.push_back(1);                       // One track
  out.push_back((uint8_t)(_division >> 8));
  out.push_back((uint8_t)(_division & 0xff));
  const uint8_t thdr[4] = { 'M','T','r','k' };
  out.insert(out.end(), thdr, thdr + 4);
  uint32_t len = (uint32_t)track.size();
  out.push_back((uint8_t)(len >> 24)); out.push_back((uint8_t)(len >> 16));
  out.push_back((uint8_t)(len >> 8));  out.push_back((uint8_t)len);
  out.insert(out.end(), track.begin(), track.end());

  return out;
}


bool MidiFile::export_file(const std::string &path, const std::string &name,
                           std::string *error) const
{
  std::vector<uint8_t> data = to_standard_midi(name);
  FILE *f = std::fopen(path.c_str(), "wb");
  if (!f) {
    if (error) *error = "Cannot write " + path;
    return false;
  }
  size_t written = std::fwrite(data.data(), 1, data.size(), f);
  std::fclose(f);

  if (written != data.size()) {
    if (error) *error = "Short write to " + path;
    return false;
  }

  return true;
}
