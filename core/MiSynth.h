#ifndef MI_SYNTH_H
#define MI_SYNTH_H

#include "RtAudio.h"
#include "RtMidi.h"
#include "SineWave.h"
#include "ADSR.h"
#include "BiQuad.h"
#include "BlitSaw.h"
#include "BlitSquare.h"
#include "Blit.h"
#include "Effect.h"
#include "PRCRev.h"
#include "JCRev.h"
#include "FreeVerb.h"
#include "NRev.h"
#include "x-fun.h"

using namespace stk;

// Wave types
#define SINE    0
#define SAW     1
#define SQUARE  2
#define PULSE   3

// REVERB TYPES
#define PRCREV    0
#define JCREV     1
#define NREV      2
#define FREEREV   3

//-----------------------------------------------------------------------------
// name: class MiOsc
// desc: feedback echo effect
//-----------------------------------------------------------------------------
class MiOsc {
public:
    // constructor
    MiOsc();
    // destructor
    virtual ~MiOsc();

public:
    StkFloat tick();
    void setWaveShape(int waveShape);
    void setVolume(StkFloat volume);
    void setFrequency(double freq);
    void setTuning(StkFloat oscTuning);

private:
    int m_waveShape;
    StkFloat m_oscVolume;
    double m_tune;
    double m_freq;

    BlitSaw m_blitSaw;
    BlitSquare m_blitSquare;
    SineWave m_sine;
};

//-----------------------------------------------------------------------------
// name: class MiVoice
// desc: feedback echo effect
//-----------------------------------------------------------------------------
class MiVoice {
public:
    // constructor
    MiVoice( int numOscillators = 3, double freqRangeLow = 20, double freqRangeHigh = 20000 );
    // destructor
    virtual ~MiVoice();

public:
    StkFloat tick();
    void setFreqRange( double freqRangeLow, double freqRangeHigh );
    void playNote(int note, int velocity = 127);
    void stopNote();
    void setADSR(StkFloat A, StkFloat D, StkFloat S, StkFloat R);
    void setWaveShape(int oscNum, int waveShape);
    void setOscVolume(int oscNum, StkFloat volume);
    void setOscTuning(int oscNum, double oscTuning);
    int getNote();

private:
    int m_note;
    bool m_playing;
    int m_numOscillators;
    std::vector<MiOsc*> m_oscillators;
    double m_freqRangeLow;
    double m_freqRangeHigh;

    StkFloat m_A;
    StkFloat m_D;
    StkFloat m_S;
    StkFloat m_R;
    ADSR m_adsr;
};

//-----------------------------------------------------------------------------
// name: class MiSynth
// desc: feedback echo effect
//-----------------------------------------------------------------------------
class MiSynth {
public:
    // constructor
    MiSynth( int numVoices = 8 );
    // destructor
    virtual ~MiSynth();

public:
    StkFloat tick();
    void noteOn(int note, int velocity);
    void noteOff(int note);
    void setADSR(StkFloat A, StkFloat D, StkFloat S, StkFloat R);
    void setFilter(StkFloat cutFreq, StkFloat resonance);
    void setWaveShape(int oscNum, int waveShape);
    void setOscVolume(int oscNum, StkFloat oscVolume);
    void setOscTuning(int oscNum, double oscTuning);
    void setFilterMix(StkFloat filterMix);
    void setReverbMix(StkFloat reverbMix);
    void setReverbType(int reverbType);
    void setReverbSize(StkFloat reverbSize);

private:
    int m_numVoices;
    std::vector<MiVoice*> m_voices;
    bool m_muted;
    double m_volume;
    int m_voiceSelect;
    BiQuad m_biquad;
    StkFloat m_filterMix;
    StkFloat m_reverbMix;
    PRCRev m_prcRev;
    NRev m_nRev;
    JCRev m_jcRev;
    FreeVerb m_freeRev;
    int m_reverbType;
    StkFloat m_reverbSize;
};

#endif
