/*! \file minicomputerCPU.c
 *  \brief sound engine core
 * 
 * This file is part of Minicomputer, which is free software:
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
/* Minicomputer
 * industrial grade digital synthesizer
 *
 * Copyright 2007,2008 Malte Steiner
 * Changes by Marc Périlleux 2018
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <jack/jack.h>
#include <jack/midiport.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <lo/lo.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <float.h>

#include "minicomputerCPU.h"
// #include <limits.h>
// This works only for c++
// #include <cmath.h>
// double epsilon = std::numeric_limits<float>::min();

// #undefine __SSE2__

// Intrinsic declarations
// see https://software.intel.com/sites/landingpage/IntrinsicsGuide/#
#if defined(__SSE2__)
#include <emmintrin.h>
#endif
#if defined(__SSE3__)
#include <pmmintrin.h>
#endif


// some common definitions
#include "../common.h" 

// defines
#define _MODCOUNT 32
/*!< Number of available modulation sources */
#define _WAVECOUNT 32
/*!< Number of available waveforms */
#define _USE_FMODF 1
#define _UPDATE_GUI 1
/*!< Enable the sound engine to send data to the gui */

// #define _CHECK_DENORM 1

// Table size must be a power of 2 so that we can use & instead of %
#define _TABLE_SIZE 4096
/*!< Number of samples in a waveform, must be a power of two */
#define _SAMPLES_PER_DEGREE (_TABLE_SIZE/360.0)
#define _TABLE_MASK 4095
#define _TABLE_SIZE_FLOAT 4096.f

//#define _DEBUG

#define _SCALE_7BIT 0.007874f
/*!< Scale factor from 0..127 to 0.0f..1.0f */

// variables
int firstTime=1;
// Should define a struct for voice (and for EG)
// float delayBuffer[_MULTITEMP][96000] __attribute__((aligned (16)));
float *delayBuffer[_MULTITEMP] __attribute__((aligned (16)));
float parameter[_MULTITEMP][_PARACOUNT] __attribute__((aligned (16)));
float modulator[_MULTITEMP][_MODCOUNT] __attribute__((aligned (16)));
float modwheel[_MULTITEMP] __attribute__((aligned (16)));
int cc[9]; // MIDI controller numbers

// Bias and scale ensure modulator range is 0..1
// This is useful when adding modulations
float modulator_bias[_MODCOUNT] __attribute__((aligned (16)))={
	1.0f, // amp. modulator #0: none aucune 
	0.0f, // amp. modulator #1: velocity vélocité
	1.0f, // amp. modulator #2: pitch bend molette de pitch
	1.0f, // amp. modulator #3: osc 1 fm out sortie fm osc 1
	1.0f, // amp. modulator #4: osc 2 fm out sortie fm osc 2
	0.0f, // amp. modulator #5: note frequency fréquence de la note
	0.0f, // amp. modulator #6: osc 2 osc 2 (unused)
	1.0f, // amp. modulator #7: filter filtre
	0.0f, // amp. modulator #8: eg 1 eg 1
	0.0f, // amp. modulator #9: eg 2 eg 2
	0.0f, // amp. modulator #10: eg 3 eg 3
	0.0f, // amp. modulator #11: eg 4 eg 4
	0.0f, // amp. modulator #12: eg 5 eg 5
	0.0f, // amp. modulator #13: eg 6 eg 6
	1.0f, // amp. modulator #14: modulation osc osc. de modulation
	0.0f, // amp. modulator #15: touch toucher
	0.0f, // amp. modulator #16: mod wheel molette de mod.
	0.0f, // amp. modulator #17: cc 12 cc 12
	1.0f, // amp. modulator #18: delay ligne à retard
	0.0f, // amp. modulator #19: midi note note midi
	0.0f, // amp. modulator #20: cc 2 cc 2
	0.0f, // amp. modulator #21: cc 4 cc 4
	0.0f, // amp. modulator #22: cc 5 cc 5
	0.0f, // amp. modulator #23: cc 6 cc 6
	0.0f, // amp. modulator #24: cc 14 cc 14
	0.0f, // amp. modulator #25: cc 15 cc 15
	0.0f, // amp. modulator #26: cc 16 cc 16
	0.0f, // amp. modulator #27: cc 17 cc 17
	0.0f, // amp. modulator #28: hold
};

float modulator_scale[_MODCOUNT] __attribute__((aligned (16)))={
	1.0f,// amp. modulator #0: none aucune 
	1.0f,// amp. modulator #1: velocity vélocité
	0.5f,// amp. modulator #2: pitch bend molette de pitch
	0.5f,// amp. modulator #3: osc 1 fm out sortie fm osc 1
	0.5f,// amp. modulator #4: osc 2 fm out sortie fm osc 2
	1.0f,// amp. modulator #5: note frequency fréquence de la note
	0.0f,// amp. modulator #6: osc 2 osc 2 (unused)
	0.5f,// amp. modulator #7: filter filtre
	1.0f,// amp. modulator #8: eg 1 eg 1
	1.0f,// amp. modulator #9: eg 2 eg 2
	1.0f,// amp. modulator #10: eg 3 eg 3
	1.0f,// amp. modulator #11: eg 4 eg 4
	1.0f,// amp. modulator #12: eg 5 eg 5
	1.0f,// amp. modulator #13: eg 6 eg 6
	0.5f,// amp. modulator #14: modulation osc osc. de modulation
	1.0f,// amp. modulator #15: touch toucher
	1.0f,// amp. modulator #16: mod wheel molette de mod.
	1.0f,// amp. modulator #17: cc 12 cc 12
	0.5f,// amp. modulator #18: delay ligne à retard
	1.0f,// amp. modulator #19: midi note note midi
	1.0f,// amp. modulator #20: cc 2 cc 2
	1.0f,// amp. modulator #21: cc 4 cc 4
	1.0f,// amp. modulator #22: cc 5 cc 5
	1.0f,// amp. modulator #23: cc 6 cc 6
	1.0f,// amp. modulator #24: cc 14 cc 14
	1.0f,// amp. modulator #25: cc 15 cc 15
	1.0f,// amp. modulator #26: cc 16 cc 16
	1.0f,// amp. modulator #27: cc 17 cc 17
	0.0f,// amp. modulator #28: hold
};
float midif[_MULTITEMP] __attribute__((aligned (16)));

/*
typedef struct _envelope_generator {
	float state __attribute__((aligned (16)));
	float Faktor __attribute__((aligned (16)));
	
	unsigned int EGtrigger __attribute__((aligned (16)));
	unsigned int EGstate __attribute__((aligned (16)));
} EG;
*/
float EG[_MULTITEMP][8][8] __attribute__((aligned (16)));
float EGFaktor[_MULTITEMP][8] __attribute__((aligned (16)));
int EGrepeat[_MULTITEMP][8] __attribute__((aligned (16)));
unsigned int EGtrigger[_MULTITEMP][8] __attribute__((aligned (16)));
unsigned int EGstate[_MULTITEMP][8] __attribute__((aligned (16)));

float phase[_MULTITEMP][4] __attribute__((aligned (16)));
unsigned int choice[_MULTITEMP][_CHOICECOUNT] __attribute__((aligned (16)));

// Filter-related variables
// Use alignas(__mm_128i) ?
// Only 3 of a possible 4 filters currently used
float high[_MULTITEMP][4] __attribute__((aligned (16)));
float band[_MULTITEMP][4] __attribute__((aligned (16)));
float low[_MULTITEMP][4] __attribute__((aligned (16)));
float f[_MULTITEMP][4] __attribute__((aligned (16)));
float q[_MULTITEMP][4] __attribute__((aligned (16)));
float v[_MULTITEMP][4] __attribute__((aligned (16)));

unsigned int keydown[_MULTITEMP]; // 0 or 1
unsigned int lastnote[_MULTITEMP];
unsigned int hold[_MULTITEMP]; // 0 or 1
int heldnote[_MULTITEMP]; // -1 for none; 0 is note C0
// unsigned int kbnotes[_MULTITEMP][128]; // Keyboard assignment state
unsigned int sostenuto[_MULTITEMP]; // 0 or 1
int sostenutonote[_MULTITEMP]; // -1 for none; 0 is note C0
unsigned int unacorda[_MULTITEMP]; // 0 or 1
float unacordacoeff[_MULTITEMP];

unsigned int polySlave[_MULTITEMP]; // Poly link
unsigned int polyMaster[_MULTITEMP]; // Poly link
float glide[_MULTITEMP];
int delayI[_MULTITEMP], delayJ[_MULTITEMP];
float sub[_MULTITEMP];
int subMSB[_MULTITEMP];

// Could implement a channel reverse lookup array to avoid scanning??
// We need to be able to stack channels
int channel[_MULTITEMP];
// note-based filtering useful for drumkit multis
int note_min[_MULTITEMP];
int note_max[_MULTITEMP];
int transpose[_MULTITEMP];
int bank[_MULTITEMP];

jack_port_t *port[_MULTITEMP + 4]; // _multitemp * ports + 2 mix and 2 aux
jack_port_t *jackMidiPort;
lo_address t;

float table [_WAVECOUNT][_TABLE_SIZE] __attribute__((aligned (16)));
float midi2freq0 [128]; // Base value A=440Hz
float midi2freq [128]; // Values after applying detune

char jackName[64]="Minicomputer"; // signifier for audio and midiconnections, to be filled with OSC port number
char jackPortName[128]; // For jack_connect

snd_seq_t *seq_handle;
int npfd;
struct pollfd *pfd;
/* a flag which will be set by our signal handler when 
 * it's time to exit */
int quit = 0;
jack_port_t* inbuf;
jack_client_t *client;

float to_filter=0.f,lfo;
float sampleRate=48000.0f; // only default, going to be overriden by the actual, taken from jack
float tabX = _TABLE_SIZE_FLOAT / 48000.0f;
float srate = 3.145f/ 48000.f; // ??
float srDivisor = 1.0f / 48000.f*100000.f; // Sample rate divisor
float glide_a, glide_b;
int i,delayBufferSize=0,maxDelayBufferSize=0,maxDelayTime=0;
jack_nframes_t bufsize;
int done = 0;
static const float antiDenormal = 1e-20; // magic number to get rid of denormalizing

// An experience with optimization
#ifdef _VECTOR  
	typedef float v4sf __attribute__ ((vector_size(16),aligned(16)));//((mode(V4SF))); // vector of four single floats
	union f4vector 
	{
		v4sf v;// __attribute__((aligned (16)));
		float f[4];// __attribute__((aligned (16)));
	};
#endif

/** \brief function to create Alsa Midiport
 * 
 * should probably be renamed
 *
 * \return handle on alsa seq
 */
