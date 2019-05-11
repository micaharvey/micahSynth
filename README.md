# micahSynth
Software Synthesizer developed by Micah.  It is a MIDI controlled 8 voice polyphonic soft synth.

Signal Flow:
> OSC -> ADSR -> Filter

micahSynth will automatically select the first midi device in the list of devices.

Compile on OSX with
> source compile.sh

Compile on Linux with
> source linuxCompile.sh

Note: There are a default of two voices in the linux version to accomadate the lower powered Raspberry Pi

Technologies used:
- C++
- [Synthesis Tool Kit (stk)](https://ccrma.stanford.edu/software/stk/)
- [Real Time Audio (RtAudio)](http://www.music.mcgill.ca/~gary/rtaudio/)
- [Real Time Midi (RtMidi)](http://www.music.mcgill.ca/~gary/rtmidi/)
- [MCD-API](https://ccrma.stanford.edu/~ge/software/mcd-api/)

Special thanks to [Gary Scavone](http://www.music.mcgill.ca/~gary/) and [Ge Wang](https://ccrma.stanford.edu/~ge/)
