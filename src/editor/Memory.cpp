/*! \file Memory.cpp
 *  \brief synth editor patch and multi handling
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
#include "Memory.h"
#include "strnutil.h"

/**
 * constructor
 */
Memory::Memory()
{
	int i, result;
	for (i=0;i<_MULTITEMP;++i)
	{
		soundNo[i] = 0;
	}	
	for ( i = 0;i<_SOUNDS;++i)
	{
		sprintf(sounds[i].name,"%i untitled sound",i);
	}
	
	for ( i = 0;i<_MULTIS;++i)
	{
		sprintf(multis[i].name,"%i untitled multi",i);
	}
	char kommand[1200];
		gotFolder = true;
		if (getenv("HOME") == NULL)
		{
			strcpy(folder,""); // ok, $HOME is not set so save it just HERE
			_homeFolder="";
		}
		else
		{ 
			_homeFolder = getenv("HOME");
			sprintf(folder, "%s/.miniComputer", _homeFolder.c_str());
			if (access(folder, R_OK) != 0)
			{
				sprintf(kommand,"mkdir %s", folder);
				result=system(kommand);
				cout<<kommand<<endl;
				printf("mkdir result: %u\n", result);

			}
		}
		#ifdef _DEBUG
		cout<<"command:"<<kommand<<" folder:"<<folder;
		#endif
}
/**
 * destructor
 */
Memory::~Memory()
{
}

/**
 * retrieve the name of a certain sound
 * @param soundNo the sound number
 * @return the name as a string
 */
string Memory::getSoundName(unsigned int soundNo)
{
	if(soundNo<_SOUNDS) return sounds[soundNo].name;
	return(NULL);
}

/**
 * retrieve the name of a certain multi
 * @param multiNo the multi number
 * @return the name as a string
 */
string Memory::getMultiName(unsigned int multiNo)
{
	if(multiNo<_MULTIS) return multis[multiNo].name;
	return(NULL);
}

/**
 * set the sound number for a certain voice
 * @param voice number
 * @param sound number
 */
int Memory::setSoundNo(unsigned int voice, unsigned int sound)
{
	if (voice<_MULTITEMP && sound<_SOUNDS)
	{
		soundNo[voice] = sound;
		return(0);
	}
	else // oha, should not happen
	{
		fprintf(stderr, "setSoundNo: illegal voice %d or sound %d\n", voice, sound);
		return(-1);
	}
}

void Memory::setSoundName(unsigned int dest, const char *new_name){
	strncpy(sounds[dest].name, new_name, _NAMESIZE);
}

void Memory::setMultiName(unsigned int dest, const char *new_name){
	strncpy(multis[dest].name, new_name, _NAMESIZE);
}

void Memory::copyPatch(const patch *src, patch *dest)
{
	*dest=*src;
}

void Memory::copySound(int src, int dest)
{
	copyPatch(&sounds[src], &sounds[dest]);
}

/**
 * return a sound id of a certain given voice number
 * @param voice the voice number
 * @return the sound number for that voice
 */
unsigned int Memory::getSoundNo(unsigned int voice)
{
	return soundNo[voice];
}

/**
 * save the complete sound memory to disk
 * @see Memory::loadSounds()
 */
void Memory::saveSounds()
{
	char kommand[2400]; // twice folder and some
	snprintf(kommand, 2400, "%s/minicomputerMemory.temp", folder);

	ofstream File (kommand); // temporary file
	int result;
	File<<"# Minicomputer v"<<_VERSION<<" sounds file"<<endl;
	for (int i=0;i<_SOUNDS;++i)
	{  
		File<< "["<<i<<"]" <<endl; // write the soundnumber
		writeSound(File, i);
	} // end of for i
	File.close();
	
	snprintf(kommand, 2400, "%s/minicomputerMemory.txt", folder);
	printf("Saving sounds to file %s\n", kommand);
	if (access(kommand, R_OK) == 0) // check if there a previous file which need to be backed up
	{
		snprintf(kommand, 2400, "mv %s/minicomputerMemory.txt %s/minicomputerMemory.txt.bak", folder, folder);
		result=system(kommand); // make a backup
		if(result) fprintf(stderr, "mv to .bak error, result: %u\n", result);
	}
	snprintf(kommand, 2400, "mv %s/minicomputerMemory.temp %s/minicomputerMemory.txt", folder, folder);
	result=system(kommand); // commit the file finally
	if(result) fprintf(stderr, "mv to .txt error, result: %u\n", result);
}

