# libEmuSC
Emulating the Sound Canvas

## About
libEmuSC is a software synthesizer library that aims to use the ROM files of the Roland Sound Canvas SC-55 lineup (and perhaps the SC-88 in the future) to recreate the original sounds of these '90s era synthesizers. Synth emulation is done by analyzing the control ROM for internal data structures and combined with the PCM ROMs recreate the original audio.

libEmuSC is currently in an early development stage and is not able to reproduce sounds anywhere near the quality of the original synths, but the goal is to be able to reproduce sounds that will make it difficult to notice the difference. If you are interested in emulating the SC-55 today your best bet is to try the [SC-55 sound font](https://github.com/Kitrinx/SC55_Soundfont) made by Kitrinx and NewRisingSun.

This project is in no way endorsed by or affiliated with Roland Corp.

## Status
We are currently able to decode most of the ROM's content and recreate the correct PCM samples for each instrument. Basic support for RIAA filtering and volume (TVA) envelope is also present, but no work has been done so far towards TVF envelope, pitch envelope, reverb and chorus effects etc.

## Requirements
In order to emulate the Sound Canvas you need the original control ROM and the PCM ROM(s). These ROM files can either be extracted from a physical unit by desoldering the ROM chips and read their content, or you can download the ROM files from the Internet.

libEmuSC should run on any modern operating systems. The only dependencies for compiling the source code is CMake and a modern C++ compiler.


## Building
CMake is used to generate the necessary files for building libEmuSC. Depending on which operating system and build environment you are using the build instructions may vary.

### Building on Linux
Build instructions on Linux varies quite a bit depending on which distribution that is beeing used. The general instructions are:
1. Install a C++11 compiler (typically g++), make, cmake and git
2. Download the source code from GitHub by running
```
git clone https://github.com/skjelten/emusc.git
```
3. Enter the correct build directory
```
cd emusc/libemusc
```
4. Run cmake
```
cmake .
```
Or, if you want to build a static library
```
cmake . -DBUILD_SHARED_LIBS=OFF
```
5. And finally build the library by running
```
make
```


### Building on Windows
On Windows common build environments are MSYS2/MinGW-w64 and Visual Studio.

For building on MSYS2/MinGW-w64 you need to do the following steps:

1. Install MSYS2 build environment: https://www.msys2.org
2. Start the **MSYS2 UCRT64** console and install the necessary programs by running
```
pacman -S git make libtool mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake
```
3. Download the source code from GitHub by running
```
git clone https://github.com/skjelten/emusc.git
```
4. Enter the correct build direcotory
```
cd emusc/libemusc
```
5. Run cmake (note that you have to specify the build generator)
```
cmake . -G "MSYS Makefiles"
```
Or, if you want to build a static library
```
cmake . -G "MSYS Makefiles" -DBUILD_SHARED_LIBS=OFF
```
6. And finally build the library by running
```
make
```

### Building on macOS
Building libemusc on macOS requires git, cmake and a C++11 compiler. The C++ compiler comes with macOS and git and cmake can either be downloaded from their respective homepages or be installed via 3rd. party package managers such as [Homebrew](https://brew.sh). For building libEmuSC you can therefore follow the following steps:

1. Install Homebrew by going to https://brew.sh and follow the install instruction
2. Install git and cmake
```
brew install git cmake
```
3. Download the source code from GitHub by running
```
git clone https://github.com/skjelten/emusc.git
```
4. Enter the correct build direcotory
```
cd emusc/libemusc
```
5. Run cmake
```
cmake .
```
Or, if you want to build a static library
```
cmake . -DBUILD_SHARED_LIBS=OFF
```
6. And finally build the library by running
```
make
```

## Contribute
Interested in contributing to this project? All contributions are welcome! Download / fork the source code and have a look. Create an issue if you have any questions (or input / suggestions) and we will do our best to help you out getting the hang of how it all works!

All libEmuSC source code is released under the LGPL 3+ license.
