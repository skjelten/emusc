# EmuSC

EmuSC is a software synthesizer that aims to emulate the Roland Sound Canvas SC-55 lineup to recreate the original sounds of these '90s era synthesizers. Emulation is done by extracting relevant information from the original control and PCM ROMs and reimplement the synth's behavior in modern C++.

This project has been in development since 2022 and is currently able to reproduce audio that is relatively similar to the original synths. There are still some significant shortcomings in the generated audio, varying on which instruments and settings are being used, but the goal is to be able to reproduce sounds that will make it very difficult to notice the difference.

If you are looking for the best possible SC-55 emulation today you might want to try the [Nuked SC-55](https://github.com/nukeykt/Nuked-SC55) project, or to use a sound font based on the SC-55, such as [SC-55 sound font](https://github.com/Kitrinx/SC55_Soundfont) made by Kitrinx and NewRisingSun.

The EmuSC project is split into two parts:
* [EmuSC](./emusc): A desktop application that serves as a frontend to libEmuSC.
* [libEmuSC](./libemusc): A library that implements all the Sound Canvas emulation.

Note that this project is in no way endorsed by or affiliated with Roland Corp.

![Screenshot of EmuSC v0.1.0](https://raw.githubusercontent.com/wiki/skjelten/emusc/images/Screenshot_EmuSC_0_1_0.png)


## Getting started

The quickest way to test EmuSC is to install precompiled packages of the [latest release](https://github.com/skjelten/emusc/releases/latest). If no packages are available for your platform, or you want to test the latest code changes, have a look at the detailed build instructions in the [Wiki](https://github.com/skjelten/emusc/wiki/Build-Instructions).

If you run into any problems please read the [troubleshooting guide](https://github.com/skjelten/emusc/wiki/Troubleshooting-Guide). If that did not help, or you have any other questions or feedback, feel free to start a new [discussion](https://github.com/skjelten/emusc/discussions) or create a [new issue](https://github.com/skjelten/emusc/issues).


## Contributing

Interested in C++ programming, reverse engineering, audio synthesis or synthesizers in general? We welcome anyone who wants to learn more and perhaps could contribute to the project. To get started we suggest that you:
* Download or fork the source code and have a look
* Read the [wiki](https://github.com/skjelten/emusc/wiki) for more information about the ROM files and other core parts of the emulator
* Create an issue if you have any questions or suggestions and we will do our best to help you out getting the hang of how it all works!


## License

EmuSC is free software and released under the GNU general public license:
* EmuSC is released under the GPLv3+ license.
* libEmuSC is released under the LGPLv3+ license.
