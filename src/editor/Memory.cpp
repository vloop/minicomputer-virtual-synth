/** Minicomputer
 * industrial grade digital synthesizer
 * editorsoftware
 * Copyright 2007 Malte Steiner
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
#include "Memory.h"

// This function probably belongs somewhere else
char *strnrtrim(char *dest, const char*source, size_t len);

/**
 * constructor
 */
Memory::Memory()
{
	int i, result;
	for (i=0;i<_MULTITEMP;++i)
	{
		choice[i] = 0;
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
				printf("mkdir result: %u", result);

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
 * retrieve name of a certain patch
 * @param voicenumber of which we want to know the entry (should be redundant)
 * @param soundnumber
 * @return the name as string
 */
string Memory::getSoundName(unsigned int soundnum)
{
	if(soundnum<_SOUNDS) return sounds[soundnum].name;
	return(NULL);
}

string Memory::getMultiName(unsigned int multinum)
{
	if(multinum<_SOUNDS) return multis[multinum].name;
	return(NULL);
}
/**
 * set the soundnumber to a certain voice
 * @param voice number
 * @param sound number
 */
int Memory::setChoice(unsigned int voice, unsigned int sound)
{
	if (voice<_MULTITEMP && sound<_SOUNDS)
	{
		choice[voice] = sound;
		return(0);
	}
	else // oha, should not happen
	{
		fprintf(stderr, "setChoice: illegal voice %d or sound %d\n", voice, sound);
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
	// printf("%lu %lu \n", sizeof(src), sizeof(*src));
	*dest=*src;
	/*
	// Could we achieve this with memcpy?!
	strnrtrim(dest->name, src->name, _NAMESIZE);
printf("copyPatch %s\n", dest->name);
	int p,j;
	for (p=0;p<9;++p) // Write the 9 frequencies (Osc1..3 + filters 2x3)
	{
		for (j=0;j<2;++j) // ??
			dest->freq[p][j]=src->freq[p][j];
	}
	for (p=0;p<_CHOICECOUNT;++p) // Write listbox items
		dest->choice[p]=src->choice[p];
	for (p=0;p<_PARACOUNT;++p)// Write the remaining parameters
		dest->parameter[p]=src->parameter[p];
		*/
}

void Memory::copySound(int src, int dest)
{
	copyPatch(&sounds[src], &sounds[dest]);
	/*
	strncpy(sounds[dest].name, sounds[src].name, _NAMESIZE);
	int p,j;
	for (p=0;p<9;++p) // Write the 9 frequencies (Osc1..3 + filters 2x3)
	{
		for (j=0;j<2;++j) // ??
			sounds[dest].freq[p][j]=sounds[src].freq[p][j];
	}
	for (p=0;p<_CHOICECOUNT;++p) // Write listbox items
		sounds[dest].choice[p]=sounds[src].choice[p];
	for (p=0;p<_PARACOUNT;++p)// Write the remaining parameters
		sounds[dest].parameter[p]=sounds[src].parameter[p];
		*/
}
/**
 * return a sound id of a certain given voice number
 * @param the voice number
 * @return the sound number of that voice
 */
unsigned int Memory::getChoice(unsigned int voice)
{
	return choice[voice];
}
/**
 * save the complete soundmemory to disk
 * two fileformats were possible, the historical
 * but deprecated binary format was removed.
 */
void Memory::saveSounds()
{
	char kommand[2400]; // twice folder and some
	snprintf(kommand, 2400, "%s/minicomputerMemory.temp", folder);

	ofstream File (kommand); // temporary file
	int result;
	int p,j;
	File<<"# Minicomputer v"<<_VERSION<<" sounds file"<<endl;
	for (int i=0;i<_SOUNDS;++i)
	 {  
		File<< "["<<i<<"]" <<endl; // write the soundnumber

		// File<< "'"<<sounds[i].name<<"'"<<endl; // write the name
		char temp_name[_NAMESIZE];
		strnrtrim(temp_name, sounds[i].name, _NAMESIZE);
	/*
		// One-shot to discard leading numbers
		int j,k;
		// Skip up to 3 digits...
		for(j=0; j<3; j++) if(!isdigit(temp_name[j])) break;
		if(temp_name[j]==' ') j++; // ... and one space
		for(k=0; k<_NAMESIZE-j; k++) temp_name[k]=temp_name[k+j];
	*/
		File<< "'"<<temp_name<<"'"<<endl; // write the name of the multi
		for (p=0;p<9;++p) // Write the 9 frequencies (Osc1..3 + filters 2x3)
		{
			for (j=0;j<2;++j) // ?? maybe x and y components
				File<< "<"<< p << ";" << j << ":" <<sounds[i].freq[p][j]<<">"<<endl;
		}
		for (p=0;p<_CHOICECOUNT;++p) // Write listbox items
			File<< "{"<< p << ":"<<sounds[i].choice[p]<<"}"<<endl;
		for (p=0;p<_PARACOUNT;++p)// write the remaining parameters
			File<< "("<< p << ":"<<sounds[i].parameter[p]<<")"<<endl;
	 }// end of for i

	File.close();
	snprintf(kommand, 2400, "%s/minicomputerMemory.txt", folder);
	printf("Saving sounds to file %s\n", kommand);
	if (access(kommand, R_OK) == 0) // check if there a previous file which need to be backed up
	{
		sprintf(kommand,"mv %s/minicomputerMemory.txt %s/minicomputerMemory.txt.bak", folder, folder);
		result=system(kommand);// make a backup
		if(result) fprintf(stderr, "mv to .bak error, result: %u\n", result);
	}
	snprintf(kommand, 2400, "mv %s/minicomputerMemory.temp %s/minicomputerMemory.txt", folder, folder);
	result=system(kommand);// commit the file finally
	if(result) fprintf(stderr, "mv to .txt error, result: %u\n", result);
}

int Memory::loadInit()
{
	char initfile[1200];
	sprintf(initfile, "%s/initsinglesound.txt", folder);
	return(importPatch(initfile, &initSound));
}
/**
 * load the soundmemory (i.e. all preset definitions) from disk
 * supports the text format.
 * @see Memory::saveSounds()
 */
int Memory::loadSounds()
{
// new fileinput in textformat which is the way to go
char path[1200];
sprintf(path,"%s/minicomputerMemory.txt", folder);
printf("loading %s\n", path);
ifstream file (path);
if(!file){
	fprintf(stderr, "loadSounds: error opening file %s\n", path);
	return(1);
}
string str,sParameter,sValue;
float fValue;
int iParameter,i2Parameter;
int current=-1;
unsigned int j;
getline(file,str);
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
			j = 1; // important, otherwise it would bail out at the first '	
			while ((j<str.length()) && (str[j]!='\'') && (j<_NAMESIZE) )
			{
				sounds[current].name[j-1] = str[j];
				++j;
			}
			sounds[current].name[j-1]=0;
			// printf("Preset # %u : \"%s\"\n", current, sounds[current].name);
/*
			while (j<_NAMESIZE) // fill the rest with blanks to clear the string
			{
				sounds[current].name[j-1]=' ';
				++j;
			}
*/
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
return(0);
}

void Memory::writeSound(ofstream& file, unsigned int current)
{
	int p,j;

	//File<< "["<<i<<"]" <<endl; // write the soundnumber
	file<< "'"<<sounds[current].name<<"'"<<endl; // write the name
	
	for (p=0;p<9;++p)
	{
		for (j=0;j<2;++j)
			file<< "<"<< p << ";" << j << ":" <<sounds[current].freq[p][j]<<">"<<endl;
	}
	for (p=0;p<_CHOICECOUNT;++p)
		file<< "{"<< p << ":"<<sounds[current].choice[p]<<"}"<<endl;
	for (p=0;p<_PARACOUNT;++p) // write the remaining parameters
		file<< "("<< p << ":"<<sounds[current].parameter[p]<<")"<<endl;
}
/** export a single sound to a textfile
 * @param the filename
 * @param the sound memory location which is exported
 */
void Memory::exportSound(string filename, unsigned int current)
{
	printf("Exporting sound %u to file %s\n", current, filename.c_str());
	ofstream file (filename.c_str());
	file<<"# Minicomputer v"<<_VERSION<<" single sound file"<<endl;
	writeSound(file, current);
	file.close();
}

int Memory::readPatch(ifstream& File, patch *p)
{
	string str, sParameter, sValue;
	float fValue;
	int iParameter, i2Parameter;
	unsigned int j;
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
				j = 1; // important, otherwise it would bail out at the first '
				while ((j<str.length()) && (str[j]!='\'') && (j<_NAMESIZE) )
				{
					p->name[j-1] = str[j];
					++j;
				}
				p->name[j-1] = 0;
	/*
				if (j<_NAMESIZE) // fill the rest with blanks to clear the string
				{
					while (j<_NAMESIZE)
					{
						sounds[current].name[j-1]=' ';
						++j;
					}
				}
	*/
				// Another name means another sound ??
				// if(gotname)
//				gotname=true;
				printf("importPatch: reading %s\n", p->name);
			}
			break;
			case '[': // Next patch
				File.seekg(pos ,std::ios_base::beg);
				return 0;
			case '#': // Comment
				printf("importPatch: %s\n", &str[1]);
			break;
			default:
				fprintf(stderr, "importPatch: unexpected leading character %c (%d)\n", str[0], str[0]);
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
		fprintf(stderr, "importPatch: cannot open file %s\n", filename.c_str());
		return(1);
	}
	printf("importPatch: reading from %s\n", filename.c_str());
	readPatch(File, p);
	File.close();
	printf("importPatch: read complete.\n");
	return(0);
}
/** import a single sound from a textfile
 * and write it to the given memory location
 * @param the filename
 * @param the sound memory location whose parameters are about to be overwritten
 */
void Memory::importSound(string filename, unsigned int current)
{
	if(current>=_SOUNDS){
		fprintf(stderr, "ERROR: unexpected sound number %d\n", current);
		return;
	}
	printf("Importing sound %u from file %s\n", current, filename.c_str());
	importPatch(filename, &sounds[current]);
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
 * the multitemperal setup, the choice of sounds and some settings
 * are saved to an extra file
 * supports the new textformat like the sound saving
 * @see Memory::saveSounds()
 */
void Memory::saveMultis()
{
	char kommand[2400];
	int i;
	int result;
	//---------------------- new text format
	// first write in temporary file, just in case

	snprintf(kommand, 2400, "%s/minicomputerMulti.temp", folder);
	ofstream File (kommand); // temporary file
	File<<"# Minicomputer v"<<_VERSION<<" multis file"<<endl;

	int p,j;
	for (i=0;i<_MULTIS;++i) // write the whole 128 multis
	{
		File<< "["<<i<<"]" <<endl; // write the multi id number
		char temp_name[_NAMESIZE];
		strnrtrim(temp_name, multis[i].name, _NAMESIZE);
		/*
		// One-shot to discard leading numbers
		int j,k;
		strnrtrim(temp_name, multis[i].name, _NAMESIZE);
		// Skip up to 3 digits...
		for(j=0; j<3; j++) if(!isdigit(temp_name[j])) break;
		if(temp_name[j]==' ') j++; // ... and one space
		for(k=0; k<_NAMESIZE-j; k++) temp_name[k]=temp_name[k+j];
		*/
		File<< "'"<<temp_name<<"'"<<endl; // write the name of the multi

		for (p=0;p<_MULTITEMP;++p) // store the sound ids of all 8 voices
		{
			File<< "("<< p << ":" <<multis[i].sound[p]<<")"<<endl;
			for (j=0;j<_MULTISETTINGS;++j)
				File<< "{"<< p << ";"<< j << ":" <<multis[i].settings[p][j]<<"}"<<endl;
		}
		for (p=0;p<_MULTIPARMS;++p) // store the global multi parameters
		{
			File<< "<"<< p << ":" <<multis[i].parms[p]<<">"<<endl;
		}
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

void Memory::exportMulti(string filename, unsigned int multi)
{
	printf("Exporting multi %u to file %s\n", multi, filename.c_str());
	ofstream File (filename.c_str());
	File<<"# Minicomputer v"<<_VERSION<<" single multi file"<<endl;
	int p,j;
	char temp_name[_NAMESIZE];
	strnrtrim(temp_name, multis[multi].name, _NAMESIZE);
	File<< "'"<<temp_name<<"'"<<endl; // write the name of the multi
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
	for (p=0;p<_MULTITEMP;++p) // embed the sounds parameters
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

	ifstream File (filename.c_str());
	getline(File,str); // get the first line from the file
	unsigned int j;
	while (File) // as long as there is anything in the file
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
				j = 1; // important, otherwise it would bail out at the first '	
				while ((j<str.length()) && (str[j]!='\'') && (j<_NAMESIZE) )
				{
					multis[multi].name[j-1] = str[j];
					++j;
				}
				multis[multi].name[j-1]=0;
				/*
				// printf("Multi # %u : \"%s\"\n", current, multis[current].name);
				while (j<_NAMESIZE)
				{
					multis[current].name[j-1]=' ';
					++j;
				}
				*/
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
						// Ugly way to exhaust file
						while (File) getline(File,str);
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
	/*
	strncpy(multis[dest].name, multis[src].name, _NAMESIZE);
	int p,j;
	for (p=0;p<_MULTITEMP;++p) // store the sound ids and volume/midi settings of all 8 voices
	{
		multis[dest].sound[p]=multis[src].sound[p];
		for (j=0;j<_MULTISETTINGS;++j)
			multis[dest].settings[p][j]=multis[src].settings[p][j];
	}
	for (p=0;p<_MULTIPARMS;++p) // store the global multi parameters
	{
		multis[dest].parms[p]=multis[src].parms[p];
	}
	*/
}

/**
 * load the multitemperal setups which are stored in an extrafile
 * no longer supports the deprecated binary format
 * @see Memory::loadSounds()
 * @see Memory::saveSounds()
 */
int Memory::loadMultis()
{
// *********************************** the new text format **********************
string str,sValue,sParameter;
int iParameter,i2Parameter;
float fValue;

char path[1200];
sprintf(path,"%s/minicomputerMulti.txt", folder);

ifstream file (path);
if(!file){
	fprintf(stderr, "loadMultis: error opening file %s\n", path);
	return(1);
}

getline(file,str); // get the first line from the file
int current=0;
unsigned int j;
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
			j = 1; // important, otherwise it would bail out at the first '	
			while ((j<str.length()) && (str[j]!='\'') && (j<_NAMESIZE) )
			{
				multis[current].name[j-1] = str[j];
				++j;
			}
			multis[current].name[j-1]=0;
			/*
			// printf("Multi # %u : \"%s\"\n", current, multis[current].name);
			while (j<_NAMESIZE)
			{
				multis[current].name[j-1]=' ';
				++j;
			}
			*/
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
		case '{': // setting additional parameter
		{
			if (parseNumbers(str,iParameter,i2Parameter,fValue))
			{
				multis[current].settings[iParameter][i2Parameter]=fValue;
			}
		}
		break;
		case '#': // Comment
			printf("loadMultis: %s\n", &str[1]);
		break;
		default:
			fprintf(stderr, "ERROR: loadMultis - unexpected token '%c'", str[0]);
	}// end of switch
	getline(file, str);// get the next line
}// end of while (file)
file.close();// done
return(0);
}

/**
 * parse parameter and values out of a given string
 * parameters are addresses of the actual variable to return the values with it
 *
 * @param string the line
 * @param int the first parameter
 * @param int the second, optional parameter
 * @param float the actual value
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
