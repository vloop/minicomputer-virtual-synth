/*! \file strnutil.cpp
 *  \brief utility functions to handle multi and sound names
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
#include "strnutil.h"
#include <stdio.h>

/**
 * Remove trailing blanks
 *
 * Destination may be the same as source.
 */
char *strnrtrim(char *dest, const char*source, size_t len){
	int last_letter=-1;
	unsigned int i;
	for(i=0; i<len && source[i]!=0; i++){
		if(!isspace(source[i])) last_letter=i;
		dest[i]=source[i];
	}
	// Beware terminating 0 is not added past end!
	if (last_letter+1 < (signed)len) // Could cause trouble with huge len
		dest[last_letter+1]=0; // Should fill with zeros to the end?
	return dest;
}

/**
 * Remove quotes and unescape string
 * 
 * This ignores source 1st character, assuming a quote.
 *
 * Destination may be the same as source.
 */
char *strnunquote(char *dest, const char *source, size_t len){
	unsigned int i, j;
	// j<i at all times
	// should we allow i (but not j) to exceed len?
	for(i=1, j=0; i<len && source[i]!=0 && source[i]!='\''; i++, j++){
		if(source[i]=='\\' && i<len-1 && source[i+1]!=0) i++;
		dest[j]=source[i];
	}
	dest[j]=0; // ok since j<i<=len
	return dest;
}

/**
 * Quote string, escaping quotes and backslash
 *
 * Source and destination must not overlap.
 */
// Add a rtrim option?
char *strnquote(char *dest, const char *source, size_t len){
	unsigned int i, j;
	if(len<3){
		if(len) dest[0]=0;
		fprintf(stderr, "strnquote: No room for quoting string\n");
		return dest;
	}
	dest[0]='\''; // Opening quote
	for(i=0, j=1; j<len-2 && source[i]!=0; i++, j++){
		if(source[i]=='\'' || source[i]=='\\'){ // Quote and backslash need to be escaped
			if(j<len-3){ // J at most len -4, room for backslash+char+quote+0
				dest[j++]='\\';
				dest[j]=source[i];
			}else{ // No room for escaping
				// Do nothing, don't copy this character
				fprintf(stderr, "strnquote: No room for quoting string\n");
			}
		}else{ // Regular character, not requiring escaping, plain copy
			dest[j]=source[i];
		}
	}
	// j is at most len-2, there is always room for quote and zero
	dest[j++]='\''; // Closing quote
	dest[j]=0;
	return dest;
}
