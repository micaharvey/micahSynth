// MiSynth.cpp
#include "MiSynth.h"

using namespace stk;

//-----------------------------------------------------------------------------
// name: MiOsc()
// desc: constructor
//-----------------------------------------------------------------------------
MiOsc::MiOsc() {
    m_waveShape = SAW;
    m_oscVolume = 0.5;
    m_tune = 1.0;
    m_freq = 200.0;

    m_sine.setFrequency(m_freq);
    m_blitSaw.setFrequency(m_freq);
    m_blitSquare.setFrequency(m_freq);
}

//-----------------------------------------------------------------------------
// name: ~MiOsc()
// desc: destructor
//-----------------------------------------------------------------------------
MiOsc::~MiOsc() { }

//-----------------------------------------------------------------------------
// name: SetWaveShape()
// desc: set the wave shape for the oscilator
//-----------------------------------------------------------------------------
void MiOsc::setWaveShape(int waveShape) {
    m_waveShape = waveShape;
}

//-----------------------------------------------------------------------------
// name: SetVolume()
// desc: set the volume of the oscilator
//-----------------------------------------------------------------------------
void MiOsc::setVolume(StkFloat volume) {
    m_oscVolume = volume;
}


//-----------------------------------------------------------------------------
// name: setFrequency()
// desc: Set frequency for the oscilator
//-----------------------------------------------------------------------------
void MiOsc::setFrequency(double freq) {
    m_freq = freq;
    freq *= m_tune;

    m_sine.setFrequency(freq);
    m_blitSaw.setFrequency(freq);
    m_blitSquare.setFrequency(freq);
}

//-----------------------------------------------------------------------------
// name: setTuning()
// desc: Set tuning of the oscilator
//-----------------------------------------------------------------------------
void MiOsc::setTuning(double tune) {
    m_tune = tune;
    setFrequency(m_freq);
}

//-----------------------------------------------------------------------------
// name: MiOsc::tick()
// desc: generate a sample of output
//-----------------------------------------------------------------------------
StkFloat MiOsc::tick() {
    StkFloat sample;

    switch (m_waveShape) {
      case SINE:
        sample = m_sine.tick();
        break;
      case SAW:
        sample = m_blitSaw.tick();
        break;
      case SQUARE:
        sample = m_blitSquare.tick();
        break;
      default:
        return 0;
    }

    sample *= m_oscVolume;
    return sample;
}

//-----------------------------------------------------------------------------
// name: MiVoice()
// desc: constructor
//-----------------------------------------------------------------------------
MiVoice::MiVoice( int numOscillators, double freqRangeLow, double freqRangeHigh ) { 
    // generate oscillators
    for( int i = 0; i < numOscillators; i++) {
        MiOsc* osc = new MiOsc();
        m_oscillators.push_back(osc);
    }

    // set adsr times
    m_A = 0.01;
    m_D = 0.2;
    m_S = 0.5;
    m_R = 0.5;
    m_adsr.setAllTimes(m_A, m_D, m_S, m_R);

    // set min and max frequencies
    m_freqRangeLow = freqRangeLow;  
    m_freqRangeHigh = freqRangeHigh;

    // set class variables
    m_numOscillators = numOscillators;
    m_note = -1;
    m_playing = false;
}

//-----------------------------------------------------------------------------
// name: ~MiVoice()
// desc: destructor
//-----------------------------------------------------------------------------
MiVoice::~MiVoice() { }

//-----------------------------------------------------------------------------
// name: setFreqRange()
// desc: set the frequency range for the voice
//-----------------------------------------------------------------------------
void MiVoice::setFreqRange( double freqRangeLow, double freqRangeHigh ) {
    // set min and max frequencies
    m_freqRangeLow = freqRangeLow;  
    m_freqRangeHigh = freqRangeHigh;
}

//-----------------------------------------------------------------------------
// name: getNote()
// desc: return the note that this voice is playing (or -1 if not playing)
//-----------------------------------------------------------------------------
int MiVoice::getNote() {
    return m_note;
}

