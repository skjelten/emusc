# EmuSC
Emulating the Sound Canvas

## About
EmuSC is a software synthesizer that aims to use the ROM files of the Roland Sound Canvas SC-55 lineup (and perhaps the SC-88 in the future) to recreate the original sounds of these '90s era synthesizers.

EmuSC is currently in an early development stage and is not able to reproduce sounds anywhere near the quality of the original synths, but the goal is to be able to reproduce sounds that will make it difficult to notice the difference. If you are interested in emulating the SC-55 today your best bet is to try the [SC-55 sound font](https://github.com/Kitrinx/SC55_Soundfont) made by Kitrinx and NewRisingSun.

The project is split in two parts:
* libEmuSC: A library that implements all the Sound Canvas emulation. It basically receives MIDI messages / control input, and returns audio data.
* EmuSC: An application that serves as a frontend to libEmuSC. It takes care of reading MIDI events from the OS and forward them to libEmuSC as well as playing the generated audio.

This project is in no way endorsed by or affiliated with Roland Corp.

## Contribute
Interested in contributing to this project? All contributions are welcome! Download / fork the source code and have a look. Create an issue if you have any questions (or input / suggestions) and we will do our best to help you out getting the hang of how it all works!

libEmuSC is released under the LGPL 3+ license.
EmuSC is released under the GPL 3+ license.
