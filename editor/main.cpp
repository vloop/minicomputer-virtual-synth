/** Minicomputer
 * industrial grade digital synthesizer
 * editorsoftware
 * Copyright 2007,2008 Malte Steiner
 * This file is part of Minicomputer, which is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Minicomputer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
// thanks to Leslie P. Polzer pointing me out to include cstring and more for gcc 4.3 onwards
#include <cstdio>
#include <cstdlib>
//#include <stdlib.h>
//#include <unistd.h>
#include <cstring>
#include <lo/lo.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include "../common.h"
#include "Memory.h"
#include "syntheditor.h"

snd_seq_t *open_seq();
snd_seq_t *seq_handle;
int npfd;
struct pollfd *pfd;
char midiName[64]="MinicomputerEditor";// signifier for midiconnections, to be filled with OSC port number
lo_address t;
bool lo_error=false;
int bank[_MULTITEMP];

// some common definitions
Memory Speicher;
UserInterface Schaltbrett;

/** open an Alsa Midiport for accepting programchanges and later more...
 *
 * @return handle to Alsaseq
 */
snd_seq_t *open_seq() {

  snd_seq_t *seq_handle;
  int portid;

  if (snd_seq_open(&seq_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
	fprintf(stderr, "Error opening ALSA sequencer.\n");
	exit(1);
  }
  snd_seq_set_client_name(seq_handle,midiName);
  if ((portid = snd_seq_create_simple_port(seq_handle, midiName,
			SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
			SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
	fprintf(stderr, "Error creating sequencer port.\n");
	exit(1);
  }
  return(seq_handle);
}
/*
Fl_Double_Window* make_window() {
  Fl_Double_Window* w;
  { Fl_Double_Window* o = new Fl_Double_Window(475, 330);
	w = o;
   
	 { Fl_Dial* knob1 = new Fl_Dial(25, 25, 25, 25,"f1");
	  knob1->callback((Fl_Callback*)callback);
}
	{Fl_Dial* o =new Fl_Dial(65, 25, 25, 25, "f2");
	o->callback((Fl_Callback*)callback);
}
   {Fl_Dial* o = new Fl_Dial(105, 25, 25, 25, "f3");
o->callback((Fl_Callback*)callback);
}
	o->end();

  
}
  return w;
}*/
/*
void reloadSoundNames()
{
  int i;
  for (i = 0;i<8;++i)
  {
  	Schaltbrett.soundchoice[i]->clear();
  } 
  
  Speicher.load();
  
  for (i=0;i<512;i++) 
  {
  	Schaltbrett.soundchoice[0]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[1]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[2]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[3]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[4]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[5]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[6]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[7]->add(Speicher.getName(0,i).c_str());
  }
}*/
/** @brief handling the midi messages in an extra thread
 *
 * the editor only reacts on program changes, note ons and the rest have to go directly to the synthcore, it means midi keyboards need to be connected to the core too
 * @param handle to alsaseq
 */
static void *midiprocessor(void *handle) {
	struct sched_param param;
	int policy;
	snd_seq_t *seq_handle = (snd_seq_t *)handle;
	pthread_getschedparam(pthread_self(), &policy, &param);

	policy = SCHED_FIFO;
	//param.sched_priority = 95;

	pthread_setschedparam(pthread_self(), policy, &param);

/*
if (poll(pfd, npfd, 100000) > 0) 
		{
		  midi_action(seq_handle);
		} */
		
  snd_seq_event_t *ev;

  do {
   while (snd_seq_event_input(seq_handle, &ev))
   {
	switch (ev->type) {
	case SND_SEQ_EVENT_PGMCHANGE:
	  {
		int channel = ev->data.control.channel;
		int value = ev->data.control.value;
#ifdef _DEBUG      
		fprintf(stderr, "Programchange event on Channel %2d: %2d %5d       \r",
				channel,  ev->data.control.param,value);
#endif		
	// see if its the control channel
	if (ev->data.control.channel == 8)
	{ // perform multi program change
		// first a range check
		if ((value>-1) && (value <128))
		{
			Schaltbrett.changeMulti(value);
		}
	}
	else if ((channel>=0) && (channel<_MULTITEMP)) 
	{
		//program change on the sounds
		if ((value>-1) && (value <128))
		{
			Schaltbrett.changeSound(channel,value+((bank[channel])<<7));
		}

	}
		break;
	  }
	  case SND_SEQ_EVENT_CONTROLLER:
	  {
#ifdef _DEBUG      
		fprintf(stderr, "Control event on Channel %2d: %2d %5d       \r",
				ev->data.control.channel, ev->data.control.param, ev->data.control.value);
#endif		
        // MIDI Controller  0 = Bank Select MSB (Most Significant Byte)
        // MIDI Controller 32 = Bank Select LSB (Least Significant Byte)
		if ((ev->data.control.param==32) && (ev->data.control.value <4) && (ev->data.control.channel<_MULTITEMP)){
            printf("Change to bank %u on channel %u\n", ev->data.control.value, ev->data.control.channel);
            bank[ev->data.control.channel]=ev->data.control.value;
        }
        /*
		if  (ev->data.control.param==1)   
			  modulator[ev->data.control.channel][ 16]=(float)ev->data.control.value*0.007874f; // /127.f;
			  else 
			  if  (ev->data.control.param==12)   
			  modulator[ev->data.control.channel][ 17]=(float)ev->data.control.value*0.007874f;// /127.f;
        */
		break;
	  }
	  /*
	  case SND_SEQ_EVENT_PITCHBEND:
	  {
#ifdef _DEBUG      
		 fprintf(stderr,"Pitchbender event on Channel %2d: %5d   \r", 
				ev->data.control.channel, ev->data.control.value);
#endif		
				if (ev->data.control.channel<_MULTITEMP)
			   	 modulator[ev->data.control.channel][2]=(float)ev->data.control.value*0.0001221f; // /8192.f;
		break;
	  }*/   
	 }// end of switch
	snd_seq_free_event(ev);
   }// end of first while, emptying the seqdata queue
  } while (1==1);// doing forever, was  (snd_seq_event_input_pending(seq_handle, 0) > 0);
  return 0;// why the compiler insists to have this here? Its a void function so what??
}

static inline int EG_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data)
{
#ifdef _DEBUG
	printf("Received EG %u %u %u\n", argv[0]->i, argv[1]->i, argv[2]->i);
#endif
	return(EG_draw(argv[0]->i, argv[1]->i, argv[2]->i));
}

static inline int sense_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data) {
	sense = true; // Server core is up
	return 0;
}

static void timer_handler(void *userdata) {
	EG_draw_all();
	Fl::repeat_timeout(0.04, timer_handler, 0); // 25 fps
}


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
	lo_error=true;
}