//-----------------------------------------------------------------------------
// name: playNote()
// desc: play note
//-----------------------------------------------------------------------------
void MiVoice::playNote(int note, int velocity) {
    // each voice has a few oscillators
    for (int i = 0; i < m_numOscillators; i++) {
        // set the freq on the oscillators
        m_oscillators.at(i)->setFrequency( XFun::midi2freq(note) );
    }

    m_note = note;
    m_playing = true;
    m_adsr.keyOn();
}

//-----------------------------------------------------------------------------
// name: stopNote()
// desc: stop any note playing on this voice
//-----------------------------------------------------------------------------
void MiVoice::stopNote() {
    m_playing = false;
    m_note = -1;
    m_adsr.keyOff();
}

//-----------------------------------------------------------------------------
// name: MiVoice::tick()
// desc: generate a sample of output
//-----------------------------------------------------------------------------
StkFloat MiVoice::tick() {
    StkFloat returnSamp = 0;
    StkFloat tickSamp = 0;

    // each voice has a few oscillators
    for (int i = 0; i < m_numOscillators; i++) {
        // tick the oscillators
        tickSamp = m_oscillators.at(i)->tick();

        // sum the oscillators
        returnSamp = returnSamp + tickSamp;
    }

    // return the sum, scale with adsr
    return m_adsr.tick() * returnSamp;
}

//-----------------------------------------------------------------------------
// name: setADSR()
// desc: set attack, decay, susatain, and release at once
//-----------------------------------------------------------------------------
void MiVoice::setADSR(StkFloat A, StkFloat D, StkFloat S, StkFloat R) {
    m_A = A;
    m_D = D;
    m_S = S;
    m_R = R;
    m_adsr.setAllTimes(A, D, S, R);
}

//-----------------------------------------------------------------------------
// name: SetWaveShape()
// desc: set the wave shape for the oscilator
//-----------------------------------------------------------------------------
void MiVoice::setWaveShape(int oscNum, int waveShape) {
    m_oscillators.at(oscNum)->setWaveShape(waveShape);
}

//-----------------------------------------------------------------------------
// name: SetOscVolume()
// desc: set the volume of the oscilator
//-----------------------------------------------------------------------------
void MiVoice::setOscVolume(int oscNum, StkFloat volume) {
    m_oscillators.at(oscNum)->setVolume(volume);
}
//-----------------------------------------------------------------------------
// name: setOscTuning()
// desc: set the tuning of the oscillator
//-----------------------------------------------------------------------------
void MiVoice::setOscTuning(int oscNum, double oscTuning) {
    m_oscillators.at(oscNum)->setTuning(oscTuning);
}

  //---------//
 // MiSynth //
//---------//

//-----------------------------------------------------------------------------
// name: MiSynth()
// desc: constructor
//-----------------------------------------------------------------------------
MiSynth::MiSynth( int numVoices) { 
    std::cout << "MiSynth inbound with " << numVoices << " voices\n";

    // add oscillators
    for( int i = 0; i < numVoices; i++) {
        MiVoice* voice = new MiVoice();
        m_voices.push_back(voice);
    }

    m_numVoices = numVoices;
    m_muted = false;
    m_volume = 0.9;
    m_voiceSelect = 0;
    m_filterMix = 0.1;
    m_reverbMix = 0.9;
    m_reverbType = NREV;

    // Filter set resonance
    m_biquad.setResonance( 440.0, 0.98, true );

    // Reverb setup
    m_prcRev.setT60(5);
    m_jcRev.setT60(5);
    m_nRev.setT60(5);
    m_freeRev.setEffectMix(1);
    m_freeRev.setRoomSize(0.95);
    m_freeRev.setDamping (0.5);

    // Echo setup
    unsigned long del = 11000;
    m_echoFeedback = 0.8;
    m_echoMix = 0.5;
    // 10 second maximum delay (in samples)
    m_echo1.setMaximumDelay(44100 * 10);
    m_echo2.setMaximumDelay(44100 * 10);
    m_echo3.setMaximumDelay(44100 * 10);
    m_echo4.setMaximumDelay(44100 * 10);
    // set the delays
    m_echo1.setDelay(del);
    m_echo2.setDelay(del * 2);
    m_echo3.setDelay(del * 3);
    m_echo4.setDelay(del * 4);
}

