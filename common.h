#ifndef COMMON_H_
#define COMMON_H_

// customizable options
// OSC base port (will also use port+1)
#define _OSCPORT "7771"

// start/stop editor with engine
#define _BUNDLE

// disable this when you experience sluggish midi reaction
//#define _MIDIBLOCK 1


// debug output
//#define _DEBUG

// prefetch support with intrinsics
//#define _PREFETCH

// vectorize calculations
//#define _VECTOR

// not customizable options ----------------------------------
#define _NAMESIZE 128
// amount of parameters per patch
#define _PARACOUNT 145
#define _CHOICECOUNT 18
#define _EGCOUNT 7

// Voices
#define _MULTITEMP 8

// how many additional settings per sound in multi to store
#define _MULTISETTINGS 11

// how many additional settings common to all sounds in multi to store
#define _MULTIPARMS 13

// Available memory slots for sounds and multis
#define _SOUNDS 512
#define _MULTIS 128

// the version number as string
#define _VERSION "1.4a"

#endif
