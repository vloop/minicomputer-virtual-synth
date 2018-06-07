#ifndef COMMON_H_
#define COMMON_H_
/** Minicomputer
 * industrial grade digital synthesizer
 *
 * Copyright 2007,2008 Malte Steiner
 * Changes by Marc Périlleux 2018
 * This file is part of Minicomputer, which is free software:
 * you can redistribute it and/or modify
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
// number of parameters per patch
#define _PARACOUNT 156
#define _CHOICECOUNT 19
#define _EGCOUNT 7

// Voices
#define _MULTITEMP 8

// how many additional settings per sound in multi to store
#define _MULTISETTINGS 12

// how many additional settings common to all sounds in multi to store
#define _MULTIPARMS 13

// Available memory slots for sounds and multis
#define _SOUNDS 512
#define _MULTIS 128

// the version number as string
#define _VERSION "1.5.0"

#endif