//-----------------------------------------------------------------------------
// name: ~MiSynth()
// desc: destructor
//-----------------------------------------------------------------------------
MiSynth::~MiSynth() { }

//-----------------------------------------------------------------------------
// name: MiSynth::tick()
// desc: generate a sample of output
//-----------------------------------------------------------------------------
StkFloat MiSynth::tick() {
    StkFloat sumSamp = 0;
    StkFloat tickSamp = 0;
    StkFloat filterSamp = 0;
    StkFloat filterMixedSamp = 0;
    StkFloat revSamp = 0;
    StkFloat revMixedSamp = 0;
    StkFloat echoSamp = 0;
    StkFloat echoMixedSamp = 0;
    StkFloat returnSamp = 0;

    // each voice has a few oscillators
    for (int i = 0; i < m_numVoices; i++) {
        // tick the oscillators
        tickSamp = m_voices.at(i)->tick();

        // sum the voices
        sumSamp = sumSamp + tickSamp;
    }

    // Apply Filter
    filterSamp = m_biquad.tick(sumSamp);
    filterMixedSamp = m_filterMix * filterSamp + (1.0 - m_filterMix) * sumSamp;

    echoSamp += m_echo1.tick(filterMixedSamp) * m_echoFeedback;
    echoSamp += m_echo2.tick(filterMixedSamp) * m_echoFeedback * m_echoFeedback;
    echoSamp += m_echo3.tick(filterMixedSamp) * m_echoFeedback * m_echoFeedback * m_echoFeedback;
    echoSamp += m_echo4.tick(filterMixedSamp) * m_echoFeedback * m_echoFeedback * m_echoFeedback * m_echoFeedback;

    echoMixedSamp = m_echoMix * echoSamp + (1.0 - m_echoMix) * filterMixedSamp;

    // Apply Reverb
    switch (m_reverbType) {
        case PRCREV:
            revSamp = m_prcRev.tick(echoMixedSamp);
            break;
        case FREEREV:
            revSamp = m_freeRev.tick(echoMixedSamp);
            break;
        case NREV:
            revSamp = m_nRev.tick(echoMixedSamp);
            break;
        case JCREV:
        default:
            revSamp = m_jcRev.tick(echoMixedSamp);
            break;
    }
    revMixedSamp = m_reverbMix * revSamp + (1.0 - m_reverbMix) * echoMixedSamp;
    returnSamp = revMixedSamp;

    return returnSamp;
}

//-----------------------------------------------------------------------------
// name: MiSynth::noteOn()
// desc: play a note
//-----------------------------------------------------------------------------
void MiSynth::noteOn(int note, int velocity) {
    // wrap around number of voices back to 0
    if (++m_voiceSelect >= m_numVoices) m_voiceSelect = 0;

    // if that voice is not held, use it to play, otherwise keep searching...
    if(m_voices.at(m_voiceSelect)->getNote() == -1) {
        m_voices.at(m_voiceSelect)->playNote(note, velocity);
        return;
    }

    // begin search... wrap around number of voices back to 0
    if (++m_voiceSelect >= m_numVoices) m_voiceSelect = 0;
    int counter = 0;
    while (m_voices.at(m_voiceSelect)->getNote() != -1) {
        if (++m_voiceSelect >= m_numVoices) m_voiceSelect = 0;
        // if all voices are currently held, drop the note
        if (counter++ > m_numVoices) return;
    }

    // took the long way but we made it
    m_voices.at(m_voiceSelect)->playNote(note, velocity);
    
}

//-----------------------------------------------------------------------------
// name: MiSynth::noteOff()
// desc: turn a note off
//-----------------------------------------------------------------------------
void MiSynth::noteOff(int note) {
    for (int i = 0; i < m_numVoices; i++) {
        if (m_voices.at(i)->getNote() == note) {
            m_voices.at(i)->stopNote();
        }
    }
}

