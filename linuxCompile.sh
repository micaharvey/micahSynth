g++ -w -D__UNIX_JACK__ -D__LITTLE_ENDIAN__ \
    -Icore/ -Irtaudio/ -Istk/ -Ix-api/ \
	-o micahSynth \
	stk/Stk.cpp stk/SineWave.cpp stk/BiQuad.cpp stk/ADSR.cpp \
	stk/BlitSaw.cpp stk/Blit.cpp stk/BlitSquare.cpp stk/PRCRev.cpp \
	x-api/x-fun.cpp \
	rtaudio/RtAudio.cpp rtaudio/RtMidi.cpp \
	core/MiSynth.cpp \
	micahSynth.cpp \
	-lpthread -lasound -ljack