int Memory::loadInit()
{
	char path[1200], kommand[2400];
	sprintf(path, "%s/initsinglesound.txt", folder);
	if (importPatch(path, &initSound)){ // Import failed
		// Attempt copy
		printf("loadInit: import failed - attempting to copy factory settings\n");
		snprintf(kommand, 2400, "cp /usr/share/minicomputer/factory_presets/initsinglesound.txt %s", path);
		if(system(kommand)){ // Copy failed
			fprintf(stderr, "Copy to %s failed.\n", path);
			return 1;
		}
		if(importPatch(path, &initSound)){ // Retry
			fprintf(stderr, "loadInit: error importing file %s\n", path);
			return(1);
		}
	}
	return 0;
}
/**
 * load the sound memory (i.e. all preset definitions) from disk
 * @see Memory::saveSounds()
 */
int Memory::loadSounds()
{
	char path[1200], kommand[2400];;
	snprintf(path, 1200, "%s/minicomputerMemory.txt", folder);
	printf("loadSounds: Loading %s\n", path);
	ifstream file (path);
	if(!file){

		// Attempt copy
		fprintf(stderr, "loadSounds: error opening file %s - attempting to copy\n", path);
		snprintf(kommand, 2400, "cp /usr/share/minicomputer/factory_presets/minicomputerMemory.txt %s", path);
		if(system(kommand)){ // Copy failed
			fprintf(stderr, "Copy to %s failed.\n", path);
			return 1;
		}

		// Fallback to factory presets
		fprintf(stderr, "loadSounds: error opening file - reverting to factory settings\n");
		snprintf(path, 1200, "/usr/share/minicomputer/factory_presets/minicomputerMemory.txt");
		printf("loadSounds: Loading %s\n", path);
		file.open(path);
		// rewind(file);
		// It looks like cp returns before actually copying?
		if(!file){
			fprintf(stderr, "loadSounds: error opening file %s\n", path);
			return(1);
		}
	}
	
	string str,sParameter,sValue;
	float fValue;
	int iParameter,i2Parameter;
	int current=-1;
	getline(file,str);
	// TODO factor out redundant code with readPatch ??
	while (file)
	{
		sParameter="";
		sValue = "";
		switch (str[0])
		{
			case '(': // setting parameter
			{
				if (parseNumbers(str,iParameter,i2Parameter,fValue))
				{
					sounds[current].parameter[iParameter]=fValue;
				}
			}
			break;
			case '{': // setting additional parameter
			{
				if (parseNumbers(str,iParameter,i2Parameter,fValue))
				{
					sounds[current].choice[iParameter]=(int)fValue;
				}
			}
			break;
			case '<': // setting additional parameter
			{
				if (parseNumbers(str,iParameter,i2Parameter,fValue))
				{
					sounds[current].freq[iParameter][i2Parameter]=fValue;
				}
			}
			break;
			case '\'': // setting the name
			{
				strnunquote(sounds[current].name, str.c_str(), _NAMESIZE);
			}
			break;
			case '[': // setting the current sound index
			{
				if (parseNumbers(str,iParameter,i2Parameter,fValue))
				{
					current = iParameter;
				}
			}
			break;
			case '#': // Comment
				printf("loadSounds: %s\n", &str[1]);
			break;

		}

		getline(file, str);// get the next line of the file
	}
	file.close();
	// printf("done.\n");
	return(0);
}

/** Write the given sound's data to an open file
 * 
 * Does not write the sound number
 * @param file an open output stream
 * @param sound the sound number
 */
