# EmuSC
Emulating the Sound Canvas

## About
EmuSC is a software synthesizer that aims to use the ROM files of the Roland Sound Canvas SC-55 lineup (and perhaps the SC-88 in the future) to recreate the original sounds of these '90s era synthesizers.

EmuSC is currently in an early development stage and is not able to reproduce sounds anywhere near the quality of the original synths, but the goal is to be able to reproduce sounds that will make it difficult to notice the difference. If you are interested in emulating the SC-55 today your best bet is to try the [SC-55 sound font](https://github.com/Kitrinx/SC55_Soundfont) made by Kitrinx and NewRisingSun.

This project is in no way endorsed by or affiliated with Roland Corp.

## Status
We are in the early stages of development and the application is limited to reading MIDI messages, some basic control functions and audio playback. In addition there is an unfinished LCD display that is relatively accurate to the original.

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
CMake is used to generate the necessary files for building EmuSC. Depending on which operating system, audio system and build environment you are using the build instructions may vary. Independent of platform, a A C++11 compiler with support for std::threads, libEmuSC and libQt5 (Core, Widgets and GUI) are absolute requirements.

### Linux
Note the following dependencies for Linux:
* ALSA (libasound2-dev on debian based distributions) is needed for MIDI input and ALSA audio.
* PulseAudio (libpulse-dev for debian based distributions) is needed for PulseAudio support.

### Windows
On Windows common build environments are MSYS2/MinGW-w64 and Visual Studio.

For building EmuSC on MSYS2/MinGW-w64 you need to do the following steps:

1. Install MSYS2 build environment: https://www.msys2.org and follow the [instructions for building and installing libEmuSC](../libemusc/README.md)
2. Start the **MSYS2 UCRT64** console and install extra libraries needed by EmuSC
```
pacman -S mingw-w64-ucrt-x86_64-qt5-base mingw-w64-ucrt-x86_64-qt5-multimedia
```
3. Enter the correct build direcotory
```
cd emusc/emusc
```
4. Run cmake (note that you have to specify the build generator) & make
```
cmake . -G "MSYS Makefiles"
```
5. And finally build the application by running
```
make
```
The `emusc.exe` binary is located in `emusc/src/` if no build directory was specified. Note that you will need to deploy Qt and include a number of DLL-files if you want to run EmuSC outside MSYS. A simple script for deploying Qt and coping all the needed DLL-files is found in the emusc/utils directory. To create a directory with all files needed for running EmuSC, make sure you have Qt tools package installed:
```
pacman -S mingw-w64-ucrt-x86_64-qt5-tools
```
And then run the create bundle script:
```
cd emusc/utils
./msys_create_dll_bundle.sh ../src
```
The `emusc/BUILD_WIN32` directory shall now contain all files needed to run EmuSC on any Windows computer.

Note that if you have a running in a Linux environment you can also cross-compile a Windows binary by using the MinGW toolchain.

### macOS
For some weird reason Apple decided to not follow the C standard in their MIDI implementation. Due to this, Clang is needed for compiling MIDI support on macOS.

If you are using homebrew, install qt@5. Remember to also specify the correct paths to configure, e.g. 
```
cmake . -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt@5/5.15.6
```
On macOS the default build is not a binary file, but a bundle. To install the application copy src/emusc.app to your application folder. To run EmuSC directly from the terminal execute
```
open src/emusc.app
```

## Contribute
Interested in contributing to this project? All contributions are welcome! Download / fork the source code and have a look. Create an issue if you have any questions (or input / suggestions) and we will do our best to help you out getting the hang of how it all works!

All EmuSC source code is released under the GPL 3+ license.
