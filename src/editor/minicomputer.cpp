/*! \file minicomputer.cpp
 *  \brief synth editor main entry point
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
 
// thanks to Leslie P. Polzer pointing me out to include cstring and more for gcc 4.3 onwards
#include <cstdio>
#include <cstdlib>
// #include <stdlib.h>
// #include <unistd.h>
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
char midiName[_NAMESIZE] = "MinicomputerEditor"; // signifier for midi connections, to be filled with OSC port number
lo_address t;
bool lo_error = false;
bool sense=false; // Core not sensed yet
int bank[_MULTITEMP];

// some common definitions
Memory Speicher;
UserInterface Schaltbrett;

/** open an Alsa Midiport for input
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
  snd_seq_set_client_name(seq_handle, midiName);
  if ((portid = snd_seq_create_simple_port(seq_handle, midiName,
			SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
			SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
	fprintf(stderr, "Error creating sequencer port.\n");
	exit(1);
  }
  return(seq_handle);
}

/** @brief an extra thread for handling the midi messages
 *
 * the editor only reacts to program changes
 * note ons and the rest have to go directly to the sound engine
 * It means midi keyboards need to be connected to the sound engine
 * Channels are hard-coded, 1..8 for voices, 9 for multi change
 * @param handle to alsaseq
 */
static void *alsaMidiProcess(void *handle) {
	struct sched_param param;
	int policy;
	snd_seq_t *seq_handle = (snd_seq_t *)handle;
	pthread_getschedparam(pthread_self(), &policy, &param);

	policy = SCHED_FIFO;
	// param.sched_priority = 95;

	pthread_setschedparam(pthread_self(), policy, &param);

/*
if (poll(pfd, npfd, 100000) > 0) 
		{
		  midi_action(seq_handle);
		} */

  snd_seq_event_t *ev;

  do {
	while (snd_seq_event_input(seq_handle, &ev)) {
		switch (ev->type) {
		case SND_SEQ_EVENT_PGMCHANGE:
		  {
			int channel = ev->data.control.channel;
			int value = ev->data.control.value;
#ifdef _DEBUG
			printf("alsaMidiProcess: program change event on Channel %2d: %2d %5d\n",
					channel, ev->data.control.param, value);
#endif
			// see if it's the control channel
			if (ev->data.control.channel == 8) { // perform multi program change
				// first a range check
				if ((value > -1) && (value < _MULTIS)) {
					Schaltbrett.changeMulti(value);
				}
			} else if ((channel >= 0) && (channel < _MULTITEMP)) {
				// program change on the sounds
				if ((value > -1) && (value < _SOUNDS)) {
					Schaltbrett.changeSound(channel, value+((bank[channel]) << 7));
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
			if ((ev->data.control.param == 32) && (ev->data.control.value < 4) && (ev->data.control.channel < _MULTITEMP)) {
#ifdef _DEBUG
				printf("Change to bank %u on channel %u\n", ev->data.control.value, ev->data.control.channel);
#endif
				bank[ev->data.control.channel] = ev->data.control.value;
			}
			break;
		  }
		} // end of switch
		snd_seq_free_event(ev);
	} // end of first while, emptying the seqdata queue
  } while (true); // doing forever, was  (snd_seq_event_input_pending(seq_handle, 0) > 0);
  return 0;
}

/** @brief OSC handler for EG message from the sound engine
 *
 */
static inline int eg_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data) {
#ifdef _DEBUG
	printf("Received EG %u %u %u\n", argv[0]->i, argv[1]->i, argv[2]->i);
#endif
	return(EG_draw(argv[0]->i, argv[1]->i, argv[2]->i));
}

/** @brief OSC handler for sense message from the sound engine
 *
 */
static inline int sense_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data) {
	sense = true; // Server core is up
	return 0;
}

/** @brief Refresh display at regular intervals
 *
 */
static void timer_handler(void *userdata) {
	EG_draw_all();
	Fl::repeat_timeout(0.04, timer_handler, 0); // 25 fps
}

/** @brief OSC handler for incoming program change request
 *
 */
static inline int program_change_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data) {
#ifdef _DEBUG
	printf("program_change_handler: %u %u\n", argv[0]->i, argv[1]->i);
#endif
	Schaltbrett.changeSound(argv[0]->i, argv[1]->i);
	return 0;
}

/** @brief OSC error handler 
 *
 * @param num errornumber
 * @param msg errormessage
 * @param path where it occured
 */
static inline void error(int num, const char *msg, const char *path) {
	printf("liblo server error %d in path %s: %s\n", num, path, msg);
	fflush(stdout);
	lo_error = true;
}

