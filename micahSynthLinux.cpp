/* micahSynth.cpp

  Author: Micah Arvey
  Date: April 17th 2019
*/

#include "MiSynth.h"
#include "RtAudio.h"
#include "RtMidi.h"
#include "SineWave.h"
#include "ADSR.h"
#include "BiQuad.h"
#include "BlitSaw.h"
#include "BlitSquare.h"
#include "Blit.h"
#include "x-fun.h"
#include <iostream>
#include <cstdlib>
#include <signal.h>

// Platform-dependent sleep routines.
#if defined(__WINDOWS_MM__)
  #include <windows.h>
  #define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds ) 
#else // Unix variants
  #include <unistd.h>
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif

using namespace stk;

#define DEFAULT_SAMPLE_RATE (44100.0)
#define NUM_CHANNELS 2
#define MIDI_DEVICE_ID 1
#define NUM_DEFALUT_VOICES 8
#define DEFAULT_VOLUME (0.9)

// global variables (good place for changing settings)
int g_numVoices = NUM_DEFALUT_VOICES;
StkFloat g_volume = DEFAULT_VOLUME;

// global mod + pitch wheel
int g_modAmount = 0;
int g_pitchValue = 64;
int g_pitchValueOffset = 64;

// MiSynth
MiSynth* g_micahSynth;

// setup interrupt funcion
bool g_done;
static void finish(int ignore){ g_done = true; }

//-----------------------------------------------------------------------------
// name: printWelcomeMessage()
// desc: prints a welcoming message
//-----------------------------------------------------------------------------
double printWelcomeMessage() {
  std::cout << "\n  Welcome to the synth! Play for fun, ctrl-c to stop.\n";
}

//-----------------------------------------------------------------------------
// name: printGoodbyeMessage()
// desc: prints a sincere goodbye message
//-----------------------------------------------------------------------------
double printGoodbyeMessage() {
  std::cout << "\n  Goodbye, Thanks for playing!\n";
}

