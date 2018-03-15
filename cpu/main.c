/** Minicomputer
 * industrial grade digital synthesizer
 *
 * Copyright 2007,2008 Malte Steiner
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
// a way to compile it was:
//  gcc -o synthesizer synth2.c -ljack -ffast-math -O3 -march=k8 -mtune=k8 -funit-at-a-time -fpeel-loops -ftracer -funswitch-loops -llo -lasound

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
// #include <limits.h>
// This works only for c++
// #include <cmath.h>
// double epsilon = std::numeric_limits<float>::min();

// some common definitions
#include "../common.h" 

// defines
#define _MODCOUNT 32
#define _WAVECOUNT 32

// Table size must be a power of 2 so that we can use & instead of %
#define TableSize 4096
#define tabM 4095
#define tabF 4096.f
//#define itabF 4096

//#define _DEBUG

// variables
// Should define a struct for voice (and for EG)
float delayBuffer[_MULTITEMP][96000] __attribute__((aligned (16)));
float parameter[_MULTITEMP][_PARACOUNT] __attribute__((aligned (16)));
float modulator[_MULTITEMP][_MODCOUNT] __attribute__((aligned (16)));
// Bias will be used for AM because we don't want negative amplitude
float modulator_bias[_MODCOUNT] __attribute__((aligned (16)))={
	1.0f, // 00 none
	0.0f, // 01 velocity
	0.0f, // 02 pitch bend
	1.0f, // 03 osc 1 fm out
	1.0f, // 04 osc 2 fm out
	1.0f, // 05 filter
	0.0f, // 06 eg 1
	0.0f, // 07 eg 2
	0.0f, // 08 eg 3
	0.0f, // 09 eg 4
	0.0f, // 10 eg 5
	0.0f, // 11 eg 6
	1.0f, // 12 modulation osc
	0.0f, // 13 touch
	0.0f, // 14 mod wheel
	0.0f, // 15 cc 12
	1.0f, // 16 delay
	0.0f, // 17 midi note
	0.0f, // 18 cc 2
	0.0f, // 19 cc 4
	0.0f, // 20 cc 5
	0.0f, // 21 cc 6
	0.0f, // 22 cc 14
	0.0f, // 23 cc 15
	0.0f, // 24 cc 16
	0.0f, // 25 cc 17
};
float modulator_scale[_MODCOUNT] __attribute__((aligned (16)))={
	1.0f, // 00 none
	1.0f, // 01 velocity
	1.0f, // 02 pitch bend
	0.5f, // 03 osc 1 fm out
	0.5f, // 04 osc 2 fm out
	0.5f, // 05 filter
	1.0f, // 06 eg 1
	1.0f, // 07 eg 2
	1.0f, // 08 eg 3
	1.0f, // 09 eg 4
	1.0f, // 10 eg 5
	1.0f, // 11 eg 6
	0.5f, // 12 modulation osc
	1.0f, // 13 touch
	1.0f, // 14 mod wheel
	1.0f, // 15 cc 12
	0.5f, // 16 delay
	1.0f, // 17 midi note
	1.0f, // 18 cc 2
	1.0f, // 19 cc 4
	1.0f, // 20 cc 5
	1.0f, // 21 cc 6
	1.0f, // 22 cc 14
	1.0f, // 23 cc 15
	1.0f, // 24 cc 16
	1.0f, // 25 cc 17
};
float midif[_MULTITEMP] __attribute__((aligned (16)));
float EG[_MULTITEMP][8][8] __attribute__((aligned (16))); // 7 8
float EGFaktor[_MULTITEMP][8] __attribute__((aligned (16)));
float phase[_MULTITEMP][4] __attribute__((aligned (16)));//=0.f;
unsigned int choice[_MULTITEMP][_CHOICECOUNT] __attribute__((aligned (16)));
int EGrepeat[_MULTITEMP][8] __attribute__((aligned (16)));
unsigned int EGtrigger[_MULTITEMP][8] __attribute__((aligned (16)));
unsigned int EGstate[_MULTITEMP][8] __attribute__((aligned (16)));
float high[_MULTITEMP][4],band[_MULTITEMP][4],low[_MULTITEMP][4],f[_MULTITEMP][4],q[_MULTITEMP][4],v[_MULTITEMP][4],faktor[_MULTITEMP][4];
unsigned int lastnote[_MULTITEMP];
unsigned int hold[_MULTITEMP];
unsigned int heldnote[_MULTITEMP];
float glide[_MULTITEMP];
int delayI[_MULTITEMP], delayJ[_MULTITEMP];
float sub[_MULTITEMP];
int subMSB[_MULTITEMP];
int channel[_MULTITEMP];
// note-based filtering useful for drumkit multis
int note_min[_MULTITEMP];
int note_max[_MULTITEMP];

jack_port_t *port[_MULTITEMP + 4]; // _multitemp * ports + 2 mix and 2 aux
jack_port_t *_jack_midipt;
lo_address t;

float table [_WAVECOUNT][TableSize] __attribute__((aligned (16)));
float midi2freq [128];

char jackName[64]="Minicomputer";// signifier for audio and midiconnections, to be filled with OSC port number
snd_seq_t *open_seq();
snd_seq_t *seq_handle;
int npfd;
struct pollfd *pfd;
/* a flag which will be set by our signal handler when 
 * it's time to exit */
int quit = 0;
jack_port_t* inbuf;
jack_client_t *client;

float temp=0.f,lfo;
float sampleRate=48000.0f; // only default, going to be overriden by the actual, taken from jack
float tabX = tabF / 48000.0f;
float srate = 3.145f/ 48000.f;
float srDivisor = 1.0f / 48000.f*100000.f;
int i,delayBufferSize=0,maxDelayBufferSize=0,maxDelayTime=0;
jack_nframes_t bufsize;
int done = 0;
static const float anti_denormal = 1e-20;// magic number to get rid of denormalizing

// I experiment with optimization
#ifdef _VECTOR  
	typedef float v4sf __attribute__ ((vector_size(16),aligned(16)));//((mode(V4SF))); // vector of four single floats
	union f4vector 
	{
		v4sf v;// __attribute__((aligned (16)));
		float f[4];// __attribute__((aligned (16)));
	};
#endif

//void midi_action(snd_seq_t *seq_handle);

/** \brief function to create Alsa Midiport
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
	if ( snd_seq_get_client_info	( seq_handle,
info) != 0){

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
	if ( snd_seq_set_client_info	( seq_handle,
info) != 0){

	fprintf(stderr, "Error setting info for eventfiltersetup.\n");
	exit(1);
}	
 snd_seq_client_info_free(info);

  return(seq_handle);
}

// some forward declarations
static inline void error(int num, const char *m, const char *path); 
static inline int generic_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data); 
static inline int foo_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data); 
static inline int quit_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);
static inline int midi_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);
static inline void doNoteOn(int voice, int note, int velocity);
static inline void doNoteOff(int voice, int note, int velocity);
static void doMidi(int t, int n, int v);
void doReset(unsigned int voice);

/* inlined manually
static inline float Oscillator(float frequency,int wave,float *phase)
{
	int i = (int) *phase ;// float to int, cost some cycles

	i=i&tabM;//i%=TableSize;
	//if (i>tabM) i=tabM;
	if (i<0) i=tabM;
	*phase += tabX * frequency;

	if(*phase >= tabF)
	{
   		 *phase -= tabF;
		// if (*phase>=tabF) *phase = 0; //just in case of extreme fm
	}


		if(*phase < 0.f)
				{
					*phase += tabF;
			//	if(*phase < 0.f) *phase = tabF-1;
				}
		return table[wave][i] ;
}
*/
//inline float filter (float input,float f, float q)
//{
//
//
//}
/**
 * start the envelope generator
 * called by a note on event for that voice
 * @param the voice number
 * @param the number of envelope generator
 */

