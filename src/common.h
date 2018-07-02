#ifndef COMMON_H_
#define COMMON_H_
/*! \file common.h
 *  \brief common defines for the GUI and sound engine
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

// customizable options

#define _OSCPORT "7771"
/*!< OSC default base port (will also use port+1) */

#define _BUNDLE
/*!< Let the GUI start/stop the sound engine */

// try disabling this if you experience sluggish midi reaction
//#define _MIDIBLOCK 1


// debug output
// #define _DEBUG

// prefetch support with intrinsics
//#define _PREFETCH

// vectorize calculations
//#define _VECTOR

// not customizable options ----------------------------------
#define _NAMESIZE 128
/*!< Text length for sound and multi names */
#define _PARACOUNT 157
/*!< Number of general parameters per sound (patch) */
#define _CHOICECOUNT 20
/*!< Number of menu parameters per sound (patch) */
#define _EGCOUNT 7
/*!< Number of envelopes per sound (patch) */

// Voices
#define _MULTITEMP 8
/*!< Number of voices */

// Settings per sound in multi
#define _MULTISETTINGS 12
/*!< Number of per-voice parameters in multi */

// Global (common to all sounds) settings in multi
#define _MULTIPARMS 22
/*!< Number of global parameters in multi */

// Available memory slots for sounds and multis
#define _SOUNDS 512
/*!< Number of sounds (patches) in memory */
#define _MULTIS 128
/*!< Number of multis in memory */

// The version number as a string
#define _VERSION "1.5.0"

#endif
