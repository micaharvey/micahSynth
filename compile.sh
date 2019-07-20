g++ -w -D__MACOSX_CORE__ \
	-Icore/ -Irtaudio/ -Istk/ -Ix-api/ \
	-o micahSynth \
	stk/Stk.cpp stk/SineWave.cpp stk/BiQuad.cpp stk/ADSR.cpp \
	stk/BlitSaw.cpp stk/Blit.cpp stk/BlitSquare.cpp \
	stk/Delay.cpp stk/OnePole.cpp stk/FreeVerb.cpp \
	stk/JCRev.cpp stk/NRev.cpp stk/PRCRev.cpp \
	x-api/x-fun.cpp \
	rtaudio/RtAudio.cpp rtaudio/RtMidi.cpp \
	core/MiSynth.cpp \
	micahSynth.cpp \
	-lpthread -framework CoreAudio -framework CoreMIDI -framework CoreFoundation \
	-framework IOKit -framework Carbon  -framework OpenGL -framework GLUT \
	-framework Foundation -framework AppKit -lstdc++ -lm