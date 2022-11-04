# libEmuSC
Emulating the Sound Canvas

## About
libEmuSC is a software synthesizer library that aims to use the ROM files of the Roland Sound Canvas SC-55 lineup (and perhaps the SC-88 in the future) to recreate the original sounds of these '90s era synthesizers.

libEmuSC is currently in an early development stage and is not able to reproduce sounds anywhere near the quality of the original synths, but the goal is to be able to reproduce sounds that will make it difficult to notice the difference. If you are interested in emulating the SC-55 today your best bet is to try the [SC-55 sound font](https://github.com/Kitrinx/SC55_Soundfont) made by Kitrinx and NewRisingSun.

This project is in no way endorsed by or affiliated with Roland Corp.

## Status
We are currently able to decode most of the ROM's content and recreate the correct PCM samples for each instrument. Basic support for RIAA filtering and volume (TVA) envelope is also present, but no work has been done so far towards TVF envelope, reverb and chorus effects etc.

## Requirements
In order to emulate the Sound Canvas you need the original control ROM and the PCM ROM(s). These ROM files can either be extracted from a physical unit by desoldering the ROM chips and read their content, or you can download the ROM files from the Internet.

libEmuSC should run on any modern operating systems. The only dependencies for compiling the source code is autoconf & automake and a modern C++ compiler.


## Building
Download the source tree and build with GNU build tools:
```
./autogen.sh
./configure
make
```

### Building on Windows
The GNU build tools can be somewhat challenging to use on windows, so here is a short tutorial for compiling libEmuSC on Windows:
1. Install MSYS2 as build environment: https://www.msys2.org
2. Start the **MSYS2 UCRT64** console and install the neccessary programs by running
```
pacman -S git make automake autoconf libtool mingw-w64-ucrt-x86_64-gcc
```
3. Download the source code from GitHub by running
```
git clone https://github.com/skjelten/emusc.git
```
4. Enter the correct build direcotory
```
cd emusc/libemusc
```
5. Run the generic build commands as specified above:
```
./autogen.sh
./configure
make
```
6. And finally install the library by running
```
make install
```

## Contribute
Interested in contributing to this project? All contributions are welcome! Download / fork the source code and have a look. Create an issue if you have any questions (or input / suggestions) and we will do our best to help you out getting the hang of how it all works!

All libEmuSC source code is released under the LGPL 3+ license.