static inline void egStart (const unsigned int voice,const unsigned int number)
{
	EGtrigger[voice][number]=1;
	EG[voice][number][0] = 1.0f; // triggerd
	EG[voice][number][5] = 1.0f; // target
	EG[voice][number][7] = 0.0f;// state ? keep value, retrigger
	EGstate[voice][number] = 0;// state  
	EGFaktor[voice][number] = 0.f;
	lo_send(t, "/Minicomputer/EG", "iii", voice, number, 1);
}
/**
 * set the envelope to release mode
 * should be called by a related noteoff event
 * @param the voice number
 * @param the number of envelope generator
 */
static inline void egStop (const unsigned int voice,const unsigned int number)
{
	// if (EGrepeat[voice][number] == 0) 
	EGtrigger[voice][number] = 0; // triggerd
	EGstate[voice][number] = 0; // target
	if(EG[voice][number][6]>0.0f)
	  lo_send(t, "/Minicomputer/EG", "iii", voice, number, 4);
	else
	  lo_send(t, "/Minicomputer/EG", "iii", voice, number, 0);
}

static inline void egOff (const unsigned int voice)
{
	// Shut down main envelope (number 0) immediately
	EG[voice][0][6] = 0.0f;
	EGtrigger[voice][0] = 0;
	EGstate[voice][0] = 0;
	lo_send(t, "/Minicomputer/EG", "iii", voice, 0, EGstate[voice][0]);
}

/**
 * calculate the envelope, done in audiorate to avoide zippernoise
 * @param the voice number
 * @param the number of envelope generator
*/
static inline float egCalc (const unsigned int voice, const unsigned int number)
{
	/* EG[x] x:
	 * 0 = trigger (0 or 1)
	 * 1 = attack rate
	 * 2 = decay rate
	 * 3 = sustain level
	 * 4 = release rate
	 * 5 = target
	 * 6 = state (value between 0.0 and 1.0)
	 * EGstate - 1:Attack 2:Decay 3:Sustain 0:Release 4:Idle
	 * OSC messages - 0:Idle 1:Attack 2:Sustain 3:Decay 4:Release
	 */
	if (EGtrigger[voice][number] != 1) 
	{
	int i = EGstate[voice][number]; 
		if (i == 1){ // attack
			if (EGFaktor[voice][number]<1.00f) EGFaktor[voice][number] += 0.002f;
			
			EG[voice][number][6] += EG[voice][number][1]*srDivisor*EGFaktor[voice][number];

			if (EG[voice][number][6]>=1.0f)// Attack phase is finished
			{
				EG[voice][number][6]=1.0f;
				EGstate[voice][number]=2;
				EGFaktor[voice][number] = 1.0f; // triggerd
				lo_send(t, "/Minicomputer/EG", "iii", voice, number, 2);
			}
		}
		else if (i == 2){ // decay
			if (EG[voice][number][6]>EG[voice][number][3]) // Sustain level not reached
			{
				EG[voice][number][6] -= EG[voice][number][2]*srDivisor*EGFaktor[voice][number];
			}
			else // Sustain level reached (may be 0)
			{
				if (EGrepeat[voice][number]==0)
				{
					EGstate[voice][number]=3; // stay on sustain
					lo_send(t, "/Minicomputer/EG", "iii", voice, number, 3);
				}
				else // Repeat ON
				{
					EGFaktor[voice][number] = 1.0f; // triggerd
					egStop(voice,number); // continue to release
				}
			}
			// what happens if sustain = 0? envelope should go in stop mode when decay reached ground
			if (EG[voice][number][6]<0.0f) 
			{	
				EG[voice][number][6]=0.0f;
				if (EGrepeat[voice][number]==0)
				{
					EGstate[voice][number]=4; // released
					lo_send(t, "/Minicomputer/EG", "iii", voice, number, 0);
				}
				else
				{
					egStart(voice, number); // repeat
				}
			}
		} // end of decay
		else if ((i == 0) && (EG[voice][number][6]>0.0f))
		{
			/* release */
			
			if (EGFaktor[voice][number]>0.025f) EGFaktor[voice][number] -= 0.002f;
			EG[voice][number][6] -= EG[voice][number][4]*srDivisor*EGFaktor[voice][number];

			if (EG[voice][number][6]<0.0f) 
			{	
				EG[voice][number][6]=0.0f;
				if (EGrepeat[voice][number]==0)
				{
					EGstate[voice][number]=4; // released
					lo_send(t, "/Minicomputer/EG", "iii", voice, number, 0);
				}
				else
				{
					egStart(voice, number);// repeat
				}
			}
		}
//		if (EG[number][5] == 1.0f){
//		    /* attack */
//		   
//		    EG[number][6] += EG[number][1]*(1.0f - EG[number][6]);
//		    EG[number][5] = (EG[number][6] > 1.0f) ? EG[number][3] : 1.0f;
//		}
//		else if ((EG[number][5] == 0.0f) && (EG[number][6]>0.0f))
//		{
//		    /* release */
//		    
//		    EG[number][6] -= EG[number][4];//*EG[number][6];
//		    if (EG[number][6]<0.0f) EG[number][6]=0.0f;
//		}
//		else{
//		    /* decay */
//		    EG[number][6] -= EG[number][2]*(EG[number][6]-EG[number][3]);
//		}
	}
	else
	{ // EGtrigger is 1
	//	if (EGtrigger[voice][number] == 1) // declick ramp down processing
	//	{
			
		/*	EG[voice][number][6] -= 0.005f;
			if (EG[voice][number][6]<0.0f) 
			{  
				if  (EG[voice][number][7]< EG[voice][number][6])
				{
			   	 EG[voice][number][7] += EG[voice][number][1]*(1.0f - EG[voice][number][7]);
				}
				else
				{*/
					EGtrigger[voice][number] = 0;
				//	EGstate[voice][number] = 1;
		/*	    }
				
			}*/
	//	}
	//	else if (EG[voice][number][0] == 2.f) // actual start
	//	{
	//		EG[voice][number][0] = 0.f;
			
		EG[voice][number][0] = 1.0f; // triggerd
		EGstate[voice][number] = 1; // target
		lo_send(t, "/Minicomputer/EG", "iii", voice, number, 1);
			//EG[voice][number][6] = 0.0f;// state
	//	}
		
	}
	return EG[voice][number][6];
}

/** @brief the audio processing function from jack
 * 
 * this is the heart of the client. the process callback. 
 * this will be called by jack every process cycle.
 * jack provides us with a buffer for every output port, 
 * which we can happily write into.
 *
 * @param nframes
 * @param *arg pointer to additional arguments
 * @return integer 0 when everything is ok
 */