void Memory::writeSound(ofstream& file, unsigned int sound)
{
	int p,j;
	// The sound number has already been written if needed
	char temp_name1[_NAMESIZE], temp_name2[_NAMESIZE]; // Worst case escaping doubles size
	strnrtrim(temp_name1, sounds[sound].name, _NAMESIZE);
	strnquote(temp_name2, temp_name1, _NAMESIZE);
	// printf("sound %s\n", temp_name2);
	file<<temp_name2<<endl;

	for (p=0;p<9;++p)
	{
		for (j=0;j<2;++j)
			file<< "<"<< p << ";" << j << ":" <<sounds[sound].freq[p][j]<<">"<<endl;
	}
	for (p=0;p<_CHOICECOUNT;++p)
		file<< "{"<< p << ":"<<sounds[sound].choice[p]<<"}"<<endl;
	for (p=0;p<_PARACOUNT;++p)
		file<< "("<< p << ":"<<sounds[sound].parameter[p]<<")"<<endl;
}

/** export a single sound to a textfile
 * @param filename the full path to the file
 * @param sound the sound memory location which is exported
 */
void Memory::exportSound(string filename, unsigned int sound)
{
	printf("Exporting sound %u to file %s\n", sound, filename.c_str());
	ofstream file (filename.c_str());
	file<<"# Minicomputer v"<<_VERSION<<" single sound file"<<endl;
	writeSound(file, sound);
	file.close();
}

int Memory::readPatch(ifstream& File, patch *p)
{
	string str, sParameter, sValue;
	float fValue;
	int iParameter, i2Parameter;
	// unsigned int j;
	int pos=File.tellg();
	getline(File,str);
//	bool gotname=false;
	while (File)
	{
		sParameter="";
		sValue = "";
		switch (str[0])
		{
			case '(': // setting parameter
			{
				if (parseNumbers(str, iParameter, i2Parameter, fValue))
				{
					p->parameter[iParameter]=fValue;
				}
			}
			break;
			case '{': // setting additional parameter
			{
				if (parseNumbers(str, iParameter, i2Parameter, fValue))
				{
					p->choice[iParameter]=(int)fValue;
				}
			}
			break;
			case '<': // setting additional parameter
			{
				if (parseNumbers(str, iParameter, i2Parameter, fValue))
				{
					p->freq[iParameter][i2Parameter]=fValue;
				}
			}
			break;
			case '\'': // setting the name
			{
				strnunquote(p->name, str.c_str(), _NAMESIZE);
				printf("readPatch: Reading patch \"%s\"\n", p->name);
			}
			break;
			case '[': // Next patch (new patch number)
				File.seekg(pos ,std::ios_base::beg);
				return 0;
			case '#': // Comment
				printf("readPatch: %s\n", &str[1]);
			break;
			default:
				fprintf(stderr, "readPatch: unexpected leading character %c (%d)\n", str[0], str[0]);
		}
		pos=File.tellg();
		getline(File,str); // get the next line of the file
	}
	return 0;
}

int Memory::importPatch(string filename, patch *p)
{
	ifstream File (filename.c_str());
	if(!File){
		fprintf(stderr, "importPatch: error opening file %s\n", filename.c_str());
		return(1);
	}
	printf("importPatch: Loading %s\n", filename.c_str());
	readPatch(File, p);
	File.close();
	// printf("importPatch: read complete.\n");
	return(0);
}

/** import a single sound from a textfile
 * and write it to the given memory location
 * @param filename the full path to the file
 * @param sound the sound memory location whose parameters are about to be overwritten
 */
