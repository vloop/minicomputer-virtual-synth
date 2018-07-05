#ifndef MINICOMPUTERCPU_H_
#define MINICOMPUTERCPU_H_
/*! \file minicomputerCPU.h
 *  \brief forward declarations only, not intended for external calls
 * 
 * This file is part of Minicomputer, which is free software:
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


static void osc_error_handler(int num, const char *m, const char *path); 
static int osc_choice_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data); 
static int osc_param_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data); 
static int osc_quit_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);
static int osc_midi_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);
static int osc_multiparm_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);
static int osc_audition_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);
static inline void doNoteOn(int voice, int note, int velocity);
static inline void doNoteOff(int voice, int note, int velocity);
static void doMidi(int t, int n, int v);
void doReset(unsigned int voice);

#ifdef _USE_ALSA_MIDI
snd_seq_t *open_seq();
#endif

#endif