int process(jack_nframes_t nframes, void *arg) {

	float tfo1, tfo2, ta1_1, ta1_2, ta1, ta2, ta3; // Osc related
	float tf1, tf2, tf3, morph, mo, mf, result, tdelay, clib1, clib2; 
	float osc1, osc2, delayMod, pan;

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

	// Integer phase for each oscillator, 0..tabM
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
	buf = jack_port_get_buffer(_jack_midipt, nframes);
//    jack_nframes_t evcount = jack_midi_get_event_count(_jack_midipt); // Always 0 ??
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
				int t, n, v, c;
				doMidi(ev.buffer[0], ev.buffer[1], ev.buffer[2]);
			} else if(ev.size==2){ // For example channel pressure
				doMidi(ev.buffer[0], ev.buffer[1], 0);
			}
			++index1;
		}
	

	/* main loop */
	register unsigned int index;
	for (index = 0; index < nframes; ++index) // for each sample 
	{

	bufferMixLeft[index]=0.f;
	bufferMixRight[index]=0.f;
	bufferAux1[index]=0.f;
	bufferAux2[index]=0.f;
	/*
	 * I dont know if its better to separate the calculation blocks, so I try it
	 * first calculating the envelopes
	 */
	register unsigned int currentvoice;
	for (currentvoice=0;currentvoice<_MULTITEMP;++currentvoice) // for each voice
	{

		// Calc the modulators
		float * mod = modulator [currentvoice];
		// Modulation sources fall in different categories
		// EG and midi controllers are between 0 and 1 (todo pitch bend -1..+1)
		// Audio outputs (osc 3, filter, delay) are between -1 and 1
		// FM outputs (Osc 1 and 2 FM) are between 0 and 2 ??
		// Maybe to spare a test on phase<0?
		// but then this should apply to all possible sources
		// Modulation destinations used to handle source in different ways:
		//	osc1 amp mod 1 (ta1): +1 @ osc1 fm out (bug?); 1.0f-ta1 @ temp (pre-filter mix)
		//	osc1 amp mod 2 (also ta1): same as above
		//	osc1 freq mod 1 (tfo1): no change except boost, 1 or 100
		//	osc1 freq mod 2 (also tfo1): no change
		//	osc2 amp mod 1 (ta2): 1.0f-ta2 @ temp (pre-filter mix)
		//	osc2 amp mod 2 (ta3): 1.0f-ta3 @ osc2 fm out
		//	osc2 freq mod 1 (tfo2): no change except boost, 1 or 100
		//	osc2 freq mod 2 (also tfo2): no change
		//	morph mod 1 (mf; mo; morph): 1.0f-(param[38]*mod[ choi[10]])
		//	morph mod 2 (also mf; mo; morph): 1.0f-(param[48]*mod[ choi[11]])
		//	amp mod: 1.0f-mod[ choi[13]]*param[100]
		//	pan mod: (param[122]*mod[choi[16]])+param[107]
		//	time mod (delayMod): 1.0f-(param[110]* mod[choi[14]])
		//	Now we use modulation_bias and _scale so that all mods are 0..1

		// Why the 1.0f- ??
		// Maybe to compensate the 1.0f- when used: 1-(1-x)==x
		// When using modulator "none", 1-0==1
		// When using waves as mod, range -1..1 becomes 0..2
		// mod[8] =1.0f-egCalc(currentvoice,1);
		// mod[9] =1.0f-egCalc(currentvoice,2);
		// mod[10]=1.0f-egCalc(currentvoice,3);
		// mod[11]=1.0f-egCalc(currentvoice,4);
		// mod[12]=1.0f-egCalc(currentvoice,5);
		// mod[13]=1.0f-egCalc(currentvoice,6);
		mod[8] =egCalc(currentvoice,1);
		mod[9] =egCalc(currentvoice,2);
		mod[10]=egCalc(currentvoice,3);
		mod[11]=egCalc(currentvoice,4);
		mod[12]=egCalc(currentvoice,5);
		mod[13]=egCalc(currentvoice,6);

		/**
		 * calc the main audio signal
		 */
		// get the parameter settings
		float * param = parameter[currentvoice];
		
		// casting floats to int for indexing the 3 oscillator wavetables with custom typecaster
		P1.f =  phase[currentvoice][1];
		P2.f =  phase[currentvoice][2];
		P3.f =  phase[currentvoice][3];
		Psub.f =  phase[currentvoice][0];
		P1.f += bias.f;
		P2.f += bias.f;
		P3.f += bias.f;
		Psub.f += bias.f;
		P1.i -= bias.i;
		P2.i -= bias.i;
		P3.i -= bias.i;
		Psub.i -= bias.i;
		// tabM is a power of 2 minus one
		// We can use bitwise & instead of modulus  
		iP1=P1.i&tabM;
		iP2=P2.i&tabM;
		iP3=P3.i&tabM;
		iPsub=Psub.i&tabM;
		/*
		int iP1 = (int) phase[currentvoice][1];// float to int, cost some cycles
		int iP2 = (int) phase[currentvoice][2];// hopefully this got optimized by compiler
		int iP3 = (int) phase[currentvoice][3];// hopefully this got optimized by compiler

		iP1=iP1&tabM;//i%=TableSize;
		iP2=iP2&tabM;//i%=TableSize;
		iP3=iP3&tabM;//i%=TableSize;
		*/

		// Can this ever happen??
		if (iP1<0) iP1=tabM;
		if (iP2<0) iP2=tabM;
		if (iP3<0) iP3=tabM;
		if (iPsub<0) iPsub=tabM;

// --------------- create the next oscillator phase step for osc 3
// Handle osc 3 first so that it is available as modulation for osc 1 & 2
		phase[currentvoice][3]+= tabX * param[90];
		#ifdef _PREFETCH
		__builtin_prefetch(&param[1],0,0);
		__builtin_prefetch(&param[2],0,1);
		__builtin_prefetch(&param[3],0,0);
		__builtin_prefetch(&param[4],0,0);
		__builtin_prefetch(&param[5],0,0);
		__builtin_prefetch(&param[7],0,0);
		__builtin_prefetch(&param[11],0,0);
		#endif
		
		phase[currentvoice][3]=fmodf(phase[currentvoice][3], tabF);
		/*
		if(phase[currentvoice][3] >= tabF)
		{
			phase[currentvoice][3]-= tabF;
		}
		*/
		/* This cannot happen in the absence of modulation
		if(phase[currentvoice][3] < 0.f)
		{
			phase[currentvoice][3]+= tabF;
		}
		*/

		unsigned int * choi = choice[currentvoice];
		// write the oscillator 3 output to modulators
		mod[14] = table[choi[12]][iP3] ;

// --------------- calculate the parameters and modulations of main oscillators 1 and 2
		glide[currentvoice]*=param[116]; // *srDivisor?? or may be unconsistent across sample rates
		// what about denormal ??
		// if(glide[currentvoice]< FLT_MIN/param[116]) glide[currentvoice]=0;
		// guard values

		tfo1 = param[1]; // Fixed frequency
		tfo1 *=param[2]; // Fixed frequency enable

		// osc1 ampmods 1 and 2
		// modulators must be in range 0..1
		// param[9] ([11] for osc2) is  Amount -1..1
		// Multiply by negative is not an issue:
		// it's the same amplitude with phase reversal
		/*
		if(param[118]){ // Mult mode - 0 mod means no sound
			ta1_1 = 1; // No modulation
			if(choi[2]!=0) ta1_1 = (mod[choi[2]]+modulator_bias[choi[2]]) * modulator_scale[choi[2]] * param[9]; // -1..1, keep 1 for modulator "none"
			ta1_2 = 1; // 0..1
			if(choi[3]!=0) ta1_2 = (mod[choi[3]]+modulator_bias[choi[3]]) * modulator_scale[choi[3]] * param[11]; // -1..1, keep 1 for modulator "none"
			ta1=ta1_1*ta1_2;
		}else{ // Add mode - 0 mod means half volume
			ta1_1 = mod[choi[2]]*param[9];
			ta1_2 = mod[choi[3]]*param[11];
			ta1=0.5+(ta1_1+ta1_2)*0.25f;
		}
		*/
		if(param[130]){ // Mult mode amp mod 1
			ta1_1 = 1; // No modulation
			if(choi[2]!=0) ta1_1 = (mod[choi[2]]+modulator_bias[choi[2]]) * modulator_scale[choi[2]] * param[9]; // -1..1, keep 1 for modulator "none"
		}else{ // Add mode - 0 mod means half volume
			ta1_1 = mod[choi[2]]*param[9]; // -1..1
		}

		if(param[118]){ // Mult mode amp mod 2
			ta1_2 = 1; // No modulation
			if(choi[3]!=0) ta1_2 = (mod[choi[3]]+modulator_bias[choi[3]]) * modulator_scale[choi[3]] * param[11]; // -1..1, keep 1 for modulator "none"
			ta1=ta1_1*ta1_2; // -1..1
		}else{ // Add mode - 0 mod means half volume
			ta1_2 = mod[choi[3]]*param[11];
			ta1=(ta1_1+ta1_2)*0.5f; // -1..1
		}

		#ifdef _PREFETCH
		__builtin_prefetch(&phase[currentvoice][1],0,2);
		__builtin_prefetch(&phase[currentvoice][2],0,2);
		#endif

		tfo1+=(midif[currentvoice]*(1.0f-param[2])*param[3]); // Note-dependant frequency
		// tf+=(param[4]*param[5])*mod[choi[0]];
		tfo1-=glide[currentvoice];
		// What about tf *= instead of += ?
		// Would 2 multiplications be faster than an if?
		if(param[117]){ // Mult. ; param[4] is boost, 1 or 100
			tfo1+=(param[4]*param[5])*mod[choi[0]]*param[7]*mod[choi[1]];
		}else{
			tfo1+=(param[4]*param[5])*mod[choi[0]]+(param[7]*mod[choi[1]]);
		}

		//static inline float Oscillator(float frequency,int wave,float *phase)
		//{
		/*   int iP1 = (int) phase[currentvoice][1];// float to int, cost some cycles
		int iP2 = (int) phase[currentvoice][2];// hopefully this got optimized by compiler

		iP1=iP1&tabM;//i%=TableSize;
		iP2=iP2&tabM;//i%=TableSize;*/
		//if (i>tabM) i=tabM;

// --------------- generate phase of oscillator 1
		phase[currentvoice][1]+= tabX * tfo1;

		// iPsub=subMSB[currentvoice]+(iP1>>1); // 0..tabM
		// sub[currentvoice]=(4.f*iPsub/tabF)-1.0f; // Ramp up -1..+1 (beware of int to float)
		if(phase[currentvoice][1] >= tabF)
		{
			// phase[currentvoice][1]-= tabF;
			// branchless sync osc2 to osc1 (param[115] is 0 or 1):
			phase[currentvoice][2]-= phase[currentvoice][2]*param[115];
			// sub[currentvoice]=-sub[currentvoice]; // Square
			// sub[currentvoice]=(4.f*subMSB[currentvoice]/tabF)-1.0f; // Alt square, OK
			// Mostly works but some regular glitches at phase 0
			// subMSB[currentvoice]^=itabF>>1; // Halfway through table
			// if (*phase>=tabF) *phase = 0; //just in case of extreme fm
		}

		#ifdef _PREFETCH
			__builtin_prefetch(&param[15],0,0);
			__builtin_prefetch(&param[16],0,0);
			__builtin_prefetch(&param[17],0,0);
			__builtin_prefetch(&param[18],0,0);
			__builtin_prefetch(&param[19],0,0);
			__builtin_prefetch(&param[23],0,0);
			__builtin_prefetch(&param[25],0,0);
			__builtin_prefetch(&choice[currentvoice][6],0,0);
			__builtin_prefetch(&choice[currentvoice][7],0,0);
			__builtin_prefetch(&choice[currentvoice][8],0,0);
			__builtin_prefetch(&choice[currentvoice][9],0,0);
		#endif

		phase[currentvoice][1]=fmodf(phase[currentvoice][1], tabF);
		if(phase[currentvoice][1]< 0.f)
				{
					phase[currentvoice][1]+= tabF;
					// subMSB[currentvoice]^=itabF>>1; // Halfway through table
					//	if(*phase < 0.f) *phase = tabF-1;
				}
		osc1 = table[choi[4]][iP1] ;
		// Osc 1 FM out
		// param[13] is fm output vol for osc 1 (values 0..1)
		// Why 1.0f+ ?? -1..3 if both inputs used, maybe intended to be 0..2
		// mod[3]=osc1*(param[13]*(1.0f+ta1));
		mod[3]=osc1*param[13]*ta1;

// --------------- generate sub
		phase[currentvoice][0]+= tabX * tfo1 / 2.0f;
		/*
		if(phase[currentvoice][0] >= tabF)
			phase[currentvoice][0]-= tabF;
		*/
		phase[currentvoice][0]=fmodf(phase[currentvoice][0], tabF);
		if(phase[currentvoice][0]< 0.f)
			phase[currentvoice][0]+= tabF;
		
		sub[currentvoice] = table[choi[15]][iPsub];

// ------------------------ calculate oscillator 2 ---------------------
		// first the modulations and frequencies
		tfo2 = param[16];
		tfo2 *=param[17];
		
		// osc2 first amp mod for mix output only
		// ta2 = param[23];
		// ta2 *=mod[choi[8]];
		if(param[131]){ // Mult mode amp mod
			ta2 = 1; // No modulation
			if(choi[8]!=0) // keep 1 for modulator "none"
				ta2 = (mod[choi[8]]+modulator_bias[choi[8]]) * modulator_scale[choi[8]] * param[23] * param[29]; // -1..1
		}else{ // Add mode - 0 mod means half volume
			ta2 = (0.5 + mod[choi[8]] * param[23] *0.5) * param[29]; // 0..1
		}


		tfo2+=midif[currentvoice]*(1.0f-param[17])*param[18];
		tfo2-=glide[currentvoice];

		// osc2 second amp mod for FM only
		// ta3 = param[25];
		// ta3 *=mod[choi[9]];
		if(param[132]){ // Mult mode FM out amp mod
			ta3 = 1; // No modulation
			if(choi[9]!=0) // keep 1 for modulator "none"
				ta3 = (mod[choi[9]]+modulator_bias[choi[9]]) * modulator_scale[choi[9]] * param[25]; // -1..1
		}else{ // Add mode - 0 mod means half volume
			ta3 = (0.5 + mod[choi[9]] * param[25] *0.5); // 0..1
		}

		if(param[119]){
			tfo2+=(param[15]*param[19])*mod[choi[6]]*param[21]*mod[choi[7]];
		}else{
			tfo2+=(param[15]*param[19])*mod[choi[6]]+(param[21]*mod[choi[7]]);
		}
		
		// param[28] is fm output vol for osc 2 (values 0..1)
		// mod[4] = (param[28]+param[28]*(1.0f-ta3)); // osc2 fm out coeff
		mod[4] = param[28]*ta3; // osc2 fm out coeff

		// then generate the actual phase:
		phase[currentvoice][2]+= tabX * tfo2;
		phase[currentvoice][2]=fmodf(phase[currentvoice][2], tabF);
		/*
		if(phase[currentvoice][2]  >= tabF)
		{
   			phase[currentvoice][2]-= tabF;
			// if (*phase>=tabF) *phase = 0; //just in case of extreme fm
		}
		*/

		#ifdef _PREFETCH
			__builtin_prefetch(&param[14],0,0);
			__builtin_prefetch(&param[29],0,0);
			__builtin_prefetch(&param[38],0,0);
			__builtin_prefetch(&param[48],0,0);
			__builtin_prefetch(&param[56],0,0);
		#endif

		if(phase[currentvoice][2]< 0.f)
		{
			phase[currentvoice][2]+= tabF;
			//	if(*phase < 0.f) *phase = tabF-1;
		}
		osc2 = table[choi[5]][iP2] ;
		mod[4] *= osc2; // osc2 fm out

// ------------- mix the 2 oscillators and sub pre filter -------------------
		//temp=(parameter[currentvoice][14]-parameter[currentvoice][14]*ta1);
		// Why was 1.0f-ta n ?? offset for mod<0 ?? -1..3
		if(param[130]){
			temp=osc1*param[14]*ta1;
		}else{
			temp=osc1*param[14]*(0.5+ta1*0.5);
		}
		temp+=osc2*ta2;
		temp+=sub[currentvoice]*param[121];
		temp*=0.333f; // 0.5f;// get the volume of the sum into a normal range	
		temp+=anti_denormal; // Does this really work in all cases?

// ------------- calculate the filter settings ------------------------------
		mf = (1.0f-(param[38]*mod[choi[10]])); 
		if(param[120]){
			mf*=1.0f-(param[48]*mod[choi[11]]);
		}else{
			mf+=1.0f-(param[48]*mod[choi[11]]);
		}
		mo = param[56]*mf;

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

		clib1 = fabs (mo);
		clib2 = fabs (mo-1.0f);
		mo = clib1 + 1.0f;
		mo -= clib2;
		mo *= 0.5f;
		/*
		if (mo<0.f) mo = 0.f;
		else if (mo>1.0f) mo = 1.0f;
		*/

		morph=(1.0f-mo);
		/*
		tf= (srate * (parameter[currentvoice][30]*morph+parameter[currentvoice][33]*mo) );
		f[currentvoice][0] = 2.f * tf - (tf*tf*tf) * 0.1472725f;// / 6.7901358;

		tf= (srate * (parameter[currentvoice][40]*morph+parameter[currentvoice][43]*mo) );
		f[currentvoice][1] = 2.f * tf - (tf*tf*tf)* 0.1472725f; // / 6.7901358;;

		tf = (srate * (parameter[currentvoice][50]*morph+parameter[currentvoice][53]*mo) );
		f[currentvoice][2] = 2.f * tf - (tf*tf*tf) * 0.1472725f;// / 6.7901358; 
		*/
		
		// parallel calculation:
		#ifdef _VECTOR
			union f4vector a __attribute__((aligned (16))), b __attribute__((aligned (16))),  c __attribute__((aligned (16))), d __attribute__((aligned (16))),e __attribute__((aligned (16)));

			b.f[0] = morph; b.f[1] = morph; b.f[2] = morph; b.f[3] = morph;
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

		#else
			tf1= param[30];
			q[currentvoice][0] = param[31];
			v[currentvoice][0] = param[32];
			tf2= param[40];

			q[currentvoice][1] = param[41];
			v[currentvoice][1] = param[42];
			tf3= param[50];
			q[currentvoice][2] = param[51];
		#endif

		v[currentvoice][2] = param[52];

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

		#ifndef _VECTOR
			tf1*= morph;
			tf2*= morph;
			q[currentvoice][0] *= morph;
			v[currentvoice][0] *= morph;

			tf3 *=  morph;
			v[currentvoice][1] *= morph;
			q[currentvoice][1] *= morph;
			q[currentvoice][2] *= morph;
		#endif

		v[currentvoice][2] *= morph;

		#ifdef _VECTOR
			a.f[0] = param[33]; a.f[1] =param[34]; a.f[2] = param[35]; a.f[3] = param[43];
			d.f[0] = param[44]; d.f[1] =param[45]; d.f[2] = param[53]; d.f[3] = param[54];
			b.f[0] = mo; b.f[1] = mo; b.f[2] = mo; b.f[3] = mo;
			c.v = a.v * b.v;
			//c.v = __builtin_ia32_mulps (a.v, b.v);
			e.v = d.v * b.v;
			//e.v = __builtin_ia32_mulps (d.v, b.v);

			tf1+= c.f[0];
			tf2+=c.f[3];
			tf3 += e.f[2];
			q[currentvoice][0] += c.f[1];//parameter[currentvoice][34]*mo;
			q[currentvoice][1] += e.f[0];//parameter[currentvoice][44]*mo;
			q[currentvoice][2] += e.f[3];//parameter[currentvoice][54]*mo;
			v[currentvoice][0] += c.f[2];//parameter[currentvoice][35]*mo;
			v[currentvoice][1] += e.f[1];//parameter[currentvoice][45]*mo;

		#else
			tf1+= param[33]*mo;
			tf2+= param[43]*mo;
			tf3+= param[53]*mo;
		#endif


		#ifndef _VECTOR
			q[currentvoice][0] += param[34]*mo;
			q[currentvoice][1] += param[44]*mo;
			q[currentvoice][2] += param[54]*mo;

			v[currentvoice][0] += param[35]*mo;
			v[currentvoice][1] += param[45]*mo;

			tf1*=srate;
			tf2*=srate;
			tf3*=srate;
		#endif

		#ifdef _VECTOR
		// prepare next calculations

			a.f[0] = param[55]; a.f[1] =tf1; a.f[2] = tf2; a.f[3] = tf3;
			g.f[0] = mo;// b.f[1] = 2.f; b.f[2] = 2.f; b.f[3] = 2.f;
			j.v = a.v * i.v; // tf * srate
			c.v = j.v * g.v; // tf * 2
			//c.v = __builtin_ia32_mulps (a.v, g.v);

			v[currentvoice][2] += c.f[0];//parameter[currentvoice][55]*mo;

			//f[currentvoice][0] = c.f[1];//2.f * tf1;
			//f[currentvoice][1] = c.f[2];//2.f * tf2;
			//f[currentvoice][2] = c.f[3];//2.f * tf3;
			//pow(c.v,3);
			d.v = c.v - ((j.v * j.v * j.v) * h.v);

			f[currentvoice][0] = d.f[1];//(tf1*tf1*tf1) * 0.1472725f;// / 6.7901358;

			f[currentvoice][1] = d.f[2];//(tf2*tf2*tf2)* 0.1472725f; // / 6.7901358;;

			f[currentvoice][2] = d.f[3];//(tf3*tf3*tf3) * 0.1472725f;// / 6.7901358; 
		#else
			v[currentvoice][2] += param[55]*mo;

			f[currentvoice][0] = 2.f * tf1;
			f[currentvoice][1] = 2.f * tf2;
			f[currentvoice][2] = 2.f * tf3; 

			f[currentvoice][0] -= (tf1*tf1*tf1) * 0.1472725f;// / 6.7901358;

			f[currentvoice][1] -= (tf2*tf2*tf2)* 0.1472725f; // / 6.7901358;;

			f[currentvoice][2] -= (tf3*tf3*tf3) * 0.1472725f;// / 6.7901358; 
		#endif
//----------------------- actual filter calculation -------------------------
		// first filter
		float reso = q[currentvoice][0]; // for better scaling the volume with rising q
		low[currentvoice][0] = low[currentvoice][0] + f[currentvoice][0] * band[currentvoice][0];
		high[currentvoice][0] = ((reso + ((1.0f-reso)*0.1f))*temp) - low[currentvoice][0] - (reso*band[currentvoice][0]);
		//high[currentvoice][0] = (reso *temp) - low[currentvoice][0] - (q[currentvoice][0]*band[currentvoice][0]);
		band[currentvoice][0]= f[currentvoice][0] * high[currentvoice][0] + band[currentvoice][0];

		// second filter
		reso = q[currentvoice][1];
		low[currentvoice][1] = low[currentvoice][1] + f[currentvoice][1] * band[currentvoice][1];
		high[currentvoice][1] = ((reso + ((1.0f-reso)*0.1f))*temp) - low[currentvoice][1] - (reso*band[currentvoice][1]);
		band[currentvoice][1]= f[currentvoice][1] * high[currentvoice][1] + band[currentvoice][1];
		/*
			low[currentvoice][1] = low[currentvoice][1] + f[currentvoice][1] * band[currentvoice][1];
			high[currentvoice][1] = (q[currentvoice][1] * band[currentvoice][1]) - low[currentvoice][1] - (q[currentvoice][1]*band[currentvoice][1]);
		band[currentvoice][1]= f[currentvoice][1] * high[currentvoice][1] + band[currentvoice][1];
		*/
		// third filter
		reso = q[currentvoice][2];
		low[currentvoice][2] = low[currentvoice][2] + f[currentvoice][2] * band[currentvoice][2];
		high[currentvoice][2] = ((reso + ((1.0f-reso)*0.1f))*temp) - low[currentvoice][2] - (reso*band[currentvoice][2]);
		band[currentvoice][2]= f[currentvoice][2] * high[currentvoice][2] + band[currentvoice][2];

		mod [7] = (low[currentvoice][0]*v[currentvoice][0])+band[currentvoice][1]*v[currentvoice][1]+band[currentvoice][2]*v[currentvoice][2];

		//---------------------------------- amplitude shaping

		result = 1.0f-mod[ choi[13]]*param[100];
		result *= mod[7];
		result *= egCalc(currentvoice,0);// the final shaping envelope

		// --------------------------------- delay unit
		if( delayI[currentvoice] >= delayBufferSize )
		{
			delayI[currentvoice] = 0;
	
			//printf("clear %d : %d : %d\n",currentvoice,delayI[currentvoice],delayJ[currentvoice]);
		}
		delayMod = 1.0f-(param[110]* mod[choi[14]]);

		delayJ[currentvoice] = delayI[currentvoice] - ((param[111]* maxDelayTime)*delayMod);

		if( delayJ[currentvoice]  < 0 )
		{
			delayJ[currentvoice]  += delayBufferSize;
		}
		else if (delayJ[currentvoice]>delayBufferSize)
		{
			delayJ[currentvoice] = 0;
		}

		//if (delayI[currentvoice]>95000)
		//printf("jab\n");

		tdelay = result * param[114] + (delayBuffer[currentvoice] [ delayJ[currentvoice] ] * param[112] );
		tdelay += anti_denormal;
		//tdelay -= anti_denormal;
		delayBuffer[currentvoice] [delayI[currentvoice] ] = tdelay;
		/*
		if (delayI[currentvoice]>95000)
		{
			printf("lll %d : %d : %d\n",currentvoice,delayI[currentvoice],delayJ[currentvoice]);
			fflush(stdout);
		}
		*/
		mod[18]= tdelay;
		result += tdelay * param[113];
		delayI[currentvoice]=delayI[currentvoice]+1;

		// --------------------------------- output
		float *buffer = (float*) jack_port_get_buffer(port[currentvoice], nframes);
		buffer[index] = result * param[101];
		bufferAux1[index] += result * param[108];
		bufferAux2[index] += result * param[109];
		result *= param[106]; // mix volume
		pan=(param[122]*mod[choi[16]])+param[107];
		if (pan<0.f) pan=0.f;
		if (pan>1.0f) pan=1.0f;
		bufferMixLeft[index] += result * (1.0f-pan);
		bufferMixRight[index] += result * pan;
		}
	}
	return 0;// thanks to Sean Bolton who was the first pointing to a bug when I returned 1
}// end of process function