snd_seq_t *open_seq() {
	snd_seq_t *seq_handle;
	int portid;
// switch for blocking behaviour for experimenting which one is better
#ifdef _MIDIBLOCK
	if (snd_seq_open(&seq_handle, "hw", SND_SEQ_OPEN_INPUT,0) < 0) 
#else
// open Alsa for input, nonblocking mode so it returns
	if (snd_seq_open(&seq_handle, "hw", SND_SEQ_OPEN_INPUT,SND_SEQ_NONBLOCK) < 0) 
#endif
	{
		fprintf(stderr, "Error opening ALSA sequencer.\n");
		exit(1);
	}
	snd_seq_set_client_name(seq_handle, jackName);
	if ((portid = snd_seq_create_simple_port(seq_handle, jackName,
			SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
			SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
		fprintf(stderr, "Error creating sequencer port.\n");
		exit(1);
	}

// filter out mididata which is not processed anyway, thanks to Karsten Wiese for the hint
	snd_seq_client_info_t * info;
	snd_seq_client_info_malloc(&info);
	if ( snd_seq_get_client_info( seq_handle, info) != 0){
		fprintf(stderr, "Error getting info for eventfiltersetup.\n");
		exit(1);
	}
	// its a bit strange, you set what you want to get, not what you want filtered out...
	// actually Karsten told me so but I got misguided by the term filter

	// snd_seq_client_info_event_filter_add	( info, SND_SEQ_EVENT_SYSEX);
	// snd_seq_client_info_event_filter_add	( info, SND_SEQ_EVENT_TICK);
	snd_seq_client_info_event_filter_add	( info, SND_SEQ_EVENT_NOTEON);
	snd_seq_client_info_event_filter_add	( info, SND_SEQ_EVENT_NOTEOFF);
	snd_seq_client_info_event_filter_add	( info, SND_SEQ_EVENT_PITCHBEND);
	snd_seq_client_info_event_filter_add	( info, SND_SEQ_EVENT_CONTROLLER);
	snd_seq_client_info_event_filter_add	( info, SND_SEQ_EVENT_CHANPRESS);
	// snd_seq_client_info_event_filter_add	( info, SND_SEQ_EVENT_CLOCK);
	// snd_seq_client_info_event_filter_add	( info, SND_SEQ_EVENT_SONGPOS);
	// snd_seq_client_info_event_filter_add	( info, SND_SEQ_EVENT_QFRAME);
	snd_seq_client_info_event_filter_add	( info, SND_SEQ_EVENT_PGMCHANGE);
	if ( snd_seq_set_client_info( seq_handle, info) != 0){
		fprintf(stderr, "Error setting info for event filter setup.\n");
		exit(1);
	}
	snd_seq_client_info_free(info);
	return(seq_handle);
}

/**
 * \brief start the specified envelope generator
 *
 * called by a note on event for that voice
 * @param voice the voice number
 * @param number the number of envelope generator
 */
static inline void egStart (const unsigned int voice,const unsigned int number)
{
	EGtrigger[voice][number]=1;
	EG[voice][number][0] = 1.0f; // triggerd
	EG[voice][number][5] = 1.0f; // target
	EG[voice][number][7] = 0.0f; // state ? keep value, retrigger
	EGstate[voice][number] = 0; // state  
	EGFaktor[voice][number] = 0.f;
#ifdef _UPDATE_GUI
	lo_send(t, "/Minicomputer/EG", "iii", voice, number, 1);
#endif
}

/**
 * \brief set the envelope to release mode
 *
 * should be called by a related note off event
 * @param voice the voice number
 * @param number the number of envelope generator
 */
static inline void egStop (const unsigned int voice,const unsigned int number)
{
	// if (EGrepeat[voice][number] == 0) 
	EGtrigger[voice][number] = 0; // triggerd
	EGstate[voice][number] = 0; // target
#ifdef _UPDATE_GUI
	if(EG[voice][number][6]>0.0f)
	  lo_send(t, "/Minicomputer/EG", "iii", voice, number, 4);
	else
	  lo_send(t, "/Minicomputer/EG", "iii", voice, number, 0);
#endif
}

/**
 * \brief Turn off the sound instantly
 *
 * @param voice the voice number
 */
static inline void egOff (const unsigned int voice)
{
	// Shut down main envelope (number 0) immediately
	EG[voice][0][6] = 0.0f;
	EGtrigger[voice][0] = 0;
	EGstate[voice][0] = 0;
#ifdef _UPDATE_GUI
	lo_send(t, "/Minicomputer/EG", "iii", voice, 0, 0);
#endif
}

/**
 * \brief calculate the envelope
 * 
 * done in audiorate to avoide zippernoise
 * @param voice the voice number
 * @param number the number of envelope generator
 * @param master the master voice in poly mode
*/
static inline float egCalc (const unsigned int voice, const unsigned int number, const unsigned int master)
{
	/* EG[x] x:
	 * 0 = trigger (0 or 1)
	 * 1 = attack rate
	 * 2 = decay rate
	 * 3 = sustain level
	 * 4 = release rate
	 * 5 = target
	 * 6 = state (=level, value between 0.0 and 1.0)
	 * EGstate - 1:Attack 2:Decay 3:Sustain 0:Release 4:Idle
	 * OSC messages - 0:Idle 1:Attack 2:Decay 3:Sustain 4:Release
	 */
	if (EGtrigger[voice][number] != 1) 
	{
		int i = EGstate[voice][number]; 
		if (i == 1){ // attack
			if (EGFaktor[voice][number]<1.00f) EGFaktor[voice][number] += 0.002f;
			
			EG[voice][number][6] += EG[master][number][1]*srDivisor*EGFaktor[voice][number];

			if (EG[voice][number][6]>=1.0f)// Attack phase is finished
			{
				EG[voice][number][6]=1.0f;
				EGstate[voice][number]=2;
				EGFaktor[voice][number] = 1.0f;
#ifdef _UPDATE_GUI
				lo_send(t, "/Minicomputer/EG", "iii", voice, number, 2);
#endif
			}
		}
		else if (i == 2){ // decay
			if (EG[voice][number][6]>EG[master][number][3]) // Sustain level not reached
			{
				EG[voice][number][6] -= EG[master][number][2]*srDivisor*EGFaktor[voice][number];
			}
			else // Sustain level reached (may be 0)
			{
				if (EGrepeat[voice][number]==0)
				{
					EGstate[voice][number]=3; // stay on sustain
#ifdef _UPDATE_GUI
					lo_send(t, "/Minicomputer/EG", "iii", voice, number, 3);
#endif
				}
				else // Repeat ON
				{
					EGFaktor[voice][number] = 1.0f; // triggerd
					egStop(voice,number); // continue to release
				}
			}
			// what happens if sustain = 0? envelope should go in stop mode when decay reaches ground
			if (EG[voice][number][6]<0.0f) 
			{	
				EG[voice][number][6]=0.0f;
				if (EGrepeat[voice][number]==0)
				{
					EGstate[voice][number]=4; // released
#ifdef _UPDATE_GUI
					lo_send(t, "/Minicomputer/EG", "iii", voice, number, 0);
#endif
				}
				else
				{
					egStart(voice, number); // repeat
				}
			}
		} // end of decay
		else if ((i == 0) && (EG[voice][number][6]>0.0f))
		{	// release
			if (EGFaktor[voice][number]>0.025f) EGFaktor[voice][number] -= 0.002f;
			EG[voice][number][6] -= EG[master][number][4]*srDivisor*EGFaktor[voice][number];

			if (EG[voice][number][6]<0.0f) 
			{	
				EG[voice][number][6]=0.0f;
				if (EGrepeat[voice][number]==0)
				{
					EGstate[voice][number]=4; // released
#ifdef _UPDATE_GUI
					lo_send(t, "/Minicomputer/EG", "iii", voice, number, 0);
#endif
				}
				else
				{
					egStart(voice, number);// repeat
				}
			}
		}
		else if(i==3) // Sustain
		{
			// Apply sustain level change
			EG[voice][number][6]=EG[master][number][3];
			// Don't get stuck on zero sustain
			if(EG[master][number][3]==0.0f){
				EGstate[voice][number]=4;
#ifdef _UPDATE_GUI
				lo_send(t, "/Minicomputer/EG", "iii", voice, number, 0);
#endif
			}
		}
	}
	else
	{ // EGtrigger is 1
		EGtrigger[voice][number] = 0;
		EG[voice][number][0] = 1.0f; // triggered
		EGstate[voice][number] = 1; // target
#ifdef _UPDATE_GUI
		lo_send(t, "/Minicomputer/EG", "iii", voice, number, 1);
#endif
	}
	return EG[voice][number][6];
}

/** @brief the audio and midi processing function from jack
 * 
 * the process callback. This is the heart of the client.
 * this will be called by jack every process cycle.
 * jack provides us with a buffer for every output port, 
 * which we can happily write into.
 *
 * @param nframes number of frames to process
 * @param *arg pointer to additional arguments
 * @return integer 0 when everything is ok
 */
int jackProcess(jack_nframes_t nframes, void *arg) {

	float tfo1_1, tfo1_2, tfo2_1, tfo2_2; // Osc freq related
	float ta1_1, ta1_2, ta1_3, ta1_4, ta2, ta3; // Osc amp related
	float tf1, tf2, tf3, morph1, morph2, modmax, mf, final_mix, tdelay;
	// float clib1, clib2; 
	float osc1, osc2, pwm, pwmMod, delayMod, pan;
	float f_scale, f_offset, bend;
	float * param;
	unsigned int * choi;
	unsigned int masterVoice;


#ifdef _CHECK_DENORM
	// Clear FPU exceptions (denormal flag)
	__asm ("fclex");
#endif

	// an union for a nice float to int casting trick which should be fast
	typedef union
	{
		int i;
		float f;
	} INTORFLOAT;
	INTORFLOAT P1 __attribute__((aligned (16)));
	INTORFLOAT P2 __attribute__((aligned (16)));
	INTORFLOAT P3 __attribute__((aligned (16)));
	INTORFLOAT Psub __attribute__((aligned (16)));
	INTORFLOAT bias; // the magic number
	bias.i = (23 +127) << 23;// generating the magic number

	// Integer phase for each oscillator, 0.._TABLE_MASK
	int iP1=0, iP2=0 ,iP3=0, iPsub=0;

	#ifdef _VECTOR
		union f4vector g __attribute__((aligned (16)));
		union f4vector h __attribute__((aligned (16)));
		union f4vector i __attribute__((aligned (16)));
		union f4vector j __attribute__((aligned (16)));
		g.f[1] = 2.f; g.f[2] = 2.f; g.f[3] = 2.f; // first entry differs always
		//g.f[1] = 2.f*srate; g.f[2] = 2.f*srate; g.f[3] = 2.f*srate; // first entry differs always
		i.f[0]=1.0f; i.f[1] = srate; i.f[2] = srate; i.f[3] = srate; 
		h.f[0]=1.0f; h.f[1] = 0.1472725f; h.f[2] = 0.1472725f; h.f[3] = 0.1472725f;
	#endif
	/* this function returns a pointer to the buffer where 
	 * we can write our frames samples */
	float *bufferMixLeft = (float*) jack_port_get_buffer(port[8], nframes);
	float *bufferMixRight = (float*) jack_port_get_buffer(port[9], nframes);
	float *bufferAux1 = (float*) jack_port_get_buffer(port[10], nframes);
	float *bufferAux2 = (float*) jack_port_get_buffer(port[11], nframes);
	
	// JACK Midi handling
	void* buf;
	jack_midi_event_t ev;
	buf = jack_port_get_buffer(jackMidiPort, nframes);
//    jack_nframes_t evcount = jack_midi_get_event_count(jackMidiPort); // Always 0 ??
//        jack_midi_port_info_t* info;
//		info = jack_midi_port_get_info(buf, bufsize);
	int index1=0;
	while ((jack_midi_event_get (&ev, buf, index1) == 0)
	  )//&& index1<evcount)
//		for(index=0; index<jack_midi_get_event_count (buf); ++index)
		{
#ifdef _DEBUG         
			printf("jack midi in event size %u\n", ev.size);
#endif
			if(ev.size==3){
				doMidi(ev.buffer[0], ev.buffer[1], ev.buffer[2]);
			} else if(ev.size==2){ // For example channel pressure
				doMidi(ev.buffer[0], ev.buffer[1], 0);
			}
			++index1;
		}

// ------------------------------------------------------------
// ------------------------ main loop -------------------------
// ------------------------------------------------------------
	register unsigned int index;
	#if defined(__SSE2__)
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	#endif
	#if defined(__SSE3__)
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	#endif
	for (index = 0; index < nframes; ++index) // for each sample 
	{

	bufferMixLeft[index]=0.f;
	bufferMixRight[index]=0.f;
	bufferAux1[index]=0.f;
	bufferAux2[index]=0.f;
	/*
	 * I don't know if its better to separate the calculation blocks, so I try it
	 * first calculating the envelopes
	 */
	register unsigned int currentvoice;
	for (currentvoice=0;currentvoice<_MULTITEMP;++currentvoice) // for each voice
	{

		// Calc the modulators
		float *mod = modulator [currentvoice];

		// Do nothing, keep previous parameters if voice is slave
		// This can never happen for voice 0!
		if(!polySlave[currentvoice]){
			masterVoice=currentvoice;
			param = parameter[currentvoice];
			choi = choice[currentvoice];
			bend=pow(2,(mod[2]*param[142]*0.08333333333)); // /12.f
			// Mod 5 is intended to help with frequency tracking
			// It is note frenquency over range scaled to 0..1
			// Mod 5 for notes 0..127 is base frequency 8.1758..12543.8556151
			f_offset=midi2freq[note_min[currentvoice]];
			if (midi2freq[note_max[currentvoice]]>f_offset){
				f_scale=1/(midi2freq[note_max[currentvoice]]-f_offset);
			}else{
				f_scale=1.0f;
			}
		}
		
		// pointer for this voice
		float * current_phase = phase[currentvoice];

		// Modulation sources fall in different categories
		// EG and midi controllers are between 0 and 1, pitch bend between -1 and +1
		// Audio outputs (osc 3, filter, delay) are between -1 and 1
		// Now we use modulation_bias and _scale so that all mods are 0..1 when needed

		mod[8] =egCalc(currentvoice, 1, masterVoice);
		mod[9] =egCalc(currentvoice, 2, masterVoice);
		mod[10]=egCalc(currentvoice, 3, masterVoice);
		mod[11]=egCalc(currentvoice, 4, masterVoice);
		mod[12]=egCalc(currentvoice, 5, masterVoice);
		mod[13]=egCalc(currentvoice, 6, masterVoice);

		// Deglitch mod wheel
		// see https://tomroelandts.com/articles/low-pass-single-pole-iir-filter
		mod[16] += 0.0005f * srDivisor * (modwheel[currentvoice] - mod[16]);

		float tracking=midif[currentvoice]*bend - glide[currentvoice];
		mod[5]=f_scale*(tracking - f_offset);
		// mod[5]=f_scale*(midif[currentvoice] - f_offset);
		// ...and clipped in case bender gets out of hand
		mod[5]=mod[5]<0.0f?0.0f:(mod[5]>1.0f?1.0f:mod[5]);

// ------------------------------------------------------------
// --------------- calc the main audio signal -----------------
// ------------------------------------------------------------

		// casting floats to int for indexing the 3 oscillator wavetables with custom typecaster
		Psub.f =  current_phase[0];
		P1.f =  current_phase[1];
		P2.f =  current_phase[2];
		P3.f =  current_phase[3];
		Psub.f += bias.f;
		P1.f += bias.f;
		P2.f += bias.f;
		P3.f += bias.f;
		Psub.i -= bias.i;
		P1.i -= bias.i;
		P2.i -= bias.i;
		P3.i -= bias.i;
		// _TABLE_MASK is a power of 2 minus one
		// We can use bitwise & instead of modulus  
		iPsub=Psub.i&_TABLE_MASK;
		iP1=P1.i&_TABLE_MASK;
		iP2=P2.i&_TABLE_MASK;
		iP3=P3.i&_TABLE_MASK;
		/*
		int iP1 = (int) current_phase[1];// float to int, cost some cycles
		int iP2 = (int) current_phase[2];// hopefully this got optimized by compiler
		int iP3 = (int) current_phase[3];// hopefully this got optimized by compiler
		*/

		// Can this ever happen??
		/*
		if (iP1<0) iP1=_TABLE_MASK;
		if (iP2<0) iP2=_TABLE_MASK;
		if (iP3<0) iP3=_TABLE_MASK;
		if (iPsub<0) iPsub=_TABLE_MASK;
		*/
// --------------- create the next oscillator phase step for osc 3
// Handle osc 3 first so that it is available as modulation for osc 1 & 2
		current_phase[3]+= tabX * param[90];
		
		#ifdef _PREFETCH
		__builtin_prefetch(&param[1],0,0);
		__builtin_prefetch(&param[2],0,1);
		__builtin_prefetch(&param[3],0,0);
		__builtin_prefetch(&param[4],0,0);
		__builtin_prefetch(&param[5],0,0);
		__builtin_prefetch(&param[7],0,0);
		__builtin_prefetch(&param[10],0,0);
		#endif
		
		// Not sure what the perf cost of fmodf could be
		// What about remainder? https://www.gnu.org/software/libc/manual/html_node/Remainder-Functions.html
#ifdef _USE_FMODF
		current_phase[3]=fmodf(current_phase[3], _TABLE_SIZE_FLOAT);
#else
		if(current_phase[3] >= _TABLE_SIZE_FLOAT)
		{
			current_phase[3]-= _TABLE_SIZE_FLOAT;
		}
		// This cannot happen in the absence of modulation
	#ifdef _DEBUG
		if(current_phase[3] < 0.f)
		{
			current_phase[3]+= _TABLE_SIZE_FLOAT;
			fprintf(stderr, "Negative phase glitch on osc 3!\n");
		}
	#endif
#endif
		// write the oscillator 3 output to modulators
		mod[14] = table[choi[12]][iP3] * (param[145]?mod[16]:1.0f);

// --------------- calculate the parameters and modulations of main oscillators 1 and 2
		// 1-((1-p)*48000/sr) = 48000/sr*p + 1 - 48000/sr
		// Should use T as parameter ?
		// glide[currentvoice]*=param[116];
		glide[currentvoice]*=glide_a*param[116]+glide_b;
		glide[currentvoice]+=copysign(antiDenormal, glide[currentvoice]); // Not needed with sse2 ??
		tfo1_1 = param[1] * param[2]; // Fixed frequency * fixed switch (0 or 1)

		// osc1 amp mods 1 and 2
		// modulators must be in range 0..1 for multiplication
		// param[9] ([10] for mod 2) is  Amount -1..1
		// Final multiply by a negative number is not an issue:
		// it's the same amplitude with phase reversal
		if(param[121]){ // Mult mode amp mod 1
			// Normalize mod 1 then multiply by signed amount
			ta1_1 = 1; // No modulation 1
			if(choi[2]) // Modulation 1 is not "none"
				ta1_1 = (mod[choi[2]]+modulator_bias[choi[2]]) * modulator_scale[choi[2]] * param[9]; // -1..1, keep 1 for modulator "none"
			ta1_2 = 1; // No modulation 2
			if(choi[3]) // Modulation 2 is not "none"
				ta1_2 = (mod[choi[3]]+modulator_bias[choi[3]]) * modulator_scale[choi[3]] * param[10]; // -1..1, keep 1 for modulator "none"
			// Apply mod 2
			if(param[118]){ // Mult mode amp mod 2
				// case mult1 mult2
				//   vol = mod1'*mod2'
#ifdef _DEBUG
				if(firstTime && index==0)
					printf("ta1 %u %f %f %f %f %f\n", choi[3], modulator_bias[choi[3]], modulator_scale[choi[3]], ta1_1, ta1_2, ta1_1*ta1_2);
#endif
				ta1_2=ta1_1*ta1_2; // -1..1
			}else{ // Add mode amp mod 2 ok
				// case mult1 add2
				//   vol = (mod1'+mod2')*0.5
				/* Use full range for mod 1 when mod 2 is none ??
				ta1_2 = 0; // No modulation 2 -- addition neutral
				if(choi[3]){ // Modulation 2 is not none
					ta1_2 = (mod[choi[3]]+modulator_bias[choi[3]]) * modulator_scale[choi[3]] * param[10]; // -1..1, keep 1 for modulator "none"
					ta1_2=(ta1_1+ta1_2)*0.5f; // -1..1
				}else{ // Modulation 2 is none
					ta1_2=ta1_1; // Use only modulation 1 ?? is this consistent with mod 1 = "none"
				}
				*/
				ta1_2=(ta1_1+ta1_2)*0.5f; // -1..1
			}
			// Apply volume to mix and FM out
			ta1_3=param[13]*ta1_2;
			ta1_4=param[14]*ta1_2;
		}else{ // Add mode amp mod 1
			// Keep full range -1..1
			ta1_2 = mod[choi[3]]*param[10]; // -1..1
			ta1_1 = mod[choi[2]]*param[9]; // -1..1
			// Apply mod 2
			if(param[118]){ // Mult mode amp mod 2 ok
				// case add1 mult2
				//   vol = 0.5 + (mod1*mod2)*0.5
				ta1_2=ta1_1*ta1_2; // -1..1
			}else{ // Add mode amp mod 2 ok
				// case add1 add2
				//   vol = 0.5 + (mod1+mod2)*0.25
				ta1_2=(ta1_1+ta1_2)*0.5f; // -1..1
			}
			// Apply volume to mix and FM out
			ta1_3=param[13]*(0.5f+ta1_2*0.5f);
			ta1_4=param[14]*(0.5f+ta1_2*0.5f);
		}

		#ifdef _PREFETCH
		__builtin_prefetch(&current_phase[1],0,2);
		__builtin_prefetch(&current_phase[2],0,2);
		#endif

		// param[2] is fixed frequency enable, 0 or 1
		// modulator[voice][2] is pitch bend
		// param[142] is pitch bend scaling (semitones)
		// tfo1_1+=(midif[currentvoice]*(1.0f-param[2])*param[3]); // Note-dependant frequency
		tfo1_1+=(midif[currentvoice]*(1.0f-param[2])*param[3]*bend); // Note-dependant frequency
		// tf+=(param[4]*param[5])*mod[choi[0]];
		tfo1_1-=glide[currentvoice]*(1.0f-param[2]);
		// What about tf *= instead of += ?
		// Would 2 multiplications be faster than an if?
		// param[4] is boost, 1 or 100
		// param[5] and [7] are amounts, -1000..1000
		if(param[117]){ // Mult.
			tfo1_2=param[4] * param[5] * mod[choi[0]] * param[7] * mod[choi[1]]; 
			if(param[6]){
				tfo1_1*=pow(2, tfo1_2*.00000001);
			}else{
				tfo1_1+=tfo1_2;
			}
		}else{
			tfo1_2=(param[4] * param[5] * mod[choi[0]]) + (param[7]*mod[choi[1]]);
			if(param[6]){
				tfo1_1*=pow(2, tfo1_2*.00001);
			}else{
				tfo1_1+=tfo1_2;
			}
		}


// --------------- generate phase of oscillator 1
		current_phase[1]+= tabX * tfo1_1;

		// iPsub=subMSB[currentvoice]+(iP1>>1); // 0.._TABLE_MASK
		// sub[currentvoice]=(4.f*iPsub/_TABLE_SIZE_FLOAT)-1.0f; // Ramp up -1..+1 (beware of int to float)
#ifdef _USE_FMODF
		if(current_phase[1] >= _TABLE_SIZE_FLOAT)
			current_phase[2]+= (param[134]*_SAMPLES_PER_DEGREE-current_phase[2])*param[115];
		current_phase[1]=fmodf(current_phase[1], _TABLE_SIZE_FLOAT);
#else
		if(current_phase[1] >= _TABLE_SIZE_FLOAT)
		{
			current_phase[1]-= _TABLE_SIZE_FLOAT;
			// branchless sync osc2 to osc1 (param[115] is 0 or 1):
			current_phase[2]+= (param[134]*_SAMPLES_PER_DEGREE-current_phase[2])*param[115];
			// sub[currentvoice]=-sub[currentvoice]; // Square
			// sub[currentvoice]=(4.f*subMSB[currentvoice]/_TABLE_SIZE_FLOAT)-1.0f; // Alt square, OK
			// Mostly works but some regular glitches at phase 0
			// subMSB[currentvoice]^=i_TABLE_SIZE_FLOAT>>1; // Halfway through table
#ifdef _DEBUG
			if (current_phase[1]>=_TABLE_SIZE_FLOAT){ //just in case of extreme fm
				fprintf(stderr, "Positive phase glitch on osc 1!\n");
				current_phase[1] = 0;
			}
#endif
		}else if(current_phase[1]< 0.f)
		{
			current_phase[1]+= _TABLE_SIZE_FLOAT;
			// subMSB[currentvoice]^=i_TABLE_SIZE_FLOAT>>1; // Halfway through table
			//	if(*phase < 0.f) *phase = _TABLE_SIZE_FLOAT-1;
#ifdef _DEBUG
			fprintf(stderr, "Negative phase glitch on osc 1!\n");
#endif
		}
#endif

		#ifdef _PREFETCH
			__builtin_prefetch(&param[19],0,0);
			__builtin_prefetch(&param[16],0,0);
			__builtin_prefetch(&param[17],0,0);
			__builtin_prefetch(&param[18],0,0);
			__builtin_prefetch(&param[20],0,0);
			__builtin_prefetch(&param[24],0,0);
			__builtin_prefetch(&param[25],0,0);
			__builtin_prefetch(&choice[currentvoice][6],0,0);
			__builtin_prefetch(&choice[currentvoice][7],0,0);
			__builtin_prefetch(&choice[currentvoice][8],0,0);
			__builtin_prefetch(&choice[currentvoice][9],0,0);
		#endif

		osc1 = table[choi[4]][iP1];
		// Osc 1 FM out
		mod[3]=osc1*ta1_3;

// --------------- generate sub
		current_phase[0]+= tabX * tfo1_1 / 2.0f;
#ifdef _USE_FMODF
		current_phase[0]=fmodf(current_phase[0], _TABLE_SIZE_FLOAT);
#else
		if(current_phase[0] >= _TABLE_SIZE_FLOAT)
			current_phase[0]-= _TABLE_SIZE_FLOAT;
		if(current_phase[0] < 0.f)
		{
			current_phase[0]+= _TABLE_SIZE_FLOAT;
#ifdef _DEBUG
			fprintf(stderr, "Negative phase glitch on sub!\n");
#endif
		}
#endif
		sub[currentvoice] = table[choi[15]][iPsub];

// ------------------------ calculate oscillator 2 ---------------------
		// first the modulations and frequencies
		tfo2_1 = param[16]*param[17]; // Fixed frequency * fixed switch (0 or 1)
		
		// osc2 first amp mod for mix output only
		// ta2 = param[24];
		// ta2 *=mod[choi[8]];
		if(param[136]){ // Mult mode amp mod
			ta2 = 1; // No modulation
			if(choi[8]!=0) // keep 1 for modulator "none"
				ta2 = (mod[choi[8]]+modulator_bias[choi[8]]) * modulator_scale[choi[8]] * param[24] * param[29]; // -1..1
		}else{ // Add mode - 0 mod means half volume
			ta2 = (0.5 + mod[choi[8]] * param[24] *0.5) * param[29]; // 0..1
		}

		tfo2_1+=midif[currentvoice]*(1.0f-param[17])*param[18]*bend;
		tfo2_1-=glide[currentvoice]*(1.0f-param[17]);

		// osc2 second amp mod for FM only
		// ta3 = param[25];
		// ta3 *=mod[choi[9]];
		if(param[133]){ // Mult mode FM out amp mod
			ta3 = 1; // No modulation
			if(choi[9]!=0) // keep 1 for modulator "none"
				ta3 = (mod[choi[9]]+modulator_bias[choi[9]]) * modulator_scale[choi[9]] * param[25]; // -1..1
		}else{ // Add mode - 0 mod means half volume
			ta3 = (0.5 + mod[choi[9]] * param[25] *0.5); // 0..1
		}

/*
		if(param[132]){
			tfo2_1+=(param[19]*param[20])*mod[choi[6]]*param[22]*mod[choi[7]];
		}else{
			tfo2_1+=(param[19]*param[20])*mod[choi[6]]+(param[22]*mod[choi[7]]);
		}
*/
		if(param[132]){ // Mult.
			tfo2_2=param[19] * param[20] * mod[choi[6]] * param[22] * mod[choi[7]]; 
			if(param[21]){
				tfo2_1*=pow(2, tfo2_2*.00000001);
			}else{
				tfo2_1+=tfo2_2;
			}
		}else{
			tfo2_2=(param[19] * param[20] * mod[choi[6]]) + (param[22]*mod[choi[7]]);
			if(param[21]){
				tfo2_1*=pow(2, tfo2_2*.00001);
			}else{
				tfo2_1+=tfo2_2;
			}
		}

		
		// param[28] is fm output vol for osc 2 (values 0..1)
		mod[4] = param[28]*ta3; // osc2 fm out coeff

		// then generate the actual phase:
		current_phase[2]+= tabX * tfo2_1;
#ifdef _USE_FMODF
		current_phase[2]=fmodf(current_phase[2], _TABLE_SIZE_FLOAT);
#else
		if(current_phase[2] >= _TABLE_SIZE_FLOAT)
		{
			current_phase[2]-= _TABLE_SIZE_FLOAT;
	#ifdef _DEBUG
			if (current_phase[2]>=_TABLE_SIZE_FLOAT){
				fprintf(stderr, "Positive phase glitch on osc 2!\n");
				current_phase[2] = 0;
			}
	#endif
		}
		if(current_phase[2]< 0.f)
		{
			current_phase[2]+= _TABLE_SIZE_FLOAT;
	#ifdef _DEBUG
			fprintf(stderr, "Negative phase glitch on osc 2!\n");
	#endif
		}
#endif
		osc2 = table[choi[10]][iP2] ;
		mod[4] *= osc2; // osc2 fm out
// --------------- generate pwm
		pwmMod=param[146]+mod[choi[18]]*param[147];
		if (pwmMod<.05) pwmMod=.05;
		if (pwmMod>.95) pwmMod=.95;
		pwm = current_phase[2]<pwmMod*_TABLE_SIZE_FLOAT?.05f/pwmMod:.05f/(pwmMod-1.0f); // Make sure there is no DC offset
		// .05f * (1/p) * p + .05f * (1/(p-1))*(1-p) = .05f *(1 + -1) = 0
#ifdef _DEBUG
		if(firstTime && index==0 && currentvoice==0)
			printf("phase osc 2 voice %u %f index %u result %f\n", currentvoice, current_phase[2], iP2, osc2);
#endif

#ifdef _PREFETCH
		__builtin_prefetch(&param[14],0,0);
		__builtin_prefetch(&param[29],0,0);
		__builtin_prefetch(&param[38],0,0);
		__builtin_prefetch(&param[48],0,0);
		__builtin_prefetch(&param[56],0,0);
#endif

// ------------- mix the 2 oscillators, sub, ring and pwm pre filter -------------------
		//temp=(parameter[currentvoice][14]-parameter[currentvoice][14]*ta1);
		// Why was 1.0f-ta n ?? offset for mod<0 ?? -1..3
		to_filter=osc1*ta1_4;
		to_filter+=osc2*ta2;
		to_filter+=sub[currentvoice]*param[141];
		to_filter+=osc1*osc2*param[138];
		to_filter+=pwm*param[148];
		to_filter*=0.2f; // get the volume of the sum into a normal range	
		to_filter+=copysign(antiDenormal, to_filter); // Absorb denormals

// ------------- calculate the filter settings ------------------------------
		if (param[137]){ // Filter bypass
			mod[7] = to_filter; // Pass input unchanged
		}else{
			// mod 1 is choice 5, amount 38 -2..2
			// mod 2 is choice 11, amount 48 -2..2, mult. 120
			// morph knob is parm 56, 0..0.5
			// TODO change all ranges to 0..1 and fix presets accordingly
			// We want to interpolate between left and right filter bank values
			// Assuming morph between 0 (full left) and 1 (full right)
			// parm = parm1 + (parm2 -parm1) * morph
			//      = parm1 (1-morph) + parm2 * morph
			// Scaling:
			// If morph1 is 0.5, mod 0..1 should result in morph 0..1
			// If morph1 is 0.1, mod 0..1 should result in morph 0..0.2
			// If morph1 is 0.9, mod 0..1 should result in morph 0.8..1.0
			morph1 = 2.0*param[56]; // 0..1
			float mf1=(mod[choi[5]]+modulator_bias[choi[5]])*modulator_scale[choi[5]]*2.0f-1.0f; // -1..1
			if (choi[11]){ // 2nd modulator is not none 
				float mf2=(mod[choi[11]]+modulator_bias[choi[11]])*modulator_scale[choi[11]]*2.0f-1.0f; // -1..1
				if(choi[5]){
					if(param[140]){ // Mult
						mf = 0.125f*param[38]*mf1*param[48]*mf2; // -0.5..0.5
						modmax = 0.125*fabs(param[38])*fabs(param[48]); // 0..0.5
					}else{ // Add
						mf = 0.125f*(param[38]*mf1+param[48]*mf2); // -0.5..0.5
						modmax = 0.125*(fabs(param[38])+fabs(param[48])); // 0..0.5
					}
				}else{ // 1st modulator is none
					mf = mf2 * param[48] * 0.25f; // -0.5..0.5
					modmax = 0.25f*fabs(param[48]); // 0..0.5
				}
			}else{ // 2nd modulator is none
				mf = mf1 * param[38] * 0.25f; // -0.5..0.5
				// if(firstTime && currentvoice==0 && index==0) printf("mod factor %f\n", mf); // -1..1 ok
				modmax = 0.25f*fabs(param[38]); // 0..0.5
			}
			
			// Prescaling ensures morph1 stays between 0 and 1
			// The conditions will never be met for modmax==0
			// We don't need to handle divide by zero
			if (morph1<modmax) mf*=morph1/modmax;
			if (1-morph1<modmax) mf*=(1-morph1)/modmax;
#ifdef _DEBUG
			if(firstTime && currentvoice==0 && index==0)
				printf("param[38] %f mod[choi[5]] %f param[48] %f mod[choi[11] %f morph1 %f mf %f modmax %f\n", param[38], mod[choi[5]], param[48], mod[choi[11]], morph1, mf, modmax);
#endif
			morph1+=mf; // 0..1

			// Clip to range (should not be needed if prescaled)
			// Clipping doesn't sound good!
#ifdef _DEBUG
			if ((morph1<0.0f) || (morph1>1.0f))
				printf("Morph clipping %f*%f %f*%f, %f %f %f!\n",param[38],mod[choi[5]],param[48],mod[choi[11]], morph1, mf, morph1-2.0f*mf);
#endif
			if (morph1<0.0f) morph1=0.0f;
			// "else" is useless if the compiler can use conditional mov
			if (morph1>1.0f) morph1=1.0f;
			// Is the abs trick actually faster?
			/*
			float clib1 = fabs (morph1);
			float clib2 = fabs (morph1-1.0f);
			morph1 = 0.5*(clib1 + 1.0f-clib2);
			*/

			#ifdef _PREFETCH
				__builtin_prefetch(&param[30],0,0);
				__builtin_prefetch(&param[31],0,0);
				__builtin_prefetch(&param[32],0,0);
				__builtin_prefetch(&param[40],0,0);
				__builtin_prefetch(&param[41],0,0);
				__builtin_prefetch(&param[42],0,0);
				__builtin_prefetch(&param[50],0,0);
				__builtin_prefetch(&param[51],0,0);
				__builtin_prefetch(&param[52],0,0);
			#endif
	/*
	// Why 1.0f - ??
			mf = (1.0f-(param[38]*mod[choi[5]])); // -1..+3 ??
			if(param[140]){
				mf*=1.0f-(param[48]*mod[choi[11]]); // -3..+9 ??
			}else{
				mf+=1.0f-(param[48]*mod[choi[11]]); // -2..+6 ??
			}
			morph1 = param[56]*mf; // -1.5..4.5 ??
			// mf not used past this point
			
			//The next four lines just clip to the [0,1] interval
			clib1 = fabs (morph1); // 0..4.5 ??
			clib2 = fabs (morph1-1.0f); // 0..3.5 ??
			morph1 = clib1 + 1.0f; // 1..5.5 ??
			morph1 -= clib2; // -2.5..5.5 ??

			//The next three lines just clip to the [0,1] interval
			float clib1 = fabs (mo);
			float clib2 = fabs (mo-1.0f);
			mo = 0.5*(clib1 + 1.0f-clib2);
 
			morph1 *= 0.5f; // -1.25..2.75 ??
	*/
			morph2 = 1.0f-morph1;

			/*
			tf= (srate * (parameter[currentvoice][30]*morph2+parameter[currentvoice][33]*morph1) );
			f[currentvoice][0] = 2.f * tf - (tf*tf*tf) * 0.1472725f;// / 6.7901358;

			tf= (srate * (parameter[currentvoice][40]*morph2+parameter[currentvoice][43]*morph1) );
			f[currentvoice][1] = 2.f * tf - (tf*tf*tf)* 0.1472725f; // / 6.7901358;;

			tf = (srate * (parameter[currentvoice][50]*morph2+parameter[currentvoice][53]*morph1) );
			f[currentvoice][2] = 2.f * tf - (tf*tf*tf) * 0.1472725f;// / 6.7901358; 
			*/
			
			// parallel calculation:
			#ifdef _VECTOR
				union f4vector a __attribute__((aligned (16))), b __attribute__((aligned (16))),  c __attribute__((aligned (16))), d __attribute__((aligned (16))),e __attribute__((aligned (16)));

				b.f[0] = morph2; b.f[1] = morph2; b.f[2] = morph2; b.f[3] = morph2;
				a.f[0] = param[30]; a.f[1] =param[31]; a.f[2] = param[32]; a.f[3] = param[40];
				d.f[0] = param[41]; d.f[1] =param[42]; d.f[2] = param[50]; d.f[3] = param[51];
				c.v = a.v * b.v;
				//c.v = __builtin_ia32_mulps (a.v, b.v);
				e.v = d.v * b.v;
				//e.v = __builtin_ia32_mulps (d.v, b.v);

				tf1 = c.f[0];
				q[currentvoice][0]=c.f[1];
				v[currentvoice][0]=c.f[2];
				tf2 = c.f[3];
				q[currentvoice][1] = e.f[0];
				v[currentvoice][1] = e.f[1];
				tf3 = e.f[2];
				q[currentvoice][2] = e.f[3];
				v[currentvoice][2] = param[52];
			#else
				// Load left filter bank parameters
				tf1 = param[30];
				q[currentvoice][0] = param[31];
				v[currentvoice][0] = param[32];
				tf2 = param[40];
				q[currentvoice][1] = param[41];
				v[currentvoice][1] = param[42];
				tf3 = param[50];
				q[currentvoice][2] = param[51];
				v[currentvoice][2] = param[52];
			#endif

			#ifdef _PREFETCH
			__builtin_prefetch(&param[33],0,0);
			__builtin_prefetch(&param[34],0,0);
			__builtin_prefetch(&param[35],0,0);
			__builtin_prefetch(&param[43],0,0);
			__builtin_prefetch(&param[44],0,0);
			__builtin_prefetch(&param[45],0,0);
			__builtin_prefetch(&param[53],0,0);
			__builtin_prefetch(&param[54],0,0);
			__builtin_prefetch(&param[55],0,0);
			#endif

			#ifdef _VECTOR
				v[currentvoice][2] *= morph2;
				a.f[0] = param[33]; a.f[1] =param[34]; a.f[2] = param[35]; a.f[3] = param[43];
				d.f[0] = param[44]; d.f[1] =param[45]; d.f[2] = param[53]; d.f[3] = param[54];
				b.f[0] = morph1; b.f[1] = morph1; b.f[2] = morph1; b.f[3] = morph1;
				c.v = a.v * b.v;
				//c.v = __builtin_ia32_mulps (a.v, b.v);
				e.v = d.v * b.v;
				//e.v = __builtin_ia32_mulps (d.v, b.v);

				tf1 += c.f[0];
				tf2 += c.f[3];
				tf3 += e.f[2];
				q[currentvoice][0] += c.f[1];//parameter[currentvoice][34]*morph1;
				q[currentvoice][1] += e.f[0];//parameter[currentvoice][44]*morph1;
				q[currentvoice][2] += e.f[3];//parameter[currentvoice][54]*morph1;
				v[currentvoice][0] += c.f[2];//parameter[currentvoice][35]*morph1;
				v[currentvoice][1] += e.f[1];//parameter[currentvoice][45]*morph1;
			#else
				tf1 *= morph2;
				tf2 *= morph2;
				tf3 *= morph2;
				q[currentvoice][0] *= morph2;
				q[currentvoice][1] *= morph2;
				q[currentvoice][2] *= morph2;
				v[currentvoice][0] *= morph2;
				v[currentvoice][1] *= morph2;
				v[currentvoice][2] *= morph2;
				// Right filter bank frequencies
				tf1 += param[33]*morph1;
				tf2 += param[43]*morph1;
				tf3 += param[53]*morph1;
			#endif


			#ifdef _VECTOR
			// prepare next calculations
				a.f[0] = param[55]; a.f[1] =tf1; a.f[2] = tf2; a.f[3] = tf3;
				g.f[0] = morph1;// b.f[1] = 2.f; b.f[2] = 2.f; b.f[3] = 2.f;
				j.v = a.v * i.v; // tf * srate
				c.v = j.v * g.v; // tf * 2
				//c.v = __builtin_ia32_mulps (a.v, g.v);

				v[currentvoice][2] += c.f[0];//parameter[currentvoice][55]*morph1;

				//f[currentvoice][0] = c.f[1];//2.f * tf1;
				//f[currentvoice][1] = c.f[2];//2.f * tf2;
				//f[currentvoice][2] = c.f[3];//2.f * tf3;
				//pow(c.v,3);
				d.v = c.v - ((j.v * j.v * j.v) * h.v);

				f[currentvoice][0] = d.f[1];
				f[currentvoice][1] = d.f[2];
				f[currentvoice][2] = d.f[3];
			#else
				// Right filter bank parameters
				q[currentvoice][0] += param[34]*morph1;
				q[currentvoice][1] += param[44]*morph1;
				q[currentvoice][2] += param[54]*morph1;

				v[currentvoice][0] += param[35]*morph1;
				v[currentvoice][1] += param[45]*morph1;
				v[currentvoice][2] += param[55]*morph1;
#ifdef _DEBUG
	if(firstTime && currentvoice==0 && index==0)
		printf("tf1 %f = param[30] %f * morph2 %f + param[33] %f * morph1 %f\n", tf1, param[30], morph2, param[33], morph1);
#endif
				// Apply filter tracking
				float fpct= (tracking-440)/440;
				tf1*=1+param[149]*fpct;
				tf2*=1+param[150]*fpct;
				tf3*=1+param[151]*fpct;
				
				tf1*=srate;
				tf2*=srate;
				tf3*=srate;
				f[currentvoice][0] = tf1;
				f[currentvoice][1] = tf2;
				f[currentvoice][2] = tf3; 
/*
				// Sin approximation sin(x) ~~ x - x^3 / 6.7901358
				// 1 / 6.7901358 = 0.1472725f
				// Why 2.f * ? Why sin?
				f[currentvoice][0] = 2.f * tf1;
				f[currentvoice][1] = 2.f * tf2;
				f[currentvoice][2] = 2.f * tf3; 
				f[currentvoice][0] -= (tf1*tf1*tf1) * 0.1472725f;
				f[currentvoice][1] -= (tf2*tf2*tf2) * 0.1472725f;
				f[currentvoice][2] -= (tf3*tf3*tf3) * 0.1472725f;
*/
// if(firstTime && currentvoice==0 && index==0)
//	printf("f[currentvoice][0] %f\n", f[currentvoice][0]);
			#endif
			// morph1 and morph2 not used past this point
	//----------------------- actual filter calculation -------------------------
			// filters
#ifdef __SSE2__
			// Could compute a 4th filter without penalty
			// Could use the same register for f4, q4 and v4 ?
			__m128 f4 = _mm_load_ps(f[currentvoice]);
			__m128 band4 = _mm_load_ps(band[currentvoice]);
			__m128 low4 = _mm_load_ps(low[currentvoice]);
			__m128 temp4 = _mm_mul_ps(f4, band4); // f*band
			low4 = _mm_add_ps(temp4, low4); // low + f*band
			__m128 q4 = _mm_load_ps(q[currentvoice]);
			_mm_store_ps(low[currentvoice], low4);
			temp4 = _mm_mul_ps(q4, band4); // q*band
			__m128 temp4b = _mm_set_ps1(0.9f);
			temp4 = _mm_add_ps(temp4, low4); // q*band+low
			temp4b = _mm_mul_ps(q4, temp4b); // 0.9f * q
			__m128 temp4c = _mm_set_ps1(0.1f);
			__m128 hi4 = _mm_load_ps1(&to_filter);
			temp4b = _mm_add_ps(temp4c, temp4b); // 0.9f * q + 0.1f
			hi4 = _mm_mul_ps(temp4b, hi4); // (0.9f * q + 0.1f)*to_filter
			hi4 = _mm_sub_ps(hi4, temp4); // (0.9f * q + 0.1f)*to_filter - (q*band + low)
			_mm_store_ps(high[currentvoice], hi4);
			
			hi4 = _mm_mul_ps(f4, hi4); // f*hi
			band4 = _mm_add_ps(band4, hi4); // band + f*hi
			_mm_store_ps(band[currentvoice], band4);

			// Total filter modulation output
			float from_filter[4];
			__m128 v4 = _mm_load_ps(v[currentvoice]);
			// No madd for float in sse2
			// use _mm_shuffle_ps?
			// Use masking and/or
			// Beware of endianness?
			temp4 = (__m128) _mm_set_epi32(0, 0, 0, 0xFFFFFFFF); // Mask array item [0]
			low4 = (__m128) _mm_and_si128((__m128i) temp4,(__m128i) low4);
			band4 = (__m128) _mm_andnot_si128((__m128i)temp4,(__m128i) band4);
			band4 = (__m128) _mm_or_si128((__m128i)low4, (__m128i)band4);
			band4 = _mm_mul_ps(v4, band4);
			_mm_store_ps(from_filter, band4);
			mod[7] = from_filter[0]+from_filter[1]+from_filter[2];

			// mod[7] = (low[currentvoice][0]*v[currentvoice][0])+band[currentvoice][1]*v[currentvoice][1]+band[currentvoice][2]*v[currentvoice][2];
#else
			low[currentvoice][0] += f[currentvoice][0] * band[currentvoice][0];
			low[currentvoice][1] += f[currentvoice][1] * band[currentvoice][1];
			low[currentvoice][2] += f[currentvoice][2] * band[currentvoice][2];

			// x+((1-x)*.1) = x + 1*.1 - x*.1 = 0.9 x + 0.1
			high[currentvoice][0] = (0.9f*q[currentvoice][0]+0.1f)*to_filter - low[currentvoice][0] - q[currentvoice][0]*band[currentvoice][0];
			//high[currentvoice][0] = (q[currentvoice] *to_filter) - low[currentvoice][0] - (q[currentvoice][0]*band[currentvoice][0]);
			high[currentvoice][1] = (0.9f*q[currentvoice][1]+0.1f)*to_filter - low[currentvoice][1] - q[currentvoice][1]*band[currentvoice][1];
			high[currentvoice][2] = (0.9f*q[currentvoice][2]+0.1f)*to_filter - low[currentvoice][2] - q[currentvoice][2]*band[currentvoice][2];

			band[currentvoice][0] += f[currentvoice][0] * high[currentvoice][0];
			band[currentvoice][1] += f[currentvoice][1] * high[currentvoice][1];
			band[currentvoice][2] += f[currentvoice][2] * high[currentvoice][2];
/*			// first filter
			float reso = q[currentvoice][0]; // for better scaling the volume with rising q
			low[currentvoice][0] = low[currentvoice][0] + f[currentvoice][0] * band[currentvoice][0];
			high[currentvoice][0] = ((reso + ((1.0f-reso)*0.1f))*to_filter) - low[currentvoice][0] - (reso*band[currentvoice][0]);
			//high[currentvoice][0] = (reso *to_filter) - low[currentvoice][0] - (q[currentvoice][0]*band[currentvoice][0]);
			band[currentvoice][0]= f[currentvoice][0] * high[currentvoice][0] + band[currentvoice][0];

			// second filter
			reso = q[currentvoice][1];
			low[currentvoice][1] = low[currentvoice][1] + f[currentvoice][1] * band[currentvoice][1];
			high[currentvoice][1] = ((reso + ((1.0f-reso)*0.1f))*to_filter) - low[currentvoice][1] - (reso*band[currentvoice][1]);
			// high[currentvoice][1] = (q[currentvoice][1] * band[currentvoice][1]) - low[currentvoice][1] - (q[currentvoice][1]*band[currentvoice][1]);
			band[currentvoice][1]= f[currentvoice][1] * high[currentvoice][1] + band[currentvoice][1];
			
			// third filter
			reso = q[currentvoice][2];
			low[currentvoice][2] = low[currentvoice][2] + f[currentvoice][2] * band[currentvoice][2];
			high[currentvoice][2] = ((reso + ((1.0f-reso)*0.1f))*to_filter) - low[currentvoice][2] - (reso*band[currentvoice][2]);
			band[currentvoice][2]= f[currentvoice][2] * high[currentvoice][2] + band[currentvoice][2];
*/
			// Total filter modulation output
			mod[7] = (low[currentvoice][0]*v[currentvoice][0])+band[currentvoice][1]*v[currentvoice][1]+band[currentvoice][2]*v[currentvoice][2];
#endif
		} // End of if filter bypass
	//---------------------------------- amplitude shaping
			final_mix = 1.0f-mod[choi[13]]*param[100];
			final_mix *= mod[7];
			final_mix *= egCalc(currentvoice, 0, masterVoice); // the final shaping envelope
			final_mix *= unacordacoeff[currentvoice]; // Should be a parameter ??

	// --------------------------------- delay unit
			if( delayI[currentvoice] >= delayBufferSize )
			{
				delayI[currentvoice] = 0;
				//printf("clear %d : %d : %d\n",currentvoice,delayI[currentvoice],delayJ[currentvoice]);
			}
			
			// param[110] is time mod. amount 0..1
			// param[143] is mult.
			// param[144] is time mod. 2 amount 0..1
			// delayMod = 1.0f-(param[110]* mod[choi[14]]); // Why 1.0f- ?? 0..2
			if(param[143]){ // Mult. on
				delayMod = 1.0f + param[110] * mod[choi[14]] * param[144] * mod[choi[17]];
			}else{// Mult. off
				delayMod = 1.0f + (param[110] * mod[choi[14]] + param[144] * mod[choi[17]]) *0.5f;
			}
			// param[111] is delay time 0..1 (seconds?)
			delayJ[currentvoice] = delayI[currentvoice] - ((param[111]* maxDelayTime)*delayMod);

			// Clip or wrap ??
			if( delayJ[currentvoice]  < 0 )
			{
				delayJ[currentvoice]  += delayBufferSize; // wrap
			}
			/*
			else if (delayJ[currentvoice]>=delayBufferSize)
			{
				delayJ[currentvoice] = 0; // ??
			}
			*/
			// May still have to clip
			delayJ[currentvoice]= delayJ[currentvoice]<0?0:(delayJ[currentvoice]>=delayBufferSize?delayBufferSize-1:delayJ[currentvoice]);

			//if (delayI[currentvoice]>95000) printf("jab\n");

			// param[114] is "to delay" knob 0..1
			// param[112] is "feedback" knob 0..1
			tdelay = final_mix * param[114] + (delayBuffer[currentvoice] [ delayJ[currentvoice] ] * param[112] );
			tdelay += copysign(antiDenormal, tdelay);
			delayBuffer[currentvoice] [delayI[currentvoice] ] = tdelay;
			/*
			if (delayI[currentvoice]>95000)
			{
				printf("lll %d : %d : %d\n",currentvoice,delayI[currentvoice],delayJ[currentvoice]);
				fflush(stdout);
			}
			*/
			mod[18]=tdelay; // delay as modulation source
			final_mix += tdelay * param[113]; // add to final mix
			delayI[currentvoice]=delayI[currentvoice]+1;

	// --------------------------------- output
			float *buffer = (float*) jack_port_get_buffer(port[currentvoice], nframes);
			buffer[index] = final_mix * param[101];
			bufferAux1[index] += final_mix * param[108];
			bufferAux2[index] += final_mix * param[109];
			final_mix *= param[106]; // mix volume
			pan=(param[122]*mod[choi[16]])+param[107];
			if (pan<0.f) pan=0.f;
			if (pan>1.0f) pan=1.0f;
			bufferMixLeft[index] += final_mix * (1.0f-pan);
			bufferMixRight[index] += final_mix * pan;
			}
		}
	firstTime=0;
#ifdef _CHECK_DENORM
	// Retrieve FPU status word
	// Not sure this actually works ??
	short int fpu_status;
	short int* fpu_status_ptr=&fpu_status;
	__asm("fstsw %0" : "=r" (fpu_status_ptr));
	if(fpu_status & 0x0002) printf("Denormal!\n");
#endif
	return 0; // thanks to Sean Bolton who was the first pointing to a bug when I returned 1
} // end of process function



/** @brief the signal handler 
 *
 * it's used here only for quitting
 * @param signal the signal
 */
void signalled(int signal) {
	quit = 1;
}

/** @brief initialization
 *
 * prepares the waveforms and initial tuning
 */
void init ()
{
	unsigned int i, k;
	for (k=0;k<_MULTITEMP;k++) // k is the voice number
	{
		channel[k]=k;
		note_min[k]=0;
		note_max[k]=127;

		for (i=0;i<_EGCOUNT;i++) // i is the number of envelope
		{
			EG[k][i][1]=0.01f;
			EG[k][i][2]=0.01f;
			EG[k][i][3]=1.0f;
			EG[k][i][4]=0.0001f;
			EGrepeat[k][i]=0;

			EGtrigger[k][i]=0;
			EGstate[k][i]=4; // released
		}
		// Filter parameters
		// Will be changed by gui but engine can start on its own
		parameter[k][30]=100.f;
		parameter[k][31]=0.5f;
		parameter[k][33]=100.f; 
		parameter[k][34]=0.5f;
		parameter[k][40]=100.f;
		parameter[k][41]=0.5f;
		parameter[k][43]=100.f; 
		parameter[k][44]=0.5f;
		parameter[k][50]=100.f;
		parameter[k][51]=0.5f;
		parameter[k][53]=100.f; 
		parameter[k][54]=0.5f;
		modulator[k][0] =0.f;// the none modulator, doing nothing

		sub[k]=1.0f;
		// Init filter state
		for (i=0;i<3;++i) 
		{
			low[k][i]=0;
			high[k][i]=0;
		}
	}

	float PI=3.14159265359;
	float increment = (float)(PI*2) / (float)_TABLE_SIZE;
	float x = 0.0f;
	float tri = -0.9f;
	// calculate wavetables (values between -1.0 and 1.0)
	for (i=0; i<_TABLE_SIZE; i++)
	{
		x = i*increment;
		table[0][i] = sin(x);
		table[1][i] = (float)i/_TABLE_SIZE_FLOAT*2.f-1.0f;// ramp up
			
		table[2][i] = (float)(_TABLE_SIZE-i-1)/_TABLE_SIZE_FLOAT*2.f-1.0f; //ramp down
		// 0.9f-(i/_TABLE_SIZE_FLOAT*1.8f-0.5f);// _TABLE_SIZE_FLOAT-((float)i/_TABLE_SIZE_FLOAT*2.f-1.0f);
			
		if (i<_TABLE_SIZE/2) 
		{ 
			tri+=(float)1.0f/_TABLE_SIZE*3.f; 
			table[3][i] = tri;
			table[4][i]=0.9f;
		}
		else
		{
			 tri-=(float)1.0f/_TABLE_SIZE*3.f;
			 table[3][i] = tri;
			 table[4][i]=-0.9f;
		}
		table[5][i] = 0.f;
		table[6][i] = 0.f;
		if (i % 2 == 0)
			table[7][i] = 0.9f;
		else
			table [7][i] = -0.9f;
	
		table[8][i]=(float) (
			((float)sin(x+((float)2.0f*(float)PI))) +
			((float)sin(x*2.f+((float)2.0f*(float)PI)))+
			((float)sin(x*3.f+((float)2.0f*(float)PI)))+
			((float)sin(x*4.f+((float)2.0f*(float)PI)))*0.9f+
			((float)sin(x*5.f+((float)2.0f*(float)PI)))*0.8f+
			((float)sin(x*6.f+((float)2.0f*(float)PI)))*0.7f+
			((float)sin(x*7.f+((float)2.0f*(float)PI)))*0.6f+
			((float)sin(x*8.f+((float)2.0f*(float)PI)))*0.5f
			) / 8.0f;

		table[9][i]=(float) (
			((float)sin(x+((float)2.0f*(float)PI))) +
			((float)sin(x*3.f+((float)2.0f*(float)PI)))+
			((float)sin(x*5.f+((float)2.0f*(float)PI)))+
			((float)sin(x*7.f+((float)2.0f*(float)PI)))*0.9f+
			((float)sin(x*9.f+((float)2.0f*(float)PI)))*0.8f+
			((float)sin(x*11.0f+((float)2.0f*(float)PI)))*0.7f+
			((float)sin(x*13.f+((float)2.0f*(float)PI)))*0.6f+
			((float)sin(x*15.f+((float)2.0f*(float)PI)))*0.5f
			) / 8.0f;

		table[10][i]=(float)(sin((double)i/(double)_TABLE_SIZE+(sin((double)i*4))/2))*0.5;
		table[11][i]=(float)(sin((double)i/(double)_TABLE_SIZE*(sin((double)i*6)/4)))*2.;
		table[12][i]=(float)(sin((double)i*(sin((double)i*1.3)/50)));
		table[13][i]=(float)(sin((double)i*(sin((double)i*1.3)/5)));
		table[14][i]=(float)sin((double)i*0.5*(cos((double)i*4)/50));
		table[15][i]=(float)sin((double)i*0.5+(sin((double)i*14)/2));
		table[16][i]=(float)cos((double)i*2*(sin((double)i*34)/400));
		table[17][i]=(float)cos((double)i*4*((double)table[7][i]/150));
		//printf("%f ",table[17][i]);

	}

	table[5][0] = -0.9f;
	table[5][1] = 0.9f;

	table[6][0] = -0.2f;
	table[6][1] = -0.6f;
	table[6][2] = -0.9f;
	table[6][3] = -0.6f;
	table[6][4] = -0.2f;
	table[6][5] = 0.2f;
	table[6][6] = 0.6f;
	table[6][7] = 0.9f;
	table[6][8] = 0.6f;
	table[6][9] = 0.2f;
	/*
		float pi = 3.145f;
		float oscfreq = 1000.0; //%Oscillator frequency in Hz
		c1 = 2 * cos(2 * pi * oscfreq / Fs);
		//Initialize the unit delays
		d1 = sin(2 * pi * oscfreq / Fs);  
		d2 = 0;*/

	// miditable for notes to frequency
	for (i = 0;i<128;++i){
		midi2freq0[i] = 8.1758f * pow(2,(i/12.f));
		midi2freq[i] = midi2freq0[i]; // No detune yet
	}
	// f_offset=midi2freq0[0];
	// f_scale=1/(midi2freq0[127]-f_offset);
	// printf("init: f offset %f scale %f max %fHz\n", f_offset, f_scale, midi2freq0[127]);

} // end of initialization

/**
 * \brief handle control change MIDI messages
 * 
 * @param voice voice number (not channel)
 * @param n controller number
 * @param v controller value
 */
void doControlChange(int voice, int n, int v){
	// Implement with a controller lookup table would be faster??
	if  (voice <_MULTITEMP)
	{
		switch(n){ // Controller number
			// MIDI Controller  0 = Bank Select MSB (Most Significant Byte) - ignored
			case 1:{ // Modulation wheel
				// modulator[voice][16]=v*_SCALE_7BIT;
				modwheel[voice]=v*_SCALE_7BIT;
				// printf("Modulation wheel %f\n", modulator[voice][16]);
				break;
			}
			case 32:{ // Bank Select LSB (Least Significant Byte)
				bank[voice]=v;
				break;
			}
			case 120:{ // All sound off
				egOff(voice); // Instant mute
				doReset(voice);
				// Fall through all note off
			}
			case 123:{ // All note off
				doNoteOff(voice, lastnote[voice], 0);
				break;
			}
			case 64:{ // Hold
				modulator[voice][28]=v*_SCALE_7BIT;
				if(v>63){ // Hold on
					hold[voice]=1;
				}else{ // Hold off
					hold[voice]=0;
					if (heldnote[voice]>=0){
					// note is held if note_off occurs while hold is on
						doNoteOff(voice, heldnote[voice], 0); // Do a note_off
						heldnote[voice]=-1;
					}
				}
				break;
			}
			case 66:{ // sostenuto
				if(v>63){ // sostenuto on
					sostenuto[voice]=keydown[voice]; // main difference with hold
					// sostenutonote[voice]=lastnote[voice];
				}else{ // sostenuto off
					sostenuto[voice]=0;
					if (sostenutonote[voice]>=0){
					// note is in sustenuto if note_off occurs while sostenuto is on
						if(keydown[voice]==0)
							doNoteOff(voice, sostenutonote[voice], 0); // Do a note_off
						sostenutonote[voice]=-1;
					}
				}
				break;
			}
			case 67:{ // una corda (soft pedal)
				unacorda[voice]=(v>63); // Could be a modulator ??
				break;
			}
			default:{
				// 9 custom controllers
				// Should renumber modulators here and in editor ??
				if(n == cc[0])
					modulator[voice][19]=v*_SCALE_7BIT;
				else if(n == cc[1])
					modulator[voice][20]=v*_SCALE_7BIT;
				else if(n == cc[2])
					modulator[voice][21]=v*_SCALE_7BIT;
				else if(n == cc[3])
					modulator[voice][22]=v*_SCALE_7BIT;
				else if(n == cc[4])
					modulator[voice][23]=v*_SCALE_7BIT;
				else if(n == cc[5])
					modulator[voice][24]=v*_SCALE_7BIT;
				else if(n == cc[6])
					modulator[voice][25]=v*_SCALE_7BIT;
				else if(n == cc[7])
					modulator[voice][26]=v*_SCALE_7BIT;
				else if(n == cc[8])
					modulator[voice][27]=v*_SCALE_7BIT;
				// else just ignore controller message
			}
		}
	}
}

/**
 * @brief handle all 3-byte MIDI messages
 * 
 * @param status midi status byte
 * @param n midi 2nd byte (note or controller number)
 * @param v midi 3rd byte (note velocity or controller value)
 * 
 */
void doMidi(int status, int n, int v){
	int c = status & 0x0F;
	int voice;
#ifdef _DEBUG      
	fprintf(stderr,"doMidi: %u %u %u\n", status, n, v);
#endif
	for(voice=0; voice<_MULTITEMP; voice++){
		if(channel[voice]==c && !polySlave[voice]){
			switch(status & 0xF0){
				case 0x90:{ // Note on
					doNoteOn(voice, n, v);
					break;
				}
				case 0x80:{ // Note off
					doNoteOff(voice, n, v);
					break;
				}
				case 0xC0:{ // Program change ?? TODO apply to slaves... or not
#ifdef _DEBUG
					printf("doMidi: Program change channel %u voice %u program %u\n", c, voice, n);
#endif
					lo_send(t, "/Minicomputer/programChange", "ii", voice, n+(bank[voice]<<7));
					break;
				}
				case 0xD0: // Channel pressure
				{
					modulator[voice][15]=(float)n*0.007874f; // /127.f
					while(polyMaster[voice]) // last voice is never master, voice+1 exists
						modulator[++voice][15]=(float)n*0.007874f; // /127.f
					break;
				}
				case 0xE0:{ // Pitch bend
					// 14 bits means 16384 values; 1/8192=0.00012207f
					// modulator[voice][2]=((v<<7)+n)*0.00012207f; // 0..2
					modulator[voice][2]=(((v<<7)+n)-8192)*0.00012207f; // -1..1
					while(polyMaster[voice]) // last voice is never master, voice+1 exists
						modulator[++voice][2]=(((v<<7)+n)-8192)*0.00012207f; // -1..1
#ifdef _DEBUG
					printf("Pitch bend %u -> %u %f\n", c, voice, (((v<<7)+n)-8192)*0.0001221f);
#endif
					break;
				}
				case 0xB0:{ // Control change
					doControlChange(voice, n, v);
					while(polyMaster[voice]) // last voice is never master, voice+1 exists
						doControlChange(++voice, n, v);
					break;
				}
			}
		}
	}
}

/**
 * \brief handle note off MIDI messages
 * 
 * @param voice the voice number (not the midi channel!)
 * @param note the midi note number, 0..127
 * @param velocity the midi velocity, 0..127
 */
static inline void doNoteOff(int voice, int note, int velocity){
	// Velocity currently ignored
	if (voice <_MULTITEMP){ // Should always be true
//		kbnotes[voice][note]=0;
		note+=transpose[voice];
		while(note!=lastnote[voice] && polyMaster[voice]) voice++;
		if(note==lastnote[voice]){ // Ignore release for old notes
			keydown[voice]=0;
			if(hold[voice])
				heldnote[voice]=note;
			if(sostenuto[voice])
				sostenutonote[voice]=note;
			if(hold[voice]==0 && sostenuto[voice]==0){
				// lastnote[voice]=-1;
				egStop(voice,0);  
				if (EGrepeat[voice][1] == 0) egStop(voice,1);  
				if (EGrepeat[voice][2] == 0) egStop(voice,2); 
				if (EGrepeat[voice][3] == 0) egStop(voice,3); 
				if (EGrepeat[voice][4] == 0) egStop(voice,4);  
				if (EGrepeat[voice][5] == 0) egStop(voice,5);  
				if (EGrepeat[voice][6] == 0) egStop(voice,6);                                
			}
		}
	}
}

/**
 * \brief handle note off MIDI messages
 * @param voice the voice number (not the midi channel!)
 * @param note the midi note number, 0..127
 * @param velocity the midi velocity, 0..127
 */
static inline void doNoteOn(int voice, int note, int velocity){
	if (voice <_MULTITEMP){ // Should always be true
		if ((note>=note_min[voice]) && (note<=note_max[voice])){
//			kbnotes[voice][note]=1;
			note+=transpose[voice];
			if(note>=0 && note<=127){
				if (velocity>0){
					while(lastnote[voice]!=note // Skip voice used by an other note
					&& EGstate[voice][0]!=4  // unless note is finished
					&& polyMaster[voice]){ // and there is a slave
						voice++;
					}
					heldnote[voice]=-1; // Note is played, not held (may be held again later)
					// sostenutonote[voice]=-1; // Note stays in sostenuto!!
					unacordacoeff[voice]=unacorda[voice]?parameter[voice][156]:1.0;
					lastnote[voice]=note;
					keydown[voice]=1;
					glide[voice]+=midi2freq[note]-midif[voice]; // Span between previous and new note
					if(EGstate[voice][0]==4){
						glide[voice]=0; // Don't glide from finished note
					}
					// Reset phase if needed
					if(parameter[voice][120]){
						phase[voice][1]=parameter[voice][119]*_SAMPLES_PER_DEGREE;
						phase[voice][0]=parameter[voice][119]*_SAMPLES_PER_DEGREE*0.5; // sub-osc
	#ifdef _DEBUG
						printf("Osc 1 voice %u phase set to %f\n", voice, parameter[voice][119]);
	#endif
					}
					if(parameter[voice][135]){
						phase[voice][2]=parameter[voice][134]*_SAMPLES_PER_DEGREE;
	#ifdef _DEBUG
						printf("Osc 2 voice %u phase set to %f\n", voice, parameter[voice][134]);
	#endif
					}
					if(parameter[voice][154]){
						phase[voice][3]=parameter[voice][153]*_SAMPLES_PER_DEGREE;
	#ifdef _DEBUG
						printf("Osc 3 voice %u phase set to %f\n", voice, parameter[voice][153]);
	#endif
					}

					midif[voice]=midi2freq[note];// lookup the frequency
					// 1/127=0,007874015748...
					modulator[voice][17]=note*0.007874f;// fill the value in as normalized modulator
					modulator[voice][1]=(float)1.0f-(velocity*0.007874f);// fill in the velocity as modulator
					// why is velocity inverted??
					// Parameter 139 is legato, don't retrigger unless released (0) or idle (4)
					if(EGstate[voice][0]==4 || EGstate[voice][0]==0 || parameter[voice][139]==0){
						egStart(voice, 0);// start the engines!
						// Maybe optionally restart repeatable envelopes too, i.e free-run button?
						if (EGrepeat[voice][1] == 0) egStart(voice, 1);
						if (EGrepeat[voice][2] == 0) egStart(voice, 2);
						if (EGrepeat[voice][3] == 0) egStart(voice, 3);
						if (EGrepeat[voice][4] == 0) egStart(voice, 4);
						if (EGrepeat[voice][5] == 0) egStart(voice, 5);
						if (EGrepeat[voice][6] == 0) egStart(voice, 6);
	#ifdef _DEBUG
					}else{
						printf("Legato\n");
	#endif
					}
	#ifdef _DEBUG
						printf("Note on %u voice %u velocity %u\n", note, voice, velocity);
	#endif
				}else{ // if velo == 0 it should be handled as noteoff...
					doNoteOff(voice, note-transpose[voice], velocity);
				}
			}
		}
	}
}

/**
 * \brief clears the filter data and the delay line
 * 
 * @param voice the voice number (not the midi channel!)
 */
void doReset(unsigned int voice){
	if (voice <_MULTITEMP){
		low[voice][0] = 0.f;
		high[voice][0] = 0.f;
		band[voice][0] = 0.f;
		low[voice][1] = 0.f;
		high[voice][1] = 0.f;
		band[voice][1] = 0.f;
		low[voice][2] = 0.f;
		high[voice][2] = 0.f;
		band[voice][2] = 0.f;

		phase[voice][1] = 0.f;
		phase[voice][2] = 0.f;
		phase[voice][3] = 0.f;

		memset(delayBuffer[voice],0,sizeof(delayBuffer[voice]));
	}
	firstTime=1; // Helps with debugging
#ifdef _DEBUG
	printf("doReset: voice %u filters and delay buffer reset\n", voice);
#endif
}

/**
 * @brief an extra thread for handling the alsa midi messages
 *
 * @param handle of alsa midi
 */
static void *alsaMidiProcess(void *handle) {
	struct sched_param param;
	int policy;
	snd_seq_t *seq_handle = (snd_seq_t *)handle;
	pthread_getschedparam(pthread_self(), &policy, &param);

	policy = SCHED_FIFO;
	param.sched_priority = 95;

	pthread_setschedparam(pthread_self(), policy, &param);

	/*
		if (poll(pfd, npfd, 100000) > 0) 
		{
		  midi_action(seq_handle);
		} */
		
	snd_seq_event_t *ev;
 	#ifdef _DEBUG
	printf("start\n");
 	fflush(stdout);
	#endif
	unsigned int c = _MULTITEMP; // channel of incoming data
	int voice;
	#ifdef _MIDIBLOCK
	do {
	#else
	   while (quit==0)
	   {
	#endif
	   while ((snd_seq_event_input(seq_handle, &ev)) && (quit==0))
	   {
		if (ev != NULL)
		{
		if (ev->type != 36) // discard SND_SEQ_EVENT_CLOCK
		switch (ev->type) 
		{	// first check the controllers
			// they usually come in hordes
			case SND_SEQ_EVENT_CONTROLLER:
			{
				c = ev->data.control.channel;
			#ifdef _DEBUG      
				fprintf(stderr, "Control event on Channel %2d: %2d %5d       \r",
				c,  ev->data.control.param, ev->data.control.value);
			#endif
				for(voice=0; voice<_MULTITEMP; voice++){
					if(channel[voice]==c && !polySlave[voice])
						doControlChange(voice, ev->data.control.param, ev->data.control.value);
						while(polyMaster[voice]) // last voice is never master, voice+1 exists
							doControlChange(++voice, ev->data.control.param, ev->data.control.value);
				}
				break;
			}
			case SND_SEQ_EVENT_PITCHBEND:
			{
				c = ev->data.control.channel;
			#ifdef _DEBUG      
				fprintf(stderr,"Pitchbender event on Channel %2d: %5d   \r", 
				c, ev->data.control.value);
			#endif
				for(voice=0; voice<_MULTITEMP; voice++){
					if(channel[voice]==c && !polySlave[voice]){
						modulator[voice][2]=ev->data.control.value*0.0001221f; // /8192.f; 0..2 ?? needs bias
						while(polyMaster[voice]) // last voice is never master, voice+1 exists
							modulator[++voice][2]=ev->data.control.value*0.0001221f; // /8192.f; 0..2 ?? needs bias
					}
				}
			break;
			}   
			case SND_SEQ_EVENT_CHANPRESS:
			{
				c = ev->data.control.channel;
				#ifdef _DEBUG      
				fprintf(stderr,"touch event on Channel %2d: %5d   \r", 
				c, ev->data.control.value);
				#endif
				for(voice=0; voice<_MULTITEMP; voice++){
					if(channel[voice]==c)
						modulator[voice][15]=(float)ev->data.control.value*0.007874f;
				}
			break;
			}

			case SND_SEQ_EVENT_NOTEON:
			{   
				c = ev->data.note.channel;
#ifdef _DEBUG      
				printf("Note On event on Channel %2d: %5d       \r",
				c, ev->data.note.note);
				printf("Note On event %u \r", EGstate[c][0]);
#endif
				for(voice=0; voice<_MULTITEMP; voice++){
					if(channel[voice]==c && !polySlave[voice]){
#ifdef _DEBUG      
						printf("alsaMidiProcess: note on voice %u channel %u %u..%u : %u %u\n",
							voice, channel[voice], note_min[voice], note_max[voice], c, ev->data.note.note);
#endif
						doNoteOn(voice, ev->data.note.note, ev->data.note.velocity);
						// if(poly[voice]) voice++; // Don't stack voices if poly set
					}
				}
				break;
			}      
			case SND_SEQ_EVENT_NOTEOFF: 
			{
				for(voice=0; voice<_MULTITEMP; voice++){
					if(channel[voice]==ev->data.note.channel && !polySlave[voice])
						doNoteOff(voice, ev->data.note.note, ev->data.note.velocity);
				}
				break;       
			}
			case SND_SEQ_EVENT_PGMCHANGE:{
				int n=ev->data.control.value;
#ifdef _DEBUG
				printf("alsaMidiProcess: program change channel %u program %u\n", c, n);
#endif
				for(voice=0; voice<_MULTITEMP; voice++){
					if(channel[voice]==c && !polySlave[voice]){
#ifdef _DEBUG
						printf("alsaMidiProcess: program change channel %u voice %u program %u\n", c, voice, n);
#endif
						lo_send(t, "/Minicomputer/programChange", "ii", voice, n+(bank[voice]<<7));
					}
				}
				break;
			}


#ifdef _DEBUG      
			default:
			{
				fprintf(stderr,"unknown event %d on Channel %2d: %5d   \r",ev->type, 
				ev->data.control.channel, ev->data.control.value);
			}
#endif
		} // end of switch
	snd_seq_free_event(ev);
	usleep(10);// absolutely necessary, otherwise stream of mididata would block the whole computer, sleep for 10 microseconds
	} // end of if
#ifdef _MIDIBLOCK
	usleep(1000);// absolutely necessary, otherwise stream of mididata would block the whole computer, sleep for 1ms == 1000 microseconds
   }
 } while ((quit==0) && (done==0));// doing it as long we are running was  (snd_seq_event_input_pending(seq_handle, 0) > 0);
#else
	usleep(100);// absolutely necessary, otherwise stream of mididata would block the whole computer, sleep for 100 microseconds
	}// end of first while, emptying the seqdata queue
	usleep(2000);// absolutly necessary, otherwise this thread would block the whole computer, sleep for 2ms == 2000 microseconds
} // end of while(quit==0)
#endif
 printf("midi thread stopped\n");
 fflush(stdout);
return 0;// its insisited on this although it should be a void function
}// end of alsaMidiProcess

/** @brief display command-line parameters summary
 */
void usage(){
	printf(
		"Usage:\n"
		"	-port nnnn		sets the base OSC port\n"
		"	-port2 nnnn		sets the secondary OSC port (default to base+1)\n"
		"	-no-connect		don't attempt to autoconnect to jack audio\n"
		"	-h or --help		show this message\n"
	);
}

/** @brief the classic c main function
 *
 * @param argc argument count
 * @param argv pointer to array of the arguments
 * @return int should be 0 if the program terminates nicely
 */
int main(int argc, char **argv) {
printf("minicomputer core version %s\n",_VERSION);
// ------------------------ decide the oscport number -------------------------
char OscPort[] = _OSCPORT; // default value for OSC port
char *oscPort1 = OscPort; // pointer to the OSC port string
char *oscPort2;
int has_port2=0;
int do_connect=1;

int i;
// process the arguments
  	for (i = 1; i<argc; ++i)
	{
		if (strcmp(argv[i],"-port")==0){ // got a OSC port argument
			++i;// looking for the next entry
			if (i<argc) {
				int tport = atoi(argv[i]);
				if (tport > 0){
					oscPort1 = argv[i]; // overwrite the default for the OSCPort
				}else{
					fprintf(stderr, "Invalid port %s\n", argv[i]);
					usage();
					exit(1);
				}
			}else{
				fprintf(stderr, "Invalid port - end of command line reached\n");
				usage();
				exit(1);
			}
		}else if (strcmp(argv[i],"-port2")==0){ // got a OSC port argument
			++i;// looking for the next entry
			if (i<argc){
				int tport2 = atoi(argv[i]);
				if (tport2 > 0){
					oscPort2 = argv[i]; // overwrite the default for the OSCPort
					has_port2 = 1;
				}else{
					fprintf(stderr, "Invalid port %s\n", argv[i]);
					usage();
					exit(1);
				}
			}else{
				fprintf(stderr, "Invalid port - end of command line reached\n");
				usage();
				exit(1);
			}
		}else if (strcmp(argv[i],"-no-connect")==0){
			do_connect=0;
		}else if (strcmp(argv[i],"--help")==0 || strcmp(argv[i],"-h")==0 ){
			usage();
			exit(0);
		}else{
			fprintf(stderr, "Unrecognized option %s\n", argv[i]);
			usage();
			exit(1);
		}
	}

	printf("Server OSC input port: %s\n", oscPort1);
	sprintf(jackName, "Minicomputer%s", oscPort1);// store globally a unique name

// ------------------------ ALSA midi init ---------------------------------
	pthread_t midithread;
	seq_handle = open_seq();
 
	npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
	pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
	snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);
	
	// create the thread and tell it to use alsaMidiProcess as thread function
	int err = pthread_create(&midithread, NULL, alsaMidiProcess,seq_handle);
	// printf("start err %i\n", err);
 

// ------------------------ OSC Init ------------------------------------   
	/* start a new server on port defined where oscPort1 points to */
	lo_server_thread st = lo_server_thread_new(oscPort1, osc_error_handler);

	/* add method that will match /Minicomputer/choice with three integers */
	lo_server_thread_add_method(st, "/Minicomputer/choice", "iii", osc_choice_handler, NULL);

	/* add method that will match the path /Minicomputer, with three numbers, int (voicenumber), int (parameter) and float (value) 
	 */
	lo_server_thread_add_method(st, "/Minicomputer", "iif", osc_param_handler, NULL);

	/* add method that will match the path Minicomputer/quit with one integer */
	lo_server_thread_add_method(st, "/Minicomputer/quit", "i", osc_quit_handler, NULL);
	
	lo_server_thread_add_method(st, "/Minicomputer/midi", "iiii", osc_midi_handler, NULL); // DSSI-like
	lo_server_thread_add_method(st, "/Minicomputer/audition", "iiii", osc_audition_handler, NULL); // On/off voice note velocity
	lo_server_thread_add_method(st, "/Minicomputer/multiparm", "if", osc_multiparm_handler, NULL);

	lo_server_thread_start(st);

	// init for output
	if(has_port2==0){
		oscPort2=(char *)malloc(80);
		snprintf(oscPort2, 80, "%d", atoi(oscPort1)+1);
	}
	printf("Server OSC output port: %s\n", oscPort2);
	t = lo_address_new(NULL, oscPort2);

	/* setup our signal handler signalled() above, so 
	 * we can exit cleanly (see end of main()) */
	signal(SIGINT, signalled);

	init();

// ------------------------ JACK Init ------------------------------------   
	/* naturally we need to become a jack client
	 * prefered with a unique name, so lets add the OSC port to it*/
	// client = jack_client_new(jackName);
	client = jack_client_open(jackName, 0, NULL);
	if (!client) {
		fprintf(stderr, "couldn't connect to jack server. Either it's not running or the client name is already taken\n");
		exit(1);
	}

	/* we register the output ports and tell jack these are 
	 * terminal ports which means we don't
	 * have any input ports from which we could somehow 
	 * feed our output */
	port[0] = jack_port_register(client, "output1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	port[1] = jack_port_register(client, "output2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	port[2] = jack_port_register(client, "output3", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	port[3] = jack_port_register(client, "output4", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	port[4] = jack_port_register(client, "output5", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	port[5] = jack_port_register(client, "output6", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	port[6] = jack_port_register(client, "output7", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	port[7] = jack_port_register(client, "output8", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	
	port[10] = jack_port_register(client, "aux out 1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	port[11] = jack_port_register(client, "aux out 2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	
	// would like to create mix ports last because qjackctrl tend to connect automatic the last ports
	port[8] = jack_port_register(client, "mix out left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	port[9] = jack_port_register(client, "mix out right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);

	jackMidiPort = jack_port_register (client, "Midi/in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	if (!jackMidiPort)
	{
		fprintf (stderr, "Error: can't create the 'Midi/in' jack port\n");
		exit (1);
	}
	// inbuf = jack_port_register(client, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	/* jack is callback based. That means we register 
	 * a callback function (see process() above)
	 * which will then get called by jack once per process cycle
	 */
	jack_set_process_callback(client, jackProcess, 0);
	bufsize = jack_get_buffer_size(client);

	// handling the sampling frequency
	sampleRate = (float) jack_get_sample_rate (client); 
	tabX = _TABLE_SIZE_FLOAT / sampleRate;
	srate = 3.145f/ sampleRate;
	srDivisor = 1.0f / sampleRate * 100000.f;
    // printf("srDivisor: %f\n", srDivisor);
	glide_a = 48000.0f/sampleRate;
	glide_b = 1.0f-glide_a;
	// depending on it the delaybuffer
	maxDelayTime = (int)sampleRate;
	delayBufferSize = maxDelayTime*2;
	// generate the delaybuffers for each voice
	int k;
	for (k=0; k<_MULTITEMP;++k)
	{
		//float dbuffer[delayBufferSize];
		//delayBuffer[k]=dbuffer;
		delayBuffer[k]=malloc(delayBufferSize*sizeof(float));
		delayI[k]=0;
		delayJ[k]=0;
		// Sub-oscillator initial state
		sub[k]=-1.0f;
		subMSB[k]=0;
	}
	#ifdef _DEBUG
	printf("bsize:%d %d\n",delayBufferSize,maxDelayTime);
	#endif
	/* tell jack that we are ready to do our thing */
	jack_activate(client);

	if(do_connect){
		snprintf(jackPortName, 128, "%s:mix out left", jackName);
		int result=jack_connect(client, jackPortName, "system:playback_1");
		if (result) printf("jack_connect \"%s\": error %i\n", jackPortName, result);
		snprintf(jackPortName, 128, "%s:mix out right", jackName);
		result=jack_connect(client, jackPortName, "system:playback_2");
		if (result) printf("jack_connect \"%s\": error %i\n", jackPortName, result);
	}
	
	/* wait until this app receives a SIGINT (i.e. press 
	 * ctrl-c in the terminal) see signalled() above */
	while (quit==0) 
	{
	// operate midi
		 /* let's not waste cycles by busy waiting */
		sleep(1);
		//printf("quit:%i %i\n",quit,done);
	
	}
	/* so we shall quit, eh? ok, cleanup time. otherwise 
	 * jack would probably produce an xrun
	 * on shutdown */
	jack_deactivate(client);

	/* shutdown cont. */
	jack_client_close(client);
#ifndef _MIDIBLOCK
	printf("wait for midithread\n");
	fflush(stdout);
	/* waiting for the midi thread to shutdown carefully */
	pthread_join(midithread,NULL);
#endif	
	/* release Alsa Midi connection */
	snd_seq_close(seq_handle);

	/* done !! */
	printf("close minicomputer\n");	
	fflush(stdout);
	return 0;
}

// ******************************************** OSC handling ***********************
// / / ! \ n a m e OSC routines
// / / / @ {
// Can't get the above to work

/** @brief OSC error handler 
 *
 * @param num errornumber
 * @param msg errormessage
 * @param path where it occured
 */
static void osc_error_handler(int num, const char *msg, const char *path)
{
	printf("liblo server error %d in path %s: %s\n", num, path, msg);
	fflush(stdout);
}

/**
 * @brief catch any incoming osc "choice" message
 * 
 * Expects to 3 integer arguments: choice id, voice, value
 * Returning 1 means that the message has not been fully handled
 * and the server should try other methods 
 *
 * @param path osc path
 * @param types osc data types string
 * @param argv pointer to array of arguments 
 * @param argc arguments count
 * @param data
 * @param user_data
 * @return int 0 if everything is ok, 1 means message is not fully handled
 * */
static int osc_choice_handler(const char *path, const char *types, lo_arg **argv,
			int argc, void *data, void *user_data)
{
	if ( (argv[0]->i < _MULTITEMP) && (argv[1]->i < _CHOICECOUNT) )
  	{
		// printf("osc_choice_handler %u %u %u\n", argv[0]->i, argv[1]->i, argv[2]->i);
		choice[argv[0]->i][argv[1]->i]=argv[2]->i;
		return 0;
	}
	else
	{
		fprintf(stderr, "WARNING: unhandled osc_choice_handler %u %u %u\n", argv[0]->i, argv[1]->i, argv[2]->i);
		return 1;
	}
}

float multiparm[_MULTIPARMS];

/** @brief catch any incoming osc "multiparm" message.
 * 
 * Expects an integer argument for the parameter id and a float for the value
 * Returning 1 means that the message has not been fully handled
 * and the server should try other methods 
 *
 * @param path osc path
 * @param types osc data types string
 * @param argv pointer to array of arguments 
 * @param argc arguments count
 * @param data
 * @param user_data
 * @return int 0 if everything is ok, 1 means message is not fully handled
 * */
static int osc_multiparm_handler(const char *path, const char *types, lo_arg **argv,
			int argc, void *data, void *user_data)
{
	int parmnum=argv[0]->i;
	float parmval=argv[1]->f;
	int note, voice;
	if(parmnum<_MULTIPARMS) multiparm[parmnum]=parmval;
	if(parmnum<12){ // 0..11 is individual detune
		// middle C note is represented by the value of 60
// printf("osc_multiparm_handler: note %i detune %f --> %f\n", parmnum, parmval, 1.0f+ (pow(2.0f, 1.0f/12.0f)-1.0f)*parmval);
		for(note=parmnum; note<128; note+=12){
			midi2freq[note] = midi2freq0[note]
				* (1.0f+ (pow(2.0f, 1.0f/12.0f)-1.0f)*parmval )
				* (1.0f+ (pow(2.0f, 1.0f/12.0f)-1.0f)*multiparm[12] ); // 2^(1/12)=1,05946309436
		}
		// Fix currently playing note if needed so detune is immediately effective
		for(voice=0; voice<_MULTITEMP; voice++){
			if (lastnote[voice]%12 == parmnum)
			// if (lastnote[voice]>=0 && lastnote[voice]%12 == parmnum)
				midif[voice]=midi2freq[lastnote[voice]];
		}
	}else if (parmnum==12){ // Master tune
// printf("osc_multiparm_handler: master tune %f --> %f\n", parmval, 1.0f+ (pow(2.0f, 1.0f/12.0f)-1.0f)*parmval);
		for(note=0; note<128; note++){
			midi2freq[note] = midi2freq0[note]
				* (1.0f+ (pow(2.0f, 1.0f/12.0f)-1.0f)*multiparm[note%12] )
				* (1.0f+ (pow(2.0f, 1.0f/12.0f)-1.0f)*parmval ); // 2^(1/12)=1,05946309436
		}
		// Fix all currently playing notes so detune is immediately effective
		for(voice=0; voice<_MULTITEMP; voice++){
			// if(lastnote[voice]>=0)
				midif[voice]=midi2freq[lastnote[voice]];
		}
	}else if (parmnum<22){ // 13..21 Custom midi control codes
		cc[parmnum-13]=(int)parmval;
	}else{
		fprintf(stderr, "WARNING: osc_multiparm_handler - unhandled parameter number %i\n", parmnum);
	}
}

/** @brief catch any incoming osc "audition" message.
 * 
 * Expects 4 integers: type (note on/off), voice, note, velocity
 * Returning 1 means that the message has not been fully handled
 * and the server should try other methods 
 *
 * @param path osc path
 * @param types osc data types string
 * @param argv pointer to array of arguments 
 * @param argc arguments count
 * @param data
 * @param user_data
 * @return int 0 if everything is ok, 1 means message is not fully handled
 * */
static int osc_audition_handler(const char *path, const char *types, lo_arg **argv,
			int argc, void *data, void *user_data)
{
	// printf("osc_audition_handler: %d %d %d %d\n", argv[0]->i, argv[1]->i, argv[2]->i, argv[3]->i);
	if(argv[0]->i){ // Note on
		doNoteOn(argv[1]->i, argv[2]->i, argv[3]->i);
	}else{ // Note off
		doNoteOff(argv[1]->i, argv[2]->i, argv[3]->i);
	}
}

/** @brief catch any incoming osc "midi" message.
 * 
 * Expects up to 4 integers, unused leading ints are set to 0
 * Returning 1 means that the message has not been fully handled
 * and the server should try other methods 
 *
 * @param path osc path
 * @param types osc data types string
 * @param argv pointer to array of arguments 
 * @param argc arguments count
 * @param data
 * @param user_data
 * @return int 0 if everything is ok, 1 means message is not fully handled
 * */
static int osc_midi_handler(const char *path, const char *types, lo_arg **argv,
			int argc, void *data, void *user_data)
{
	int i, c;
	// first byte argv[0] is 0 for 3-byte messages such as note on/off
#ifdef _DEBUG
	printf("osc_midi_handler %u %u %u %u \n", argv[0]->i, argv[1]->i, argv[2]->i, argv[3]->i);
#endif
	if (argv[0]->i == 0){ // At most 3 bytes follow
		if(argv[1]->i == 0){ // At most 2 bytes follow
			if(argv[2]->i == 0){ // Single byte message follows
				if(argv[3]->i == 0xFE){ // Active sense
					// Do this even if UPDATE_GUI is not set !!
					lo_send(t, "/Minicomputer/sense", "");
				}
			}
		}else{ // 3 MIDI bytes follow
			c=(argv[1]->i)&0x0F;
			// Proper channel handling requires the following loop
			// Could be moved inside the cases for speed?
			int voice;
			for(voice=0; voice<_MULTITEMP; voice++){
				if(channel[voice]==c && !polySlave[voice]){
					switch ((argv[1]->i)&0xF0){
						case 0x90:{ // Note on
							doNoteOn(voice, argv[2]->i, argv[3]->i);
		#ifdef _DEBUG
							printf("doNoteOn %u %u %u \n", argv[1]->i&0x0F, argv[2]->i, argv[3]->i);
		#endif
							break;
						}
						case 0x80:{ // Note off
							doNoteOn(voice, argv[2]->i, 0);
		#ifdef _DEBUG
							printf("doNoteOff %u %u \n", argv[1]->i&0x0F, argv[2]->i);
		#endif
							break;
						}
						case 0xB0:{ // Control change
							// TODO apply to slaves if any
							switch(argv[2]->i){ // Controller number
							  case 120:{ // All sound off
								egOff(voice);
								doReset(voice);
								// Fall through all note off
							  }
							  case 123:{ // All note off
								doNoteOn(voice, lastnote[voice], 0);
								break;
							  }
							  // What about the other controllers?? TODO
							}
							while(polyMaster[voice]){ // last voice is never master, voice+1 exists
								voice++;
								switch(argv[2]->i){ // Controller number
								  case 120:{ // All sound off
									egOff(voice);
									doReset(voice);
									// Fall through all note off
								  }
								  case 123:{ // All note off
									doNoteOn(voice, lastnote[voice], 0);
									break;
								  }
								  // What about the other controllers?? TODO
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

/** @brief specific message handler for float parameter messages
 *
 * expects arguments voice number, parameter number, parameter value
 *
 * @param path osc path
 * @param types osc data types string
 * @param argv pointer to array of arguments 
 * @param argc amount of arguments
 * @param data
 * @param user_data
 * @return int 0 if everything is ok, 1 means message is not fully handled
 */
static int osc_param_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	/* pull the argument values out of the argv array */
	int voice =  argv[0]->i;
	int i =  argv[1]->i;
	if ((voice<_MULTITEMP) && (i>0) && (i<_PARACOUNT)) 
	{
		parameter[voice][i]=argv[2]->f;
#ifdef _DEBUG
		printf("osc_param_handler set voice %i, parameter %i = %f \n", voice, i, argv[2]->f);
#endif   

	}

	switch (i)
	{
	 // reset the filters and delay buffer
	 case 0:{
		doReset(voice);
		break;
	 }
	 
	 case 60:EG[voice][1][1]=argv[2]->f;break;
	 case 61:EG[voice][1][2]=argv[2]->f;break;
	 case 62:EG[voice][1][3]=argv[2]->f;break;
	 case 63:EG[voice][1][4]=argv[2]->f;break;
	 case 64:
	 {
	 	EGrepeat[voice][1] = (argv[2]->f>0) ? 1:0;
	 	if (EGrepeat[voice][1] > 0 ) egStart(voice,1);
	 	break;
	 }
	 case 65:EG[voice][2][1]=argv[2]->f;break;
	 case 66:EG[voice][2][2]=argv[2]->f;break;
	 case 67:EG[voice][2][3]=argv[2]->f;break;
	 case 68:EG[voice][2][4]=argv[2]->f;break;
	 case 69:
	 {
	 	EGrepeat[voice][2] = (argv[2]->f>0) ? 1:0;
	 	if (EGrepeat[voice][2] > 0 ) egStart(voice,2);
	 	break;
	 }
	 case 70:EG[voice][3][1]=argv[2]->f;break;
	 case 71:EG[voice][3][2]=argv[2]->f;break;
	 case 72:EG[voice][3][3]=argv[2]->f;break;
	 case 73:EG[voice][3][4]=argv[2]->f;break;
	 case 74:
	 {
	 	EGrepeat[voice][3] = (argv[2]->f>0) ? 1:0;
	 	if (EGrepeat[voice][3] > 0 ) egStart(voice,3);
	 	break;
	 }
	 case 75:EG[voice][4][1]=argv[2]->f;break;
	 case 76:EG[voice][4][2]=argv[2]->f;break;
	 case 77:EG[voice][4][3]=argv[2]->f;break;
	 case 78:EG[voice][4][4]=argv[2]->f;break; 
	 case 79:
	 {
	 	EGrepeat[voice][4] = (argv[2]->f>0) ? 1:0;
	 	if (EGrepeat[voice][4] > 0 ) egStart(voice,4);
	 	break;
	 }
	 case 80:EG[voice][5][1]=argv[2]->f;break;
	 case 81:EG[voice][5][2]=argv[2]->f;break;
	 case 82:EG[voice][5][3]=argv[2]->f;break;
	 case 83:EG[voice][5][4]=argv[2]->f;break;
	 case 84:
	 {
	 	EGrepeat[voice][5] = (argv[2]->f>0) ? 1:0;
	 	if (EGrepeat[voice][5] > 0 ) egStart(voice,5);
	 	break;
	 }
	 case 85:EG[voice][6][1]=argv[2]->f;break;
	 case 86:EG[voice][6][2]=argv[2]->f;break;
	 case 87:EG[voice][6][3]=argv[2]->f;break;
	 case 88:EG[voice][6][4]=argv[2]->f;break;
	 case 89:
	 {
		EGrepeat[voice][6] = (argv[2]->f>0) ? 1:0;
		if (EGrepeat[voice][6] > 0 ) egStart(voice,6);
		break;
	 }
	 case 102:EG[voice][0][1]=argv[2]->f;break;
	 case 103:EG[voice][0][2]=argv[2]->f;break;
	 case 104:EG[voice][0][3]=argv[2]->f;break;
	 case 105:EG[voice][0][4]=argv[2]->f;break;

	 // 127 test note and 126 test velocity intentionally ignored
	 case 125:channel[voice]=(argv[2]->f)-1;break;
	 case 128:note_min[voice]=argv[2]->f;break;
	 case 129:note_max[voice]=argv[2]->f;break;
	 case 130:transpose[voice]=argv[2]->f;break;
	 case 155:
	 {
		// printf("link %u = %u", voice, argv[2]->f);
		if(voice<_MULTITEMP-1){
			polyMaster[voice]=argv[2]->f;
			polySlave[voice+1]=argv[2]->f;
		}else{
			fprintf(stderr, "ERROR: voice %u cannot be a poly master!\n", voice);
		}
		break;
	 }
	}
#ifdef _DEBUG
	printf("osc_param_handler %i %i %f \n",voice,i,argv[2]->f);
#endif   
	return 0;
}

/** message handler for osc quit messages
 *
 * Doesn't expect any argument
 * @param path osc path
 * @param types osc data types string
 * @param argv pointer to array of arguments 
 * @param argc amount of arguments
 * @param data
 * @param user_data
 * @return int 0 if everything is ok, 1 means message is not fully handled
 */
static int osc_quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	done = 1;
	quit = 1;
	printf("about to shutdown minicomputer core\n");
	fflush(stdout);
	return 0;
}
// / / / @ }

