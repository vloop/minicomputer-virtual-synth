#ifndef STRNUTIL_H_
#define STRNUTIL_H_
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

// The type size_t is defined in <stddef.h>. (It is also defined in <stdlib.h>, <wchar.h>, <stdio.h>, and <string.h>
#include <string.h>
#include <ctype.h>

char *strnrtrim(char *dest, const char *source, size_t len);
char *strnunquote(char *dest, const char *source, size_t len);
char *strnquote(char *dest, const char *source, size_t len);

#endif