/** @brief the signal handler 
 *
 * its used here only for quitting
 * @param the signal
 */
void signalled(int signal) {
	quit = 1;
}
/** @brief initialization
 *
 * preparing for instance the waveforms
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

	float PI=3.145;
	float increment = (float)(PI*2) / (float)TableSize;
	float x = 0.0f;
	float tri = -0.9f;
	// calculate wavetables (values between -1.0 and 1.0)
	for (i=0; i<TableSize; i++)
	{
		table[0][i] = (float)((float)sin(x+(
				(float)2.0f*(float)PI))); // sin x+2pi == sin x ??
		x += increment;
		table[1][i] = (float)i/tabF*2.f-1.0f;// ramp up
			
		table[2][i] = 0.9f-(i/tabF*1.8f-0.5f);// tabF-((float)i/tabF*2.f-1.0f);//ramp down
			
		if (i<TableSize/2) 
		{ 
			tri+=(float)1.0f/TableSize*3.f; 
			table[3][i] = tri;
			table[4][i]=0.9f;
		}
		else
		{
			 tri-=(float)1.0f/TableSize*3.f;
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

		table[10][i]=(float)(sin((double)i/(double)TableSize+(sin((double)i*4))/2))*0.5;
		table[11][i]=(float)(sin((double)i/(double)TableSize*(sin((double)i*6)/4)))*2.;
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
	// Make it tunable ?
	// Allow different temperaments?
	for (i = 0;i<128;++i) midi2freq[i] = 8.1758f * pow(2,(i/12.f));

} // end of initialization

void doControlChange(int voice, int n, int v){
	// voice number (not necessarily the same as channel)
	if  (voice <_MULTITEMP)
	{
		switch(n){ // Controller number
			case 1:{ // Modulation
				modulator[voice][16]=v*0.007874f; // /127.f
				break;
			}
			case 12:{ // Effect controller 1
				modulator[voice][17]=v*0.007874f; // /127.f
				break;
			}
			case 2:{ // Breath controller
				modulator[voice][20]=v*0.007874f; // /127.f
				break;
			}
			case 3:{ // Undefined
				modulator[voice][21]=v*0.007874f; // /127.f
				break;
			}
			case 4:{ // Foot controller
				modulator[voice][22]=v*0.007874f; // /127.f
				break;
			}
			case 5:{ // Portamento time
				modulator[voice][23]=v*0.007874f; // /127.f
				break;
			}
			case 14:{ // Undefined
				modulator[voice][24]=v*0.007874f; // /127.f
				break;
			}
			case 15:{ // Undefined
				modulator[voice][25]=v*0.007874f; // /127.f
				break;
			}
			case 16:{ // General purpose
				modulator[voice][26]=v*0.007874f; // /127.f
				break;
			}
			case 17:{ // General purpose
				modulator[voice][27]=v*0.007874f; // /127.f
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
				if(v>63){ // Hold on
					hold[voice]=1;
				}else{ // Hold off
					hold[voice]=0;
					if (heldnote[voice]){
					// note is held if note_off occurs while hold is on
					// There is no attempt at polyphonic keyboard handling
						doNoteOff(voice, heldnote[voice], 0); // Do a note_off
						heldnote[voice]=0;
					}
				}
				break;
			}
		}
	}
}

void doMidi(int status, int n, int v){
	int c = status & 0x0F;
	int t = status & 0xF0;
	int voice;
#ifdef _DEBUG      
	fprintf(stderr,"doMidi %u %u %u\n", status, n, v);
#endif
	// if (voice<_MULTITEMP){
	for(voice=0; voice<_MULTITEMP; voice++){
		if(channel[voice]==c){
			switch(t){
				case 0x90:{ // Note on
					doNoteOn(voice, n, v);
					break;
				}
				case 0x80:{ // Note off
					doNoteOff(voice, n, v);
					break;
				}
				case 0xD0: // Channel pressure
				{
					modulator[voice][15]=(float)n*0.007874f; // /127.f
					break;
				}
				case 0xE0:{ // Pitch bend ?? needs bias; f mod has a strange behaviour for <0
					modulator[voice][2]=((v<<7)+n)*0.00012207f; // 0..2
					// modulator[voice][2]=(((v<<7)+n)-8192)*0.00012207f; // -1..1
#ifdef _DEBUG
					printf("Pitch bend %u -> %u %f\n", c, voice, (((v<<7)+n)-8192)*0.0001221f);
#endif
					break;
				}
				case 0xB0:{ // Control change
					doControlChange(voice, n, v);
					break;
				}
			}
		}
	}
}

static inline void doNoteOff(int voice, int note, int velocity){
	// Velocity currently ignored
	if (voice <_MULTITEMP){
		if(note==lastnote[voice]){ // Ignore release for old notes
			if(hold[voice]){
				heldnote[voice]=note;
			}else{
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

static inline void doNoteOn(int voice, int note, int velocity){
	if (voice <_MULTITEMP){
		if ((note>=note_min[voice]) && (note<=note_max[voice])){
			if (velocity>0){
				heldnote[voice]=0;
				lastnote[voice]=note;
				glide[voice]+=midi2freq[note]-midif[voice]; // Span between previous and new note
				if(EGstate[voice][0]==4){
					glide[voice]=0; // Don't glide from finished note
				}

				midif[voice]=midi2freq[note];// lookup the frequency
				// 1/127=0,007874015748...
				modulator[voice][19]=note*0.007874f;// fill the value in as normalized modulator
				modulator[voice][1]=(float)1.0f-(velocity*0.007874f);// fill in the velocity as modulator
				// why is velocity inverted??
				egStart(voice,0);// start the engines!
				// Maybe optionally restart repeatable envelopes too, i.e free-run boutton?
				if (EGrepeat[voice][1] == 0) egStart(voice,1);
				if (EGrepeat[voice][2] == 0) egStart(voice,2);
				if (EGrepeat[voice][3] == 0) egStart(voice,3);
				if (EGrepeat[voice][4] == 0) egStart(voice,4);
				if (EGrepeat[voice][5] == 0) egStart(voice,5);
				if (EGrepeat[voice][6] == 0) egStart(voice,6);
			}else{ // if velo == 0 it should be handled as noteoff...
				doNoteOff(voice, note, velocity);
			}
		}
	}
}

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
#ifdef _DEBUG
	printf("doReset: voice %u filters and delay buffer reset\n", voice);
#endif
}

/** @brief handling the midi messages in an extra thread
 *
 * @param pointer/handle of alsa midi
 */
