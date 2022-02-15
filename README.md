# EmuSC
Emulating the Sound Canvas

## About
EmuSC is a software synthesiser that uses the original ROM files of the Roland SC-55 (and hopefully the SC-88 in the future) to recreate the original sounds of these 90s era synthesizers. The project is in no way endorsed by or affiliated with Roland Corp.

EmuSC is currently in an early development stage and is not able to reproduce sounds anywhere near the orignal, but aims towards the same perfection as [munt](https://github.com/munt/munt) has for the MT-32. If you are interested in emulating the Sound Canvas today your best bet is to use the [sound font made by Kitrinx and NewRisingSun](https://github.com/Kitrinx/SC55_Soundfont).

## Requirements
In order to emulate the Sound Canvas you need the original control ROM and the PCM / wave ROMs. These ROMs can either be extracted from your own hardware synthesizer, or you can download the ROMs from someone else. Google is your friend.

Currently only ALSA MIDI input is supported. ALSA, PulseAudio and wind32 are supported for audio output. Note that PulseAudio has a much higher latency (with default settings) than ALSA, so ALSA is recommended. The win32 output is not really useful before we also have win32 MIDI input. Support for MAC is expected soon.

## Building
Download the source tree and build with GNU build tools:
`./autogen.sh`
`./configure`
`make`

EmuSC needs C++17 and std::threads to compile.

## Contribute
All contributions are welcome! This project's success depends on other people to join as there is a lot of work ahead.

All code is released under the GPL 3+ license.


## Known issues
Too many to comment at the moment as the project is in an early stage of development.
