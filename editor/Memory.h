/** Minicomputer
 * industrial grade digital synthesizer
 * editorsoftware
 * Copyright 2007, 2008 Malte Steiner
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


#ifndef MEMORY_H_
#define MEMORY_H_
#include <iostream> //iostream
#include <fstream> //fstream
#include <sstream> //sstream
// thanks to Leslie P. Polzer pointing me out to include cstring and more for gcc 4.3 onwards
#include <cstring> //string
//#include <boost/archive/text_oarchive.hpp>
//#include <boost/archive/text_iarchive.hpp>
#include <cstdio>
#include <cstdlib>

    #include <unistd.h>
#include <cerrno> 
#include "../common.h"
using namespace std;
//#include "patch.h"
/**
 * the struct of an single sound setting
 */
typedef struct
{
	char name[_NAMESIZE];
	float parameter[_PARACOUNT];
	float freq[9][2];
	int choice[_CHOICECOUNT];
} patch;
/**
 * the struct of a multitemperal setup
 */
typedef struct
{
	char name[_NAMESIZE];
	unsigned int sound[_MULTITEMP]; // sound ids for the 8 voices
	float settings[_MULTITEMP][_MULTISETTINGS]; // additional settings per voice
	float parms[_MULTIPARMS]; // additional global settings (fine tune)
} multi;

/**
 * the class for the whole memory
 * see Memory.cpp for more documentation
 */
class Memory
{
public:
	Memory();
	void copySound(int src, int dest);
	void copyPatch(const patch *src, patch *dest);
	void copyMulti(int src, int dest);
	void saveSounds();
	int loadSounds();
	int loadInit();
	void saveMultis();
	int loadMultis();
	void store(patch Sound);
//	void overwrite(patch Sound);
	void clearMulti(unsigned int m);
	void clearSound(unsigned int m);
	void clearPatch(patch *dest);
	void clearInit();
	void clearEG(patch *dest, unsigned int n);
	void importSound(string filename, unsigned int current); // import a single sound
	void exportSound(string filename, unsigned int current); // export a single sound
	int importPatch(string filename, patch *p); // import single sound to patch
	void importMulti(string filename, unsigned int current); // import a single multi
	void exportMulti(string filename, unsigned int current); // export a single multi

	patch initSound;
	string getSoundName(unsigned int soundnum);
	string getMultiName(unsigned int multinum);
	void setSoundName(unsigned int soundnum, const char *new_name);
	void setMultiName(unsigned int multinum, const char *new_name);
	virtual ~Memory();
	patch sounds[512];
	multi multis[128];
	int setChoice(unsigned int voice,unsigned int i);
	unsigned int getChoice(unsigned int voice);
	
	private:
	unsigned int choice[_MULTITEMP];
	bool parseNumbers(string &str,int &iParameter,int &i2Parameter,float &fValue);
	char folder[1024]; // the directory to write stuff in
	bool gotFolder;
};

#endif /*MEMORY_H_*/