static void *midiprocessor(void *handle) {
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
					if(channel[voice]==c)
						doControlChange(voice, ev->data.control.param, ev->data.control.value);
				}
			/* Factored out, see above
				if  (c <_MULTITEMP)
				{
					if  (ev->data.control.param==1)   
						modulator[c][ 16]=ev->data.control.value*0.007874f; // /127.f;
					else 
					if  (ev->data.control.param==12)   
						modulator[c][ 17]=ev->data.control.value*0.007874f;// /127.f;
					else 
					if  (ev->data.control.param==2)   
						modulator[c][ 20]=ev->data.control.value*0.007874f;// /127.f;
					else 
					if  (ev->data.control.param==3)   
						modulator[c][ 21]=ev->data.control.value*0.007874f;// /127.f;
					else 
					if  (ev->data.control.param==4)   
						modulator[c][ 22]=ev->data.control.value*0.007874f;// /127.f;
					else 
					if  (ev->data.control.param==5)   
						modulator[c][ 23]=ev->data.control.value*0.007874f;// /127.f;
					else 
					if  (ev->data.control.param==14)   
						modulator[c][ 24]=ev->data.control.value*0.007874f;// /127.f;
					else 
					if  (ev->data.control.param==15)   
						modulator[c][ 25]=ev->data.control.value*0.007874f;// /127.f;
					else 
					if  (ev->data.control.param==16)   
						modulator[c][ 26]=ev->data.control.value*0.007874f;// /127.f;
					else 
					if  (ev->data.control.param==17)   
						modulator[c][ 27]=ev->data.control.value*0.007874f;// /127.f;
					else 
					if  (ev->data.control.param==64){ // Hold
		 				if (c<_MULTITEMP){
							if(ev->data.control.value>63){ // Hold on
								hold[c]=1;
							}else{ // Hold off
								hold[c]=0;
								if (heldnote[c]){
								// note is held if note_off occurs while hold is on
								// There is no attempt at polyphonic keyboard handling
									doNoteOn(c, heldnote[c], 0); // Do a note_off
									heldnote[c]=0;
								}
							}
						}
					}
				}
				*/
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
					if(channel[voice]==c)
						modulator[voice][2]=ev->data.control.value*0.0001221f; // /8192.f; 0..2 ?? needs bias
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
				fprint("Note On event on Channel %2d: %5d       \r",
				c, ev->data.note.note);
				fprint("Note On event %u \r",EGstate[c][0]);