/** @brief the main routine
 *
 * @param argc the amount of arguments
 * @param pointer to array with arguments
 * @return integer, 0 when terminated correctly
 */
char *oport;
char *oport2;
int main(int argc, char **argv)
{
  printf("minieditor version %s\n",_VERSION);

// ---------- Handle command line parameters ----------
  // check color settings in arguments and add some if missing
  // TODO -remote IP for remote server
  bool needcolor=true; // true means user didnt give some so I need to take care myself
  int i;
  char OscPort[] = _OSCPORT; // default value for OSC port
  oport = OscPort;
  oport2=(char *)malloc(80);
  bool launched=false;
  if (argc > 1)
  {
  	for (i = 0;i<argc;++i)
	{
	  	if ((strcmp(argv[i],"-bg")==0) || (strcmp(argv[i],"-fg")==0))
		{
			needcolor = false;
		}
		else if (strcmp(argv[i],"-port")==0) // got a OSC port argument
		{
			++i;// looking for the next entry
			if (i<argc)
			{
				int tport = atoi(argv[i]);
				if (tport > 0){
					oport = argv[i]; // overwrite the default for the OSCPort
				}// TODO what if not a port number??
			}
			else break; // we are through
		}
	}
  }
  snprintf(oport2, 80, "%d", atoi(oport)+1);

// ------------------------ create gui --------------
	char temp_name[128];
	strcpy(temp_name, "minicomputer ");
	strncpy(temp_name+13, oport, 100);
	Fenster* w =Schaltbrett.make_window(temp_name);
  //
  //for (int i = 0;i<8;++i)
  //{
  	//printf("bei %i\n",i);
  	//fflush(stdout);
  	//Schaltbrett.soundchoice[i]->clear();
  //} 
	Speicher.load();
	//printf("und load...\n");
  /*
  for (int i=0;i<512;i++) 
  {
  	Schaltbrett.soundchoice[0]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[1]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[2]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[3]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[4]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[5]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[6]->add(Speicher.getName(0,i).c_str());
  	Schaltbrett.soundchoice[7]->add(Speicher.getName(0,i).c_str());
  }*/
  //multichoice->damage(FL_DAMAGE_ALL);
  //multichoice->redraw();


  Speicher.loadMulti();
  
  // Load data in the GUI
  // Schaltbrett.changeMulti(1); // segfault
  // Fix multi name display
  int multi=multiDisplay->value();
  /*
  // printf("Multi %u: \"%s\"\n", multi, Speicher.multis[0].name); // OK
  strnrtrim(temp_name, Speicher.multis[multi].name, 128);
  // printf("Multi %u: \"%s\"\n", multi, temp_name); // OK
  Schaltbrett.multichoice->value(temp_name);
  multiRoller->value(multi);
  */
  
  // int preset=soundchoice[i]->value();

  /*for (int i=0;i<128;i++) 
  {
  	Schaltbrett.multichoice->add(Speicher.multis[i].name);
  }*/
  //printf("weiter...\n");


// ------------------------ OSC init ---------------------------------
	// TODO error checking and exit
	// init for output
	t = lo_address_new(NULL, oport);
	printf("\nGUI OSC output port: \"%s\"\n",oport);
	sprintf(midiName,"miniEditor%s",oport);// store globally a unique name

	// init for input
	printf("GUI OSC input port: \"%s\"\n", oport2);
	/* start a new server on port defined where oport2 points to */
	lo_server_thread st = lo_server_thread_new(oport2, error);
	/* add method that will match /Minicomputer/EG with three integers */
	if(!lo_error) lo_server_thread_add_method(st, "/Minicomputer/EG", "iii", EG_handler, NULL);
	if(!lo_error) lo_server_thread_add_method(st, "/Minicomputer/sense", "", sense_handler, NULL);
	if(!lo_error) lo_server_thread_start(st);

	if(lo_error){
		fprintf(stderr, "OSC failure (port conflict?)\n");
		exit(1); // No clean-up needed yet?
	}

	Fl::add_timeout(.1, timer_handler, 0); // Attempt to refresh display

// -------------------------------------------------------------------
#ifdef _BUNDLE
//------------------------- start engine -----------------------------
	// probe the OSC port
	// It is safe to send now that OSC is setup
	lo_send(t, "/Minicomputer/midi", "iiii", 0, 0, 0, 0xFE);
	usleep(100000); // 0.1 s - should be enough even for remote
	if(!sense){ // Don't start it if already done
		char engineName[32];// the name of the core program + given port, if any.
		sprintf(engineName,"minicomputerCPU -port %s &",oport);
		system(engineName);// actual start
		launched=true;
	}
#endif
// ------------------------ midi init ---------------------------------
  pthread_t midithread;
  seq_handle = open_seq();
  npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
  pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
  snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);
	
	// create the thread and tell it to use Midi::work as thread function
	int err=pthread_create(&midithread, NULL, midiprocessor,seq_handle);
	if(err){
		fprintf(stderr,"Error %u creating MIDI thread\n", err);
		// should exit?
	}

