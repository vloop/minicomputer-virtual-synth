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
		}
		else
		{ 
			string t = getenv("HOME");
			sprintf(folder,"%s/.miniComputer",t.c_str());
			if (access(folder, R_OK) != 0)
			{
				sprintf(kommand,"mkdir %s",folder);
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
 * retreive name of a certain patch
 * @param voicenumber of which we want to know the entry (should be redundant)
 * @param soundnumber
 * @return the name as string
 */
string Memory::getName(unsigned int soundnum)
{
	if(soundnum<_SOUNDS) return sounds[soundnum].name;
	return("???");
}

string Memory::getMultiName(unsigned int multinum)
{
	if(multinum<_SOUNDS) return multis[multinum].name;
	return("???");
}
/**
 * set the soundnumber to a certain voice
 * @param voice number
 * @param sound number
 */
void Memory::setChoice(unsigned int voice, unsigned int i)
{
	if ((i>=0) && (i<_SOUNDS)) // check if its in the range
	{
		choice[voice] = i;
	}
	else // oha, should not happen
	{
		printf("illegal sound choice\n");
		fflush(stdout);
	}
}
void Memory::setName(unsigned int dest, const char *new_name){
	strncpy(sounds[dest].name, new_name, _NAMESIZE);
}

void Memory::setMultiName(unsigned int dest, const char *new_name){
	strncpy(multis[dest].name, new_name, _NAMESIZE);
}

void Memory::copypatch(patch *src, patch *dest)
{
	// Could we achieve this with memcpy?!
	strnrtrim(dest->name, src->name, _NAMESIZE);
printf("copypatch %s\n", dest->name);
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
}

void Memory::copysound(int src, int dest)
{
	copypatch(&sounds[src], &sounds[dest]);
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
void Memory::save()
{
	char kommand[1200];
	// new fileoutput as textfile with a certain coding which is
	// documented in the docs
	sprintf(kommand, "%s/minicomputerMemory.temp", folder);

	ofstream File (kommand); // temporary file
	int result;
	int p,j;
	for (int i=0;i<_SOUNDS;++i)
	 {  
		File<< "["<<i<<"]" <<endl;// write the soundnumber

		File<< "'"<<sounds[i].name<<"'"<<endl;// write the name

	/*
		// One-shot to discard leading numbers
		int j,k;
		char temp_name[_NAMESIZE];
		strnrtrim(temp_name, sounds[i].name, _NAMESIZE);
		// Skip up to 3 digits...
		for(j=0; j<3; j++) if(!isdigit(temp_name[j])) break;
		if(temp_name[j]==' ') j++; // ... and one space
		for(k=0; k<_NAMESIZE-j; k++) temp_name[k]=temp_name[k+j];
		File<< "'"<<temp_name<<"'"<<endl;// write the name
	*/
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
	sprintf(kommand,"%s/minicomputerMemory.txt",folder);
	if (access(kommand, R_OK) == 0) // check if there a previous file which need to be backed up
	{
		sprintf(kommand,"mv %s/minicomputerMemory.txt %s/minicomputerMemory.txt.bak",folder,folder);
		result=system(kommand);// make a backup
		if(result) fprintf(stderr, "mv to .bak error, result: %u\n", result);
	}
	sprintf(kommand,"mv %s/minicomputerMemory.temp %s/minicomputerMemory.txt",folder,folder);
	result=system(kommand);// commit the file finally
	if(result) fprintf(stderr, "mv to .txt error, result: %u\n", result);
}

void Memory::loadInit()
{
	char initfile[1200];
	sprintf(initfile,"%s/initsinglesound.txt", folder);
	importPatch(initfile, &initSound);
}
/**
 * load the soundmemory (i.e. all preset definitions) from disk
 * supports the text format.
 * @see Memory::save()
 */
void Memory::load()
{
// new fileinput in textformat which is the way to go
char path[1200];
sprintf(path,"%s/minicomputerMemory.txt",folder);
printf("loading %s\n",path);
ifstream File (path);

string str,sParameter,sValue;
float fValue;
int iParameter,i2Parameter;
int current=-1;
unsigned int j;
getline(File,str);
while (File)
{
	sParameter="";
	sValue = "";
	switch (str[0])
	{
		case '(':// setting parameter
		{
			if (parseNumbers(str,iParameter,i2Parameter,fValue))
			{
				sounds[current].parameter[iParameter]=fValue;
			}
		}
		break;
		case '{':// setting additional parameter
		{
			if (parseNumbers(str,iParameter,i2Parameter,fValue))
			{
				sounds[current].choice[iParameter]=(int)fValue;
			}
		}
		break;
		case '<':// setting additional parameter
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
			// printf("Preset # %u : \"%s\"\n", current, sounds[current].name);
			while (j<_NAMESIZE) // fill the rest with blanks to clear the string
			{
				sounds[current].name[j-1]=' ';
				++j;
			}
		}
		break;
		case '[':// setting the current sound index
		{
			if (parseNumbers(str,iParameter,i2Parameter,fValue))
			{
				current = iParameter;
			}
		}
		break;

	}

	getline(File, str);// get the next line of the file
/*
for (int i=0;i<512;++i)
	{
	File<< "["<<i<<"]" <<endl;
	File<< "'"<<sounds[i].name<<"'"<<endl;
	
	for (p=0;p<9;++p)
	{
		for (j=0;j<2;++j)
			File<< "<"<< p << ";" << j << ":" <<sounds[i].freq[p][j]<<">"<<endl;
	}
	for (p=0;p<17;++p)
		File<< "{"<< p << ":"<<sounds[i].choice[p]<<"}"<<endl;
	for (p=0;p<139;++p)
		File<< "("<< p << ":"<<sounds[i].parameter[p]<<")"<<endl;
	}
*/
}
File.close();
}

/** export a single sound to a textfile
 * @param the filename
 * @param the sound memory location which is exported
 */
void Memory::exportSound(string filename, unsigned int current)
{
ofstream File (filename.c_str()); // temporary file
int p,j;
	//File<< "["<<i<<"]" <<endl;// write the soundnumber
	File<< "'"<<sounds[current].name<<"'"<<endl;// write the name
	
	for (p=0;p<9;++p)
	{
		for (j=0;j<2;++j)
			File<< "<"<< p << ";" << j << ":" <<sounds[current].freq[p][j]<<">"<<endl;
	}
	for (p=0;p<_CHOICECOUNT;++p)
		File<< "{"<< p << ":"<<sounds[current].choice[p]<<"}"<<endl;
	for (p=0;p<_PARACOUNT;++p)// write the remaining parameters
		File<< "("<< p << ":"<<sounds[current].parameter[p]<<")"<<endl;

File.close();
}

void Memory::importPatch(string filename, patch *p)
{
	ifstream File (filename.c_str());
	if(!File){
		fprintf(stderr, "importPatch: cannot read from %s\n", filename.c_str());
		return;
	}
	printf("importPatch: reading from %s\n", filename.c_str());
	string str, sParameter, sValue;
	float fValue;
	int iParameter, i2Parameter;
	unsigned int j;
	getline(File,str);
	while (File)
	{
		sParameter="";
		sValue = "";
		switch (str[0])
		{
			case '(':// setting parameter
			{
				if (parseNumbers(str, iParameter, i2Parameter, fValue))
				{
					p->parameter[iParameter]=fValue;
				}
			}
			break;
			case '{':// setting additional parameter
			{
				if (parseNumbers(str, iParameter, i2Parameter, fValue))
				{
					p->choice[iParameter]=(int)fValue;
				}
			}
			break;
			case '<':// setting additional parameter
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
				printf("importPatch: reading %s\n", p->name);
			}
			break;
			case '#': // Comment
				printf("importPatch: %s\n", &str[1]);
			break;
			default:
				fprintf(stderr, "importPatch: unexpected leading character %c (%d)\n", str[0], str[0]);
		}

		getline(File,str); // get the next line of the file
	}
	File.close();
	printf("importPatch: read complete.\n");
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
	importPatch(filename, &sounds[current]);
	// now the new sound is in RAM but need to be saved to the main file
	save();
}
/**
 * the multitemperal setup, the choice of sounds and some settings
 * are saved to an extra file
 * supports the old depricated binary format and the new textformat like the sound saving
 * @see Memory::save()
 */
void Memory::saveMulti()
{
	char kommand[1200];
	int i;
	int result;
	//---------------------- new text format
	// first write in temporary file, just in case

	sprintf(kommand,"%s/minicomputerMulti.temp",folder);
	ofstream File (kommand); // temporary file

	int p,j;
	for (i=0;i<_MULTIS;++i)// write the whole 128 multis
	{
		File<< "["<<i<<"]" <<endl;// write the multi id number
		File<< "'"<<multis[i].name<<"'"<<endl;// write the name of the multi
		/*
		// One-shot to discard leading numbers
		int j,k;
		char temp_name[_NAMESIZE];
		strnrtrim(temp_name, multis[i].name, _NAMESIZE);
		// Skip up to 3 digits...
		for(j=0; j<3; j++) if(!isdigit(temp_name[j])) break;
		if(temp_name[j]==' ') j++; // ... and one space
		for(k=0; k<_NAMESIZE-j; k++) temp_name[k]=temp_name[k+j];
		File<< "'"<<temp_name<<"'"<<endl;// write the name of the multi
		*/
		
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

	File.close();// done

	sprintf(kommand,"%s/minicomputerMulti.txt",folder);
	if (access(kommand, R_OK) == 0) // check if there a previous file which need to be backed up
 	{
		sprintf(kommand,"mv %s/minicomputerMulti.txt %s/minicomputerMulti.txt.bak",folder,folder);
		result=system(kommand);// make a backup of the original file
		if(result) fprintf(stderr, "mv old file to .bak failed, result: %u\n", result);
	}
	sprintf(kommand,"mv %s/minicomputerMulti.temp %s/minicomputerMulti.txt",folder,folder);
	result=system(kommand);// commit the file
	if(result) fprintf(stderr, "mv new file to .txt failed, result: %u\n", result);
}

void Memory::copymulti(int src, int dest)
{
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
}

/**
 * load the multitemperal setups which are stored in an extrafile
 * no longer supports the deprecated binary format
 * @see Memory::load
 * @see Memory::save
 */
void Memory::loadMulti()
{
// *********************************** the new text format **********************
string str,sValue,sParameter;
int iParameter,i2Parameter;
float fValue;

char path[1200];
sprintf(path,"%s/minicomputerMulti.txt",folder);

ifstream File (path);
getline(File,str);// get the first line from the file
int current=0;
unsigned int j;
while (File)// as long as there is anything in the file
{
	// reset some variables
	sParameter="";
	sValue = "";
	// parse the entry (line) based on the first character
	switch (str[0])
	{
		case '(':// setting per voice multi parameter
		{
			if (parseNumbers(str, iParameter, i2Parameter, fValue))
			{
				if(iParameter<_MULTITEMP)
					multis[current].sound[iParameter]=(int)fValue;
				else
					fprintf(stderr, "ERROR: loadMulti - unexpected parameter number %i", iParameter);
			}
		}
		break;
		case '<':// setting global multi parameter
		{
			if (parseNumbers(str, iParameter, i2Parameter, fValue))
			{
				if(iParameter<_MULTIPARMS)
					multis[current].parms[iParameter]=(int)fValue;
				else
					fprintf(stderr, "ERROR: loadMulti - unexpected parameter number %i", iParameter);
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
			// printf("Multi # %u : \"%s\"\n", current, multis[current].name);
			while (j<_NAMESIZE)
			{
				multis[current].name[j-1]=' ';
				++j;
			}
		}
		break;
		case '[':// setting the current sound index
		{
			if (parseNumbers(str,iParameter,i2Parameter,fValue))
			{
				current = iParameter;
			}
		}
		break;
		case '{':// setting additional parameter
		{
			if (parseNumbers(str,iParameter,i2Parameter,fValue))
			{
				multis[current].settings[iParameter][i2Parameter]=fValue;
			}
		}
		break;
	}// end of switch
	getline(File,str);// get the next line
}// end of while (file)
File.close();// done
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