#endif
				for(voice=0; voice<_MULTITEMP; voice++){
					if(channel[voice]==c){
#ifdef _DEBUG      
						printf("midiprocessor note on voice %u channel %u %u..%u : %u %u\n",
							voice, channel[voice], note_min[voice], note_max[voice], c, ev->data.note.note);
#endif
						doNoteOn(voice, ev->data.note.note, ev->data.note.velocity);
					}
				}
				break;
			}      
			case SND_SEQ_EVENT_NOTEOFF: 
			{
				for(voice=0; voice<_MULTITEMP; voice++){
					if(channel[voice]==ev->data.note.channel)
						doNoteOff(voice, ev->data.note.note, ev->data.note.velocity);
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
		}// end of switch
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
}// end of midiprocessor

/** @brief the classic c main function
 *
 * @param argc the amount of arguments we get from the commandline
 * @param pointer to array of the arguments
 * @return int the result, should be 0 if program terminates nicely
 */
int main(int argc, char **argv) {
printf("minicomputer version %s\n",_VERSION);
// ------------------------ decide the oscport number -------------------------
char OscPort[] = _OSCPORT; // default value for OSC port
char *oport = OscPort;// pointer of the OSC port string
char *oport2;

int i;
// process the arguments
  if (argc > 1)
  {
  	for (i = 0;i<argc;++i)
	{
		if (strcmp(argv[i],"-port")==0) // got a OSC port argument
		{
			++i;// looking for the next entry
			if (i<argc)
			{
				int tport = atoi(argv[i]);
				if (tport > 0) oport = argv[i]; // overwrite the default for the OSCPort
			}
			else break; // we are through
		}
	}
  }

	printf("Server OSC input port %s\n",oport);
	sprintf(jackName,"Minicomputer%s",oport);// store globally a unique name

// ------------------------ ALSA midi init ---------------------------------
	pthread_t midithread;
	seq_handle = open_seq();
 
	npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
	pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
	snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);
	
	// create the thread and tell it to use midiprocessor as thread function
	int err = pthread_create(&midithread, NULL, midiprocessor,seq_handle);
	// printf("start err %i\n", err);
 

