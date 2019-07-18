/* micahSynth.cpp

  Author: Micah Arvey
  Date: April 2019
  Description:
  Development: Mac OSX
  Also tested on: Linux (with Jack, 2 voice polyphony)

  This code is released under the [MIT License](http://opensource.org/licenses/MIT).
  Distributed as-is; no warranty is given.
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

#define MIDI_DEVICE_ID 0
#define DEFAULT_SAMPLE_RATE (44100.0)
#define NUM_CHANNELS 2
#define NUM_DEFALUT_VOICES 8
#define DEFAULT_VOLUME (0.9)
#define NOTE_ON 144
#define NOTE_OFF 128
#define CONTROL_CHANGE 176
#define KNOBULE_LAYOUT 0
#define AKAIMPK_LAYOUT 1

// global variables (good place for changing settings)
int g_numVoices = NUM_DEFALUT_VOICES;
StkFloat g_volume = DEFAULT_VOLUME;

// global mod + pitch wheel
int g_modAmount = 0;
int g_pitchValue = 64;
int g_pitchValueOffset = 64;

// knob layout mode
int g_layoutMode = KNOBULE_LAYOUT;

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

  // Knobule input name
  std::string knobuleName ("Knobule");
  int knobuleIdNum = -1;

  // MIMIDI 25 detection
  std::string soundStickName ("MIMIDI 25");
  int soundStickIdNum = -1;

  // AKAI MPK mini detection
  std::string akaiMPKName ("MPKmini2");
  int akaiMPKIdNum = -1;

  // MIDI variables
  std::vector<unsigned char> message;
  int nBytes;
  double stamp;

  // RtMidiIn creation and constructor
  RtMidiIn  *mainMidiIn = 0;
  RtMidiIn  *soundStickMidiIn = 0;
  try {
    mainMidiIn = new RtMidiIn();
    soundStickMidiIn = new RtMidiIn();
  }
  catch ( RtMidiError &error ) {
    error.printMessage();
    exit( EXIT_FAILURE );
  }

  // Check and print MIDI inputs.
  unsigned int nPorts = mainMidiIn->getPortCount();
  std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
  std::string portName;

  // Find the midi ports we care about
  for ( unsigned int i=0; i<nPorts; i++ ) {
    try {
      portName = mainMidiIn->getPortName(i);
      if( knobuleName.compare(portName) == 0 ) {
        knobuleIdNum = i;
      } else if ( soundStickName.compare(portName) == 0 ) {
        soundStickIdNum = i;
      } else if ( akaiMPKName.compare(portName) == 0 ) {
        akaiMPKIdNum = i;
      }
    }
    catch ( RtMidiError &error ) {
      error.printMessage();
      goto cleanup;
    }
    std::cout << "  Input Port #" << i+1 << ": " << portName << '\n';
  }

  if(knobuleIdNum != -1 && soundStickIdNum != -1) {
    // Open midi port at device ids
    mainMidiIn->openPort( knobuleIdNum );
    soundStickMidiIn->openPort( soundStickIdNum );

    // Don't ignore sysex, timing, or active sensing messages.
    mainMidiIn->ignoreTypes( false, false, false );
    soundStickMidiIn->ignoreTypes( false, false, false );
  } else if (akaiMPKIdNum != -1) {
    // a bit hacky - Save a RtMidi object and logic by piggybacking on "main midi input"
    // Open midi port at device ids
    mainMidiIn->openPort( akaiMPKIdNum );

    // Don't ignore sysex, timing, or active sensing messages.
    mainMidiIn->ignoreTypes( false, false, false );

    // update layout mode
    g_layoutMode = AKAIMPK_LAYOUT;
  } else {
    std::cout << "Please plug in soundstick + knobule or the akai mpk mini and try again\n";
    goto cleanup;
  }
  

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
    stamp = mainMidiIn->getMessage( &message );
    nBytes = message.size();

    if (nBytes == 0) {
      stamp = soundStickMidiIn->getMessage( &message );
      nBytes = message.size();
    }

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
        switch(g_layoutMode) {
          case AKAIMPK_LAYOUT:
            knobNumber = (int)message[1];
            intensity  = (int)message[2];
            switch (knobNumber) {
              case 1:  // mod wheel, filter cutoff
                cutoff = (StkFloat)( 20.0 + intensity * 10000.0 / 128.0 );
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
              }
            break;
          case KNOBULE_LAYOUT:
          default:
            knobNumber = (int)message[1];
            intensity  = (int)message[2];
            switch (knobNumber) {
              case 24:  // filter resonance
                resonance = (StkFloat)((intensity+1)/ 130.0 );
                g_micahSynth->setFilter(cutoff, resonance);
                break;
              case 29:  // mod wheel, filter cutoff
                cutoff = (StkFloat)( 20.0 + intensity * 10000.0 / 128.0 );
                g_micahSynth->setFilter(cutoff, resonance);
                break;
              case 1:
                waveShape = intensity / 32;
                g_micahSynth->setWaveShape(0, waveShape);
                break;
              case 4:
                waveShape = intensity / 32;
                g_micahSynth->setWaveShape(1, waveShape);
                break;
              case 7:
                waveShape = intensity / 32;
                g_micahSynth->setWaveShape(2, waveShape);
                break;
              case 21:
                A = (StkFloat)(intensity+1) / 130.0;
                A *= A;
                g_micahSynth->setADSR(A, D, S, R);
                break;
              case 22:
                D = (StkFloat)(intensity+1) / 130.0;
                D *= D;
                g_micahSynth->setADSR(A, D, S, R);
                break;
              case 26:
                S = (StkFloat)(intensity+1) / 130.0;
                S *= S;
                g_micahSynth->setADSR(A, D, S, R);
                break;
              case 23:
                R = (StkFloat)(intensity+1) / 130.0;
                R *= R;
                g_micahSynth->setADSR(A, D, S, R);
                break;
              case 2:
                g_micahSynth->setOscVolume(0, ((StkFloat)(intensity+1) / 130.0));
                break;
              case 5:
                g_micahSynth->setOscVolume(1, ((StkFloat)(intensity+1) / 130.0));
                break;
              case 8:
                g_micahSynth->setOscVolume(2, ((StkFloat)(intensity+1) / 130.0));
                break;
              case 28:
                g_micahSynth->setFilterMix((StkFloat)(intensity+1) / 130.0);
                break;
              case 27:
                g_volume = (StkFloat)(intensity+1) / 130.0;
              default:
                break;
              }
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
  delete mainMidiIn;
  delete soundStickMidiIn;
  return 0;
}