//-----------------------------------------------------------------------------
// name: audioCallback()
// desc: This audioCallback() function handles sample computation only.  It will be
// called automatically when the system needs a new buffer of audio
// samples.
//-----------------------------------------------------------------------------
int audioCallback( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *dataPointer ) {
  // set samples to point to the beginning of the buffer
  register StkFloat *samples = (StkFloat*) outputBuffer;
  StkFloat tickSamp;

  // loop over the buffer, ticking the synth each frame
  for (int frameIndex = 0; frameIndex < nBufferFrames; frameIndex++) {
    tickSamp = g_micahSynth->tick();
    tickSamp *= g_volume;
    // duplicate over each channel
    for (int chanIndex = 0; chanIndex < NUM_CHANNELS; chanIndex++){
      *samples++ = tickSamp;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
// name: main()
// desc: entry point
//-----------------------------------------------------------------------------
int main() {
  // Set the global sample rate before creating class instances.
  Stk::setSampleRate( DEFAULT_SAMPLE_RATE );
  RtAudio dac;

  // RtAudio stream setup
  RtAudio::StreamParameters parameters;
  parameters.deviceId = dac.getDefaultOutputDevice();
  parameters.nChannels = NUM_CHANNELS;

  // Figure out how many bytes in an StkFloat 
  RtAudioFormat format = ( sizeof(StkFloat) == 8 ) ? RTAUDIO_FLOAT64 : RTAUDIO_FLOAT32;
  unsigned int bufferFrames = RT_BUFFER_SIZE;

  // setup our MicahSynth
  g_micahSynth = new MiSynth(g_numVoices);

  // Install an interrupt handler function.
  g_done = false;
  (void) signal(SIGINT, finish);

  // loop variables
  int note = 0;
  int intensity = 0;
  int knobNumber = 0;
  int waveShape = 0;
  StkFloat cutoff = 440.0;
  StkFloat resonance = 0.98;
  StkFloat maxResonance = 0.98;

  // ADSR variables
  StkFloat A = 0.01;
  StkFloat D = 0.2;
  StkFloat S = 0.5;
  StkFloat R = 0.5;

  // MIDI variables
  std::vector<unsigned char> message;
  int nBytes;
  double stamp;

  // RtMidiIn creation and constructor
  RtMidiIn  *midiin = 0;
  try {
    midiin = new RtMidiIn();
  }
  catch ( RtMidiError &error ) {
    error.printMessage();
    exit( EXIT_FAILURE );
  }

  // Check and print MIDI inputs.
  unsigned int nPorts = midiin->getPortCount();
  std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
  std::string portName;
  for ( unsigned int i=0; i<nPorts; i++ ) {
    try {
      portName = midiin->getPortName(i);
    }
    catch ( RtMidiError &error ) {
      error.printMessage();
      goto cleanup;
    }
    std::cout << "  Input Port #" << i+1 << ": " << portName << '\n';
  }

  // Open midi port at device id
  midiin->openPort( MIDI_DEVICE_ID );

  // Don't ignore sysex, timing, or active sensing messages.
  midiin->ignoreTypes( false, false, false );

  // Open and start audio stream
  try {
    dac.openStream( &parameters, NULL, format, (unsigned int)Stk::sampleRate(), &bufferFrames, &audioCallback );
    dac.startStream();
  }
  catch ( RtAudioError &error ) {
    error.printMessage();
    goto cleanup;
  }

  // Print Welcome
  printWelcomeMessage();

  // Event loop
  while(!g_done) {
    stamp = midiin->getMessage( &message );
    nBytes = message.size();

    // if there are no messages, sleep for 5ms then check again
    if (nBytes == 0) {
      SLEEP( 5 );
      continue;
    }

    int controlNumber = (int)message[0];
    switch (controlNumber) {
      case 144: // note on
        note = (int)message[1];
        intensity = (int)message[2];
        g_micahSynth->noteOn(note, intensity);
        break;

      case  128: // note off
        g_micahSynth->noteOff((int)message[1]);
        break;

      case  176: // knobs
        knobNumber = (int)message[1];
        intensity = (int)message[2];
        switch (knobNumber) {
          case 1:  // mod wheel is filter cutoff
            cutoff = (StkFloat)( 200.0 + intensity * 1000 / 128.0 );
            g_micahSynth->setFilter(cutoff, resonance);
            break;
          case 2:
            waveShape = intensity / 32;
            g_micahSynth->setWaveShape(0, waveShape);
            break;
          case 3:
            waveShape = intensity / 32;
            g_micahSynth->setWaveShape(1, waveShape);
            break;
          case 4:
            waveShape = intensity / 32;
            g_micahSynth->setWaveShape(2, waveShape);
            break;
          case 5:
            A = (StkFloat)(intensity+1) / 130.0;
            A *= A;
            g_micahSynth->setADSR(A, D, S, R);
            break;
          case 6:
            D = (StkFloat)(intensity+1) / 130.0;
            D *= D;
            g_micahSynth->setADSR(A, D, S, R);
            break;
          case 7:
            S = (StkFloat)(intensity+1) / 130.0;
            S *= S;
            g_micahSynth->setADSR(A, D, S, R);
            break;
          case 8:
            R = (StkFloat)(intensity+1) / 130.0;
            R *= R;
            g_micahSynth->setADSR(A, D, S, R);
            break;
          default:
            break;
        }
        break;

      case 224: // pitch wheel
        g_pitchValue = (int)message[2];
        break;

      default:
        std::cout << "\n \
          Zero: " << (int)message[0] << " \n \
          One: " << (int)message[1] << " \n \
          Two: " << (int)message[2] << " \n \n";
    }

    // Sleep for 5 milliseconds
    SLEEP( 5 );
  }
  
  // print goodbye message
  printGoodbyeMessage();

  // Shut down the output stream.
  try {
    dac.closeStream();
  }
  catch ( RtAudioError &error ) {
    error.printMessage();
  }
 cleanup:
  delete midiin;
  return 0;
}