/** @brief Outputs command line options help
 *
 */
void usage() {
	printf(
		"Usage:\n"
		"	-port nnnn			sets the base OSC port\n"
		"	-port2 nnnn			sets the secondary OSC port (default to base+1)\n"
		"	-no-launch			don't launch sound engine core (for example if it's remote)\n"
		"	-no-connect			core will not attempt to autoconnect to jack audio\n"
		"	-ask-before-save	always prompt before saving sounds or multis\n"
		"	-h or --help		show this message\n"
		"other options (such as -bg and -fg) will be forwarded to FLTK engine\n"
	);
}


/** @brief the main routine
 *
 * @param argc the amount of arguments
 * @param pointer to array with arguments
 * @return integer, 0 when terminated correctly
 */
char *oport;
char *oport2;
bool noEscape;
int main(int argc, char **argv) {
  printf("minicomputer editor version %s\n", _VERSION);

// ---------- Handle command line parameters ----------
  // check color settings in arguments and add some if missing
  // TODO -remote IP for remote server
  bool needcolor = true; // true means user didnt give some so I need to take care myself
  int i;
  char OscPort[] = _OSCPORT; // default value for OSC port
  oport = OscPort;
  oport2 = (char *)malloc(_NAMESIZE);
  bool no_connect = false;
  bool no_launch = false;
  bool launched = false;
  bool has_port2 = false;
  int ac = 1; // Argument count for FLTK
  // There is at least the command name  av[0]=argv[0])
	for (i = 1; i < argc; ++i) {
		// BEWARE! any change below must be matched
		// in the "Prepare command line for FLTK" section
		if ( (strcmp(argv[i], "-bg") == 0) || (strcmp(argv[i], "-fg") == 0) ) {
			needcolor = false;
		} else if (strcmp(argv[i], "-port") == 0) { // got a OSC port argument
			++i; // looking for the next entry
			if (i < argc) {
				int tport = atoi(argv[i]);
				if (tport > 0) {
					oport = argv[i]; // overwrite the default for the OSCPort
				} else {
					fprintf(stderr, "Invalid port %s\n", argv[i]);
					usage();
					exit(1);
				}
			} else {
				fprintf(stderr, "Invalid port - end of command line reached\n");
				usage();
				exit(1);
			}
		} else if (strcmp(argv[i], "-port2") == 0) { // got a OSC port argument
			++i; // looking for the next entry
			if (i < argc) {
				int tport2 = atoi(argv[i]);
				if (tport2 > 0) {
					oport2 = argv[i]; // overwrite the default for the OSCPort
					has_port2 = true;
				} else {
					fprintf(stderr, "Invalid port %s\n", argv[i]);
					usage();
					exit(1);
				}
			} else {
				fprintf(stderr, "Invalid port - end of command line reached\n");
				usage();
				exit(1);
			}
		} else if (strcmp(argv[i], "-no-connect") == 0) {
			no_connect = true;
		} else if (strcmp(argv[i], "-no-launch") == 0) {
			no_launch = true;
		} else if (strcmp(argv[i], "-no-escape") == 0) {
			noEscape = true;
		} else if (strcmp(argv[i], "-ask-before-save") == 0) {
			alwaysSave = false;
		} else if ( strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 ) {
			usage();
			exit(0);
		} else { // Count as an FLTK argument
			ac++; // Not recognized, assume an FLTK parameter
			// Cannot store value yet, av array not yet created
		}
	}

	if (!has_port2) // Port not specified, use default
		snprintf(oport2, _NAMESIZE, "%d", atoi(oport)+1);

// ------------------------ create gui --------------
	char temp_name[_NAMESIZE];
	int fileerrors=0;
	strncpy(temp_name, "minicomputer ", _NAMESIZE);
	strncpy(temp_name+13, oport, _NAMESIZE-13);
	MiniWindow* w = Schaltbrett.make_window(temp_name);
	if(Speicher.loadSounds()){
		fileerrors++;
		printf("Cannot load sounds - clearing all sounds\n");
		for(int j=0; j<_SOUNDS; j++) Speicher.clearSound(j);
	}
	if(Speicher.loadMultis()){
		fileerrors++;
		printf("Cannot load multis - clearing all multis\n");
		for(int j=0; j<_MULTIS; j++) Speicher.clearMulti(j);
	}
	if(Speicher.loadInit()){
		fileerrors++;
		printf("Cannot load init - clearing init sound\n");
		Speicher.clearInit();
	}

// ------------------------ OSC init ---------------------------------
	// init for output
	t = lo_address_new(NULL, oport);
	printf("\nGUI OSC output port: \"%s\"\n", oport);
	snprintf(midiName, _NAMESIZE, "miniEditor%s", oport); // store globally a unique name

	// init for input
	printf("GUI OSC input port: \"%s\"\n", oport2);
	/* start a new server on port defined where oport2 points to */
	lo_server_thread st = lo_server_thread_new(oport2, error);
	/* add methods that will match /Minicomputer/... messages*/
	if (!lo_error) lo_server_thread_add_method(st, "/Minicomputer/EG", "iii", eg_handler, NULL);
	if (!lo_error) lo_server_thread_add_method(st, "/Minicomputer/sense", "", sense_handler, NULL);
	if (!lo_error) lo_server_thread_add_method(st, "/Minicomputer/programChange", "ii", program_change_handler, NULL);
	if (!lo_error) lo_server_thread_start(st);

	if (lo_error) {
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
	if(!sense && !no_launch) { // Don't start it if already done
		char engineName[_NAMESIZE]; // the name of the core program + given port, if any.
		if(no_connect)
			snprintf(engineName, _NAMESIZE, "minicomputerCPU -no-connect -port %s -port2 %s &", oport, oport2);
		else
			snprintf(engineName, _NAMESIZE, "minicomputerCPU -port %s -port2 %s &", oport, oport2);
		printf("Launching %s\n", engineName);
		system(engineName); // actual start
		launched = true;
	}
#endif
// ------------------------ midi init ---------------------------------
  pthread_t midithread;
  seq_handle = open_seq();
  npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
  pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
  snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);

	// create the thread and tell it to use Midi::work as thread function
	int err = pthread_create(&midithread, NULL, alsaMidiProcess, seq_handle);
	if (err) {
		fprintf(stderr, "Error %u creating MIDI thread\n", err);
		// should exit? This is non-blocking for the GUI.
	}

// ---------- Prepare command line for FLTK -----------
  if (needcolor) ac += 4; // add 2 more arguments and their values
  char * av[ac]; // the new array

  av[0] = argv[0];
  int j = 1;
  for (i = 1; i < argc; ++i) { // now actually copying it
	if (strcmp(argv[i], "-port") == 0 || strcmp(argv[i], "-port2") == 0) {
		i++; // skip this parameter and its argument
		// We know this is safe because parm checking already occured above
	} else if (strcmp(argv[i], "-no-connect") == 0
		|| strcmp(argv[i], "-no-launch") == 0
		|| strcmp(argv[i], "-no-escape") == 0
		|| strcmp(argv[i], "-ask-before-save") == 0) {
		// ++i; // skip this parameter
	} else {
		av[j] = argv[i];
		j++;
#ifdef _DEBUG
		printf("Copied %s\n", argv[i]);
#endif
	}
  }

	// Make sure the following are in the same scope as w->show
	char bg[]="-bg";
	char bgv[]="grey";
	char fg[]="-fg";
	char fgv[]="black";
	if (needcolor) { // add the arguments in case they are needed
		av[ac-4] = bg;
		av[ac-3] = bgv;
		av[ac-2] = fg;
		av[ac-1] = fgv;
	}

  // Wait for core before sending GUI data
  if (!sense) printf("Waiting for core...\n");
  while (!sense) {
		lo_send(t, "/Minicomputer/midi", "iiii", 0, 0, 0, 0xFE);
		usleep(100000); // 0.1 s
  }
  printf("Communication with minicomputerCPU established\n");

  // Schaltbrett.changeMulti(multi); // Transmit multi data to the sound engine
  Schaltbrett.changeMulti(0); // Transmit multi 0 data to the sound engine

#ifdef _DEBUG
  printf("FLTK argument count: %u\n", ac);
  printf("FLTK arguments:\n");
  for (int argnum = 0; argnum < ac; argnum++)
    printf("%u %s\n", argnum, av[argnum]);
#endif
  // printf("FLTK API version %u\n", Fl::api_version()); // not found
  // printf("FLTK API version %u\n", Fl::version()); // 0 ???
  Fl::lock(); // see http://www.fltk.org/doc-1.3/advanced.html
  if(fileerrors) fl_alert("Found %u file error(s).\nLaunching installpresets.sh should fix the problem.", fileerrors);
  w->show(ac, av);
  int result = Fl::run();
  if (launched) lo_send(t, "/Minicomputer/quit", "i", 1);
  /* waiting for the midi thread to shutdown carefully */
  pthread_cancel(midithread);
  /* release Alsa Midi connection */
  snd_seq_close(seq_handle);
  return result;
}
