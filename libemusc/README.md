# libEmuSC
## About
libEmuSC is a software synthesizer library that aims to emulate the Roland Sound Canvas SC-55 lineup to recreate the original sounds of these '90s era synthesizers. Emulation is done by extracting relevant information from the original control and PCM ROMs and reimplement the synth's behavior in modern C++.

This project is in no way endorsed by or affiliated with Roland Corp.


## Status
We are currently able to decode most of the ROM's content and recreate the correct PCM samples for each instrument. Basic support for RIAA filtering and volume (TVA) envelope is also present, but no work has been done so far towards TVF envelope, pitch envelope, reverb and chorus effects.


## Requirements
libEmuSC only depends on C++11 with threads and should be very portable.


## Building
CMake is used to generate the necessary files for building libEmuSC. Depending on which operating system and build environment you are using the build instructions may vary.

### Building on Linux
Depends on distribution used, but general instructions are:
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
On Windows the common build environments are MSYS2/MinGW-w64 and Visual Studio.

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
The C++ compiler comes with macOS and git and cmake can either be downloaded from their respective homepages or be installed via 3rd. party package managers such as [Homebrew](https://brew.sh). For building libEmuSC you can follow the following steps:

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