//-----------------------------------------------------------------------------
// name: setADSR()
// desc: set attack, decay, susatain, and release at once
//-----------------------------------------------------------------------------
void MiSynth::setADSR(StkFloat A, StkFloat D, StkFloat S, StkFloat R) {
    for (int i = 0; i < m_numVoices; i++)
        m_voices.at(i)->setADSR(A, D, S, R);
}

//-----------------------------------------------------------------------------
// name: setFilter() 
// desc: set filter cutFreq and resonance
//-----------------------------------------------------------------------------
void MiSynth::setFilter(StkFloat cutFreq, StkFloat resonance) {
    m_biquad.setResonance( cutFreq, resonance, true );
}

//-----------------------------------------------------------------------------
// name: setWaveShape()
// desc: set the wave shape for the oscilator
//-----------------------------------------------------------------------------
void MiSynth::setWaveShape(int oscNum, int waveShape) {
    for (int i = 0; i < m_numVoices; i++) {
        m_voices.at(i)->setWaveShape(oscNum, waveShape);
    }
}

//-----------------------------------------------------------------------------
// name: setOscVolume()
// desc: set the volume of the oscilator
//-----------------------------------------------------------------------------
void MiSynth::setOscVolume(int oscNum, StkFloat volume) {
    for (int i = 0; i < m_numVoices; i++) {
        m_voices.at(i)->setOscVolume(oscNum, volume);
    }
}

//-----------------------------------------------------------------------------
// name: setFilterMix()
// desc: set the level of the filter mix
//-----------------------------------------------------------------------------
void MiSynth::setFilterMix(StkFloat filterMix) {
    m_filterMix = filterMix;
}

//-----------------------------------------------------------------------------
// name: setReverbMix()
// desc: set the level of the reverb mix
//-----------------------------------------------------------------------------
void MiSynth::setReverbMix(StkFloat reverbMix) {
    m_reverbMix = reverbMix;
}

//-----------------------------------------------------------------------------
// name: setReverbType()
// desc: set the reverb type
//-----------------------------------------------------------------------------
void MiSynth::setReverbType(int reverbType) {
    m_reverbType = reverbType;

    // Also interesting to not clear them
    m_prcRev.clear();
    m_jcRev.clear();
    m_nRev.clear();
    m_freeRev.clear();
}

//-----------------------------------------------------------------------------
// name: setReverbSize()
// desc: set the reverb size
//-----------------------------------------------------------------------------
void MiSynth::setReverbSize(StkFloat reverbSize) {
    m_reverbSize = reverbSize;

    // Reverb settings
    m_prcRev.setT60(reverbSize);
    m_jcRev.setT60(reverbSize);
    m_nRev.setT60(reverbSize);
    m_freeRev.setEffectMix(1);
    m_freeRev.setRoomSize(reverbSize / 7.11);
    m_freeRev.setDamping (0.5);
}

//-----------------------------------------------------------------------------
// name: setEchoLength()
// desc: set the length of the echo
//-----------------------------------------------------------------------------
void MiSynth::setEchoLength(unsigned long echoLength) {
    m_echoLength = echoLength;

    // set the delays
    m_echo1.setDelay(echoLength);
    m_echo2.setDelay(echoLength * 2);
    m_echo3.setDelay(echoLength * 3);
    m_echo4.setDelay(echoLength * 4);
}

//-----------------------------------------------------------------------------
// name: setEchoFeedback()
// desc: set the length of the echo
//-----------------------------------------------------------------------------
void MiSynth::setEchoFeedback(StkFloat echoFeedback) {
    m_echoFeedback = echoFeedback;
}

//-----------------------------------------------------------------------------
// name: setEchoMix()
// desc: set the mix of the echo
//-----------------------------------------------------------------------------
void MiSynth::setEchoMix(StkFloat echoMix) {
    m_echoMix = echoMix;
}

//-----------------------------------------------------------------------------
// name: setOscTuning()
// desc: set the tuning of the oscillator
//-----------------------------------------------------------------------------
void MiSynth::setOscTuning(int oscNum, double oscTuning) {
    for (int i = 0; i < m_numVoices; i++) {
        m_voices.at(i)->setOscTuning(oscNum, oscTuning);
    }
}
