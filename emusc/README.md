# EmuSC

## Status

The EmuSC application have a GUI pretty similar to the original synth. It supports configuring ROMs, MIDI & audio devices, and tweaking all the synth parameters, such as reverb time, different pitch tunings etc. Supported platforms are Linux, macOS and Windows. See [current status](https://github.com/skjelten/emusc/wiki/Status#status-emusc-frontend) page in the wiki for more information.

Note that this project is in no way endorsed by or affiliated with Roland Corp.


## Dependencies

EmuSC depends on C++11 and libQT (version 5 or 6) in addition to platform dependent APIs for MIDI and audio. In addition you will need the original control ROM and PCM ROMs.

> [!IMPORTANT]  
> All modern versions of Windows are supported for both MIDI input and audio output. Windows has however no default MIDI sequencer, so you will need to have either a hardware MIDI port with appropriate device driver, or a "virtual loopback MIDI cable" program. There is unfortunately no free software alternative for the latter alternative today, but for example [LoopBe1](https://www.nerds.de/en/loopbe1.html) and [loopMIDI](https://www.nerds.de/en/loopbe1.html) are freely available for non-commercial use.

See [the wiki](https://github.com/skjelten/emusc/wiki/Build-Instructions) for detailed build instructions.


## License

EmuSC is released under the GPLv3+ license.