void Memory::importSound(string filename, unsigned int sound)
{
	if(sound>=_SOUNDS){
		fprintf(stderr, "ERROR: unexpected sound number %d\n", sound);
		return;
	}
	printf("Importing sound %u from file %s\n", sound, filename.c_str());
	importPatch(filename, &sounds[sound]);
	// now the new sound is in RAM but need to be saved to the main file
	// saveSounds();
}
void Memory::clearMulti(unsigned int m)
{
	if (m>=_MULTIS) return;
	setMultiName(m, "");
	int p;
	for (p=0;p<_MULTITEMP;++p) // init the sound ids of all 8 voices
	{
		multis[m].sound[p]=p;
		multis[m].settings[p][0]=1.0; // 101 Id vol
		multis[m].settings[p][1]=1.0; // 106 Mix vol
		multis[m].settings[p][2]=0.5; // 107 Pan
		multis[m].settings[p][3]=0; // 108 Aux 1
		multis[m].settings[p][4]=0; // 109 Aux 2
		multis[m].settings[p][5]=p+1; // 125 Channel
		multis[m].settings[p][6]=80; // 126 Velocity
		multis[m].settings[p][7]=69; // 127 Test note A5
		multis[m].settings[p][8]=0; // 128 Note min C0
		multis[m].settings[p][9]=127; // 129 Note max G10
		multis[m].settings[p][10]=0; // 130 Transpose
		multis[m].settings[p][11]=0; // 155 poly link
	}
	for (p=0;p<_MULTIPARMS;++p) // clear the global multi parameters
	{
		multis[m].parms[p]=0;
	}
}
void Memory::clearEG(patch *dest, unsigned int n)
{
	if (n>6) return;
	int egbase[]={102, 60, 65, 70, 75, 80, 85};
	dest->parameter[egbase[n]]=0.5f; // EG attack
	dest->parameter[egbase[n]+1]=0.5f; // EG decay
	dest->parameter[egbase[n]+2]=1.0f; // EG sustain
	dest->parameter[egbase[n]+3]=0.5f; // EG release
}

void Memory::clearPatch(patch *dest){
	strncpy(dest->name, "", _NAMESIZE); // setSoundName(m, "");
	int p, j;
	for (p=0;p<9;++p)
	{
		for (j=0;j<2;++j)
			dest->freq[p][j]=0;
	}
	dest->freq[0][0]=1.0f; // Osc 1 freq
	dest->freq[2][0]=1000; // Filter 1 left freq
	dest->freq[3][0]=4000; // Filter 1 right freq
	for (p=0;p<_CHOICECOUNT;++p)
		dest->choice[p]=0;
	for (p=0;p<_PARACOUNT;++p) // write the remaining parameters
		dest->parameter[p]=0;
	dest->parameter[14]=1.0f; // Osc 1 vol
	dest->parameter[31]=0.9f; // Filter 1 left Q
	dest->parameter[32]=1.0f; // Filter 1 left vol
	dest->parameter[34]=0.9f; // Filter 1 right Q
	dest->parameter[35]=1.0f; // Filter 1 right vol
	dest->parameter[41]=0.9f; // Filter 2 left Q
	dest->parameter[44]=0.9f; // Filter 2 right Q
	dest->parameter[51]=0.9f; // Filter 3 left Q
	dest->parameter[54]=0.9f; // Filter 3 right Q
	for (p=0; p<7; p++)
		clearEG(dest, p);
}
void Memory::clearSound(unsigned int m)
{
	if (m>=_SOUNDS) return;
	patch *dest=&sounds[m];
	clearPatch(dest);
}
void Memory::clearInit()
{
	clearPatch(&initSound);
}
/**
 * the multitimbral setup, the choice of sounds and some settings
 * are saved to an extra file
 * supports the new textformat like the sound saving
 * @see Memory::loadMultis()
 */
