# EmuSC

## Status
The EmuSC application is in development, but is already able to display a relatively correct GUI compared to the original synth. MIDI input and audio output works, albeit with some quirks, for Linux, macOS and Windows. It also supports tweaking all known synth parameters, such as reverb time, different pitch tunings etc.

For current status on the quality of the synth audio emulation, see Status section in [libEmuSC](../libemusc/README.md).

Note that this project is in no way endorsed by or affiliated with Roland Corp.

## Requirements
EmuSC depends on C++11 and libQT (version 5 or 6) in addition to platform dependent APIs for MIDI and audio. In addition you will need the original control and PCM ROMs.

### Linux
ALSA sequencer is needed for MIDI input. Recommended audio output is ALSA with device set to "PipeWire Sound Server" for those who have PipeWire. Pulse Audio and QT Multimedia are known to be buggy at this time.

### Windows
All modern versions of Windows are supported for both MIDI input and audio output. Windows has however no default MIDI sequencer, so you will either need to have a hardware MIDI port with appropriate device driver, or you will have to use a "virtual loopback MIDI cable" program. There is unfortunately no free software alternative for the latter alternative today, but for example [LoopBe1](https://www.nerds.de/en/loopbe1.html) and [loopMIDI](https://www.nerds.de/en/loopbe1.html) are freely available for non-commercial use.

### macOS
macOS 10.6 and newer are supported for both MIDI input and audio output. Only default audio output device is currently supported.

## Building
CMake is used to generate the necessary files for building EmuSC. Depending on which operating system, audio system and build environment you are using the build instructions may vary. Independent of platform, a A C++11 compiler with support for std::threads, libEmuSC and libQt version 5 or 6 (Core, Widgets and GUI) are absolute requirements.

### Linux
Note the following dependencies for Linux:
* ALSA development files (libasound2-dev on debian based distributions) is needed for MIDI input and ALSA audio.
* PulseAudio development files (libpulse-dev on debian based distributions) is needed for PulseAudio support.
* JACK development files (libjack-jackd2-dev on debian based distributions) is needed for JACK support
* QT Multimedia development framework (qtmultimedia5-dev or qt6-multimedia-dev on debian based distributions) is needed for QT Multimedia support

To build EmuSC follow these steps:
1. Enter the correct build direcotory
```
cd emusc/emusc
```
2. Run cmake (note that you have to specify the build generator) & make
```
cmake . -DCMAKE_BUILD_TYPE=Release
```
3. Build the application by running
```
make
```

### Windows
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
cmake . -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release
```
5. And finally build the application by running
```
make
```
The `emusc.exe` binary is located in `emusc/src/` if no build directory was specified. Note that you will need to deploy Qt and include a number of DLL-files if you want to run EmuSC outside MSYS. A simple script for automating the deployment of Qt and copying all the necessary DLL-files is found in the emusc/utils directory. To use this script you must have the Qt tools package installed:
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
For some reason Apple decided to not follow the C standard in their MIDI implementation. Due to this, Clang is needed for compiling MIDI support on macOS.

If you are using homebrew, install qt@5. Remember to also specify the correct paths to configure, e.g. 
```
cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt@5/5.15.6
```
On macOS the default build is not a binary file, but a bundle. To install the application copy src/emusc.app to your application folder. To run EmuSC directly from the terminal:
```
open src/emusc.app
```

## Contribute
Interested in contributing to this project? All contributions are welcome! Download / fork the source code and have a look. Create an issue if you have any questions (or input / suggestions) and we will do our best to help you out getting the hang of how it all works!

All EmuSC source code is released under the GPL 3+ license.
