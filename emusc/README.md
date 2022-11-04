# EmuSC
Emulating the Sound Canvas

## About
EmuSC is a software synthesizer that aims to use the ROM files of the Roland Sound Canvas SC-55 lineup (and perhaps the SC-88 in the future) to recreate the original sounds of these '90s era synthesizers.

EmuSC is currently in an early development stage and is not able to reproduce sounds anywhere near the quality og the original synths, but the goal is to be able to reproduce sounds that will make it difficult to notice the difference. If you are interested in emulating the SC-55 today your best bet is to try the [SC-55 sound font](https://github.com/Kitrinx/SC55_Soundfont) made by Kitrinx and NewRisingSun.

This project is in no way endorsed by or affiliated with Roland Corp.

## Status
We are in the early stages of development and the application is limited to reading MIDI messages, some basic control functions and audio playback. In adition there is an unfinshed LCD display that is relatively accurate to the orignal.

Currently EmuSC supports all modern versions of Linux, macOS and Windows.

## Requirements
In order to emulate the Sound Canvas you need the original control ROM and the PCM ROM(s). These ROM files can either be extracted from a physical unit by desoldering the ROM chips and read their content, or you can download the ROM files from the Internet.

### Linux
ALSA sequencer is needed for MIDI input. Both ALSA and PulseAudio are supported for audio output. Note that PulseAudio has a lot of latency in its default configuration so it is recommended to use ALSA if possible.

### Windows
There is rudimentary support for all modern versions of Windows for both MIDI input and audio output. Windows has however no default MIDI sequencer, so you will either need to have a hardware MIDI port with appropriate device driver, or you will have to use a "virtual loopback MIDI cable" program. There is unfortunately no free software alternative for the latter alternative today, but for example [LoopBe1](https://www.nerds.de/en/loopbe1.html) and [loopMIDI](https://www.nerds.de/en/loopbe1.html) are freely available for non-commercial use.

### macOS
There is rudimentary support for macOS 10.6 and newer for both MIDI input and audio output. Only default audio output device is currently supported.

## Building
Download the source tree and build with GNU build tools:
```
./autogen.sh
./configure
make
```

A C++11 compiler with support for std::threads is required. Library dependencies depends on which MIDI input and audio output systems that are needed.

### Linux
Note that libasound2-dev is needed for ALSA support and libpulse-dev for PulseAudio support.

### Windows
The GNU build tools can be somewhat challenging to use on windows, so here is a short tutorial for compiling EmuSC on Windows:
1. Install MSYS2 and follow the [instructions for building and installing libEmuSC](../libemusc/README.md)
2. Start the **MSYS2 UCRT64** console and install extra libraries needed by EmuSC
```
pacman -S mingw-w64-ucrt-x86_64-qt5-base mingw-w64-ucrt-x86_64-qt5-multimedia mingw-w64-ucrt-x86_64-pkg-config
```
3. Enter the correct build direcotory
```
cd emusc/emusc
```
4. Run the generic build commands as specified above:
```
./autogen.sh
./configure
make
```
The `emusc.exe` binary is located in `emusc/src/`. Note that you will need to include a list of DLL-files if you want to use the binary in another location / computer.

If you have a running Linux environment you can also crosscompile a Windows binary by specifying the correct host to configure, e.g. `./configure --host=x86_64-w64-mingw32`. This obviously requires that you have the crosscompiler toolchain installed.


### macOS
For some weird reason Apple decided to not follow the C standard in their MIDI implementation. Due to this, Clang is needed for compiling MIDI support on macOS.

If you are using homebrew, install autoconf, automake, llvm and qt@5. Remember to also specify the correct paths to configure, e.g.
`QT_BIN_DIRECTORY=/usr/local/Cellar/qt@5/5.15.6/bin  PKG_CONFIG_PATH=/usr/local/Cellar/qt@5/5.15.6/lib/pkgconfig ./configure`

## Contribute
Interested in contributing to this project? All contributions are welcome! Download / fork the source code and have a look. Create an issue if you have any questions (or input / suggestions) and we will do our best to help you out getting the hang of how it all works!

All EmuSC source code is released under the GPL 3+ license.