void Memory::saveMultis()
{
	char kommand[2400];
	int i;
	int result;

	// first write in temporary file, just in case
	snprintf(kommand, 2400, "%s/minicomputerMulti.temp", folder);
	ofstream File (kommand); // temporary file
	File<<"# Minicomputer v"<<_VERSION<<" multis file"<<endl;

	// int p,j;
	for (i=0;i<_MULTIS;++i) // write the whole 128 multis
	{
		// TODO factor out redundant code with exportMulti ??
		File<< "["<<i<<"]" <<endl; // write the multi id number
		writeMulti(File, i);
	}

	File.close(); // done

	snprintf(kommand, 2400, "%s/minicomputerMulti.txt", folder);
	printf("Saving multis to file %s\n", kommand);
	if (access(kommand, R_OK) == 0) // check if there a previous file which need to be backed up
 	{
		sprintf(kommand,"mv %s/minicomputerMulti.txt %s/minicomputerMulti.txt.bak", folder, folder);
		result=system(kommand);// make a backup of the original file
		if(result) fprintf(stderr, "mv old file to .bak failed, result: %u\n", result);
	}
	sprintf(kommand,"mv %s/minicomputerMulti.temp %s/minicomputerMulti.txt", folder, folder);
	result=system(kommand);// commit the file
	if(result) fprintf(stderr, "mv new file to .txt failed, result: %u\n", result);
}


void Memory::writeMulti(ofstream& File, unsigned int multi)
{
	int p,j;
	// TODO factor out writing
	char temp_name1[_NAMESIZE], temp_name2[_NAMESIZE];
	strnrtrim(temp_name1, multis[multi].name, _NAMESIZE);
	strnquote(temp_name2, temp_name1, _NAMESIZE);
	// printf("Writing multi %s\n", temp_name2);
	File<<temp_name2<<endl; // write the name of the multi
	for (p=0; p<_MULTITEMP; ++p) // store the sound ids and settings of all 8 voices
	{
		File<< "("<< p << ":" <<multis[multi].sound[p]<<")"<<endl;
		for (j=0;j<_MULTISETTINGS;++j)
			File<< "{"<< p << ";"<< j << ":" <<multis[multi].settings[p][j]<<"}"<<endl;
	}
	for (p=0;p<_MULTIPARMS;++p) // store the global multi parameters
	{
		File<< "<"<< p << ":" <<multis[multi].parms[p]<<">"<<endl;
	}
}
void Memory::exportMulti(string filename, unsigned int multi)
{
	printf("Exporting multi %u to file %s\n", multi, filename.c_str());
	ofstream File (filename.c_str());
	File<<"# Minicomputer v"<<_VERSION<<" single multi file"<<endl;
	// Do not record the original multi number
	writeMulti(File, multi); // Record the multi data
	for (int p=0;p<_MULTITEMP;++p) // embed the sounds parameters
	{
		unsigned int s=multis[multi].sound[p];
		// Did we already save it?
		bool found=false;
		for(int j=0; j<p; j++){
			if(multis[multi].sound[j]==s){
				found=true;
				break;
			}
		}
		if(found){
			printf("exportMulti: skipping sound %u %3u %s (already embedded)\n", p, s, sounds[s].name);
		}else{
			printf("exportMulti: embedding sound %u %3u %s\n", p, s, sounds[s].name);
			File<< "["<< s <<"]"<<endl;
			writeSound(File, s);
		}
	}
	File.close();
	printf("Export complete\n");
}
void Memory::importMulti(string filename, unsigned int multi, bool with_sounds)
{
	if(multi>=_MULTIS){
		fprintf(stderr, "ERROR: unexpected multi number %d\n", multi);
		return;
	}
	printf("Importing multi %u from file %s\n", multi, filename.c_str());

	string str;
	int iParameter, i2Parameter;
	float fValue;
	bool done=false;

	ifstream File (filename.c_str());
	getline(File,str); // get the first line from the file
	// unsigned int j;
	while (File && !done) // as long as there is anything in the file
	{
		// parse the entry (line) based on the first character
		switch (str[0])
		{
			case '(': // setting per voice multi parameter
			{
				if (parseNumbers(str, iParameter, i2Parameter, fValue))
				{
					if(iParameter<_MULTITEMP)
						multis[multi].sound[iParameter]=fValue;
					else
						fprintf(stderr, "ERROR: loadMultis - unexpected parameter number %i\n", iParameter);
				}
			}
			break;
			case '<': // setting global multi parameter
			{
				if (parseNumbers(str, iParameter, i2Parameter, fValue))
				{
					if(iParameter<_MULTIPARMS)
						multis[multi].parms[iParameter]=fValue;
					else
						fprintf(stderr, "ERROR: loadMultis - unexpected global parameter number %i\n", iParameter);
				}
			}
			break;
			case '\'': // setting the name
			{
				/*
				j = 1; // important, otherwise it would bail out at the first '
				while ((j<str.length()) && (str[j]!='\'') && (j<_NAMESIZE) )
				{
					// Allow escaping a quote with backslash
					if((j+1<str.length()) && str[j]=='\\') j++; // Will copy next char
					multis[multi].name[j-1] = str[j];
					++j;
				}
				multis[multi].name[j-1]=0;
				*/
				strnunquote(multis[multi].name, str.c_str(), _NAMESIZE);
			}
			break;
			case '{': // setting additional parameter
			{
				if (parseNumbers(str, iParameter, i2Parameter, fValue))
				{
					multis[multi].settings[iParameter][i2Parameter]=fValue;
				}
			}
			break;
			case '[': // setting embedded sound parameters
			{
				if (parseNumbers(str, iParameter, i2Parameter, fValue))
				{
					if(with_sounds){
						printf("importMulti: loading embedded sound %u\n", iParameter);
						readPatch(File, &sounds[iParameter]);
					}else{
						printf("importMulti: ignoring embedded sound(s)\n");
						done = true;
					}
				}
			}
			break;
			case '#': // Comment
				printf("loadMultis: %s\n", &str[1]);
			break;
			default:
				fprintf(stderr, "importMulti: unexpected leading character %c (%d)\n", str[0], str[0]);
		} // end of switch
		if(File) getline(File,str);// get the next line
	} // end of while (file)
	File.close();// done
}

