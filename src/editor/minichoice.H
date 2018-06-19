/*! \file minichoice.H
 *  \brief function declarations for minichoice and miniinput
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

#ifndef minichoice_H
#define minichoice_H

int minichoice(const char *q,const char *b0,const char *b1,const char *b2, const char *title,...);
const char* miniinput(const char *fmt, const char *defstr, const char *title, ...);

#endif