// ---------- Prepare command line for FLTK -----------
  int ac = 0; // new argumentcount
  // copy existing arguments, filtering out osc port arguments
  // step one, parsing and determine the final count of arguments
  for (i = 0;i<argc;++i)
  {
  	if (strcmp(argv[i],"-port") == 0)
	{
		++i; // skip this parameter
		if (i<argc)
		{
			int tport = atoi(argv[i]);
			if (tport > 0)
			{
				++i; // skip the port number too	
				if (i>=argc) 
				{
					break; // we are through
				}
			}
		}
		else break; // we are through
	}
  	else 
	{
		++ac;
	}
  }
  
  if (needcolor)
  {
  	ac += 4;// add 2 more arguments and their values
  }
  char * av[ac]; // the new array
  
  for (i = 0;i<argc;++i) // now actually copying it
  {
  	if (strcmp(argv[i],"-port") == 0)
	{
		++i; // skip this parameter
		if (i<argc)
		{
			int tport = atoi(argv[i]);
			if (tport > 0)
			{
				++i; // skip the port number too	
				if (i>=argc) 
				{
					break; // we are through
				}
			}
		}
		else break; // we are through
	}
  	else 
	{
		av[i] = argv[i];
#ifdef _DEBUG
		printf("%s\n",argv[i]);
#endif
	}
  }

  if (needcolor) // add the arguments in case they are needed
  {
  	char bg[]="-bg";
	char bgv[]="grey";
	char fg[]="-fg";
	char fgv[]="black";
	av[ac-4] = bg;
	av[ac-3] = bgv;
	av[ac-2] = fg;
	av[ac-1] = fgv;
  }
  
  // Wait for core before sending GUI data
  printf("Waiting for core...\n");
  while(!sense){
		lo_send(t, "/Minicomputer/midi", "iiii", 0, 0, 0, 0xFE);
		usleep(100000); // 0.1 s
  }
  
  Schaltbrett.changeMulti(multi); // Transmit multi data to the sound engine

  Fl::lock(); 
  w->show(ac, av);
	/* an address to send messages to. sometimes it is better to let the server
	 * pick a port number for you by passing NULL as the last argument */
 int result = Fl::run();
 if (launched) lo_send(t, "/Minicomputer/quit", "i",1);
 /* waiting for the midi thread to shutdown carefully */
 pthread_cancel(midithread);
/* release Alsa Midi connection */
 snd_seq_close(seq_handle);
 return result;
}