void Memory::copyMulti(int src, int dest)
{
	multis[dest]=multis[src];
}

/**
 * load the multitimbral setups which are stored in an extrafile
 * no longer supports the deprecated binary format
 * @see Memory::saveMultis()
 */
int Memory::loadMultis()
{
	string str,sValue,sParameter;
	int iParameter,i2Parameter;
	float fValue;

	char path[1200], kommand[2400];
	snprintf(path, 1200, "%s/minicomputerMulti.txt", folder);
	printf("loadMultis: Loading %s\n", path);

	ifstream file (path);
	if(!file){
		
		// Attempt copy
		fprintf(stderr, "loadMultis: error opening file %s - attempting to copy\n", path);
		snprintf(kommand, 2400, "cp /usr/share/minicomputer/factory_presets/minicomputerMulti.txt %s", path);
		if(system(kommand)){ // Copy failed
			fprintf(stderr, "Copy to %s failed.\n", path);
			return 1;
		}
		
		// Fallback to factory settings
		fprintf(stderr, "loadMultis: error opening file - reverting to factory settings\n");
		snprintf(path, 1200, "/usr/share/minicomputer/factory_presets/minicomputerMulti.txt");
		printf("loadMultis: Loading %s\n", path);
		file.open(path);
		if(!file){
			fprintf(stderr, "loadMultis: error opening file %s\n", path);
			return(1);
		}
	}

	getline(file, str); // get the first line from the file
	int current=0;
	// unsigned int j;
	// TODO factor out common code with importMulti ??
	while (file) // as long as there is anything in the file
	{
		// reset some variables
		sParameter="";
		sValue = "";
		// parse the entry (line) based on the first character
		switch (str[0])
		{
			case '(': // setting per voice multi parameter
			{
				if (parseNumbers(str, iParameter, i2Parameter, fValue))
				{
					if(iParameter<_MULTITEMP)
						multis[current].sound[iParameter]=fValue;
					else
						fprintf(stderr, "ERROR: loadMultis - unexpected parameter number %i", iParameter);
				}
			}
			break;
			case '<': // setting global multi parameter
			{
				if (parseNumbers(str, iParameter, i2Parameter, fValue))
				{
					if(iParameter<_MULTIPARMS)
						multis[current].parms[iParameter]=fValue;
					else
						fprintf(stderr, "ERROR: loadMultis - unexpected global parameter number %i", iParameter);
				}
			}
			break;
			case '\'': // setting the name
			{
				// printf(".");
				/*
				j = 1; // important, otherwise it would bail out at the first '
				int k = 0;
				while ((j<str.length()) && (str[j]!='\'') && (j<_NAMESIZE) )
				{
					// Allow escaping a quote with backslash
					if((j+1<str.length()) && str[j]=='\\') j++; // Will copy next char
					multis[current].name[k] = str[j];
					++j; ++k;
				}
				multis[current].name[k]=0;
				*/
				strnunquote(multis[current].name, str.c_str(), _NAMESIZE);
			}
			break;
			case '[': // setting the current multi index
			{
				if (parseNumbers(str,iParameter,i2Parameter,fValue))
				{
					current = iParameter;
				}
			}
			break;
			case '{': // setting additional parameter
			{
				if (parseNumbers(str,iParameter,i2Parameter,fValue))
				{
					multis[current].settings[iParameter][i2Parameter]=fValue;
				}
			}
			break;
			case '#': // Comment in file
				printf("loadMultis: %s\n", &str[1]);
			break;
			default:
				fprintf(stderr, "ERROR: loadMultis - unexpected token '%c'", str[0]);
		} // end of switch
		getline(file, str); // get the next line
	} // end of while (file)
	file.close(); // done
	// printf("done.\n");
	return(0);
}