// ------------------------ OSC Init ------------------------------------   
	/* start a new server on port definied where oport points to */
	lo_server_thread st = lo_server_thread_new(oport, error);

	/* add method that will match /Minicomputer/choice with three integers */
	lo_server_thread_add_method(st, "/Minicomputer/choice", "iii", generic_handler, NULL);

	/* add method that will match the path /Minicomputer, with three numbers, int (voicenumber), int (parameter) and float (value) 
	 */
	lo_server_thread_add_method(st, "/Minicomputer", "iif", foo_handler, NULL);

	/* add method that will match the path Minicomputer/quit with one integer */
	lo_server_thread_add_method(st, "/Minicomputer/quit", "i", quit_handler, NULL);
	lo_server_thread_add_method(st, "/Minicomputer/midi", "iiii", midi_handler, NULL); // DSSI-like

	lo_server_thread_start(st);

	// init for output
	oport2=(char *)malloc(80);
	snprintf(oport2, 80, "%d", atoi(oport)+1);
	printf("Server OSC output port: \"%s\"\n", oport2);
	t = lo_address_new(NULL, oport2);

	/* setup our signal handler signalled() above, so 
	 * we can exit cleanly (see end of main()) */
	signal(SIGINT, signalled);

	init();
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

	_jack_midipt = jack_port_register (client, "Midi/in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	if (!_jack_midipt)
	{
		fprintf (stderr, "Error: can't create the 'Midi/in' jack port\n");
		exit (1);
	}
	//inbuf = jack_port_register(client, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	/* jack is callback based. That means we register 
	 * a callback function (see process() above)
	 * which will then get called by jack once per process cycle */
	jack_set_process_callback(client, process, 0);
	bufsize = jack_get_buffer_size(client);

	// handling the sampling frequency
	sampleRate = (float) jack_get_sample_rate (client); 
	tabX = tabF / sampleRate;
	srate = 3.145f/ sampleRate;
	srDivisor = 1.0f / sampleRate * 100000.f;
	// depending on it the delaybuffer
	maxDelayTime = (int)sampleRate;
	delayBufferSize = maxDelayTime*2;
	// generate the delaybuffers for each voice
	int k;
	for (k=0; k<_MULTITEMP;++k)
	{
		//float dbuffer[delayBufferSize];
		//delayBuffer[k]=dbuffer;
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
// ******************************************** OSC handling for editors ***********************
//!\name OSC routines
//!{ 
/** @brief OSC error handler 
 *
 * @param num errornumber
 * @param pointer msg errormessage
 * @param pointer path where it occured
 */
static inline void error(int num, const char *msg, const char *path)
{
	printf("liblo server error %d in path %s: %s\n", num, path, msg);
	fflush(stdout);
}

/** catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods 
 *
 * @param pointer path osc path
 * @param pointer types
 * @param argv pointer to array of arguments 
 * @param argc amount of arguments
 * @param pointer data
 * @param pointer user_data
 * @return int 0 if everything is ok, 1 means message is not fully handled
 * */
static inline int generic_handler(const char *path, const char *types, lo_arg **argv,
			int argc, void *data, void *user_data)
{
	if ( (argv[0]->i < _MULTITEMP) && (argv[1]->i < _CHOICECOUNT) )
  	{
		// printf("generic_handler %u %u %u\n", argv[0]->i, argv[1]->i, argv[2]->i);
		choice[argv[0]->i][argv[1]->i]=argv[2]->i;
		return 0;
	}
	else
	{
		fprintf(stderr, "WARNING unhandled generic_handler %u %u %u\n", argv[0]->i, argv[1]->i, argv[2]->i);
		return 1;
	}
}

static inline int midi_handler(const char *path, const char *types, lo_arg **argv,
			int argc, void *data, void *user_data)
{
	int i, c;
	// first byte argv[0] is 0 for 3-byte messages such as note on/off
#ifdef _DEBUG
	printf("midi_handler %u %u %u %u \n", argv[0]->i, argv[1]->i, argv[2]->i, argv[3]->i);
#endif
	if (argv[0]->i == 0){ // At most 3 bytes follow
		if(argv[1]->i == 0){ // At most 2 bytes follow
			if(argv[2]->i == 0){ // Single byte message follows
				if(argv[3]->i == 0xFE){ // Active sense
					lo_send(t, "/Minicomputer/sense", "");
				}
			}
		}else{ // 3 MIDI bytes follow
			c=(argv[1]->i)&0x0F;
			switch ((argv[1]->i)&0xF0){
				case 0x90:{ // Note on
					doNoteOn(c, argv[2]->i, argv[3]->i);
#ifdef _DEBUG
					printf("doNoteOn %u %u %u \n", argv[1]->i&0x0F, argv[2]->i, argv[3]->i);
#endif
					break;
				}
				case 0x80:{ // Note off
					doNoteOn(c, argv[2]->i, 0);
#ifdef _DEBUG
					printf("doNoteOff %u %u \n", argv[1]->i&0x0F, argv[2]->i);
#endif
					break;
				}
				case 0xB0:{ // Control change
					switch(argv[2]->i){ // Controller number
					  case 120:{ // All sound off
						egOff(c);
						doReset(c);
						// Fall through all note off
					  }
					  case 123:{ // All note off
						doNoteOn(c,lastnote[c], 0);
						break;
					  }
					}
				}
			}
		}
	}
	return 0;
}

/** specific message handler for iif messages
 * expects voice number, parameter number, parameter value
 *
 * @param pointer path osc path
 * @param pointer types
 * @param argv pointer to array of arguments 
 * @param argc amount of arguments
 * @param pointer data
 * @param pointer user_data
 * @return int 0 if everything is ok, 1 means message is not fully handled
 */
static inline int foo_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	/* example showing pulling the argument values out of the argv array */
	int voice =  argv[0]->i;
	int i =  argv[1]->i;
	if ((voice<_MULTITEMP) && (i>0) && (i<_PARACOUNT)) 
	{
		parameter[voice][i]=argv[2]->f;
	}

	//if ((i==10) && (parameter[10]!=0)) parameter[10]=1000.f;
	// printf("%s <- f:%f, i:%d\n\n", path, argv[0]->f, argv[1]->i);
	// fflush(stdout);
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

	 // 125 test note and 126 test velocity intentionally ignored
	 case 127:channel[voice]=(argv[2]->f)-1;break;
	 case 128:note_min[voice]=argv[2]->f;break;
	 case 129:note_max[voice]=argv[2]->f;break;
	}
#ifdef _DEBUG
	printf("foo_handler %i %i %f \n",voice,i,argv[2]->f);
#endif   
	return 0;
}

/** message handler for quit messages
 *
 * @param pointer path osc path
 * @param pointer types
 * @param argv pointer to array of arguments 
 * @param argc amount of arguments
 * @param pointer data
 * @param pointer user_data
 * @return int 0 if everything is ok, 1 means message is not fully handled
 */
static inline int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	done = 1;
	quit = 1;
	printf("about to shutdown minicomputer core\n");
	fflush(stdout);
	return 0;
}
//!}
