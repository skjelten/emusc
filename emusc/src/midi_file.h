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

// This class is a standard MIDI file parser that also supports the Roland
// SC-55mkII control ROM demo song variant. It parses SMF format 0 and 1 into
// a single merged, tick-sorted event list, and can serialize that list back
// to a fully standard SMF (format 0) for export.


#ifndef MIDIFILE_H
#define MIDIFILE_H

#include <cstdint>
#include <string>
#include <vector>


struct MidiEvent {
  uint32_t tick;               // Absolute tick
  uint8_t  status;             // 0x80-0xEF channel msg, 0xF0/0xF7 sysex, 0xFF meta
  uint8_t  meta;               // Meta type when status == 0xFF
  uint8_t  data[2];            // Channel message data bytes
  std::vector<uint8_t> blob;   // Complete message if SysEx
};


class MidiFile {
public:
  enum class Variant {
    Auto,          // Try Standard first, fall back to MkIIRomDemo
    Standard,      // Plain SMF (mkI ROM demos, user files)
    MkIIRomDemo    // SC-55mkII ROM variant: 1-byte note-off
  };

  MidiFile() = default;

  bool load(const uint8_t *data, size_t size,
            Variant variant = Variant::Auto, std::string *error = nullptr);
  bool load_file(const std::string &path,
		 Variant variant = Variant::Auto, std::string *error = nullptr);

  const std::vector<MidiEvent> &events() const { return _events; }
  uint16_t division()  const { return _division; }   // Ticks per quarter note
  Variant  detected()  const { return _detected; }   // Actual format found
  bool     truncated() const { return _truncated; }  // Input ended mid-event
  uint32_t duration_ticks() const;

  double duration_seconds() const;
  std::string meta_text(uint8_t metaType) const;
  std::string song_name() const;

  // Serialize to a fully standards-compliant SMF format 0 file.
  // ROM-variant note-offs are written as regular 3-byte note-offs with
  // release velocity 0x40; an End of Track is appended if missing.
  std::vector<uint8_t> to_standard_midi(const std::string &name = std::string()) const;
  bool export_file(const std::string &path,
                   const std::string &name = std::string(),
                   std::string *error = nullptr) const;

private:
  bool _parse(const uint8_t *d, size_t size, bool shortNoteOff,
              std::string *error);
  bool _parse_track(const uint8_t *d, size_t size, size_t &pos, size_t end,
                    bool shortNoteOff, std::vector<MidiEvent> &out,
                    std::string *error);

  std::vector<MidiEvent> _events;
  uint16_t _division  = 96;
  Variant  _detected  = Variant::Standard;
  bool     _truncated = false;
};

#endif // MIDIFILE_H