/**
 * @brief parse parameter and values out of a given string
 * 
 * parameters are addresses of the actual variable to return the values with it
 *
 * @param str the line to be parsed
 * @param iParameter the first integer parameter
 * @param i2Parameter the second, optional integer parameter
 * @param fValue the actual vfloat alue
 * @return bool, true if the parsing worked
 */
bool Memory::parseNumbers(string &str, int &iParameter, int &i2Parameter, float &fValue)
{
 bool rueck = false;
 if (!str.empty())// just to make sure
 {
	istringstream sStream,fStream,s2Stream;
	string sParameter="";
	string sValue="";
	string sP2="";
	unsigned int index = 0;
	bool hasValue = false, hasP2 = false, isValue=false, isP2=false;
	// first getting each digit character
	while (index<str.length())// examine character per character
	{
		if ((str[index]>='0') && (str[index]<='9'))// is it a number?
		{
			if (isValue)
			{
				sValue+=str[index];
			}
			else if (isP2)
			{
				sP2+=str[index];
			}
			else
			{	
				sParameter+=str[index];
			}
		}
		else if (str[index] == '.')
		{
			if (isValue)
			{
				sValue+='.';
			}
		}
		else if (str[index] == '-')
		{
			if (isValue)
			{
				sValue+='-';
			}
		}
		else if (str[index] == ':')
		{
			hasValue = true;
			isValue = true;
			isP2 = false;
		}
		else if (str[index] == ';')
		{
			hasP2 	= true;
			isP2	= true;
			isValue = false;
		}
		++index;// next one please
	}
//	cout << sParameter<<" sp2 "<<sP2<<" value "<<sValue<<endl;
	// now actually turn them to ints or float
	sStream.str(sParameter);

	if (sStream>>iParameter)
	{
		rueck = true;
		if (hasValue)// turn the value into a float
		{
			fStream.str(sValue);
			if (fStream>>fValue)
				rueck = true;
			else
				rueck = false;
		}
		
		if (hasP2)// turn the second parameter into an int
		{
			s2Stream.str(sP2);
			if (s2Stream>>i2Parameter)
				rueck = true;
			else
				rueck = false;
		}
	}
 }
// cout << "p1: " << iParameter << " p2: " << i2Parameter<<" value: " << fValue<<" rueck: "<<rueck<<endl;
 return rueck;
}
