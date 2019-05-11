g++ -w -D__UNIX_JACK__ -D__LITTLE_ENDIAN__ \
	-o micahSynth \
	Stk.cpp SineWave.cpp BiQuad.cpp ADSR.cpp x-fun.cpp \
	BlitSaw.cpp Blit.cpp BlitSquare.cpp \
	RtAudio.cpp RtMidi.cpp \
	MiSynth.cpp micahSynthLinux.cpp \
	-lpthread -lasound -ljack