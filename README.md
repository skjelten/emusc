# EmuSC
Emulating the Sound Canvas

## About
EmuSC is a software synthesizer that aims to use the ROM files of the Roland Sound Canvas SC-55 lineup (and perhaps the SC-88 in the future) to recreate the original sounds of these '90s era synthesizers.

EmuSC is currently in an early development stage and is not able to reproduce sounds anywhere near the original synths, but the goal is to be able to reproduce sounds that will make it difficult to notice the difference. If you are interested in emulating the SC-55 today your best bet is to try the [SC-55 sound font](https://github.com/Kitrinx/SC55_Soundfont) made by Kitrinx and NewRisingSun.

This project is in no way endorsed by or affiliated with Roland Corp.

## Status
We are currently able to decode most of the ROM's content and recreate the correct PCM samples for each instrument. Current focus is on investigating how to apply some kind of RIAA filter and adding support for ADSR (volume) envelope. Note that development is for the time being focused on the original SC-55 (mkI) ROMs. Other versions, such as the mkII, might not work at this stage.

## Requirements
In order to emulate the Sound Canvas you need the original control ROM and the PCM ROM(s). These ROM files can either be extracted from a physical unit by desoldering the ROM chips and read their content, or you can download the ROM files from the Internet.

### Linux
ALSA sequencer is needed for MIDI input. Both ALSA and PulseAudio are supported for audio output. Note that PulseAudio has a lot of latency in its default configuration so it is recommended to use ALSA if possible.

### Windows
There is rudimentary support for all modern versions of Windows for both MIDI input and audio output. Windows has however no default MIDI sequencer, so you will either need to have a hardware MIDI port with appropriate device driver, or you will have to use a "virtual loopback MIDI cable" program. There is unfortunately no free software alternative for the latter alternative today, but for example [LoopBe1](https://www.nerds.de/en/loopbe1.html) and [loopMIDI](https://www.nerds.de/en/loopbe1.html) are freely available for non-commercial use.

### macOS
There is rudimentary support for macOS 10.6 and newer for both MIDI input and audio output.

## Building
Download the source tree and build with GNU build tools:
```
./autogen.sh
./configure
make
```

A C++17 compiler with support for std::threads is required. Library dependencies depends on which MIDI input and audio output systems that are needed.

Note that Clang is needed for compiling MIDI support on macOS. For ALSA support in Linux you need to have the libasound2-dev package installed.

## Contribute
Interested in contributing to this project? All contributions are welcome! Download / fork the source code and have a look. Create an issue if you have any questions (or input / suggestions) and we will do our best to help you out getting the hang of how it all works!

All source code is released under the GPL 3+ license.
