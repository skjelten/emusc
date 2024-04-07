# EmuSC
EmuSC is a software synthesizer that aims to emulate the Roland Sound Canvas SC-55 lineup to recreate the original sounds of these '90s era synthesizers. Emulation is done by extracting relevant information from the original control and PCM ROMs and reimplement the synth's behavior in modern C++.

This project is currently in development and is not able to reproduce sounds near the quality of the original synths yet, but the goal is to be able to reproduce sounds that will make it difficult to notice the difference.

If you are looking for the best possible SC-55 emulation today you might want to try the [Nuked SC-55](https://github.com/nukeykt/Nuked-SC55) project, or to use a sound font based on the SC-55, such as [SC-55 sound font](https://github.com/Kitrinx/SC55_Soundfont) made by Kitrinx and NewRisingSun.

The EmuSC project is split into two parts:
* [libEmuSC](./libemusc/README.md): A library that implements all the Sound Canvas emulation.
* [EmuSC](./emusc/README.md): A desktop application that serves as a frontend to libEmuSC.

Note that this project is in no way endorsed by or affiliated with Roland Corp.

## Building
The generic build instructions are:
1. Enter the project's root direcotory
```
cd emusc
```
2. Run cmake
```
cmake . -DCMAKE_BUILD_TYPE=Release
```
3. Build the application by running
```
make
```
For information about how to handle software dependencies and platform specific build options, read the separate README files for [libEmuSC](./libemusc/README.md) and [EmuSC](./emusc/README.md).

## License
libEmuSC is released under the LGPL 3+ license.
EmuSC is released under the GPL 3+ license.

## Contribute
Interested in contributing to this project? All contributions are welcome! Download / fork the source code and have a look. Create an issue if you have any questions (or input / suggestions) and we will do our best to help you out getting the hang of how it all works!
