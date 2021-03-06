/*! \file syntheditor.cxx
 *  \brief synth GUI editor
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

#include "syntheditor.h"

// Set default widgets look and behaviour
short MiniKnob::_defaultdragtype=MiniKnob::VERTICAL;
int MiniKnob::_defaultmargin=3;
int MiniKnob::_defaultbevel=3;
int MiniKnob::_defaultshadow=2;
int MiniKnob::_defaultbgcolor=_BGCOLOR;
int MiniKnob::_defaultcolor=0xB0B0B000;
int MiniKnob::_defaultselectioncolor=0xD0D0D000;

int MiniButton::_defaultbevel=3;
int MiniButton::_defaultshadow=0;
int MiniButton::_defaultbgcolor=_BGCOLOR;
int MiniButton::_defaultcolor=0xB0B0B000;
int MiniButton::_defaultselectioncolor=0xC1A3D000; // 0xD0D0D000;

bool alwaysSave = true;

// #define _DEBUG
// TODO: try to move globs to class variables
// Problem: some UI items need to be accessible from window resize function
static Fl_RGB_Image image_miniMini(idata_miniMini, _LOGO_WIDTH, _LOGO_HEIGHT, 3, 0);
Fl_Box* logo1Box;
Fl_Box* logo2Box;

Fl_Widget* knobs[_MULTITEMP][_PARACOUNT];
int needs_finetune[_PARACOUNT];
int needs_multi[_PARACOUNT];
int is_button[_PARACOUNT];
// Avoid copy_tooltip to spare memory management overhead
// This cost us about 150Kb, nothing compared to delay buffers...
char knob_tooltip[_MULTITEMP][_PARACOUNT][_NAMESIZE];

Fl_Choice* choices[_MULTITEMP][_CHOICECOUNT];
Fl_Value_Input* miniInput[_MULTITEMP][_MINICOUNT];
Fl_Widget* tab[_TABCOUNT];
Fl_Tabs* tabs;
static const char *voiceName[_MULTITEMP]={"1", "2", "3", "4", "5", "6", "7", "8"};
static const char *noteNames[12]={"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

/**
 * predefined menu with all modulation sources
 */
Fl_Menu_Item menu_amod []= { // UserInterface::
 {_("none"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("velocity"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("pitch bend"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("osc 1 fm out"), 0,  0, 0, 0, FL_NORMAL_LABEL , 0, 8, 0},
 {_("osc 2 fm out"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("note frequency"), 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0}, // Was osc 1
 {_("osc 2"), 0,  0, 0, FL_MENU_INVISIBLE, FL_NORMAL_LABEL, 0, 8, 0},
 {_("filter"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 1"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 2"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 3"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 4"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 5"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 6"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("modulation osc"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("touch"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("mod wheel"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("midi note"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("delay"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #1    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #2    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #3    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #4    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #5    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #6    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #7    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #8    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #9    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("damper pedal"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {0,0,0,0,0,0,0,0,0}
};
// redundant for now...
Fl_Menu_Item menu_fmod []= { // UserInterface::
 {_("none"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("velocity"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("pitch bend"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("osc 1 fm out"), 0,  0, 0, 0, FL_NORMAL_LABEL , 0, 8, 0},
 {_("osc 2 fm out"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 // {_("osc 1"), 0,  0, 0, FL_MENU_INVISIBLE, FL_NORMAL_LABEL, 0, 8, 0},
 {_("note frequency"), 0, 0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("osc 2"), 0,  0, 0, FL_MENU_INVISIBLE, FL_NORMAL_LABEL, 0, 8, 0},
 {_("filter"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 1"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 2"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 3"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 4"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 5"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("eg 6"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("modulation osc"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("touch"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("mod wheel"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("midi note"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("delay"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #1    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #2    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #3    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #4    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #5    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #6    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #7    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #8    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("cc #9    "), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("damper pedal"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {0,0,0,0,0,0,0,0,0}
};
/**
 * waveform list for menu
 */
Fl_Menu_Item menu_wave []= { // UserInterface::
 {_("sine"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("ramp up"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("ramp down"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("tri"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("square"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("bit"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("spike"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("comb"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0}, 
 {_("add Saw"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("add Square"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("Microcomp 1"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("Microcomp 8"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("Microcomp 9"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("Microcomp 10"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("Microcomp 11"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("Microcomp 12"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("Microcomp 13"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("Microcomp 14"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {0,0,0,0,0,0,0,0,0}
};

Fl_Menu_Item menu_filter []= { // UserInterface::
 {_("state variable 2p"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("state variable 4p"), 0,  0, 0, FL_MENU_INVISIBLE, FL_NORMAL_LABEL, 0, 8, 0},
 {_("bi-quad 2p"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {_("bi-quad 4p"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {0,0,0,0,0,0,0,0,0}
};


Fl_Box* voiceDisplayBox[_MULTITEMP];
Fl_Group *soundGroup;
Fl_Int_Input *soundNoInput[_MULTITEMP];
char soundNoTooltip[_MULTITEMP][_NAMESIZE];
Fl_Input *soundNameInput[_MULTITEMP];
Fl_Button *loadSoundBtn[_MULTITEMP], *soundstoreBtn[_MULTITEMP];
Fl_Button *importSoundBtn[_MULTITEMP], *exportSoundBtn[_MULTITEMP];
Fl_Button *voiceLoadBtn[_MULTITEMP];
Fl_Roller *soundRoller[_MULTITEMP];
bool sound_changed[_MULTITEMP]; // Not stored to memory or not recorded to disk ??
SoundTable *soundTable;
// Fl_Menu_Button *soundMenu;
Fl_Button *saveSoundsBtn;
Fl_Output *soundNameDisplay;
Fl_Output *soundNoDisplay[_MULTITEMP];
char soundName[_MULTITEMP][_NAMESIZE];
Fl_Input *noteInput[_MULTITEMP][3];

int auditionState[_MULTITEMP];
Fl_Toggle_Button *auditionBtn;
Fl_Button *panicBtn;
Fl_Button *clearStateBtn[_MULTITEMP];
Fl_Value_Input* parmInput;
// bool parmInputFocused=false;
Fl_Box* sounding[_MULTITEMP];

Fl_Group *multiGroup;
Fl_Int_Input *multiNoInput;
Fl_Input* multiNameInput;
Fl_Button *loadMultiBtn, *loadMultiBtn2, *multistoreBtn, *importMultiBtn, *exportMultiBtn;
Fl_Button *decMultiBtn, *incMultiBtn;
Fl_Roller* multiRoller;
bool multi_changed; // Not stored to memory or not recorded to disk ??
Fl_Menu_Button *multiMenu;
MultiTable *multiTable;
Fl_Button *saveMultisBtn;
Fl_Output *multiNameDisplay;

Fl_Value_Input* multiParmInput[_MULTIPARMS];

Fl_Group *filtersGroup[_MULTITEMP];
Fl_Group *logoGroup;

 
int currentParameter=106; // 106 is mix vol
char currentParameterName[32]="...............................";

unsigned int currentVoice=0, currentMulti=0;
int currentTab=0; // -1 means none
bool transmit;

int nGroups=0;
// Fl_Group
Fl_Widget *groups[159];
Fl_Group *midiGroups[_MULTITEMP];

patch compare_buffer;
Fl_Toggle_Button *compareBtn;

patch copy_buffer;

// This function probably belongs somewhere else
void dump_replace_color (const unsigned char * bits, int pixcount, unsigned char r1, unsigned char g1, unsigned char b1, unsigned char r2, unsigned char g2, unsigned char b2){
	printf("==========\n");
	for(int i=0; i<pixcount*3; i+=3){
		if(i%12==0) printf("\n");
		// printf("pixel %u: %u, %u, %u\n", i/3, bits[i], bits[i+1], bits[i+2]);
		// bits[3*i]=0; segfault!
		if(bits[i]==r1 && bits[i+1]==g1 && bits[i+2]==b1){
			printf("0x%02x,0x%02x,0x%02x,", r2, g2, b2);
		}else{
			printf("0x%02x,0x%02x,0x%02x,", bits[i], bits[i+1], bits[i+2]);
		}
	}
	printf("==========\n");
}

// This function probably belongs somewhere else
void replace_color (unsigned char * bits, unsigned int pixcount, unsigned char r1, unsigned char g1, unsigned char b1, unsigned char r2, unsigned char g2, unsigned char b2){
	for(unsigned int i=0; i<pixcount*3; i+=3){
		if(bits[i]==r1 && bits[i+1]==g1 && bits[i+2]==b1){
			bits[i]=r2;
			bits[i+1]=g2;
			bits[i+2]=b2;
		}
	}
}

// This function probably belongs somewhere else
void dump_frame(const unsigned char * bits, int w, int h, int src_x, int src_y, int src_w, int src_h){
	printf("==========\n");
	for (int i=0; i<src_h; i++){
		for (int j=0; j<src_w; j++){
			if(j%12==0) printf("\n");
			int k=(src_y + i)*w + src_x + j;
			k *= 3; // 3 bytes per pixel
			printf("0x%02x,0x%02x,0x%02x,", bits[k], bits[k+1], bits[k+2]);
		}
	}
	printf("==========\n");
}

// This function probably belongs somewhere else
void copy_frame(unsigned char * dest, int dest_w, int dest_h, const unsigned char * src, int src_x, int src_y, int src_w, int src_h){
	// Check that destination is totally included in source
	if (src_x+dest_w>src_w || src_y+dest_h>src_h){
		fprintf(stderr, "ERROR: copy_frame destination out of bounds\n");
		return;
	}
	for (int i=0; i<dest_h; i++){
		// Should replace inner loop by memcpy
		for (int j=0; j<dest_w; j++){
			int k=3*((src_y + i)*src_w + src_x + j);
			int l=3*(i*dest_w+j);
			dest[l]=src[k];
			dest[l+1]=src[k+1];
			dest[l+2]=src[k+2];
		}
	}
}

// These functions may belong somewhere else
int noteNo(const char *noteName){
	char c;
	int i, n;
	c=toupper(noteName[0]);
	switch(c){
		case 'A':{
			n=9;
			break;
		}
		case 'B':{
			n=11;
			break;
		}
		case 'C':{
			n=0;
			break;
		}
		case 'D':{
			n=2;
			break;
		}
		case 'E':{
			n=4;
			break;
		}
		case 'F':{
			n=5;
			break;
		}
		case 'G':{
			n=7;
			break;
		}
		default:
			return(-1);
	}
	i=1;
	if(noteName[i]=='#'){
		n++;
		i++;
	}else if(noteName[i]=='b'){
		n--;
		i++;
	}
	c=noteName[i++];
	if(c<'0' or c>'9') return(-1);
	if(c=='1' && noteName[i]=='0'){
		n+=120;
		i++;
	}else{
		n+=12*(c-'0');
	}
	if(noteName[i]!=0) return(-1);
	if( n > 127) return(-1);
	return(n);
}
char * noteName(unsigned int n, char *name){
	if(n > 127) return(NULL);
	const char *s=noteNames[n%12];
	int octave=n/12;
	sprintf(name, "%s%d", s, octave);
	return(name);
}

static void setsound_changed(int voice=-1){
// printf("setsound_changed %d\n", voice);
	if (voice==-1) voice=currentVoice;
	if(!sound_changed[voice]){
		sound_changed[voice]=true;
		soundNameInput[voice]->textcolor(FL_RED);
		soundNameInput[voice]->redraw();
	}
}
static void clearsound_changed(int voice=-1){
// printf("clearsound_changed %d\n", voice);
	if (voice==-1) voice=currentVoice;
	if(sound_changed[voice]){
		sound_changed[voice]=false;
		soundNameInput[voice]->textcolor(FL_BLACK);
		soundNameInput[voice]->redraw();
	}
}
static void setmulti_changed(){
// printf("setmulti_changed\n");
	if(!multi_changed){
		multi_changed=true;
		multiNameInput->textcolor(FL_RED);
		multiNameInput->redraw();
	}
}
static void clearmulti_changed(){
// printf("clearmulti_changed\n");
	if(multi_changed){
		multi_changed=false;
		multiNameInput->textcolor(FL_BLACK);
		multiNameInput->redraw();
	}
}

static void choiceSet(Fl_Widget* o, void*)
{
#ifdef _DEBUG
	printf("choiceSet voice %u, parameter %li, value %u\n", currentVoice, ((Fl_Choice*)o)->argument(), ((Fl_Choice*)o)->value());
#endif
	if (transmit) lo_send(t, "/Minicomputer/choice", "iii", currentVoice, ((Fl_Choice*)o)->argument(), ((Fl_Choice*)o)->value());
}

static void choiceCallback(Fl_Widget* o, void*)
{
	if(o){
	#ifdef _DEBUG
		printf("choiceCallback voice %u, parameter %li, value %u\n", currentVoice, ((Fl_Choice*)o)->argument(), ((Fl_Choice*)o)->value());
	#endif
	//	parmInputFocused=false;
		parmInput->hide(); // Possibly redundant
		if (transmit) lo_send(t, "/Minicomputer/choice", "iii", currentVoice, ((Fl_Choice*)o)->argument(), ((Fl_Choice*)o)->value());
		setsound_changed(); // All choices relate to the sound, none to the multi
	}else{
		fprintf(stderr, "choiceCallback: undefined widget.\n");
	}
}

static void do_audition(Fl_Widget* o, void*)
{
	if (transmit){
		Fl::lock();
		int note=((Fl_Valuator*)knobs[currentVoice][127])->value();
		int velocity=((Fl_Valuator*)knobs[currentVoice][126])->value();
		auditionState[currentVoice]=((Fl_Toggle_Button*)o)->value();
		if ( ((Fl_Toggle_Button*)o)->value()){
			lo_send(t, "/Minicomputer/audition", "iiii", 1, currentVoice, note, velocity);
		}else{
			lo_send(t, "/Minicomputer/audition", "iiii", 0, currentVoice, note, velocity);
		}
		Fl::awake();
		Fl::unlock();
	}
}
static void do_all_off(Fl_Widget* o, void*)
{
	int i;
	// make sure audition is off
	auditionBtn->clear();
	if (transmit) for (i=0; i<_MULTITEMP; i++){
	  lo_send(t, "/Minicomputer/midi", "iiii", 0, 0xB0+i, 120, 0);
	  auditionState[i]=((Fl_Toggle_Button*)auditionBtn)->value();
	}
}

int EG_changed[_MULTITEMP];
int EG_stage[_MULTITEMP][_EGCOUNT];
int EG_parm_num[7]={102, 60, 65, 70, 75, 80, 85}; // First parameter for each EG (attack)
// ADSR controls are required to be numbered sequentially, i.e. if A is 60, D is 61...

int EG_draw(unsigned int voice, unsigned int EGnum, unsigned int stage){
	if ((voice < _MULTITEMP) && (EGnum < _EGCOUNT ))
	{
		// Schedule redraw for next run of timer_handler
		EG_changed[voice]=1;
		EG_stage[voice][EGnum]=stage;
		if(EGnum==0 && stage == 0){
			auditionState[voice]=0;
			if(voice==currentVoice)
				((Fl_Toggle_Button*)auditionBtn)->value(0);
		}
		return 0;
	}
	return 1;
}

static void tabCallback(Fl_Widget* o, void* );
static void parmCallback(Fl_Widget* o, void*);

void EG_draw_all(){
	// Refresh EG label lights
	// Called by timer_handler
	Fl::lock();
	int voice, EGnum, stage;
	for(voice=0; voice<_MULTITEMP; voice++){
		if (EG_changed[voice]){
			EG_changed[voice]=0;
			sounding[voice]->color(EG_stage[voice][0]?FL_RED:FL_DARK3);
			sounding[voice]->redraw();
			// Can't find a way to actually refresh the tab label color
			// tab[voice]->labelcolor(EG_stage[voice][0]>0?FL_RED:FL_FOREGROUND_COLOR); // Ok
			// tab[voice]->color(EG_stage[voice][0]>0?FL_RED:FL_BACKGROUND_COLOR); // Ok
			// tab[voice]->selection_color(EG_stage[voice][0]>0?FL_RED:FL_BACKGROUND_COLOR); // Ok
			// tab[voice]->damage(0xFF); // No Fl::DAMAGE_ALL ? // Doesn't help
			// tab[voice]->set_changed(); // Doesn't help
			// tab[voice]->redraw_label(); // effective only after focus change ??
			// tabs->set_changed(); // Doesn't help
			// tabCallback(tabs,NULL); // KO
			// tabs->do_callback(tabs,NULL); // KO
			// tab[voice]->redraw();
			// tab[voice]->draw_label(); // Protected
			// tabs->redraw_label(); // Doesn't help
			// tabs->redraw(); // Doesn't help
			for(EGnum=0; EGnum<_EGCOUNT; EGnum++){
				// Stages: Idle -> 0, Attack -> 1, Decay -> 2, Sustain -> 3, Release -> 4
				for(stage=1; stage<5; stage++){ // for A D S R
					knobs[voice][EG_parm_num[EGnum]+stage-1]->labelcolor(EG_stage[voice][EGnum]==stage?FL_RED:FL_FOREGROUND_COLOR);
					knobs[voice][EG_parm_num[EGnum]+stage-1]->redraw_label();
				}
			}
		}
	}
	// Fl::check(); // Doesn't help - tab label not redrawn
	Fl::awake();
	Fl::unlock();

}

/* // not good:
static void changemulti(Fl_Widget* o, void*)
{
	if (multiNameInput != NULL)
	{
		int t = multiNameInput->menubutton()->value();
		if ((t!=currentMulti) && (t>-1) && (t<128))
		{
			// ok, we are on somewhere other multi so we need to copy the settings
		//	for (int i=0;i<8;++i)
		//	{
		//		Speicher.multis[t].sound[i] = Speicher.multis[currentMulti].sound[i];
		//	}
			currentMulti = t;
		}
	}
	else
		printf("problems with the multichoice widgets!\n");
}
*/
static void clearstateCallback(Fl_Widget* o, void* )
{
	lo_send(t, "/Minicomputer", "iif", currentVoice, 0, 0.0f);
}
/**
 * Callback when a tab is chosen
 * @param o the calling widget
 */
static void tabCallback(Fl_Widget* o, void* )
{
// Fl::lock();
	Fl_Widget* e =((Fl_Tabs*)o)->value();
	currentTab=-1;
	for (int i=0; i<_TABCOUNT; i++){
		if (e==tab[i]){ 
			currentTab=i;
			break;
		}
	}
	if(currentTab==-1){
		fprintf(stderr, "tabCallback: tab not found");
		return;
	}

    if (currentTab==9){
		Fl::focus(soundTable);
		soundNameDisplay->value(Speicher.getSoundName(soundTable->selected_index()).c_str());
		soundNameDisplay->redraw();
	}
    if (currentTab==10){
		Fl::focus(multiTable);
		multiNameDisplay->value(Speicher.getMultiName(multiTable->selected_index()).c_str());
		multiNameDisplay->redraw();
	}
	if (currentTab==9 || currentTab==10) // The "Sounds" and "Multis" tabs
	{
		panicBtn->hide();
		logoGroup->hide();
	}else{
		panicBtn->show();
		logoGroup->show();
	}
	if (currentTab>=9 && currentTab<=11) // The "Sounds", "Multis" and "About" tabs
	{
		multiGroup->hide();
	}else{
		multiGroup->show();
	}
	if (currentTab<_MULTITEMP)  // Voice tabs
	{
		currentVoice=currentTab;
		// parmCallback may set sound/multi changed, clear again if needed
		bool sc=sound_changed[currentVoice];
		bool mc=multi_changed;
		parmCallback(knobs[currentVoice][currentParameter], NULL);
		if(!sc) clearsound_changed(currentVoice);
		if(!mc) clearmulti_changed();
		parmInput->show();
		auditionBtn->show();
		((Fl_Toggle_Button*)auditionBtn)->value(auditionState[currentVoice]);
		compareBtn->show();
#ifdef _DEBUG
		printf("tabCallback: sound %i\n", currentVoice );
		fflush(stdout);
#endif
	}
	else // The "midi", "Sounds, "Multis" and "About" tabs
	{
		// The controls below are displayed only on the voice tabs
		if (parmInput != NULL)
			parmInput->hide();
		else
			printf("there seems to be something wrong with parmInput widget");
		if (auditionBtn != NULL)
			auditionBtn->hide();
		else
			printf("there seems to be something wrong with audition widget");
		if (compareBtn!= NULL)
			compareBtn->hide();
		else
			printf("there seems to be something wrong with the compare button widget");
 	} // end of else
//	fl_cursor(FL_CURSOR_DEFAULT, FL_WHITE, FL_BLACK);
// Fl::awake();
// Fl::unlock();
}

static void parmInput_flash(void *userdata){
//	Fl::lock();
	parmInput->labelcolor(FL_BLACK);
	parmInput->redraw_label();
//	Fl::awake();
//	Fl::unlock();
}

/**
 * parmSet, quick set and send parameter value
 *
 * used by parmCallback
 * will *not* set changed or parmInput
 * Beware! This refers to current voice
 * @param o the calling widget
 */
static void parmSet(Fl_Widget* o, void*) {
	if (o){
		// Fl::lock();
		currentParameter = o->argument();
#ifdef _DEBUG
		printf("parmSet parameter %u\n", currentParameter);
#endif
		float val;
		if(is_button[currentParameter]){
			if(currentParameter==2){ // Fixed frequency button Osc 1
				if (((Fl_Button *)o)->value())
				{
					knobs[currentVoice][1]->show();
					knobs[currentVoice][3]->hide();
					miniInput[currentVoice][0]->show();
					miniInput[currentVoice][1]->hide();
				}
				else
				{
					knobs[currentVoice][1]->hide();
					knobs[currentVoice][3]->show();
					miniInput[currentVoice][0]->hide();
					miniInput[currentVoice][1]->show();
				}
			}else if(currentParameter==17){ // Fixed frequency button Osc 2
				if (((Fl_Button *)o)->value())
				{
					knobs[currentVoice][16]->show();
					knobs[currentVoice][18]->hide();
					miniInput[currentVoice][2]->show();
					miniInput[currentVoice][3]->hide();
				}
				else
				{
					knobs[currentVoice][16]->hide();
					knobs[currentVoice][18]->show();
					miniInput[currentVoice][2]->hide();
					miniInput[currentVoice][3]->show();
				}
			}else if(currentParameter==137){ // Filter bypass
				if (((Fl_Button *)o)->value())
				{
					filtersGroup[currentVoice]->deactivate();
				}else{
					filtersGroup[currentVoice]->activate();
				}
				// Redraw knobs above groups border
				knobs[currentVoice][14]->redraw();
				knobs[currentVoice][29]->redraw();
				knobs[currentVoice][138]->redraw();
				knobs[currentVoice][141]->redraw();
				knobs[currentVoice][148]->redraw();
			}
			if(currentParameter==4 || currentParameter==19){ // Boost buttons, transmit 1 or 100
				val=((Fl_Button*)o)->value()?100.0f:1.0f;
			}else { // Regular buttons, transmit plain value as float
				val=((Fl_Button*)o)->value()?1.0f:0.0f;
			}
			if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, currentParameter, val);
#ifdef _DEBUG
			printf("parmSet button %i : %u --> %f\n", currentParameter, ((Fl_Button*)o)->value(), val);
#endif
		}else{ // Not a button
			char knob_tooltip_template[]="%f"; // May override below
			float knob_tooltip_value;
			// Beware not all controls are valuators !
			if(needs_finetune[currentParameter]==2){ // Positioner
				val=((Fl_Positioner*)o)->xvalue() + ((Fl_Positioner*)o)->yvalue();
			}else{
				val=((Fl_Valuator*)o)->value();
			}
			knob_tooltip_value=val; // May override below
			switch (currentParameter)
			{
				case 1: // Osc 1 fixed frequency dial
				{
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
					miniInput[currentVoice][0]->value(val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
			#endif
					break;
				}
				case 3: // Osc 1 tune
				{
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
					miniInput[currentVoice][1]->value(val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
			#endif
				break;
				}
				case 16: // Osc 2 fixed frequency dial
				{
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
					miniInput[currentVoice][2]->value(val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
			#endif
					break;
				}
				case 18: // Osc 2 tune
				{ 
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
					miniInput[currentVoice][3]->value(val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
			#endif
				break;
				}
				// Envelope parameters except sustain
				case 60:
				case 61:
				case 63:
				case 65:
				case 66: 
				case 68:

				case 70:
				case 71:
				case 73:
				case 75:
				case 76: 
				case 78:

				case 80:
				case 81:
				case 83:
				case 85:
				case 86: 
				case 88:

				case 102:
				case 103:
				case 105:
				{
					float tr=val; // 0.5..0.01
					tr*= tr*tr/2.f; // 0.125..0.000001
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), tr);
			#ifdef _DEBUG
					printf("parmSet eg %li : %g\n", o->argument(), tr);
			#endif
					break;
				}
				case 116: // Glide
				{
					float tr=val; // 0..1
					// .9999 is slow .99995 is slower, must stay <1
					tr= 1-pow(10, -tr*5);
					if(tr>=1) tr = 0.99995f;
					if(tr<0) tr = 0.f;
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), tr);
			#ifdef _DEBUG
					printf("parmSet glide %li : %f --> %f \n", o->argument(), ((Fl_Valuator*)o)->value(), tr);
			#endif
					break;
				}

				//************************************ filter cuts *****************************
				case 30:{	
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
			#endif
					miniInput[currentVoice][4]->value(val);
					break;
				}
				case 33:{	
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
			#endif		
					miniInput[currentVoice][5]->value(val);
					break;
				}
				case 40:{	
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
			#endif
					miniInput[currentVoice][6]->value(val);
					break;
				}
				case 43:{	
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
			#endif
					miniInput[currentVoice][7]->value(val);
					break;
				}
				case 50: // Filter 3 a cut
				{
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
					fflush(stdout);
			#endif
					miniInput[currentVoice][8]->value(val);
					break;
				}
				case 53: // Filter 3 b cut
				{	
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
			#endif
					miniInput[currentVoice][9]->value(val);
					break;
				}
				case 90: // OSC 3 tune
				{	
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
			#endif
					miniInput[currentVoice][10]->value(val);
					miniInput[currentVoice][11]->value(round(val*6000)/100); // BPM
					break;
				}
				case 111: // Delay time
				{	float bpm=0.0f;
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
			#ifdef _DEBUG
					printf("parmSet %li : %g\n", o->argument(), val);
			#endif
					if (val!=0.0f) bpm=round(6000/val)/100;
					miniInput[currentVoice][12]->value(bpm); // BPM
					break;
				}
				default:
				{
					if (transmit) lo_send(t, "/Minicomputer", "iif", currentVoice, o->argument(), val);
			#ifdef _DEBUG
					printf("parmSet default %li : %g\n", o->argument(), val);
			#endif
					break;
				}
			} // end of switch
            snprintf(knob_tooltip[currentVoice][o->argument()], _NAMESIZE, knob_tooltip_template, knob_tooltip_value);
            o->tooltip(knob_tooltip[currentVoice][o->argument()]);
			// printf("parmSet tooltip %li : %g\n", o->argument(),((Fl_Valuator*)o)->value());
		} // end of if is_button
#ifdef _DEBUG
		fflush(stdout);
#endif
//		Fl::awake();
//		Fl::unlock();
	} // end of o != NULL
} // end of parmSet

/**
 * main parameter callback, called whenever a parameter has changed
 * @param o the calling widget
 */
static void parmCallback(Fl_Widget* o, void*) {
	if (o){
//		Fl::lock();
		currentParameter = o->argument();
		// printf("parm %u changed %u\n", currentParameter, o->changed());
		if (o->changed()){
			if(needs_multi[currentParameter]){
				setmulti_changed();
			}else{
				setsound_changed();
			}
		}

		// show parameter on fine tune only when relevant (not a frequency...)
		// printf("current %u %lu parminput %u %lu focus %u %lu\n", o, o->argument(), parmInput, parmInput->argument(), Fl::focus(), Fl::focus()->argument());
		if(needs_finetune[currentParameter] && (Fl::focus() == o)){
			if(needs_finetune[currentParameter] == 2){ // MiniPositioner
				// printf("x %f y %f\n", ((MiniPositioner*)o)->xvalue(), ((MiniPositioner*)o)->yvalue());
				// printf("min x %f y %f\n", ((MiniPositioner*)o)->xminimum(), ((MiniPositioner*)o)->yminimum());
				// printf("max x %f y %f\n", ((MiniPositioner*)o)->xmaximum(), ((MiniPositioner*)o)->ymaximum());
				// Assume y is reversed (min>max) ??
				parmInput->value(((MiniPositioner*)o)->xvalue()+((MiniPositioner*)o)->yvalue());
				parmInput->minimum(((MiniPositioner*)o)->xminimum()+((MiniPositioner*)o)->ymaximum());
				parmInput->maximum(((MiniPositioner*)o)->xmaximum()+((MiniPositioner*)o)->yminimum());
				// printf("min %f max %f\n", parmInput->minimum(), parmInput->maximum());
			}else{
				parmInput->value(((Fl_Valuator*)o)->value());
				parmInput->minimum(((Fl_Valuator*)o)->minimum());
				parmInput->maximum(((Fl_Valuator*)o)->maximum());
			}
			snprintf(currentParameterName, 32, "%31s", o->label());
			parmInput->label(currentParameterName);
			parmInput->labelcolor(FL_YELLOW);
			Fl::add_timeout(0.5, parmInput_flash, 0);
			parmInput->show();
			// printf("parm %u value %u\n", currentParameter, parmInput->value());
		}else{
			// if (Fl::focus() != parmInput)
			if (Fl::focus() && (Fl_Widget*)Fl::focus()->argument() != parmInput) // Don't ask why
			// if(!parmInputFocused && !needs_finetune[currentParameter])
				parmInput->hide();
		}

		parmSet(o, NULL); // Do the actual handling
		parmInput->tooltip(o->tooltip()); // Does this really help the user?

//		Fl::awake();
//		Fl::unlock();
	}
}

static void midiparmCallback(Fl_Widget* o, void*) {
	int voice = (((Fl_Valuator*)o)->argument()) >> 8;
	int parm = (((Fl_Valuator*)o)->argument()) & 0xFF;
	double val = ((Fl_Valuator*)o)->value();
	val = min(val,((Fl_Valuator*)o)->maximum());
	val = max(val,((Fl_Valuator*)o)->minimum());
	((Fl_Valuator*)o)->value(val);
	if (transmit) lo_send(t, "/Minicomputer", "iif", voice, parm, val);
#ifdef _DEBUG
	printf("midiparmCallback %u %u (%li) : %g\n", voice, parm, ((Fl_Valuator*)o)->argument(), val);
#endif
	if(parm >= 127 && parm <=129){ // Notes (test, min, max)
		char s[]="....";
		noteName(val, s);
		// printf("%u %s\n", voice, s);
		noteInput[voice][parm-127]->value(s); // Display as note
	}
	setmulti_changed(); // All midi parameters belong to the multi
}
static void midibuttonCallback(Fl_Widget* o, void*) {
	int voice = (((Fl_Button*)o)->argument()) >> 8;
	int parm = (((Fl_Button*)o)->argument()) & 0xFF;
	double val = ((Fl_Button*)o)->value();
	if (transmit) lo_send(t, "/Minicomputer", "iif", voice, parm, val);
#ifdef _DEBUG
	printf("midibuttonCallback %u %u (%li) : %g\n", voice, parm, ((Fl_Button*)o)->argument(), val);
#endif
	if(val){
		midiGroups[voice+1]->deactivate();
	}else{
		midiGroups[voice+1]->activate();
	}
	setmulti_changed(); // All midi parameters belong to the multi
}
static void checkrangeCallback(Fl_Widget* o, void*) {
	double val=((Fl_Valuator*)o)->value();
	val=min(val,((Fl_Valuator*)o)->maximum());
	val=max(val,((Fl_Valuator*)o)->minimum());
	((Fl_Valuator*)o)->value(val);
}
static void setNoteCallback(Fl_Widget* o, void*) {
    int n=noteNo(((Fl_Input*)o)->value());
    if(n<0){
        // ((Fl_Value_Input* )o)->textcolor(FL_RED);
        ((Fl_Value_Input* )o)->color(FL_RED);
        ((Fl_Value_Input* )o)->redraw();
        // printf("noteNo %d out of range\n", n);
        return;
    }
    // ((Fl_Value_Input* )o)->textcolor(FL_BLACK);
    ((Fl_Value_Input* )o)->color(FL_WHITE);
    ((Fl_Value_Input* )o)->redraw();
    int voice=(((Fl_Input*)o)->argument())>>8;
    int knobNo=(((Fl_Input*)o)->argument()) & 0xFF;
    // printf("noteNo %d voice %d knobNo %d\n", n, voice, knobNo);
    if(((Fl_Valuator*)knobs[voice][knobNo])->value()!=n){
        ((Fl_Valuator*)knobs[voice][knobNo])->value(n);
        midiparmCallback(knobs[voice][knobNo],NULL);
    }
}

/* *
 * copybutton parmCallback, called whenever the user wants to copy filterparameters
 * @param o the calling widget
 */
/*
static void copyparmCallback(Fl_Widget* o, void*) {

	int filter = ((Fl_Valuator*)o)->argument();// what to copy there
	switch (filter)
	{
	case 21:
	{
		
		((Fl_Valuator* )knobs[currentVoice][33])->value(((Fl_Valuator* )knobs[currentVoice][30])->value());
		parmCallback(knobs[currentVoice][33],NULL);
		((Fl_Valuator* )knobs[currentVoice][34])->value(((Fl_Valuator* )knobs[currentVoice][31])->value());
		parmCallback(knobs[currentVoice][34],NULL);
		((Fl_Valuator* )knobs[currentVoice][35])->value(((Fl_Valuator* )knobs[currentVoice][32])->value());
		parmCallback(knobs[currentVoice][35],NULL);
	}
	break;
	}
} */

/**
 * parmCallback for finetuning the current parameter
 * @param o the calling widget
 */
static void parminputCallback(Fl_Widget* o, void*)
{
	if (currentParameter<_PARACOUNT) // range check
	{
		if(needs_finetune[currentParameter]){
			// Fl::lock();
#ifdef _DEBUG
			printf("parminputCallback voice %u parameter %u", currentVoice, currentParameter);
#endif
			double val=((Fl_Valuator* )o)->value();
			double val_min = ((Fl_Valuator* )o)->minimum();
			double val_max = ((Fl_Valuator* )o)->maximum();
			if(val_max<val_min){ // Reversed range
				val_max=val_min;
				val_min=((Fl_Valuator* )o)->maximum();
			}
#ifdef _DEBUG
			printf("=%f [%f..%f]\n", val, val_min, val_max);
#endif
			if(val<=val_max && val>=val_min){
				((Fl_Value_Input* )o)->textcolor(FL_BLACK);
				if(needs_finetune[currentParameter]==2){ // MiniPositioner
					float x, y, ymax;
					// y is reversed (min>max) ?? Should handle non reversed too
					// How should we handle ymaximum() != 0 ?
					ymax=((MiniPositioner* )knobs[currentVoice][currentParameter])->yminimum();
					x = (int)(val/ymax)*ymax;
					y = val-x;
					// printf("x %f y %f ymax %f\n", x, y, ymax);
					((MiniPositioner* )knobs[currentVoice][currentParameter])->xvalue(x);
					((MiniPositioner* )knobs[currentVoice][currentParameter])->yvalue(y);
				}else{ // Any regular valuator
					((Fl_Valuator* )knobs[currentVoice][currentParameter])->value(val);
				}
				if(o->changed()) knobs[currentVoice][currentParameter]->set_changed();
				parmCallback(knobs[currentVoice][currentParameter], NULL);
			}else{
				((Fl_Value_Input* )o)->textcolor(FL_RED);
			}
			// Fl::awake();
			// Fl::unlock();
		}else{
#ifdef _DEBUG
			printf("parminputCallback - ignoring %i\n", currentParameter);
#endif
			parmInput->hide();
		}
	}else{
		fprintf(stderr, "parminputCallback - Error: unexpected current parameter %i\n", currentParameter);
	}
}

/** parmCallback when a cutoff has changed
 * the following two parmCallbacks are specialized
 * for the Positioner widget which is 2 dimensional
 * @param o the calling widget
 */
static void cutoffCallback(Fl_Widget* o, void*)
{
	// Fl::lock();
	float f = ((Fl_Valuator* )o)->value();
	if(f<=(((Fl_Valuator* )o)->maximum()) && f>=(((Fl_Valuator* )o)->minimum())){
		((Fl_Value_Input* )o)->textcolor(FL_BLACK);
		int Faktor = ((int)(f/500)*500);
		float Rem = f-Faktor;
		int Argument = ((Fl_Valuator* )o)->argument();
		((MiniPositioner* )knobs[currentVoice][Argument])->xvalue(Faktor);
		((MiniPositioner* )knobs[currentVoice][Argument])->yvalue(Rem);
		parmCallback(knobs[currentVoice][Argument],NULL);
	}else{
		((Fl_Value_Input* )o)->textcolor(FL_RED);
	}
	// Fl::awake();
	// Fl::unlock();
}
/** parmCallback for frequency positioners in the oscillators
 * which are to be treated a bit differently
 *
 * @param o the calling widget
 */
static void tuneCallback(Fl_Widget* o, void*)
{
	// Fl::lock();
	float f = ((Fl_Valuator* )o)->value();
	if(f<=(((Fl_Valuator* )o)->maximum()) && f>=(((Fl_Valuator* )o)->minimum())){
		((Fl_Value_Input* )o)->textcolor(FL_BLACK);
		int Faktor = (int)f;
		float Rem = f-Faktor;
		int Argument = ((Fl_Valuator* )o)->argument();
		((MiniPositioner* )knobs[currentVoice][Argument])->xvalue(Faktor);
		((MiniPositioner* )knobs[currentVoice][Argument])->yvalue(Rem);
		if(o->changed()) knobs[currentVoice][Argument]->set_changed();
		parmCallback(knobs[currentVoice][Argument],NULL);
	}else{
		((Fl_Value_Input* )o)->textcolor(FL_RED);
	}
	// Fl::awake();
	// Fl::unlock();
}

static void fixedfrequencyCallback(Fl_Widget* o, void*)
{
	// Fl::lock();
	float f = ((Fl_Valuator* )o)->value();
	// No conversion needed
	if(f<=(((Fl_Valuator* )o)->maximum()) && f>=(((Fl_Valuator* )o)->minimum())){
		((Fl_Value_Input* )o)->textcolor(FL_BLACK);
		int Argument = ((Fl_Valuator* )o)->argument();
		((Fl_Valuator*)knobs[currentVoice][Argument])->value(f);
		if(o->changed()) knobs[currentVoice][Argument]->set_changed();
		parmCallback(knobs[currentVoice][Argument], NULL);
	}else{
		((Fl_Value_Input* )o)->textcolor(FL_RED);
	}
	// Fl::awake();
	// Fl::unlock();
}

static void BPMtuneCallback(Fl_Widget* o, void*)
{ // Used for BPM to tune conversion
// like for osc 3 tune (90)
	// Fl::lock();
	float f = ((Fl_Valuator* )o)->value();
	if(f<=(((Fl_Valuator* )o)->maximum()) && f>=(((Fl_Valuator* )o)->minimum())){
		((Fl_Value_Input* )o)->textcolor(FL_BLACK);
		f=f/60.f;
		int Faktor = (int)f;
		float Rem = f-Faktor;
		int Argument = ((Fl_Valuator* )o)->argument();
		((MiniPositioner* )knobs[currentVoice][Argument])->xvalue(Faktor);
		((MiniPositioner* )knobs[currentVoice][Argument])->yvalue(Rem);
		if(o->changed()) knobs[currentVoice][Argument]->set_changed();
		parmCallback(knobs[currentVoice][Argument], NULL);
	}else{
		((Fl_Value_Input* )o)->textcolor(FL_RED);
	}
	// Fl::awake();
	// Fl::unlock();
}

static void BPMtimeCallback(Fl_Widget* o, void*)
{ // Used for BPM to time conversion
// like for delay time (111)
// In this case we already know minimum()<maximum()
	// Fl::lock();
	float f = ((Fl_Valuator* )o)->value();
	/*
	f=(f<((Fl_Valuator*)o)->minimum())?(((Fl_Valuator* )o)->minimum()):f;
	f=(f>((Fl_Valuator*)o)->maximum())?(((Fl_Valuator* )o)->maximum()):f;
	*/
	if(f<=(((Fl_Valuator* )o)->maximum()) && f>=(((Fl_Valuator* )o)->minimum())){
		((Fl_Value_Input* )o)->textcolor(FL_BLACK);
		if(f!=0) f=60.f/f; // else f remains 0
		int argument = ((Fl_Valuator* )o)->argument();
		((Fl_Valuator*)knobs[currentVoice][argument])->value(f);
		if(o->changed()) knobs[currentVoice][argument]->set_changed();
		parmCallback(knobs[currentVoice][argument], NULL);
	}else{
		((Fl_Value_Input* )o)->textcolor(FL_RED);
	}
	// Fl::awake();
	// Fl::unlock();
}

/**
 * multiParmInputCallback for global and per note fine tuning
 * @param o the calling widget
 */
static void multiParmInputCallback(Fl_Widget* o, void*)
{
	// Fl::lock();
	int argument = ((Fl_Valuator* )o)->argument();
	if(argument<_MULTIPARMS){
		double value = ((Fl_Valuator*)o)->value();
		value=min(value, max(((Fl_Valuator*)o)->minimum(), ((Fl_Valuator*)o)->maximum()));
		value=max(value, min(((Fl_Valuator*)o)->minimum(), ((Fl_Valuator*)o)->maximum()));
		((Fl_Valuator*)o)->value(value);
		// printf("multiParmInputCallback %u %f\n", argument, value);
		if (transmit) lo_send(t, "/Minicomputer/multiparm", "if", argument, value);
	}else{
		fprintf(stderr, "ERROR: multiParmInputCallback - unexpected argument %i\n", argument);
	}
	setmulti_changed();
	// Fl::awake();
	// Fl::unlock();
}

static void soundRollerCallback(Fl_Widget* o, void*)
{
//	Fl::lock();
	int soundnum = (int)((Fl_Valuator* )o)->value();
	soundNameInput[currentVoice]->value((Speicher.getSoundName(soundnum)).c_str());
	soundNameInput[currentVoice]->position(0);
	char ssoundnum[]="***";
	snprintf(ssoundnum, 4, "%i", soundnum);
	soundNoInput[currentVoice]->value(ssoundnum);// set gui
	snprintf(soundNoTooltip[currentVoice], _NAMESIZE, _("Bank %u program %u"), soundnum/128, soundnum%128);
	soundNoInput[currentVoice]->tooltip(soundNoTooltip[currentVoice]);
	setsound_changed();
//	Fl::awake();
//	Fl::unlock();
}
static void soundincdecCallback(Fl_Widget* o, void*)
{
	// Fl::lock();
	int soundnum=soundRoller[currentVoice]->value() + o->argument();
	if(soundnum<0) soundnum=0;
	if(soundnum>511) soundnum=511;
	// printf("soundincdecCallback sound %+ld # %u\n", o->argument(), soundnum);
	soundRoller[currentVoice]->value(soundnum);
	// Fl::awake();
	// Fl::unlock();
	soundRollerCallback(soundRoller[currentVoice], NULL);
}
static void soundNoInputCallback(Fl_Widget* o, void*)
{
//	Fl::lock();
	int soundnum=atoi(soundNoInput[currentVoice]->value());
	if(soundnum<0) soundnum=0;
	if(soundnum>511) soundnum=511;
	char ssoundnum[]="***";
	snprintf(ssoundnum, 4, "%i", soundnum);
	soundNoInput[currentVoice]->value(ssoundnum);
	snprintf(soundNoTooltip[currentVoice], _NAMESIZE, _("Bank %u program %u"), soundnum/128, soundnum%128);
	soundNoInput[currentVoice]->tooltip(soundNoTooltip[currentVoice]);
	if(soundRoller[currentVoice]->value()!=soundnum){
		soundRoller[currentVoice]->value(soundnum);
//		Fl::awake();
//		Fl::unlock();
		soundRollerCallback(soundRoller[currentVoice], NULL);
	}else{
//		Fl::awake();
//		Fl::unlock();
	}
}
static void multiRollerCallback(Fl_Widget* o, void*)
{
//	Fl::lock();
	int pgm = (int)((Fl_Valuator* )o)->value();
	char spgm[]="***";
	snprintf(spgm, 4, "%i", pgm);
	multiNameInput->value(Speicher.getMultiName(pgm).c_str());// set gui
	multiNameInput->position(0);// put cursor at the beginning, make sure the start of the string is visible
	multiNoInput->value(spgm);
	setmulti_changed();
//	Fl::awake();
//	Fl::unlock();
}
static void multiincdecCallback(Fl_Widget* o, void*)
{
//	Fl::lock();
	int multinum=multiRoller->value() + o->argument();
	if(multinum<0) multinum=0;
	if(multinum>127) multinum=127;
	multiRoller->value(multinum);
//	Fl::awake();
//	Fl::unlock();
	multiRollerCallback(multiRoller, NULL);
}
static void multiNoInputCallback(Fl_Widget* o, void*)
{
//	Fl::lock();
	int multinum=atoi(multiNoInput->value());
	if(multinum<0) multinum=0;
	if(multinum>=_MULTIS) multinum=_MULTIS-1;
	char smultinum[]="***";
	snprintf(smultinum, 4, "%i", multinum);
	multiNoInput->value(smultinum);
	if(multiRoller->value()!=multinum){
		multiRoller->value(multinum);
//		Fl::awake();
//		Fl::unlock();
		multiRollerCallback(multiRoller, NULL);
	}else{
//		Fl::awake();
//		Fl::unlock();
	}
}
/**
 * parmCallback from the export file dialog
 * do the actual export of current single sound
 */
/*
static void exportSound(Fl_File_Chooser *w, void *userdata)
{
		printf("to %d\n",w->shown());
		fflush(stdout);
	if ((w->shown() !=1) && (w->value() != NULL))
	{
		printf("export to %s\n",w->value());
		fflush(stdout);
	}
}*/
/**
 * Transfer parameter values from the GUI to the multi
 */
static void multistore(const unsigned int multinum)
{
	if (multinum>_MULTIS){
		fprintf(stderr, "multistore: Multi %u out of range\n", multinum);
		return;
	}
	int i;
	// Get the name
	// Speicher.setMultiName(multinum, ((Fl_Input*)e)->value());
	Speicher.setMultiName(multinum, multiNameInput->value());

	// Get the knobs of the mix and midi settings for each voice
	for (i=0;i<_MULTITEMP;++i)
	{
		Speicher.multis[multinum].sound[i]=Speicher.getSoundNo(i);
#ifdef _DEBUG
		printf("sound slot: %d = %d\n",i,Speicher.getSoundNo(i));
#endif
		// Indices must match those in loadmultiCallback
		Speicher.multis[multinum].settings[i][0]=((Fl_Valuator*)knobs[i][101])->value();
		Speicher.multis[multinum].settings[i][1]=((Fl_Valuator*)knobs[i][106])->value();
		Speicher.multis[multinum].settings[i][2]=((Fl_Valuator*)knobs[i][107])->value();
		Speicher.multis[multinum].settings[i][3]=((Fl_Valuator*)knobs[i][108])->value();
		Speicher.multis[multinum].settings[i][4]=((Fl_Valuator*)knobs[i][109])->value();
		Speicher.multis[multinum].settings[i][5]=((Fl_Valuator*)knobs[i][125])->value();
		Speicher.multis[multinum].settings[i][6]=((Fl_Valuator*)knobs[i][126])->value();
		Speicher.multis[multinum].settings[i][7]=((Fl_Valuator*)knobs[i][127])->value();
		Speicher.multis[multinum].settings[i][8]=((Fl_Valuator*)knobs[i][128])->value();
		Speicher.multis[multinum].settings[i][9]=((Fl_Valuator*)knobs[i][129])->value();
		Speicher.multis[multinum].settings[i][10]=((Fl_Valuator*)knobs[i][130])->value();
		if(i<_MULTITEMP-1)
			Speicher.multis[multinum].settings[i][11]=((Fl_Button*)knobs[i][155])->value();
	}
	// Get global multi parameters
	for (i=0;i<_MULTIPARMS;++i){
		Speicher.multis[multinum].parms[i]=((Fl_Valuator*)multiParmInput[i])->value();
	}
	clearmulti_changed(); // Memory is in sync with GUI, file may not be
}

static void do_exportsound(int soundnum)
{
	char warn[256], path[1024], name[_NAMESIZE];
	strnrtrim(name, Speicher.getSoundName( soundnum).c_str(), _NAMESIZE);
	snprintf(warn, 256, "export %s", name);
	snprintf(path, 1024, "%s/%s.txt", Speicher.getHomeFolder().c_str(), name);
	Fl_File_Chooser *fc = new Fl_File_Chooser(path, "TEXT Files (*.txt)\t", Fl_File_Chooser::CREATE, warn);
	fc->textsize(9);
	fc->show();
	while(fc->shown()) Fl::wait(); // block until choice is done
	if ((fc->value() != NULL))
	{
	#ifdef _DEBUG
		printf("export to %s\n",fc->value());
		fflush(stdout);
	#endif
		// Fl::lock();
		// fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
		// Fl::check();

		Speicher.exportSound(fc->value(), soundnum);

		// fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
		// Fl::check();
		// Fl::awake();
		// Fl::unlock();
	}
}
static void do_exportmulti(const unsigned int multinum)
{
	if(multi_changed && (minichoice(_("Refresh multi %u before export?"), _("Yes"), _("No"), 0, _("Confirm"), multinum)==1))
		multistore(multinum);
	char warn[1024], path[1024], name[_NAMESIZE];
	strnrtrim(name, Speicher.getMultiName(multinum).c_str(), _NAMESIZE);
	snprintf(warn, 1024, "export %s", name);
	snprintf(path, 1024, "%s/multi_%s.txt", Speicher.getHomeFolder().c_str(), name);
	Fl_File_Chooser *fc = new Fl_File_Chooser(path, "TEXT Files (*.txt)\t", Fl_File_Chooser::CREATE,warn);
	fc->textsize(9);
	fc->show();
	while(fc->shown()) Fl::wait(); // block until choice is done
	if ((fc->value() != NULL))
	{
	#ifdef _DEBUG
		printf("export to %s\n",fc->value());
		fflush(stdout);
	#endif
		// Fl::lock();
		// fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
		// Fl::check();

		Speicher.exportMulti(fc->value(), multinum);

		// fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
		// Fl::check();
		// Fl::awake();
		// Fl::unlock();
	}
}

/** parmCallback when export button was pressed, shows a file dialog
 */
static void soundexportbtnCallback(Fl_Widget* o, void*)
{
	do_exportsound(atoi(soundNoInput[currentVoice]->value()));
	// What if name has been edited? Not saved = not exported !?
	// maybe should propose saving first?
	/*
	char warn[256];
	sprintf (warn,"export %s:", soundNameInput[currentVoice]->value());
	Fl_File_Chooser *fc = new Fl_File_Chooser(".","TEXT Files (*.txt)\t",Fl_File_Chooser::CREATE,warn);
	//fc->callback(exportSound); // not practical, is called to often
	fc->textsize(9);
	fc->show();
	while(fc->shown()) Fl::wait(); // block until choice is done
	if ((fc->value() != NULL))
	{
	#ifdef _DEBUG
		printf("export to %s\n",fc->value());
		fflush(stdout);
	#endif
		Fl::lock();
		fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
		Fl::check();

		Speicher.exportSound(fc->value(),atoi(soundNoInput[currentVoice]->value()));

		fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
		Fl::check();
		Fl::awake();
		Fl::unlock();
	}
	*/
}

void updatesoundNameInput(int dest, const char* new_name){
	// update name input field in any voice tab using sound number dest
	for(int voice=0; voice<_MULTITEMP; voice++){
		// What if soundNameInput has been changed manually?
		if(atoi(soundNoInput[voice]->value())==dest){
			soundNameInput[voice]->value(new_name);
			soundNameInput[voice]->position(0);
			setsound_changed(voice);
		}
	}
}

static void do_importsound(int soundnum)
{
	// printf("About to import into sound %d\n", soundnum);
	if(strlen(Speicher.getSoundName(soundnum).c_str())
		&& minichoice(_("Overwrite sound #%u %s?"), _("Yes"), _("No"), 0, _("Confirm"), soundnum, Speicher.getSoundName(soundnum).c_str())!=1)
		return;

	char warn[256], path[1024];
	sprintf (warn, "overwrite %s:", Speicher.getSoundName(soundnum).c_str()); // Is this useful for import ??
	snprintf(path, 1024, "%s/", Speicher.getHomeFolder().c_str());
	Fl_File_Chooser *fc = new Fl_File_Chooser(path, "TEXT Files (*.txt)\t", Fl_File_Chooser::SINGLE, warn);
	fc->textsize(9);
	fc->show();
	while(fc->shown()) Fl::wait(); // block until choice is done
	if ((fc->value() != NULL))
	{
//		Fl::lock();
//		fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
//		Fl::check();

		Speicher.importSound(fc->value(), soundnum);
		// ok, now we have a new sound loaded, but we should flag voices that use it as changed
		// but not load them?!
		updatesoundNameInput(soundnum, Speicher.getSoundName(soundnum).c_str());
		// Does any voice use that sound number?
		int found=false;
		for(int i=0; i<_MULTITEMP; i++){
			if(atoi(soundNoInput[i]->value())==soundnum){
				found=true;
				break;
			}
		}
		// If sound is in use, propose to apply imported version
		if(found && minichoice(_("Load the imported sound #%u in voice(s) using that sound number?"), _("Yes"), _("No"), 0, _("Confirm"), soundnum)==1){
			for(int i=0; i<_MULTITEMP; i++){
				// printf("voice %u sound %s\n", i, soundNoInput[i]->value());
				if(atoi(soundNoInput[i]->value())==soundnum)
					sound_recall(i, soundnum);
			}
		}
		if(alwaysSave || minichoice(_("Save all sounds (including #%u)?"), _("Yes"), _("No"), 0, _("Confirm"), soundnum)==1)
			Speicher.saveSounds();
//		fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
//		Fl::check();
//		Fl::awake();
//		Fl::unlock();
	}

}

/** parmCallback when import button was pressed, shows a filedialog
 */
static void soundimportbtnCallback(Fl_Widget* o, void* )
{
	do_importsound(atoi(soundNoInput[currentVoice]->value()));
/*
	char warn[256];
	sprintf (warn,"overwrite %s:",soundNameInput[currentVoice]->value());
	Fl_File_Chooser *fc = new Fl_File_Chooser(".","TEXT Files (*.txt)\t",Fl_File_Chooser::SINGLE,warn);
	fc->textsize(9);
	fc->show();
	while(fc->shown()) Fl::wait(); // block until choice is done
	if ((fc->value() != NULL))
	{
		Fl::lock();
		fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
		Fl::check();

		Speicher.importSound(fc->value(),atoi(soundNoInput[currentVoice]->value()));
		// ok, now we have a new sound saved but we should update the userinterface
		soundNameInput[currentVoice]->value(Speicher.getSoundName(atoi(soundNoInput[currentVoice]->value())).c_str());
		soundNameInput[currentVoice]->position(0);
		setsound_changed();
		fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
		Fl::check();
		Fl::awake();
		Fl::unlock();
	}
*/
}

/** Store given sound # parameters into given patch
 */
static void soundstore(unsigned int srcVoice, patch *destpatch)
{
//	Fl::lock();
//	fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
//	Fl::check();
#ifdef _DEBUG
	printf("soundstore: source voice %i\n", srcVoice);
	fflush(stdout);
#endif
	if(srcVoice>=_MULTITEMP){
		fprintf(stderr, "soundstore: unexpected voice %u - ignoring", srcVoice);
		return;
	}
	// clean first the name string
	sprintf(destpatch->name, "%s", soundNameInput[srcVoice]->value());

	for (int i=0; i<_PARACOUNT; ++i) // go through all parameters
	{
		if (knobs[srcVoice][i] != NULL && needs_multi[i]==0)
		{
			if (is_button[i]){
				destpatch->parameter[i]=((Fl_Valuator*)knobs[srcVoice][i])->value()?1.0f:0.0f;
#ifdef _DEBUG
				printf("soundstore: button %d = %f\n", i, destpatch->parameter[i]);
#endif
			}else{
				destpatch->parameter[i]=((Fl_Valuator*)knobs[srcVoice][i])->value();
#ifdef _DEBUG
				printf("soundstore: parameter %d = %f\n", i, destpatch->parameter[i]);
#endif
				switch (i)
				{
				case 3:
					{
					destpatch->freq[0][0]=((MiniPositioner*)knobs[srcVoice][i])->xvalue();
					destpatch->freq[0][1]=((MiniPositioner*)knobs[srcVoice][i])->yvalue();
				break;
				}
				case 18:
						{
					destpatch->freq[1][0]=((MiniPositioner*)knobs[srcVoice][i])->xvalue();
					destpatch->freq[1][1]=((MiniPositioner*)knobs[srcVoice][i])->yvalue();
				break;
				}
				//************************************ filter cuts *****************************
				case 30:
						{
					destpatch->freq[2][0]=((MiniPositioner*)knobs[srcVoice][i])->xvalue();
					destpatch->freq[2][1]=((MiniPositioner*)knobs[srcVoice][i])->yvalue();
				break;
				}
				case 33:
						{
					destpatch->freq[3][0]=((MiniPositioner*)knobs[srcVoice][i])->xvalue();
					destpatch->freq[3][1]=((MiniPositioner*)knobs[srcVoice][i])->yvalue();
				break;
				}
				case 40:
						{
					destpatch->freq[4][0]=((MiniPositioner*)knobs[srcVoice][i])->xvalue();
					destpatch->freq[4][1]=((MiniPositioner*)knobs[srcVoice][i])->yvalue();
				break;
				}
				case 43:
						{
					destpatch->freq[5][0]=((MiniPositioner*)knobs[srcVoice][i])->xvalue();
					destpatch->freq[5][1]=((MiniPositioner*)knobs[srcVoice][i])->yvalue();
				break;
				}
				case 50:
						{
					destpatch->freq[6][0]=((MiniPositioner*)knobs[srcVoice][i])->xvalue();
					destpatch->freq[6][1]=((MiniPositioner*)knobs[srcVoice][i])->yvalue();
				break;
				}
				case 53:
						{
					destpatch->freq[7][0]=((MiniPositioner*)knobs[srcVoice][i])->xvalue();
					destpatch->freq[7][1]=((MiniPositioner*)knobs[srcVoice][i])->yvalue();
				break;
				}
				case 90:
				{
					destpatch->freq[8][0]=((MiniPositioner*)knobs[srcVoice][i])->xvalue();
					destpatch->freq[8][1]=((MiniPositioner*)knobs[srcVoice][i])->yvalue();
				break;
				}
				} // end of switch
			} // end of if not button
		} // end of if not null & not multi
	} // end of for

	for (int i=0; i<_CHOICECOUNT; ++i)
	{
		if (choices[srcVoice][i] != NULL)
		{
			destpatch->choice[i]=choices[srcVoice][i]->value();
#ifdef _DEBUG
			printf("soundstore: choice #%i: %i\n", i, choices[srcVoice][i]->value());
#endif
		}
	}
//	fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
//	Fl::check();
//	Fl::awake();
//	Fl::unlock();
}
/** parmCallback when the store button on the voice tab is pressed
 * relies on currentVoice
 */
static void soundstoreCallback(Fl_Widget*, void*)
{
	int preset=atoi(soundNoInput[currentVoice]->value());
#ifdef _DEBUG
	printf("soundstoreCallback: source sound %i\n", preset);
	fflush(stdout);
#endif
	soundstore(currentVoice, &Speicher.sounds[preset]);
	Speicher.saveSounds();
	clearsound_changed();
}
/**
 * recall a single sound into given tab
 * by calling each parameter's parmCallback
 * which should handle both display update
 * and OSC transmission
 */
static void sound_recall0(unsigned int destvoice, patch *srcpatch)
{
#ifdef _DEBUG
	printf("sound_recall0: destination voice %u\n", destvoice);
	fflush(stdout);
#endif
	if (destvoice > _MULTITEMP) return;
	int oldVoice=currentVoice;
	currentVoice=destvoice; // needed by parmSet and choiceCallback
	for(int i=0;i<_PARACOUNT;++i)
	{
		if (knobs[destvoice][i] != NULL && needs_multi[i]==0)
		{
			if(is_button[i]){
#ifdef _DEBUG
				printf("sound_recall0 voice %u button # %u\n", destvoice, i);
				fflush(stdout);
#endif
				if (srcpatch->parameter[i]==0.0f)
				{
					((Fl_Light_Button*)knobs[destvoice][i])->value(0);
				}
				else
				{
					((Fl_Light_Button*)knobs[destvoice][i])->value(1);
				}
			}else{
#ifdef _DEBUG
				printf("sound_recall0 voice %u parameter # %u\n", destvoice, i);
				fflush(stdout);
#endif
				switch (i)
				{
				case 3:
						{
					((MiniPositioner*)knobs[destvoice][i])->xvalue(srcpatch->freq[0][0]);
					((MiniPositioner*)knobs[destvoice][i])->yvalue(srcpatch->freq[0][1]);
					break;
				}
				case 18:
						{
					((MiniPositioner*)knobs[destvoice][i])->xvalue(srcpatch->freq[1][0]);
					((MiniPositioner*)knobs[destvoice][i])->yvalue(srcpatch->freq[1][1]);
					break;
				}
				//************************************ filter cuts *****************************
				case 30:
						{
					((MiniPositioner*)knobs[destvoice][i])->xvalue(srcpatch->freq[2][0]);
					((MiniPositioner*)knobs[destvoice][i])->yvalue(srcpatch->freq[2][1]);
				break;
				}
				case 33:
						{
					((MiniPositioner*)knobs[destvoice][i])->xvalue(srcpatch->freq[3][0]);
					((MiniPositioner*)knobs[destvoice][i])->yvalue(srcpatch->freq[3][1]);
				break;
				}
				case 40:
						{
					((MiniPositioner*)knobs[destvoice][i])->xvalue(srcpatch->freq[4][0]);
					((MiniPositioner*)knobs[destvoice][i])->yvalue(srcpatch->freq[4][1]);
				break;
				}
				case 43:
						{
					((MiniPositioner*)knobs[destvoice][i])->xvalue(srcpatch->freq[5][0]);
					((MiniPositioner*)knobs[destvoice][i])->yvalue(srcpatch->freq[5][1]);
				break;
				}
				case 50:
						{
					((MiniPositioner*)knobs[destvoice][i])->xvalue(srcpatch->freq[6][0]);
					((MiniPositioner*)knobs[destvoice][i])->yvalue(srcpatch->freq[6][1]);
				break;
				}
				case 53:
				{
					((MiniPositioner*)knobs[destvoice][i])->xvalue(srcpatch->freq[7][0]);
					((MiniPositioner*)knobs[destvoice][i])->yvalue(srcpatch->freq[7][1]);
				break;
				}
				case 90:
				{
					((MiniPositioner*)knobs[destvoice][i])->xvalue(srcpatch->freq[8][0]);
					((MiniPositioner*)knobs[destvoice][i])->yvalue(srcpatch->freq[8][1]);
				break;
				}
				default:
				{
				// Beware! All controls that are not an Fl_Valuator must have their own case
#ifdef _DEBUG
					printf("sound_recall0 default: value %f\n", srcpatch->parameter[i]);
#endif
					((Fl_Valuator*)knobs[destvoice][i])->value(srcpatch->parameter[i]);
				break;
				}
				} // End of switch
			} // End of if is_button 
			parmSet(knobs[destvoice][i], NULL); // Beware this relies on currentVoice
#ifdef _DEBUG
		}else{
			printf("sound_recall0: voice %i parameter # %i : null or multi\n", destvoice, i);
			fflush(stdout);
#endif
		} // End of if not null & not multi
	} // End of loop for parameters

	for (int i=0; i<_CHOICECOUNT; ++i)
	{
		if (choices[destvoice][i])
		{
			choices[destvoice][i]->value(srcpatch->choice[i]);
			choiceSet(choices[destvoice][i], NULL); // Beware this relies on currentVoice
#ifdef _DEBUG 
			printf("sound_recall0 voice %i choice # %i : %i\n", destvoice, i, Speicher.sounds[Speicher.getSoundNo(destvoice)].choice[i]);
		}else{
			printf("sound_recall0 voice %i choice %i : null or multi\n", destvoice, i);
#endif
		}
	}
	currentVoice=oldVoice;

	// send a reset (clears filters and delay buffer)
	if (transmit) lo_send(t, "/Minicomputer", "iif", destvoice, 0, 0.0f);
#ifdef _DEBUG 
	fflush(stdout);
#endif
}

/**
 * recall a single sound into current or given tab
 */
void sound_recall(int voice, unsigned int sound)
{
	// Fl::lock();
	if(voice==-1) voice=currentVoice;
	if(voice>=_MULTITEMP){
		fprintf(stderr, "sound_recall: voice %u out of range - ignoring.\n", voice);
		return;
	}
	if(sound>=_SOUNDS){
		fprintf(stderr, "sound_recall: sound %u out of range - ignoring.\n", sound);
		return;
	}
// #ifdef _DEBUG
	printf("sound_recall: voice %u sound %u\n", voice, sound);
	fflush(stdout);
// #endif
	Speicher.setSoundNo(voice, sound);
	// Update display
	char s[]="####";
	snprintf(s, 4, "%d", sound);
	soundNoDisplay[voice]->value(s);
    strnrtrim(soundName[voice], Speicher.getSoundName(sound).c_str(), _NAMESIZE);
	soundNoDisplay[voice]->tooltip(soundName[voice]);
	soundNoDisplay[voice]->redraw();
	soundNoInput[voice]->value(s);
	snprintf(soundNoTooltip[voice], _NAMESIZE, _("Bank %u program %u"), sound/128, sound%128);
	soundNoInput[voice]->tooltip(soundNoTooltip[voice]);
	soundNoInput[voice]->redraw();
	soundRoller[voice]->value(sound);
	// In case name has been edited
	soundNameInput[voice]->value(soundName[voice]);
	soundNameInput[voice]->position(0);
	soundNameInput[voice]->redraw();
	sound_recall0(voice, &Speicher.sounds[sound]);
	clearsound_changed(voice);
	if (sound!=Speicher.multis[currentMulti].sound[voice])
		setmulti_changed();
	// Fl::awake();
	// Fl::unlock();
}

static void do_compare(Fl_Widget* o, void* ){
	if(((Fl_Valuator*)o)->value()){
		soundstore(currentVoice, &compare_buffer);
		int preset=atoi(soundNoInput[currentVoice]->value());
		sound_recall0(currentVoice, &Speicher.sounds[preset]);
	}else{
		sound_recall0(currentVoice, &compare_buffer);
	}
}
/**
 * parmCallback when the load button on voice tab is pressed
 * @param o the calling widget
 */
// Load button on voice tab - based on soundNoInput
static void soundLoadBtn1Callback(Fl_Widget* o, void* )
{
//	Fl::lock();
//	int oldParameter=currentParameter;
	sound_recall(-1, (unsigned int)(atoi(soundNoInput[currentVoice]->value())));
//	parmCallback(knobs[currentVoice][oldParameter], NULL);
//	Fl::awake();
//	Fl::unlock();
}
// Load button on sounds tab - based on table selection
static void soundLoadBtn2Callback(Fl_Widget* o, void* )
{
	sound_recall(o->argument(), soundTable->selected_index());
}

static void soundNameInputCallback(Fl_Widget* o, void* ){
	setsound_changed();
}
static void multiNameInputCallback(Fl_Widget* o, void* ){
	setmulti_changed();
}

static void loadmulti(unsigned int multi)
{
	printf("loadmulti: multi #%d\n", multi);
	if(multi>=_MULTIS) return;
	currentMulti = multi;
	char smulti[]="***";
	snprintf(smulti, 4, "%i", multi);
	multiNoInput->value(smulti);
	multiNameInput->value(Speicher.getMultiName(multi).c_str()); // set gui
	multiNameInput->position(0); // put cursor at the beginning, make sure the start of the string is visible
	multiRoller->value(multi);
	int oldVoice=currentVoice;
	for (int i=0; i<_MULTITEMP; ++i) {
		currentVoice = i; // Required by parmSet
		unsigned int soundnum = Speicher.multis[multi].sound[i];
		if (soundnum >= 0 && soundnum < _SOUNDS) {
			sound_recall(i, soundnum);
#ifdef _DEBUG
			printf("i ist %i Speicher ist %i\n", i, soundnum);
			fflush(stdout);
#endif
			char temp_name[_NAMESIZE];
			strnrtrim(temp_name, Speicher.getSoundName(soundnum).c_str(), _NAMESIZE);
#ifdef _DEBUG
			printf("loadmulti voice %u: \"%s\"\n", i, temp_name);
#endif
			// Update gui
			soundNameInput[i]->value(temp_name);
			soundRoller[i]->value(soundnum);
			char ssoundnum[]="***";
			snprintf(ssoundnum, 4, "%i", soundnum);
			soundNoInput[i]->value(ssoundnum);
			snprintf(soundNoTooltip[i], _NAMESIZE, _("Bank %u program %u"), soundnum/128, soundnum%128);
			soundNoInput[i]->tooltip(soundNoTooltip[i]);
		} else {
			fprintf(stderr, "loadmulti: sound %d is out of range!\n", soundnum);
		}
		// set the knobs of the mix, 0..4
		int MULTI_parm_num[]={101, 106, 107, 108, 109};
		for (unsigned int j=0; j<sizeof(MULTI_parm_num)/sizeof(int); j++) {
			((Fl_Valuator*)knobs[i][MULTI_parm_num[j]])->value(Speicher.multis[multi].settings[i][j]);
			parmSet(knobs[i][MULTI_parm_num[j]],NULL); // Beware! This relies on currentVoice
		}
		// set the midi parameters, 5..10
		int MIDI_parm_num[]={125, 126, 127, 128, 129, 130};
		int start=5; // sizeof(MULTI_parm_num)/sizeof(int);
		for (unsigned int j=0; j<sizeof(MIDI_parm_num)/sizeof(int); j++) {
			((Fl_Valuator*)knobs[i][MIDI_parm_num[j]])->value(Speicher.multis[multi].settings[i][j+start]);
			midiparmCallback(knobs[i][MIDI_parm_num[j]],NULL);
		}
		// set the only midi switch, not available for last voice
		if(i<_MULTITEMP-1){
			int j=155;
			((Fl_Button*)knobs[i][j])->value(Speicher.multis[multi].settings[i][11]);
			midibuttonCallback(knobs[i][j],NULL);
		}
	}
	for (int i=0;i<_MULTITEMP;++i)
		clearsound_changed(i);
	for (int i=0;i<_MULTIPARMS;++i){
		// printf("multiparm N°%d at %lu = %f\n", i, multiParmInput[i], Speicher.multis[currentMulti].parms[i]);
		((Fl_Valuator*)multiParmInput[i])->value(Speicher.multis[currentMulti].parms[i]);
		// printf("value set\n");
		multiParmInputCallback(multiParmInput[i], NULL);
		// printf("callback done\n");
	}
	// Restore previous state
	currentVoice = oldVoice;
	// tabs->value(oldtab);
	// tabCallback(tabs,NULL);
	// parmCallback(knobs[currentVoice][currentParameter], NULL);
	parmInput->hide(); // Should set to a certain parameter and do what's needed?
	clearmulti_changed(); // current multi == selected multi !
#ifdef _DEBUG
	printf("loadmulti multi %u voice %i parameter %i\n", currentMulti, currentVoice, currentParameter);
	fflush(stdout);
#endif
	//fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
	//Fl::check();
//	Fl::awake();
//	Fl::unlock();
}
static void loadmultibtn2Callback(Fl_Widget*, void*)
{
	int multinum=multiTable->selected_index();
	loadmulti(multinum);
}
/**
 * parmCallback when the load multi button is pressed
 * recall a multitimbral setup
 */
static void loadmultiCallback(Fl_Widget*, void*)
{
	loadmulti(multiRoller->value());
}

/**
 * parmCallback when the store multi button is pressed
 * 
 * store a multitimbral setup in memory and optionally to disk
 */
static void multistoreCallback(Fl_Widget*, void*)
{
//	Fl::lock();
//	fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
//	Fl::check();
	// Get current multi number from GUI
	if (multiNameInput && multiRoller)
	{
		unsigned int t = (unsigned int) multiRoller->value();
#ifdef _DEBUG
		printf("was:%d is:%d\n",currentMulti,t);
#endif
		if ((t!=currentMulti) && (t>=0) && (t<128))
		{
			currentMulti = t;
		}
	}
	else
		fprintf(stderr, "problems with the multichoice widgets!\n");

	multistore(currentMulti);

	// write to disk
	if(alwaysSave || minichoice("Save all multis (including #%u) to disk?", _("Yes"), _("No"), 0, _("Confirm"), currentMulti)==1)
		Speicher.saveMultis();
//	fl_cursor(FL_CURSOR_DEFAULT, FL_WHITE, FL_BLACK);
//	Fl::check();
//	Fl::awake();
//	Fl::unlock();
}

int MiniValue_Input::handle(int event){
	// printf("MiniValue_Input::Handle!\n");
	// parmInput->show(); // May have been hidden by current parameter unfocus ??
//			parmInputFocused=true;
	switch (event) 
	{
/*
		case FL_FOCUS:
//			parmInputFocused=true;
			// parmInput->show(); // May have been hidden by current parameter unfocus
			break;
			*/
			// printf("MiniValue_Input::Focus %lu!\n", this->argument());
			
			// Fl::focus(this);
			
//			this->callback()((Fl_Widget*)this, 0);
//			if (visible()) damage(FL_DAMAGE_ALL);

			// return 1;

		case FL_UNFOCUS:
			// printf("MiniValue_Input::Unfocus %lu!\n", this->argument());
//			parmInputFocused=false;
			parmInput->hide();
			break;
			/*
			this->callback()((Fl_Widget*)this, 0);
			if (visible()) damage(FL_DAMAGE_ALL);
			return 1;
			*/
	}
	return Fl_Value_Input::handle(event);
}

void SoundTable::draw_cell(TableContext context, 
			  int R, int C, int X, int Y, int W, int H)
{
	static char s[40];
	// int m=R*(cols()/2)+(C/2); // 2 cells per sound
	// int m=R+rows()*(C/2); // 2 cells per sound
	int m=index(R, C);
	if(m<0 || m>=_SOUNDS) return;
	// Cannot get this to actually refresh display
	// except after switching to a different window or tab
	// Probably can't do it from here, but handle doesn't catch keyboard?
	/*
	if((C&1) && C==_selected_col && R==_selected_row){
		printf("display %s\n", Speicher.getSoundName(m).c_str());
		soundNameDisplay->value(Speicher.getSoundName(m).c_str());
		soundNameDisplay->damage(FL_DAMAGE_ALL);
		soundNameDisplay->redraw();
		tab[9]->redraw();
		soundNameDisplay->hide();
		soundNameDisplay->show();
		soundNameDisplay->color(FL_RED);
		printf("display value %s\n", soundNameDisplay->value());
	}
	*/

	switch ( context )
	{
	case CONTEXT_STARTPAGE:
		fl_font(FL_HELVETICA, 12);
		return;

	case CONTEXT_CELL:
		// BG COLOR
		if (C==_selected_col && R==_selected_row){
			fl_color((C&1) ? FL_RED: _BGCOLOR);

		}else if (Speicher.getSoundName(m).length()==0){
			fl_color((C&1) ? FL_BLACK: _BGCOLOR);
		}else{
			fl_color((C&1) ? FL_WHITE: _BGCOLOR);
		}
		fl_push_clip(X, Y, W, H);
		fl_rectf(X, Y, W, H);

		// TEXT
		fl_color(FL_BLACK);
		if(C&1){
			// printf("%d %s\n", m, Speicher.getSoundName(m).c_str());
			fl_draw(Speicher.getSoundName(m).c_str(), X+2, Y, W-2, H, FL_ALIGN_LEFT);
		}else{
			sprintf(s, "%d", m); // text for sound number cell
			fl_draw(s, X+1, Y, W-1, H, FL_ALIGN_LEFT);
		}

		// BORDER
		fl_color(color()); 
		fl_rect(X, Y, W, H);

		fl_pop_clip();
		return;

	case CONTEXT_TABLE:
	    fprintf(stderr, "TABLE CONTEXT CALLED\n");
	    return;

	case CONTEXT_ROW_HEADER:
	case CONTEXT_COL_HEADER:
	case CONTEXT_ENDPAGE:
	case CONTEXT_RC_RESIZE:
	case CONTEXT_NONE:
	    return;
    }
}

void SoundTable::event_callback(Fl_Widget*, void *data)
{
	SoundTable *o = (SoundTable*)data;
	o->event_callback2();
}

void SoundTable::event_callback2()
{
	// printf("SoundTable::event_callback2() %u\n", m);
//	int m;
//	TableContext context = callback_context();
//	printf("Row=%d Col=%d Context=%d Event=%d sound %d InteractiveResize? %d\n",
//		R, C, (int)context, (int)Fl::event(), m, (int)is_interactive_resize());
	Fl_Menu_Item menu_rclick[] = {
		{ _("&Copy"), 0, soundcopymnuCallback, (void*)this },
		{ _("Cu&t"), 0, soundcutmnuCallback, (void*)this },
		{ _("&Paste"), 0, soundpastemnuCallback, (void*)this },
		{ _("Rename"), 0, soundrenamemnuCallback, (void*)this },
		{ _("Clear"), 0, soundclearmnuCallback, (void*)this },
		{ _("Import"), 0, soundimportmnuCallback, (void*)this },
		{ _("Export"), 0, soundexportmnuCallback, (void*)this },
		{ 0 }
	};
#ifdef _DEBUG
	printf("SoundTable::event_callback2(%u)\n", Fl::event());
#endif
	switch(Fl::event()){
		case FL_PUSH:{
			int R = callback_row(), C = callback_col();
			// int m=R*(cols()/2)+(C/2); // 2 cells per sound (horizontal)
			// m=R+rows()*(C/2); // 2 cells per sound (vertical)
			if(R>=0 && C>=0 && (C&1)==1){
				_selected_row=R;
				_selected_col=C;
				// printf("m %u index %u\n", m, selected_index());
				soundNameDisplay->value(Speicher.getSoundName(selected_index()).c_str());
				if ( Fl::event_button() == FL_RIGHT_MOUSE ) {
					// printf("Right mouse button pushed\n");
					const Fl_Menu_Item *mnu = menu_rclick->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
					if ( mnu ) mnu->do_callback(0, mnu->user_data());
					// return(1);          // (tells caller we handled this event)
				}else{
					// printf("Left or middle mouse button pushed\n");
				}
				redraw();
			}
			break;
		}
		/*
		case FL_RELEASE: // Never caught?
			if ( Fl::event_button() == FL_RIGHT_MOUSE ) {
				printf("Right mouse button released\n");
			}else{
				printf("Left or middle mouse button released\n");
			}
			break;
		*/
		case FL_KEYDOWN:
		// case FL_SHOW: // Never caught?
			// Standard movement was already handled in MiniTable's handler
			// We only have to handle delete and redraw slave field
			int m=selected_index();
#ifdef _DEBUG
			printf("SoundTable::event_callback2 event FL_KEYDOWN key %u\n", Fl::event_key());
#endif
			if (Fl::event_key()==FL_Delete && (minichoice(_("Delete sound %u?"), _("Yes"), _("No"), 0, _("Confirm"), m)==1))
				soundclearmnuCallback(0, this);
			else if (Fl::event_key()=='v' && Fl::event_ctrl())
				soundpastemnuCallback(0, this);
			else if (Fl::event_key()==FL_Menu){
					int X, Y, W, H;
					find_cell (CONTEXT_CELL, selected_row(), selected_col(), X, Y, W, H);
					// const Fl_Menu_Item *m = menu_rclick->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
					const Fl_Menu_Item *m = menu_rclick->popup(X+W/2, Y+H/2, 0, 0, 0);
					if ( m ) m->do_callback(0, m->user_data());
			}
			// We cannot trust R and C due to our special selection handling
			soundNameDisplay->value(Speicher.getSoundName(m).c_str());
			redraw();
			break;
	}
}

void MiniTable::resize_cols(int W){
	// base width: 968 = (96+25)*8
	// printf("resize_cols: %f %d %d\n", scale, int(96*scale), int(25*scale));
	if(W>=_INIT_WIDTH){
		int new_width=(float)((W-220)/8.0f); // 220=8x25+20 for scrollbar
		for(int c=1; c<_TABLE_COLUMNS; c+=2) col_width(c, new_width);
	}
	return;
}

void soundcopymnuCallback(Fl_Widget*, void*T) {
	((MiniTable*)T)->copy();
	/*
	int cell=((SoundTable *)T)->selected_cell();
//	printf("copy sound from cell %d\n", cell);
	((SoundTable *)T)->copied_cell(cell);
	((SoundTable *)T)->clear_cell_is_cut();
	*/
}
void soundcutmnuCallback(Fl_Widget*, void*T) {
	((MiniTable*)T)->cut();
	/*
	int cell=((SoundTable *)T)->selected_cell();
//	printf("cut sound from cell %d\n", cell);
	((SoundTable *)T)->copied_cell(cell);
	((SoundTable *)T)->set_cell_is_cut();
	*/
}
void soundpastemnuCallback(Fl_Widget*, void*T) {
	int src=((SoundTable *)T)->copied_index();
	if(src<0 || src>=_SOUNDS) return;
	int dest=((SoundTable *)T)->selected_index();
	if(dest<0 || dest>=_SOUNDS) return;
	printf("paste sound %d to %d\n", src, dest);
	Speicher.copySound(src, dest);
	soundNameDisplay->value(Speicher.getSoundName(dest).c_str());
	updatesoundNameInput(dest, Speicher.getSoundName(dest).c_str());
	if(((SoundTable *)T)->cell_is_cut()){
		printf("clear sound %d\n", src);
		((SoundTable *)T)->clear_cell_is_cut();
		((MiniTable *)T)->copy(); // Previous has been cleared !!
		Speicher.copyPatch(&Speicher.initSound, &Speicher.sounds[src]);
		Speicher.setSoundName(src, "");
	}
}
void soundrenamemnuCallback(Fl_Widget*, void*T) {
	int dest=((SoundTable *)T)->selected_index();
	if(dest<0 || dest>=_SOUNDS) return;
	char old_name[_NAMESIZE];
	strnrtrim(old_name, Speicher.getSoundName(dest).c_str(), _NAMESIZE);
	const char *new_name=miniinput(_("New name?"), old_name, _("Rename"));
	if(new_name){
		printf("rename sound %d %s\n", dest, new_name);
		soundNameDisplay->value(new_name);
		Speicher.setSoundName(dest, new_name);
		updatesoundNameInput(dest, new_name);
		((SoundTable *)T)->clear_cell_is_cut();
	}
}
void soundinitmnuCallback(Fl_Widget*, void*T) { // Is this still needed??
	int dest=((SoundTable *)T)->selected_index();
	printf("init %d\n", dest);
	Speicher.copyPatch(&Speicher.initSound, &Speicher.sounds[dest]);
	soundNameDisplay->value(Speicher.getSoundName(dest).c_str());
	updatesoundNameInput(dest, Speicher.getSoundName(dest).c_str());
}
void soundclearmnuCallback(Fl_Widget*, void*T) {
	int dest=((SoundTable *)T)->selected_index();
	printf("clear %d\n", dest);
	Speicher.copyPatch(&Speicher.initSound, &Speicher.sounds[dest]);
	Speicher.setSoundName(dest, "");
	soundNameDisplay->value(Speicher.getSoundName(dest).c_str());
	updatesoundNameInput(dest, Speicher.getSoundName(dest).c_str());
}
void soundimportmnuCallback(Fl_Widget*, void*T) {
	int dest=((SoundTable *)T)->selected_index();
	do_importsound(dest);
	soundNameDisplay->value(Speicher.getSoundName(dest).c_str());
}
void soundexportmnuCallback(Fl_Widget*, void*T) {
	int src=((SoundTable *)T)->selected_index();
	do_exportsound(src);
}
void soundssavebtnCallback(Fl_Widget*, void*) {
	Speicher.saveSounds();
}
void multissavebtnCallback(Fl_Widget*, void*) {
	Speicher.saveMultis();
}
void updatemultiNameInput(unsigned int dest, const char* new_name){
	// update name input field
	// What if name has been changed manually?
	if(currentMulti==dest){
		multiNameInput->value(new_name);
		multiNameInput->position(0);
		setmulti_changed();
	}
}

void multiloadmnuCallback(Fl_Widget*, void*T) {
	int multinum=((MultiTable*)T)->selected_index();
	loadmulti(multinum);
}
void multicopymnuCallback(Fl_Widget*, void*T) {
	((MiniTable*)T)->copy();
}
void multicutmnuCallback(Fl_Widget*, void*T) {
	((MiniTable*)T)->cut();
}
void multiclearmnuCallback(Fl_Widget*, void*T) {
	int dest=((MiniTable*)T)->selected_index();
printf("clear multi number %d\n", dest);
	if(dest<0 || dest>=_MULTIS) return;
	printf("clear multi number %d\n", dest);
	Speicher.clearMulti(dest);
	Speicher.setMultiName(dest, "");
	multiNameDisplay->value(Speicher.getMultiName(dest).c_str());
	updatemultiNameInput(dest, Speicher.getMultiName(dest).c_str());

}
void multipastemnuCallback(Fl_Widget*, void*T) {
	int src=((MultiTable *)T)->copied_index();
	if(src<0 || src>=_MULTIS) return;
	int dest=((MultiTable *)T)->selected_index();
	if(dest<0 || dest>=_MULTIS) return;
	printf("paste multi %d to %d\n", src, dest);
	Speicher.copyMulti(src, dest);
	multiNameDisplay->value(Speicher.getMultiName(dest).c_str());
	updatemultiNameInput(dest, Speicher.getMultiName(dest).c_str());
	if(((MultiTable *)T)->cell_is_cut()){
		printf("clear multi %d\n", src);
		((MultiTable *)T)->clear_cell_is_cut();
		Speicher.clearMulti(src);
		((MiniTable*)T)->copy(); // Previous has been cleared
	}
}
void multipastetuningmnuCallback(Fl_Widget*, void*T) {
	int src=((MultiTable *)T)->copied_index();
	if(src<0 || src>=_MULTIS) return;
	int dest=((MultiTable *)T)->selected_index();
	if(dest<0 || dest>=_MULTIS) return;
	printf("paste multi tuning %d to %d\n", src, dest);
	Speicher.copyMultiTuning(src, dest);
}
void multipastecontrollersmnuCallback(Fl_Widget*, void*T) {
	int src=((MultiTable *)T)->copied_index();
	if(src<0 || src>=_MULTIS) return;
	int dest=((MultiTable *)T)->selected_index();
	if(dest<0 || dest>=_MULTIS) return;
	printf("paste multi controllers %d to %d\n", src, dest);
	Speicher.copyMultiControllers(src, dest);
}
void multirenamemnuCallback(Fl_Widget*, void*T) {
	int dest=((MultiTable *)T)->selected_index();
	if(dest<0 || dest>=_MULTIS) return;
	printf("rename %d\n", dest);
	char old_name[_NAMESIZE];
	strnrtrim(old_name, Speicher.getMultiName(dest).c_str(), _NAMESIZE);
	const char *new_name=miniinput(_("New name?"), old_name, _("Rename"));
	if(new_name){
		multiNameDisplay->value(new_name);
		Speicher.setMultiName(dest, new_name);
		updatemultiNameInput(dest, new_name);
		((MultiTable *)T)->clear_cell_is_cut();
	}
}

void multiexportCallback(Fl_Widget*, void*T) {
	int src=((MultiTable *)T)->selected_index();
	do_exportmulti(src);
}

void do_importmulti(int multinum)
{
	// printf("About to import into multi %d\n", multinum);
	if( strlen(Speicher.getMultiName(multinum).c_str())
		&& minichoice("Overwrite multi #%u %s?", _("Yes"), _("No"), 0, _("Confirm"), multinum, Speicher.getMultiName(multinum).c_str())!=1)
		return;
	char warn[256], path[1024];
	bool with_sounds=false;
	snprintf(warn, 256, "overwrite multi %u", multinum);
	snprintf(path, 1024, "%s/", Speicher.getHomeFolder().c_str());
	Fl_File_Chooser *fc = new Fl_File_Chooser(path, "TEXT Files (*.txt)\t", Fl_File_Chooser::SINGLE, warn);
	fc->textsize(9);
	fc->show();
	while(fc->shown()) Fl::wait(); // block until choice is done
	if ((fc->value() != NULL))
	{
//		Fl::lock();
//		fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
//		Fl::check();
		with_sounds=minichoice(_("Do you want to import embedded sounds, overwriting existing sounds?"), _("Yes"), _("No"), 0, _("Confirm"))==1;
		Speicher.importMulti(fc->value(), multinum, with_sounds);
		// ok, now we have a new multi loaded
		// but not applied?!
		updatemultiNameInput(multinum, Speicher.getMultiName(multinum).c_str());
		if(minichoice(_("Use the imported multi as current?"), _("Yes"), _("No"), 0, _("Confirm"))==1) loadmulti(multinum);
		// now the new multi is in RAM but need to be saved to the main file
		if(alwaysSave || minichoice(_("Save all multis?"), _("Yes"), _("No"), 0, _("Confirm"))==1) Speicher.saveMultis();
		if(with_sounds && (alwaysSave || minichoice(_("Save all sounds?"), _("Yes"), _("No"), 0, _("Confirm"))==1)) Speicher.saveSounds();

//		fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
//		Fl::check();
//		Fl::awake();
//		Fl::unlock();
	}
}

void multiimportCallback(Fl_Widget*, void*T) {
	int dest=((MultiTable *)T)->selected_index();
	do_importmulti(dest);
	multiNameDisplay->value(Speicher.getMultiName(dest).c_str());
	((Fl_Widget*)T)->redraw();
}

void MultiTable::draw_cell(TableContext context, 
			  int R, int C, int X, int Y, int W, int H)
{
	static char s[40];
	// int m=R*(cols()/2)+(C/2); // 2 cells per sound
	// int m=R+rows()*(C/2); // 2 cells per sound
	int m=index(R, C);
	if(m<0 || m>=_MULTIS) return;

	switch ( context )
	{
	case CONTEXT_STARTPAGE:
		fl_font(FL_HELVETICA, 12);
		return;

	case CONTEXT_CELL:
	{
		// fl_font(FL_HELVETICA, 12);
		// Keyboard nav and mouse selection highlighting
		// fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, is_selected(R, C) ? FL_YELLOW : FL_WHITE);
		// fl_push_clip(X+2, Y+2, W-4, H-4);
		fl_push_clip(X, Y, W, H);
		{
		// BG COLOR
		if (C==_selected_col && R==_selected_row){
			fl_color((C&1) ? FL_RED: _BGCOLOR);
		}else if (Speicher.getMultiName(m).length()==0){
			fl_color((C&1) ? FL_BLACK: _BGCOLOR);
		}else{
			fl_color((C&1) ? FL_WHITE: _BGCOLOR);
		}
		fl_rectf(X, Y, W, H);

		// TEXT
		fl_color(FL_BLACK);
		if(C&1){
			fl_draw(Speicher.getMultiName(m).c_str(), X+2, Y, W-2, H, FL_ALIGN_LEFT);
		}else{
			sprintf(s, "%d", m); // text for sound number cell
			fl_draw(s, X+1, Y, W-1, H, FL_ALIGN_LEFT);
		}

		// BORDER
		fl_color(color()); 
		fl_rect(X, Y, W, H);
		}
		fl_pop_clip();
		return;
	}

	case CONTEXT_TABLE:
		fprintf(stderr, "TABLE CONTEXT CALLED\n");
		return;

	case CONTEXT_ROW_HEADER:
	case CONTEXT_COL_HEADER:
	case CONTEXT_ENDPAGE:
	case CONTEXT_RC_RESIZE:
	case CONTEXT_NONE:
	    return;
    }
}

void MultiTable::event_callback(Fl_Widget*, void *data)
{
	MultiTable *o = (MultiTable*)data;
	o->event_callback2();
}

void MultiTable::event_callback2()
{
//	printf("'%s' callback: ", (label() ? label() : "?"));
	int m;
//	TableContext context = callback_context();
//	printf("Row=%d Col=%d Context=%d Event=%d InteractiveResize? %d\n",
//		R, C, (int)context, (int)Fl::event(), (int)is_interactive_resize());
	Fl_Menu_Item menu_rclick[] = {
		{_("Load"), 0, multiloadmnuCallback, (void*)this },
		{_("&Copy"), 0, multicopymnuCallback, (void*)this },
		{_("Cu&t"), 0, multicutmnuCallback, (void*)this },
		{_("&Paste all"), 0, multipastemnuCallback, (void*)this },
		{_("Paste tuning"), 0, multipastetuningmnuCallback, (void*)this },
		{_("Paste controllers"), 0, multipastecontrollersmnuCallback, (void*)this },
		{_("Rename"), 0, multirenamemnuCallback, (void*)this },
		{_("Clear"), 0, multiclearmnuCallback, (void*)this },
		{_("Import"), 0, multiimportCallback, (void*)this },
		{_("Export"), 0, multiexportCallback, (void*)this },
		{ 0 }
	};
	switch(Fl::event()){
		// printf("MultiTable::event_callback2(%u)\n", Fl::event());
		case FL_PUSH:{
			int R = callback_row(), C = callback_col();
			if(Fl::event_clicks()){ // Double click or more
				loadmulti(selected_index());
			}
			// We get an event on C0 R0 outside the table contents??
			// Fortunately we need only odd rows
			// Otherwise event_y might do the trick
			if(R>=0 && C>=0 && (C&1)==1){
				_selected_row=R;
				_selected_col=C;
				m=selected_index();
				multiNameDisplay->value(Speicher.getMultiName(m).c_str());
				if ( Fl::event_button() == FL_RIGHT_MOUSE ) {
					const Fl_Menu_Item *m = menu_rclick->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
					if ( m ) m->do_callback(0, m->user_data());
				}
				redraw();
			}
			break;
		}
		/* How can we do this properly? We're not in handle method
		case FL_FOCUS:		// tells FLTK we're interested in keyboard events
		case FL_UNFOCUS:
				return 1;
		*/
		// Never triggered (unless called from MiniTable?) ??
		/*
		case FL_KEYUP:
			break;
		*/
		case FL_KEYDOWN:
		// case FL_SHOW: // Never caught?
			// Standard movement was handled in MiniTable's handler
			// We have to redraw slave field and handle the enter key
			// We cannot trust R and C due to our special selection handling
			m=selected_index();
			// printf("MultiTable::event_callback2(%u) FL_KEYDOWN %u at %u %u %u\n", Fl::event(), Fl::event_key(), R, C, m);
			if (Fl::event_key()==FL_Enter) loadmulti(m);
			else if (Fl::event_key()==FL_Delete && (minichoice(_("Delete multi %u?"), _("Yes"), _("No"), 0, _("Confirm"), m)==1))
				multiclearmnuCallback(0, this); // Speicher.clearMulti(m);
			else if (Fl::event_key()=='v' && Fl::event_ctrl())
				multipastemnuCallback(0, this);
			else if (Fl::event_key()==FL_Menu){
					int X, Y, W, H;
					find_cell (CONTEXT_CELL, selected_row(), selected_col(), X, Y, W, H);
					// const Fl_Menu_Item *m = menu_rclick->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
					const Fl_Menu_Item *m = menu_rclick->popup(X+W/2, Y+H/2, 0, 0, 0);
					if ( m ) m->do_callback(0, m->user_data());
			}
			multiNameDisplay->value(Speicher.getMultiName(m).c_str());
			redraw();
			break;
	}
}

/**
 * change the multisetup, should be called from a Midi Program Change event on Channel 9
 * @param pgm the Program number between 0 and 127
 */
void UserInterface::changeMulti(int pgm)
{
	char temp_name[_NAMESIZE];
	char spgm[]="***";
	snprintf(spgm, 4, "%i", pgm);
	// Will be called from different thread, hence the lock
	Fl::lock();
	strnrtrim(temp_name, Speicher.getMultiName(pgm).c_str(), _NAMESIZE);
	multiNameInput->value(temp_name); // multichoice
#ifdef _DEBUG
	printf("UserInterface::changeMulti # %u: \"%s\"\n", pgm, temp_name);
#endif
	multiNameInput->redraw(); // multichoice
	multiRoller->value(pgm);// set gui
	multiRoller->redraw();
	multiNoInput->value(spgm);
	multiNoInput->redraw();
	loadmultiCallback(NULL, NULL); // multichoice
	//	Fl::redraw();
	//	Fl::flush();
	Fl::unlock();
	Fl::awake();
}

/**
 * change the sound for a certain voice, should be called from a Midi Program Change event on Channel 1 - 8
 * @param voice the voice between 0 and 7 (it is not clear if the first Midi channel is 1 (which is usually the case in the hardware world) or 0)
 * @param pgm the Program number between 0 and 127
 */
void UserInterface::changeSound(int voice, int pgm)
{
	// Will be called from different thread, hence the lock
	Fl::lock();
	sound_recall(voice, pgm);
	Fl::unlock();
	Fl::awake();
// printf("changeSound: after sound_recall\n");
	/*
	if ((voice >-1) && (voice < _MULTITEMP) && (pgm>-1) && (pgm<_SOUNDS))
	{
	Fl::lock();
		int oldVoice = currentVoice;
        char temp_name[_NAMESIZE];
		currentVoice = voice;
        strnrtrim(temp_name, Speicher.getSoundName(pgm).c_str(),_NAMESIZE);
// #ifdef _DEBUG
        printf("changeSound: change voice %u to sound %u \"%s\"\n", voice, pgm, temp_name);
// #endif
		soundNameInput[voice]->value(temp_name);
		//soundNameInput[voice]->damage(FL_DAMAGE_ALL);
		soundNameInput[voice]->redraw();
		soundRoller[voice]->value(pgm);// set gui
		soundRoller[voice]->redraw();
		char spgm[]="***";
		snprintf(spgm, 4, "%i", pgm);
		soundNoInput[voice]->value(spgm);
		soundNoInput[voice]->redraw();
		soundLoadBtn1Callback(NULL, NULL);
//		Fl::redraw();
//		Fl::flush();
		currentVoice = oldVoice;
	Fl::awake();
	Fl::unlock();
	}
	*/
}

// ---------------------------------------------------------------
// -------------------- screen initialization --------------------
// ---------------------------------------------------------------

// These should be members of UserInterface?
static unsigned char idata_miniMini1[_LOGO_WIDTH1*_LOGO_HEIGHT1*3];
Fl_RGB_Image image_miniMini1(idata_miniMini1, _LOGO_WIDTH1, _LOGO_HEIGHT1, 3, 0);
static unsigned char idata_miniMini2[_LOGO_WIDTH2*_LOGO_HEIGHT2*3];
Fl_RGB_Image image_miniMini2(idata_miniMini2, _LOGO_WIDTH2, _LOGO_HEIGHT2, 3, 0);

float EG_rate_min=0.5;
float EG_rate_max=0.01;

void UserInterface::make_EG(int voice, int EG_base, int x, int y, const char* EG_label){
	  // ----------- knobs for envelope generator n ---------------
	  { Fl_Group* o = new Fl_Group(x, y, 200, 44, EG_label);
		groups[nGroups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(_TEXT_SIZE);
		// Attack
		{ MiniKnob* o = new MiniKnob(x+10, y+6, 25, 25, "A");
		  o->labelsize(_TEXT_SIZE); 
		  o->argument(EG_base);  
		  o->minimum(EG_rate_min);
		  o->maximum(EG_rate_max);
		  o->callback((Fl_Callback*)parmCallback);
		  knobs[voice][o->argument()] = o;
		}
		// Decay
		{ MiniKnob* o = new MiniKnob(x+40, y+6, 25, 25, "D");
		  o->labelsize(_TEXT_SIZE);
		  o->argument(EG_base+1);
		  o->minimum(EG_rate_min);
		  o->maximum(EG_rate_max);
		  o->callback((Fl_Callback*)parmCallback);
		  knobs[voice][o->argument()] = o;
		}
		// Sustain
		{ MiniKnob* o = new MiniKnob(x+70, y+6, 25, 25, "S");
		  o->labelsize(_TEXT_SIZE);
		  o->argument(EG_base+2);
		 // o->minimum(0);
		 // o->maximum(0.001);
		  o->callback((Fl_Callback*)parmCallback);
		  knobs[voice][o->argument()] = o;
		}
		// Release
		{ MiniKnob* o = new MiniKnob(x+100, y+6, 25, 25, "R");
		  o->labelsize(_TEXT_SIZE);
		  o->argument(EG_base+3);
		  o->minimum(EG_rate_min);
		  o->maximum(EG_rate_max);
		  o->callback((Fl_Callback*)parmCallback);
		  knobs[voice][o->argument()] = o;
		}
		// Repeat
		{ Fl_Light_Button* o = new Fl_Light_Button(x+136, y+11, 55, 15, _("repeat"));
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(_TEXT_SIZE);
		  o->argument(EG_base+4);
		  o->callback((Fl_Callback*)parmCallback);
		  knobs[voice][o->argument()] = o;
		}
		o->end();
	  }
}

void make_labeltip(Fl_Widget* o, const char* tip){
// measure_label gives inconsistent results
// possibly depending on previous widget font
// ok if called right before o->end
// Should set font and size anyway??
// fl_font(o->labelfont(), o->labelsize());
	int wl=0, hl=0, x, y, a;
	a=o->align();
	o->measure_label(wl, hl);
	x=o->x()+(o->w()-wl)/2; // Todo handle X alignment ??
	y=o->y();
	if (a & FL_ALIGN_BOTTOM){
		y+=o->h();
		if (a & FL_ALIGN_INSIDE) y -= hl;
	}
	if (a & FL_ALIGN_TOP){
		if ((a & FL_ALIGN_INSIDE)==0) y -= hl;
	}
	// what about border width ??
// printf("make_labeltip: %u %u %u %u %u %u\n", o->y(), o->h(), o->labelsize(), o->align(), hl, wl); // hl is 9 or 18 ??
// printf("make_labeltip: %u %u %u\n", o->align(), hl, wl); // hl is 9 or 18 ??
	Fl_Box* b = new Fl_Box(x, y, wl, hl);//, o->label());
	b->tooltip(tip);
	// b->box(FL_BORDER_FRAME);
	b->box(FL_NO_BOX);
}

void UserInterface::make_filter(int voice, int filter_base, int minidisplay, int x, int y){
	{MiniPositioner* o = new MiniPositioner(x, y, 70, 79, _("cut"));
	o->xbounds(0,9000);
	o->ybounds(500,0);
	o->selection_color(0);
	o->xstep(500);
	o->box(FL_BORDER_BOX);
	o->labelsize(_TEXT_SIZE);
	o->argument(filter_base);
	o->yvalue(200);
	o->callback((Fl_Callback*)parmCallback);
	knobs[voice][o->argument()] = o;
	/* MiniKnob* o = f1cut1 = new Fl_SteinerKnob(344, 51, 34, 34, "cut");
	  o->labelsize(_TEXT_SIZE);
	  o->argument(30);
	  o->maximum(10000);
	  o->value(50);
	  o->callback((Fl_Callback*)parmCallback);
	*/
	}
	{ MiniKnob* o = new MiniKnob(x+75, y+2, 25, 25, _("q"));
	  o->labelsize(_TEXT_SIZE);
	  o->argument(filter_base+1);
	  o->minimum(0.9);
	  o->value(0.9);
	  o->maximum(0.01);
	  o->callback((Fl_Callback*)parmCallback);
	  knobs[voice][o->argument()] = o;
	}
	{ MiniKnob* o = new MiniKnob(x+85, y+39, 20, 20, _("vol"));
	  o->labelsize(_TEXT_SIZE);
	  o->argument(filter_base+2);
	  o->callback((Fl_Callback*)parmCallback);
	  o->minimum(-1);
	  o->value(0.5);
	  o->maximum(1);
	  knobs[voice][o->argument()] = o;
	}
	{ Fl_Value_Input* o = new Fl_Value_Input(x+72, y+69, 38, 15);
	  o->box(FL_ROUNDED_BOX);
	  o->labelsize(_TEXT_SIZE);
	  o->textsize(_TEXT_SIZE);
	  o->maximum(9499);
	  o->step(0.01);
	  o->value(200);
	  o->argument(filter_base);
	  o->callback((Fl_Callback*)cutoffCallback);
	  miniInput[voice][minidisplay]=o;
	}
}

void UserInterface::make_osc(int voice, int osc_base, int minidisplay_base, int choice_base, int x, int y, const char* osc_label, const char* mod_label){
	/*
	  {
		Fl_Box* d = new Fl_Box(x+130, y+190, 30, 22, osc_label);
		d->labelsize(_TEXT_SIZE);
		d->labelcolor(FL_BACKGROUND2_COLOR);
	  }
	  */
	int w=63;
	{ MiniKnob* o= new MiniKnob(x, y+30, w, 60, _("frequency"));
	// { Fl_Dial* o= new Fl_Dial(x, y+30, w, 60, "frequency");
		// o->tooltip("trouloulou"); // Works for Fl_Dial
		o->labelsize(_TEXT_SIZE);
		o->maximum(1000); 
		o->argument(osc_base);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ MiniPositioner* o = new MiniPositioner(x, y+10, w, 100, _("tune"));
		o->xbounds(0,16);
		o->ybounds(1,0);
		o->box(FL_BORDER_BOX);
		o->xstep(1);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+2);
		o->selection_color(0);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()]=o;
	}
	{ Fl_Value_Input* o = new Fl_Value_Input(x, y+120, w, 15); // Fixed frequency display for oscillator
		o->box(FL_ROUNDED_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->maximum(1000);
		o->step(0.001);
		o->argument(osc_base); // Same as (fixed) frequency dial
		o->callback((Fl_Callback*)fixedfrequencyCallback);
		miniInput[voice][minidisplay_base]=o;
	}
	{ Fl_Value_Input* o = new Fl_Value_Input(x, y+120, w, 15); // Tune display for oscillator
		o->box(FL_ROUNDED_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->maximum(10000);
		o->argument(osc_base+2); // Same as "tune" Fl_positioner
		o->step(0.001);
		o->callback((Fl_Callback*)tuneCallback);
		miniInput[voice][minidisplay_base+1]=o;
	}
	{ Fl_Light_Button* o = new Fl_Light_Button(x, y+140, w, 15, _("fixed freq."));
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+1);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ Fl_Light_Button* o = new Fl_Light_Button(x+65, y+7, 40, 15, _("boost"));
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+3);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ Fl_Choice* o = new Fl_Choice(x+107, y+7, 120, 15, _("freq modulator 1"));
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(choice_base);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_fmod);
		choices[voice][o->argument()]=o;
	}
	{ MiniKnob* o = new MiniKnob(x+233, y+3, 25, 25, _("amount"));
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+4);  
		o->minimum(-1000);
		o->maximum(1000);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ Fl_Light_Button* o = new Fl_Light_Button(x+65, y+26, 40, 15, _("exp."));
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+5);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	} 
	{ Fl_Light_Button* o = new Fl_Light_Button(x+65, y+45, 40, 15, _("mult."));
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+116);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ Fl_Choice* o = new Fl_Choice(x+107, y+45, 120, 15, _("freq modulator 2"));
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(choice_base+1);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_fmod);
		choices[voice][o->argument()]=o;
	}
	{ MiniKnob* o = new MiniKnob(x+232, y+40, 25, 25, _("amount"));
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+6); 
		o->minimum(-1000);
		o->maximum(1000);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ Fl_Light_Button* o = new Fl_Light_Button(x+77, y+82, 40, 15, _("mult."));
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+120);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ Fl_Choice* o = new Fl_Choice(x+119, y+82, 120, 15, _("amp modulator 1"));
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(choice_base+2);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_amod);
		choices[voice][o->argument()]=o;
	}
	{ MiniKnob* o = new MiniKnob(x+245, y+77, 25, 25, _("amount"));
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+8); 
		o->minimum(-1);
		o->maximum(1);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ Fl_Light_Button* o = new Fl_Light_Button(x+77, y+116, 40, 15, _("mult."));
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+117);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ Fl_Choice* o = new Fl_Choice(x+119, y+116, 120, 15, mod_label);
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(choice_base+3);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_amod);
		choices[voice][o->argument()]=o;
	}
	{ MiniKnob* o = new MiniKnob(x+245, y+111, 25, 25, _("amount"));
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+9);  
		o->minimum(-1);
		o->maximum(1);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ MiniKnob* o = new MiniKnob(x+77, y+144, 20, 20, _("start phase"));
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+118); 
		o->minimum(0);
		o->maximum(360);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ Fl_Light_Button* o = new Fl_Light_Button(x+102, y+147, 10, 15); // Phase reset enable
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+119);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
	{ Fl_Choice* j = new Fl_Choice(x+119, y+147, 120, 15, _("waveform"));
		j->box(FL_BORDER_BOX);
		j->down_box(FL_BORDER_BOX);
		j->labelsize(_TEXT_SIZE);
		j->textsize(_TEXT_SIZE);
		j->align(FL_ALIGN_TOP_LEFT);
		j->argument(choice_base+4);
		choices[voice][j->argument()] = j;
		j->callback((Fl_Callback*)choiceCallback);
		j->menu(menu_wave);
	}
	  { MiniKnob* o = new MiniKnob(x+245, y+144, 20, 20, _("fm out vol"));
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+12);
		o->callback((Fl_Callback*)parmCallback);
		knobs[voice][o->argument()] = o;
	}
}

void do_close(Fl_Widget * o, void *){
	bool changed=multi_changed;
	for(int i=0; i<_MULTITEMP && !changed; i++)
		changed|=sound_changed[i];
	if(changed){
		switch (minichoice(_("What do you want to do?"), _("Save and exit"), _("Don't exit"), _("Abandon changes and exit"), _("Exit"))){
			case 1:
				multistoreCallback((Fl_Widget*)0, (void*)0);
				// loop for voices and soundstoreCallback(Fl_Widget*, void*) ??
				for(int i=0; i<_MULTITEMP; i++)
					soundstore(i, &Speicher.sounds[atoi(soundNoInput[i]->value())]);
				Speicher.saveSounds();
				Speicher.saveMultis();
				// Fall through next case
			case 3:
				Fl::first_window()->hide();
				break;
			default: // Don't exit
				break;
		}
	}else{
		if(minichoice(_("Do you really want to exit?"), _("Yes"), _("No"), 0, _("Exit"))==1)
			Fl::first_window()->hide();
	}
}

MiniWindow* UserInterface::make_window(const char* title) {

	// Internationalization
	// see http://www.labri.fr/perso/fleury/posts/programming/a-quick-gettext-tutorial.html
	// printf("LC_ALL: %s\n", getenv("LC_ALL"));
	// printf("LC_MESSAGES: %s\n", getenv("LC_MESSAGES"));
	// printf("LC_TYPE: %s\n", getenv("LC_TYPE"));
	printf("LANG: %s\n", getenv("LANG"));
	// printf("LANGUAGE: %s\n", getenv("LANGUAGE"));
	setlocale (LC_ALL, "");
	// bindtextdomain ("minicomputer", "/usr/share/locale/"); // Does not work ??
	// bindtextdomain ("minicomputer", getenv("PWD"));
	textdomain ("minicomputer");
	// Translation files were generated as follows:
	// xgettext --keyword=_ --language=C --add-comments --sort-output -o editor/po/minicomputer.pot editor/syntheditor.cxx
	// msginit --input=editor/po/minicomputer.pot --locale=fr --output=editor/po/fr/minicomputer.po
	// msgfmt --output-file=editor/po/fr/minicomputer.mo editor/po/fr/minicomputer.po
	// su -c "cp editor/po/fr/minicomputer.mo /usr/share/locale/fr/LC_MESSAGES/"
	// When more text strings have been added:
	// msgmerge --update editor/po/fr/minicomputer.po editor/po/minicomputer.pot
	// Test:
	// gettext minicomputer "Voice " --> "Voix "
	// printf("none -> %s\n", _("none")); --> aucune
// #define _DEBUG
	// Ugly hack - these should come from minichoice.cxx
	_("OK");
	_("Cancel");

	for(unsigned int i=0; i<sizeof(menu_amod)/sizeof(menu_amod[0]); i++){
#ifdef _DEBUG
		printf("label amp. modulator #%u: %s %s\n", i, menu_amod[i].label(), _(menu_amod[i].label()));
#endif
		menu_amod[i].label(_(menu_amod[i].label()));
	}
	for(unsigned int i=0; i<sizeof(menu_fmod)/sizeof(menu_fmod[0]); i++){
#ifdef _DEBUG
		printf("label freq. modulator #%u: %s %s\n", i, menu_fmod[i].label(), _(menu_fmod[i].label()));
#endif
		menu_fmod[i].label(_(menu_fmod[i].label()));
	}
	for(unsigned int i=0; i<sizeof(menu_wave)/sizeof(menu_wave[0]); i++){
#ifdef _DEBUG
		printf("label wave #%u: %s %s\n", i, menu_wave[i].label(), _(menu_wave[i].label()));
#endif
		menu_wave[i].label(_(menu_wave[i].label()));
	}
	for(unsigned int i=0; i<sizeof(menu_filter)/sizeof(menu_filter[0]); i++){
#ifdef _DEBUG
		printf("label filter #%u: %s %s\n", i, menu_filter[i].label(), _(menu_filter[i].label()));
#endif
		menu_filter[i].label(_(menu_filter[i].label()));
	}

	currentVoice=0;
	currentMulti=0;

	// special treatment for the mix knobs and MIDI settings
	// they are saved in the multi, not in the sound
	needs_multi[101]=1; // Id vol
	needs_multi[106]=1; // Mix vol
	needs_multi[107]=1; // Pan
	needs_multi[108]=1; // Aux 1
	needs_multi[109]=1; // Aux 2
	needs_multi[125]=1; // Channel
	needs_multi[126]=1; // Velocity
	needs_multi[127]=1; // Test note
	needs_multi[128]=1; // Note min
	needs_multi[129]=1; // Note max
	needs_multi[130]=1; // Transpose
	needs_multi[155]=1; // Poly link


	// show parameter on fine tune only when relevant
	for (int i=0;i<_PARACOUNT;++i) needs_finetune[i]=1;
	needs_finetune[0]=0; // clear state
	needs_finetune[3]=2; // Osc 1 tune
	needs_finetune[18]=2; // Osc 2 tune
	needs_finetune[30]=2; // Filter 1 A cut
	needs_finetune[33]=2; // Filter 1 B cut
	needs_finetune[40]=2; // Filter 2 A cut
	needs_finetune[43]=2; // Filter 2 B cut
	needs_finetune[50]=2; // Filter 3 A cut
	needs_finetune[53]=2; // Filter 3 B cut
	needs_finetune[90]=2; // Osc 3 tune
	needs_finetune[125]=0; // Midi channel
	needs_finetune[126]=0; // Audition note velocity
	needs_finetune[127]=0; // Audition note number
	needs_finetune[128]=0; // Min note
	needs_finetune[129]=0; // Max note
	needs_finetune[130]=0; // Transpose
	needs_finetune[155]=0; // Poly link
	needs_finetune[2]=0; // Osc 1 Fixed frequency
	needs_finetune[4]=0; // Osc 1 Boost
	needs_finetune[117]=0; // Osc 1 Mult freq modulator 2
	needs_finetune[121]=0; // Osc 1 Mult amp modulator 1
	needs_finetune[118]=0; // Osc 1 Mult amp modulator 2
	needs_finetune[120]=0; // Osc 1 Start phase enable
	needs_finetune[17]=0; // Osc 2 Fixed frequency
	needs_finetune[19]=0; // Osc 2 Boost
	needs_finetune[132]=0; // Osc 2 Mult freq modulator 2
	needs_finetune[136]=0; // Osc 2 Mult amp modulator 1
	needs_finetune[133]=0; // Osc 2 Mult amp modulator 2
	needs_finetune[135]=0; // Osc 2 Start phase enable
	needs_finetune[115]=0; // Osc 2 sync to osc 1
	needs_finetune[154]=0; // Osc 3 Start phase enable
	needs_finetune[140]=0; // Mult morph mod 2
	needs_finetune[137]=0; // Bypass filter
	needs_finetune[64]=0; // EG 1 repeat
	needs_finetune[69]=0; // EG 2 repeat
	needs_finetune[74]=0; // EG 3 repeat
	needs_finetune[79]=0; // EG 4 repeat
	needs_finetune[84]=0; // EG 5 repeat
	needs_finetune[89]=0; // EG 6 repeat
	needs_finetune[139]=0; // Legato
	needs_finetune[143]=0; // Time modulator 2 mult.
	needs_finetune[145]=0; // oscillator 3 mult. mod wheel

	for (int i=0;i<_PARACOUNT;++i) is_button[i]=0;
	is_button[2]=1; // Fixed frequency osc 1
	is_button[4]=1; // boost button osc 1
	is_button[6]=1; // exp. button osc 1
	is_button[17]=1; // Fixed frequency osc 2
	is_button[19]=1; // boost button osc 2
	is_button[21]=1; // exp. button osc 2
	// the repeat buttons of the mod egs
	is_button[64]=1;
	is_button[69]=1;
	is_button[74]=1;
	is_button[79]=1;
	is_button[84]=1;
	is_button[89]=1;
	is_button[115]=1;
	// modulation mult. buttons
	is_button[117]=1;
	is_button[118]=1;
	is_button[132]=1;
	is_button[140]=1;
	is_button[121]=1;
	is_button[136]=1;
	is_button[133]=1;
	is_button[120]=1; // Osc 1 start phase enable
	is_button[135]=1; // Osc 2 start phase enable
	is_button[154]=1; // Osc 3 start phase enable
	is_button[137]=1; // Bypass filters
	is_button[139]=1; // Legato
	is_button[143]=1; // Time modulator 2 mult.
	is_button[145]=1; // oscillator 3 mult. mod wheel
	is_button[155]=1; // Poly link

	transmit=true;

	MiniWindow* o = new MiniWindow(_INIT_WIDTH, _INIT_HEIGHT, title);
	o->color((Fl_Color)_BGCOLOR);
	o->user_data((void*)(this));
	// o->default_callback((Fl_Window *) o, (void *)do_close); // Not working
	o->callback((Fl_Callback*)do_close);

	for (int i=0;i<_CHOICECOUNT;++i) {
		choices[currentVoice][i]=NULL;
	}
	for (int i=0;i<_PARACOUNT;++i) {
		knobs[currentVoice][i]=NULL;
	}

// tabs beginning ------------------------------------------------------------
	{ Fl_Tabs* o = new Fl_Tabs(0, 0, _INIT_WIDTH, _INIT_HEIGHT);
		o->callback((Fl_Callback*)tabCallback);
	int i;
	for (i=0; i<_MULTITEMP; ++i) // generate a tab for each voice
	{
		{ 
		ostringstream oss;
		oss<<_("Voice ")<<(i+1); // create name for tab
		tablabel[i]=oss.str();
		Fl_Group* o = new Fl_Group(1, 10, _INIT_WIDTH, _INIT_HEIGHT, tablabel[i].c_str());
		groups[nGroups++]=o;
		o->color((Fl_Color)_BGCOLOR);
		o->labelsize(_TEXT_SIZE);
		// o->labelcolor(FL_BACKGROUND2_COLOR); 
		o->box(FL_BORDER_FRAME);

	// Show voice number next to the logo
	{ Fl_Box* d = new Fl_Box(845, 455, 40, 40, voiceName[i]);
		d->labelsize(48);
		d->labelcolor(FL_RED);
		voiceDisplayBox[i]=d;
		// d->tooltip(voiceName[i]);
	}

	// Oscillator 1
	{ Fl_Group* o = new Fl_Group(5, 17, 300, 212, _("oscillator 1"));
		groups[nGroups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_BACKGROUND2_COLOR);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor(FL_BACKGROUND2_COLOR);
		o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE); // No FL_ALIGN_WRAP!!
		make_osc(i, 1, 0, 0, 16, 20, _("oscillator 1"), _("amp modulator 2"));
		// Oscillator 1 specific setting
		{ Fl_Choice* j = new Fl_Choice(134, 196, 120, 15, _("sub waveform"));
			j->box(FL_BORDER_BOX);
			j->down_box(FL_BORDER_BOX);
			j->labelsize(_TEXT_SIZE);
			j->textsize(_TEXT_SIZE);
			j->align(FL_ALIGN_TOP_LEFT);
			j->argument(15);
			choices[i][j->argument()] = j;
			j->callback((Fl_Callback*)choiceCallback);
			j->menu(menu_wave);
		}
		make_labeltip(o, _("One of the primary sound sources. Sounds originate from here."));
		o->end();
	}
	// Oscillator 2
	{ Fl_Group* o = new Fl_Group(5, 238, 300, 212, _("oscillator 2"));
		groups[nGroups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_BACKGROUND2_COLOR);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor(FL_BACKGROUND2_COLOR);
		o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
		int x=16;
		make_osc(i, 16, 2, 6, x, 244, _("oscillator 2"), _("fm out amp modulator"));
		// Oscillator 2 specific settings

		{ Fl_Light_Button* o = new Fl_Light_Button(x, 404, 63, 15, _("sync to osc1"));
			o->box(FL_BORDER_BOX);
			o->selection_color((Fl_Color)89);
			o->labelsize(_TEXT_SIZE);
			o->argument(115);
			o->callback((Fl_Callback*)parmCallback);
			knobs[i][o->argument()] = o;
		}

		{ MiniKnob* o = new MiniKnob(x+77, 417, 20, 20, _("base width"));
			o->labelsize(_TEXT_SIZE);
			o->argument(146); 
			o->minimum(0.01);
			o->maximum(0.99);
			o->callback((Fl_Callback*)parmCallback);
			knobs[i][o->argument()] = o;
		}

		{ Fl_Choice* j = new Fl_Choice(x+119, 420, 120, 15, _("pulse width modulator"));
			j->box(FL_BORDER_BOX);
			j->down_box(FL_BORDER_BOX);
			j->labelsize(_TEXT_SIZE);
			j->textsize(_TEXT_SIZE);
			j->align(FL_ALIGN_TOP_LEFT);
			j->argument(18);
			choices[i][j->argument()] = j;
			j->callback((Fl_Callback*)choiceCallback);
			j->menu(menu_fmod);
		}
		{ MiniKnob* o = new MiniKnob(x+245, 417, 20, 20, _("amount"));
			o->labelsize(_TEXT_SIZE);
			o->argument(147);
			o->minimum(-1);
			o->maximum(1);
			o->callback((Fl_Callback*)parmCallback);
			knobs[i][o->argument()] = o;
		}
		make_labeltip(o, _("One of the primary sound sources. Sounds originate from here."));
		o->end();
	}

	// Filters
	{ Fl_Group* o = new Fl_Group(312, 17, 277, 433, _("filters"));
	  groups[nGroups++]=o;
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	  o->labelsize(_TEXT_SIZE);
	  o->labelcolor(FL_BACKGROUND2_COLOR);
	  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
	  { Fl_Group* o = new Fl_Group(330, 28, 241, 92, _("filter 1"));
		groups[nGroups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(_TEXT_SIZE);
		make_filter(i, 30, 4, 340, 31);
		make_filter(i, 33, 5, 456, 31);
		o->end();
	  }
		{ MiniKnob* o = new MiniKnob(559, 40, 20, 20, _("tracking"));
			o->labelsize(_TEXT_SIZE);
			o->argument(150);
			o->minimum(-1);
			o->value(0);
			o->maximum(1);
			o->callback((Fl_Callback*)parmCallback);
			knobs[i][o->argument()] = o;
		}
	  { Fl_Group* o = new Fl_Group(330, 132, 241, 92, _("filter 2"));
		groups[nGroups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(_TEXT_SIZE);
		make_filter(i, 40, 6, 340, 135);
		make_filter(i, 43, 7, 456, 135);
	/*
		{ Fl_Button* o = new Fl_Button(426, 139, 45, 15, "copy ->");
		  o->box(FL_BORDER_BOX);
		  o->labelsize(_TEXT_SIZE);
		  o->argument(21);
		//o->callback((Fl_Callback*)copyparmCallback);
		}
		{ Fl_Button* o = new Fl_Button(426, 163, 45, 15, "<- copy");
		  o->box(FL_BORDER_BOX);
		  o->labelsize(_TEXT_SIZE);
		  o->argument(22);
	  //o->callback((Fl_Callback*)copyparmCallback);
		}*/
		o->end();
	  }
		{ MiniKnob* o = new MiniKnob(559, 144, 20, 20, _("tracking"));
			o->labelsize(_TEXT_SIZE);
			o->argument(151);
			o->minimum(-1);
			o->value(0);
			o->maximum(1);
			o->callback((Fl_Callback*)parmCallback);
			knobs[i][o->argument()] = o;
		}
	  { Fl_Group* o = new Fl_Group(330, 238, 241, 92, _("filter 3"));
		groups[nGroups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(_TEXT_SIZE);
		make_filter(i, 50, 8, 340, 241);
		make_filter(i, 53, 9, 456, 241);
		o->end();
	  } 
		{ MiniKnob* o = new MiniKnob(559, 250, 20, 20, _("tracking"));
			o->labelsize(_TEXT_SIZE);
			o->argument(152);
			o->minimum(-1);
			o->value(0);
			o->maximum(1);
			o->callback((Fl_Callback*)parmCallback);
			knobs[i][o->argument()] = o;
		}
	  { Fl_Choice* o = new Fl_Choice(327, 351, 80, 15, _("morph mod 1"));
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(5);
		o->callback((Fl_Callback*)choiceCallback);
		choices[i][o->argument()]=o;
	  }
	  { MiniKnob* o = new MiniKnob(326, 377, 25, 25, _("amount"));
		o->labelsize(_TEXT_SIZE);
		o->minimum(-2);
		o->maximum(2);
		o->argument(38);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { MiniKnob* o = new MiniKnob(420, 340, 60, 57, _("morph"));
		o->type(MiniKnob::DOTLIN);
		o->scaleticks(0);
		o->labelsize(_TEXT_SIZE);
		o->maximum(0.5f);
		o->argument(56);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(495, 351, 80, 15, _("morph mod 2"));
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(11);
		o->callback((Fl_Callback*)choiceCallback);
		choices[i][o->argument()]=o;
	  }
	  // Mult button morph mod 2
	  { Fl_Light_Button* o = new Fl_Light_Button(495, 382, 40, 15, _("mult."));
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(140);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { MiniKnob* o = new MiniKnob(551, 377, 25, 25, _("amount"));
		o->labelsize(_TEXT_SIZE);
		o->argument(48);
		o->minimum(-2);
		o->maximum(2);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(410, 420, 80, 15, _("filter type"));
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_filter);
		o->argument(19);
		o->callback((Fl_Callback*)choiceCallback);
		choices[i][o->argument()]=o;
	  }

	  make_labeltip(o, _("Filters (unless bypassed) shape the harmonic contents of the sound."));
	  o->end(); // Group "filters"
	  filtersGroup[i]=o;
	  // The two following buttons are not really part of the filter parameters
	  { Fl_Light_Button* o = new Fl_Light_Button(327, 420, 80, 15, _("bypass filters"));
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		// o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->selection_color(FL_RED);
		// o->color(FL_LIGHT1, FL_RED);
		o->argument(137);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { Fl_Button* o = new Fl_Button(495, 420, 80, 15, _("clear state"));
	  // { MiniButton* o = new MiniButton(490, 430, 85, 15, _("clear state"));
		o->tooltip(_("reset the filter and delay"));
		o->box(FL_BORDER_BOX);
		// o->box(FL_RSHADOW_BOX); // Ugly
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->argument(0);
		o->callback((Fl_Callback*)clearstateCallback);
		clearStateBtn[i]=o;
	  }
	}

	// Modulators
	{ Fl_Group* o = new Fl_Group(595, 17, 225, 433, _("modulators"));
	  groups[nGroups++]=o;
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	  o->labelsize(_TEXT_SIZE);
	  o->labelcolor(FL_BACKGROUND2_COLOR);
	  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
	  // envelope generators
	  int h=57;
	  make_EG(i, 60, 608, 31, _("Envelope generator 1"));
	  make_EG(i, 65, 608, 31+h, _("Envelope generator 2"));
	  make_EG(i, 70, 608, 31+2*h, _("Envelope generator 3"));
	  make_EG(i, 75, 608, 31+3*h, _("Envelope generator 4"));
	  make_EG(i, 80, 608, 31+4*h, _("Envelope generator 5"));
	  make_EG(i, 85, 608, 31+5*h, _("Envelope generator 6"));
	  
	  // mod oscillator
	  int y=31+6*h;
	  { Fl_Group* o = new Fl_Group(608, y, 200, 65, _("mod osc"));
		groups[nGroups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(_TEXT_SIZE);

		{  MiniPositioner* o = new MiniPositioner(620, y+5, 55, 50, _("tune"));
		// {  Fl_Positioner* o = new Fl_Positioner(620, y+5, 55, 50,"tune");
		// o->tooltip("Tralala"); // Never displayed for Fl_Positioner ??
		o->xbounds(0,128);
		o->ybounds(1,0);
		o->box(FL_BORDER_BOX);
		o->xstep(1);
		o->labelsize(_TEXT_SIZE);
		o->argument(90);
		o->selection_color(0);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
		}
		{ Fl_Choice* o = new Fl_Choice(680, y+12, 85, 15, _("waveform"));
		  o->box(FL_BORDER_BOX);
		  o->down_box(FL_BORDER_BOX);
		  o->labelsize(_TEXT_SIZE);
		  o->textsize(_TEXT_SIZE);
		  o->align(FL_ALIGN_TOP_LEFT);
		  o->menu(menu_wave);
		  o->argument(12);
		  o->callback((Fl_Callback*)choiceCallback);
		  choices[i][o->argument()]=o;
		} 
		{ MiniKnob* o = new MiniKnob(768, y+7, 20, 20, _("start phase"));
			o->labelsize(_TEXT_SIZE);
			o->argument(153); 
			o->minimum(0);
			o->maximum(360);
			o->callback((Fl_Callback*)parmCallback);
			knobs[i][o->argument()] = o;
		}
		{ Fl_Light_Button* o = new Fl_Light_Button(790, y+12, 10, 15); // Phase reset enable
			o->box(FL_BORDER_BOX);
			o->selection_color((Fl_Color)89);
			o->labelsize(_TEXT_SIZE);
			o->argument(154);
			o->callback((Fl_Callback*)parmCallback);
			knobs[i][o->argument()] = o;
		}
		{ Fl_Value_Input* o = new Fl_Value_Input(680, y+30, 50, 15, _("Hz")); // frequency display for modulation oscillator
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(_TEXT_SIZE);
		  o->align(FL_ALIGN_RIGHT);
		  o->maximum(10000);
		  o->step(0.0001);
		  o->textsize(_TEXT_SIZE);
		  o->callback((Fl_Callback*)tuneCallback);
		  o->argument(90);
		  miniInput[i][10]=o;
		}
		{ Fl_Value_Input* o = new Fl_Value_Input(680, y+48, 50, 15, _("BPM")); // BPM display for modulation oscillator
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(_TEXT_SIZE);
		  o->align(FL_ALIGN_RIGHT);
		  o->maximum(10000);
		  o->step(0.0001);
		  o->textsize(_TEXT_SIZE);
		  o->callback((Fl_Callback*)BPMtuneCallback);
		  o->argument(90);
		  miniInput[i][11]=o;
		}
		{ Fl_Light_Button* o = new Fl_Light_Button(760, y+40, 40, 15, _("mod."));
		  o->box(FL_BORDER_BOX);
		  o->labelsize(_TEXT_SIZE);
		  o->tooltip(_("Multiply modulation oscillator by modulation wheel"));
		  o->selection_color(FL_RED);
		  o->argument(145);
		  o->callback((Fl_Callback*)parmCallback);
		  knobs[i][o->argument()] = o;
		}
		o->end(); // of mod oscillator
		make_labeltip(o,
			_("The modulation oscillator aka LFO applies repetitive change to the sound parameters.\n"
			"When the mod button is active, you may use the mod wheel to scale the effect."));
	  }
  	  make_labeltip(o, _("Modulators allow various parameters of the sound to vary with time."));
	  o->end(); // of modulators
	}

	// Amp group
	{ Fl_Group* o = new Fl_Group(825, 17, 160, 212, _("amp"));
	  groups[nGroups++]=o;
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	  o->labelsize(_TEXT_SIZE);
	  o->labelcolor(FL_BACKGROUND2_COLOR);
	  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
	{ Fl_Choice* o = new Fl_Choice(844, 35, 85, 15, _("amp modulator"));
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(13);
		o->callback((Fl_Callback*)choiceCallback);
		choices[i][o->argument()]=o;
	}
	{ MiniKnob* o = new MiniKnob(934, 29, 25, 25, _("amount"));
		o->labelsize(_TEXT_SIZE);
		o->argument(100);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}
	{ Fl_Light_Button* o = new Fl_Light_Button(844, 55, 40, 15, _("legato"));
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(_TEXT_SIZE);
		  o->argument(139);
		  o->callback((Fl_Callback*)parmCallback);
		  o->tooltip(_("Don't restart envelope if previous note is still held"));
		  knobs[i][o->argument()] = o;
	}
	{ MiniKnob* o = new MiniKnob(904, 52, 20, 20, "una corda");
		o->labelsize(_TEXT_SIZE);
		o->argument(156);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}
	// amplitude envelope
	{ MiniKnob* o = new MiniKnob(844, 83, 25, 25, "A");
		o->labelsize(_TEXT_SIZE);
		o->argument(102); 
		o->minimum(EG_rate_min);
		o->maximum(EG_rate_max);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { MiniKnob* o = new MiniKnob(874, 83, 25, 25, "D");
		o->labelsize(_TEXT_SIZE);
		o->argument(103); 
		o->minimum(EG_rate_min);
		o->maximum(EG_rate_max);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { MiniKnob* o = new MiniKnob(904, 83, 25, 25, "S");
		o->labelsize(_TEXT_SIZE);
		o->argument(104);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { MiniKnob* o = new MiniKnob(934, 83, 25, 25, "R");
		o->labelsize(_TEXT_SIZE);
		o->argument(105); 
		o->minimum(EG_rate_min);
		o->maximum(EG_rate_max);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	{ MiniKnob* o = new MiniKnob(844, 120, 25, 25, _("id vol"));
		o->labelsize(_TEXT_SIZE); 
		o->argument(101);
		o->minimum(0);
		o->maximum(2);
		o->color(fl_rgb_color(190,160,255));
		o->value(0);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}
	  { MiniKnob* o = new MiniKnob(874, 120, 25, 25, _("aux 1"));
		o->labelsize(_TEXT_SIZE); 
		o->argument(108);
		o->minimum(0);
		o->maximum(2);
		o->color(fl_rgb_color(140,140,255));
		o->value(0);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { MiniKnob* o = new MiniKnob(904, 120, 25, 25, _("aux 2"));
		o->labelsize(_TEXT_SIZE); 
		o->argument(109);
		o->minimum(0);
		o->color(fl_rgb_color(140,140,255));
		o->maximum(2);
		o->value(0);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { MiniKnob* o = new MiniKnob(934, 120, 25, 25, _("mix vol"));
		o->labelsize(_TEXT_SIZE); 
		o->argument(106);
		o->minimum(0);
		o->maximum(2);
		o->color(fl_rgb_color(170,140,255));
		o->value(1);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { Fl_Slider* o = new Fl_Slider(864, 160, 80, 10, _("mix pan"));
		o->labelsize(_TEXT_SIZE); 
		o->box(FL_BORDER_BOX);
		o->argument(107);
		o->minimum(0);
		o->maximum(1);
		o->color(fl_rgb_color(170,140,255));
		o->value(0.5);
		o->type(FL_HORIZONTAL);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { Fl_Choice* j = new Fl_Choice(844, 190, 85, 15, _("pan modulator"));
		j->box(FL_BORDER_BOX);
		j->down_box(FL_BORDER_BOX);
		j->labelsize(_TEXT_SIZE);
		j->textsize(_TEXT_SIZE);
		j->align(FL_ALIGN_TOP_LEFT);
		j->argument(16);
		choices[i][j->argument()] = j;
		j->callback((Fl_Callback*)choiceCallback);
		j->menu(menu_amod);
	  }
	  { MiniKnob* o = new MiniKnob(934, 185, 25, 25, _("amount"));
		o->labelsize(_TEXT_SIZE);
		o->argument(122);
		o->minimum(-1);
		o->maximum(1);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  make_labeltip(o, _("The amp section controls the global volume and pan position of the sound."));
	  o->end(); // of amp
	}

	// Delay group
	{ Fl_Group* o = new Fl_Group(825, 238, 160, 149, _("delay"));
	  groups[nGroups++]=o;
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	  o->labelsize(_TEXT_SIZE);
	  o->labelcolor(FL_BACKGROUND2_COLOR);
	  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);

	  { Fl_Choice* o = new Fl_Choice(844, 265, 85, 15, _("time modulator 1"));
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(14);
		o->callback((Fl_Callback*)choiceCallback);
		choices[i][o->argument()]=o;
	  }
	  { MiniKnob* o = new MiniKnob(934, 260, 25, 25, _("amount"));
		o->labelsize(_TEXT_SIZE);
		o->argument(110);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
		{ Fl_Light_Button* o = new Fl_Light_Button(844, 285, 40, 15, _("mult."));
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(_TEXT_SIZE);
		  o->argument(143);
		  o->callback((Fl_Callback*)parmCallback);
		  knobs[i][o->argument()] = o;
		}
	  { Fl_Choice* o = new Fl_Choice(844, 310, 85, 15, _("time modulator 2"));
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(17);
		o->callback((Fl_Callback*)choiceCallback);
		choices[i][o->argument()]=o;
	  }
	  { MiniKnob* o = new MiniKnob(934, 305, 25, 25, _("amount"));
		o->labelsize(_TEXT_SIZE);
		o->argument(144);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  { MiniKnob* o = new MiniKnob(844, 340, 25, 25, _("delay time"));
		o->labelsize(_TEXT_SIZE);
		o->argument(111);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
		{ Fl_Value_Input* o = new Fl_Value_Input(879, 345, 38, 15, _("BPM")); // BPM display for delay time
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(_TEXT_SIZE);
		  o->align(FL_ALIGN_BOTTOM);
		  o->range(60, 9999);
		  o->step(0.01);
		  o->textsize(_TEXT_SIZE);
		  o->callback((Fl_Callback*)BPMtimeCallback);
		  o->argument(111);
		  miniInput[i][12]=o;
		}
	  { MiniKnob* o = new MiniKnob(934, 340, 25, 25, _("feedback"));
		o->labelsize(_TEXT_SIZE);
		o->argument(112);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	  }
	  make_labeltip(o,
		_("The delay section adds a delayed version of itself to the sound.\n"
		"It is suitable for effects ranging from slight phasing to echo.")
	  );
	  o->end(); // of delay
	}


	// Sounds group
	  int x0=319;
	{ Fl_Group* d = new Fl_Group(x0-7, 461, 364, 48, _("sound"));
	  groups[nGroups++]=d;
	  soundGroup=d;
	  d->box(FL_ROUNDED_FRAME);
	  d->color(FL_BACKGROUND2_COLOR);
	  d->labelsize(_TEXT_SIZE);
	  d->labelcolor(FL_BACKGROUND2_COLOR);
	  d->begin();
	  { Fl_Button* o = new Fl_Button(x0, 471, 10, 30, "@<");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
		o->argument(-1);
		o->callback((Fl_Callback*)soundincdecCallback);
	  }
	  { Fl_Int_Input* o = new Fl_Int_Input(x0+10, 471, 35, 30,_("sound #"));
		o->box(FL_BORDER_BOX);
		o->color(FL_BLACK);
		o->labelsize(_TEXT_SIZE);
		// o->maximum(512);
		o->align(FL_ALIGN_TOP_LEFT);
		o->textsize(16);
		o->textcolor(FL_RED);
		o->cursor_color(FL_RED);
		soundNoInput[i]=o;
		o->callback((Fl_Callback*)soundNoInputCallback);
		o->tooltip(_("Target slot number in the sound memory"));
	  }
	  { Fl_Button* o = new Fl_Button(x0+45, 471, 10, 30, "@>");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
		o->argument(+1);
		o->callback((Fl_Callback*)soundincdecCallback);
	  }
	  { Fl_Input* o = new Fl_Input(x0+60, 471, 150, 14, _("name"));
		o->box(FL_BORDER_BOX);
		o->tooltip(_("Enter the sound name here before storing it"));
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		//o->menubutton()->textsize(_TEXT_SIZE);
		//o->menubutton()->type(Fl_Menu_Button::POPUP1);
		o->align(FL_ALIGN_TOP_LEFT);
		soundchoice[i] = o;
		soundNameInput[i] = o; // ?? global
		o->callback((Fl_Callback*)soundNameInputCallback,NULL);
	  }
	  { Fl_Roller* o = new Fl_Roller(x0+60, 487, 150, 14);
		o->type(FL_HORIZONTAL);
		o->tooltip(_("roll the list of sounds"));
		o->minimum(0);
		o->maximum(511);
		o->step(1);
		//o->slider_size(0.04);
		o->box(FL_BORDER_FRAME);
		soundRoller[i]=o;
		o->callback((Fl_Callback*)soundRollerCallback, NULL);
	  }
	  { Fl_Button* o = new Fl_Button(x0+217, 466, 60, 19, _("load sound"));
		o->tooltip(_("actually load the dialed sound"));
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->callback((Fl_Callback*)soundLoadBtn1Callback,soundchoice[i]);
		loadSoundBtn[i]=o;
	  }
	  { Fl_Button* o = new Fl_Button(x0+217, 487, 60, 19, _("store sound"));
		o->tooltip(_("store current sound in dialed memory"));
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
		o->callback((Fl_Callback*)soundstoreCallback,soundchoice[i]);
		soundstoreBtn[i]=o; // For resize, no need for array here, any will do
	  }

	  { Fl_Button* o = new Fl_Button(x0+284, 466, 60, 19, _("import sound"));
		o->tooltip(_("import single sound to dialed memory slot, you need to load it for playing"));
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		//o->labelcolor((Fl_Color)1);
		o->callback((Fl_Callback*)soundimportbtnCallback,soundchoice[i]);
		importSoundBtn[i]=o;
	  }

	  { Fl_Button* o = new Fl_Button(x0+284, 487, 60, 19, _("export sound"));
		o->tooltip(_("export sound data of dialed memory slot"));
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		//o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->callback((Fl_Callback*)soundexportbtnCallback,soundchoice[i]);
		exportSoundBtn[i]=o;
	  }
	  d->end();
		make_labeltip(d,
		_("These controls allow storing current sound parameters to the selected memory slot,\n"
		"or retrieving them from it. The name is displayed in red when the current settings\n"
		"do not match the selected memory slot."));
	}
	{ MiniKnob* o = new MiniKnob(295, 151, 25, 25, _("osc1 vol"));
		o->labelsize(_TEXT_SIZE);
		// o->align(FL_ALIGN_TOP);
		o->argument(14);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}
	{ MiniKnob* o = new MiniKnob(295, 191, 25, 25, _("sub vol"));
		o->labelsize(_TEXT_SIZE);
		o->argument(141);
		// o->align(FL_ALIGN_TOP);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}
	{ MiniKnob* o = new MiniKnob(295, 252, 25, 25, _("osc2 vol"));
		o->labelsize(_TEXT_SIZE);
		o->argument(29);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}
	{ MiniKnob* o = new MiniKnob(295, 292, 25, 25, _("ring vol"));
		o->labelsize(_TEXT_SIZE);
		o->argument(138);
		// o->align(FL_ALIGN_TOP);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}
	{ MiniKnob* o = new MiniKnob(295, 332, 25, 25, _("pwm vol"));
		o->labelsize(_TEXT_SIZE);
		o->argument(148);
		// o->align(FL_ALIGN_TOP);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}
	{ MiniKnob* o = new MiniKnob(844, 221, 25, 25, _("to delay"));
		o->labelsize(_TEXT_SIZE);
		o->argument(114);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}
	{ MiniKnob* o = new MiniKnob(934, 221, 25, 25, _("from delay"));
		o->labelsize(_TEXT_SIZE);
		o->argument(113);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}

	{ MiniKnob* o = new MiniKnob(52, 221, 25, 25, _("glide"));
		o->labelsize(_TEXT_SIZE);
		o->argument(116);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}
	{ MiniKnob* o = new MiniKnob(92, 221, 25, 25, _("bender"));
		o->tooltip(_("Pitch bend range (semitones)"));
		o->minimum(0);
		o->maximum(12);
		o->labelsize(_TEXT_SIZE);
		o->argument(142);
		o->align(FL_ALIGN_TOP);
		o->callback((Fl_Callback*)parmCallback);
		knobs[i][o->argument()] = o;
	}

	o->end(); 
	tab[i]=o;
	} // ==================================== end single voice tab

	} // end of voice tabs loop

	// ==================================== midi config tab
	{ 
		tablabel[i]=_("MIDI");
		Fl_Group* o = new Fl_Group(1, 10, _INIT_WIDTH, _INIT_HEIGHT, tablabel[i].c_str());
		groups[nGroups++]=o;
		o->color((Fl_Color)_BGCOLOR);
		o->labelsize(_TEXT_SIZE);
		// o->callback((Fl_Callback*)tabCallback,&xtab);  
		o->box(FL_BORDER_FRAME);
		// draw logo
		/*
		{ Fl_Box* o = new Fl_Box(855, 450, 25, 25);
			o->image(image_miniMini2);
		}
		*/
		int voice;
		char * voice_label[_MULTITEMP];
		for(voice=0; voice<_MULTITEMP; voice++){
			voice_label[voice]=(char *)malloc(20);
			sprintf(voice_label[voice], _("Voice %u"), voice+1); // ?? trailing \0
			{ Fl_Group* d = new Fl_Group(11+60*voice, 30, 50, 240, voice_label[voice]);
				groups[nGroups++]=d;
				midiGroups[voice]=d;
				d->box(FL_ROUNDED_FRAME);
				d->color(FL_BACKGROUND2_COLOR);
				d->labelsize(_TEXT_SIZE);
				d->labelcolor(FL_BACKGROUND2_COLOR);
				d->align(FL_ALIGN_TOP);
		  // Channel
		  // "If step() is non-zero and integral, then the range of numbers is limited to integers"
		  // range and bounds don't seem to be enforced when typing a value like 12345
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 46, 40, 15, _("Channel"));
			o->box(FL_ROUNDED_BOX);
			o->align(FL_ALIGN_TOP_LEFT);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(1, 16);
			o->bounds(1, 16);
			o->step(1);
			o->value(voice);
			o->argument(125+(voice<<8)); // Special encoding for voice
			o->callback((Fl_Callback*)midiparmCallback);
			o->value(voice);
			knobs[voice][125] = o; // NOT argument!!
		  }
		  // Note min as number
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 86, 40, 15, _("Note min"));
			o->box(FL_ROUNDED_BOX);
			o->align(FL_ALIGN_TOP_LEFT);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(0, 127);
			o->step(1);
			o->argument(128+(voice<<8)); // Special encoding for voice
			o->callback((Fl_Callback*)midiparmCallback);
			knobs[voice][128] = o; // NOT argument!!
			o->hide();
		  }
		  // Note min as string
		  { Fl_Input* o = new Fl_Input(16+60*voice, 86, 40, 14, _("Note min"));
			o->box(FL_ROUNDED_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->align(FL_ALIGN_TOP_LEFT);
			o->callback((Fl_Callback*)setNoteCallback);
			o->argument(128+(voice<<8));
            noteInput[voice][1] = o;
          }
		  // Note max as number
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 126, 40, 14, _("Note max"));
			o->box(FL_ROUNDED_BOX);
			o->align(FL_ALIGN_TOP_LEFT);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(0, 127);
			o->step(1);
			o->value(127);
			o->argument(129+(voice<<8)); // Special encoding for voice
			o->callback((Fl_Callback*)midiparmCallback);
			knobs[voice][129] = o; // NOT argument!!
			o->hide();
		  }
		  // Note max as string
		  { Fl_Input* o = new Fl_Input(16+60*voice, 126, 40, 14, _("Note max"));
			o->box(FL_ROUNDED_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->align(FL_ALIGN_TOP_LEFT);
			o->callback((Fl_Callback*)setNoteCallback);
			o->argument(129+(voice<<8));
            noteInput[voice][2] = o;
          }
		  // Transpose
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 166, 40, 14, _("Transpose"));
			o->box(FL_ROUNDED_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(-127,127);
			o->align(FL_ALIGN_TOP_LEFT);
			o->step(1);
			o->value(0);
			// o->callback((Fl_Callback*)parmCallback);
			o->argument(130+(voice<<8)); // Special encoding for voice
			o->callback((Fl_Callback*)midiparmCallback);
			knobs[voice][130] = o; // NOT argument!!
		  }
		  // Test note as number
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 206, 40, 14, _("Test note"));
			o->box(FL_ROUNDED_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(0,127);
			o->align(FL_ALIGN_TOP_LEFT);
			o->step(1);
            o->value(69);
			o->callback((Fl_Callback*)checkrangeCallback);
			o->argument(127+(voice<<8)); // Special encoding for voice
			knobs[voice][127] = o; // NOT argument!!
			o->hide();
		  }
		  // Test note as string
		  { Fl_Input* o = new Fl_Input(16+60*voice, 206, 40, 14, _("Test note"));
			o->box(FL_ROUNDED_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->align(FL_ALIGN_TOP_LEFT);
			o->callback((Fl_Callback*)setNoteCallback);
			o->argument(127+(voice<<8));
            noteInput[voice][0] = o;
          }
		  // Test velocity
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 246, 40, 14, _("Test vel."));
			o->box(FL_ROUNDED_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(0, 127);
			o->align(FL_ALIGN_TOP_LEFT);
			o->step(1);
			o->value(80);
			o->callback((Fl_Callback*)checkrangeCallback);
			o->argument(126+(voice<<8)); // Special encoding for voice
			knobs[voice][126] = o;
		  }
		d->end();
			}
			// Poly link
			if(voice<_MULTITEMP-1){ // Last voice cannot be linked to next
				{ Fl_Light_Button* o = new Fl_Light_Button(46+60*voice, 276, 40, 14, _("poly"));
					o->box(FL_ROUNDED_BOX);
					o->selection_color((Fl_Color)89);
					o->labelsize(_TEXT_SIZE);
					o->argument(155+(voice<<8)); // Special encoding for voice
					o->callback((Fl_Callback*)midibuttonCallback);
					knobs[voice][155] = o; // NOT argument!!
				}
			}

		} // End of for voice

		// Global detune, multi parameter 12
		{ Fl_Value_Input* o = new Fl_Value_Input(76, 300, 40, 14, _("Master tune"));
			o->box(FL_ROUNDED_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(-0.5, 0.5);
			o->align(FL_ALIGN_LEFT);
			o->step(.01);
			o->value(0);
			o->callback((Fl_Callback*)multiParmInputCallback);
			o->argument(12);
			multiParmInput[12]=o;
		}
		// Note detune, multi parameters 0..11, common to all voices
		{
			int y=330;
			Fl_Group* d = new Fl_Group(11, y, 471, 45, _("Detune"));
			groups[nGroups++]=d;
			d->box(FL_ROUNDED_FRAME);
			d->color(FL_BACKGROUND2_COLOR);
			d->labelsize(_TEXT_SIZE);
			d->labelcolor(FL_BACKGROUND2_COLOR);
			d->begin();
			for(int notenum=0, x=0; notenum<12; notenum++){
				{ Fl_Value_Input* o = new Fl_Value_Input(46+60*x, y+26, 40, 14, noteNames[notenum]);
					o->box(FL_ROUNDED_BOX);
					o->labelsize(_TEXT_SIZE);
					o->textsize(_TEXT_SIZE);
					o->range(-0.5, 0.5);
					o->align(FL_ALIGN_LEFT);
					o->step(.01);
					o->value(0);
					o->callback((Fl_Callback*)multiParmInputCallback);
					o->argument(notenum);
					multiParmInput[notenum]=o;
				}
				if(notenum<10 && notenum!=4 && strlen(noteNames[notenum])==1){
					// We need a sharp version of this note
					notenum++;
					{
						Fl_Value_Input* o = new Fl_Value_Input(76+60*x, y+6, 40, 14, noteNames[notenum]);
						o->box(FL_ROUNDED_BOX);
						o->labelsize(_TEXT_SIZE);
						o->textsize(_TEXT_SIZE);
						o->range(-0.5, 0.5);
						o->align(FL_ALIGN_LEFT);
						o->step(.01);
						o->value(0);
						o->callback((Fl_Callback*)multiParmInputCallback);
						o->argument(notenum);
						multiParmInput[notenum]=o;
					}
				}
				x++;
			}
			d->end();
		}


		// 9 custom controller numbers, multi parameters 13..21, , common to all voices
		{
			int y=400;
			Fl_Group* d = new Fl_Group(11, y, 471, 24, _("Controllers"));
			groups[nGroups++]=d;
			d->box(FL_ROUNDED_FRAME);
			d->color(FL_BACKGROUND2_COLOR);
			d->labelsize(_TEXT_SIZE);
			d->labelcolor(FL_BACKGROUND2_COLOR);
			d->begin();
			for(int cc=0; cc<9; cc++){
					Fl_Value_Input* o = new Fl_Value_Input(18+52*cc, y+5, 40, 14);
					o->box(FL_ROUNDED_BOX);
					o->labelsize(_TEXT_SIZE);
					o->textsize(_TEXT_SIZE);
					o->range(0, 127);
					o->align(FL_ALIGN_LEFT);
					o->step(1);
					o->value(0);
					o->callback((Fl_Callback*)multiParmInputCallback);
					o->argument(13+cc);
					multiParmInput[13+cc]=o;
			}
			d->end();
		}

		o->end(); 
		tab[i]=o;
		i++;
	} // ==================================== end midi config tab
	// ==================================== sound librarian tab
	{ 
		tablabel[i]=_("Sounds");
		Fl_Group* d = new Fl_Group(0, 10, _INIT_WIDTH, _INIT_HEIGHT, tablabel[i].c_str());
		groups[nGroups++]=d;
		d->labelsize(_TEXT_SIZE);
		d->color(_BGCOLOR);
		{
		SoundTable* o = new SoundTable(0, 10, _INIT_WIDTH, _INIT_HEIGHT-35);
		soundTable=o;
		o->selection_color(FL_YELLOW);
		o->color(_BGCOLOR);
		o->when(FL_WHEN_CHANGED|FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED); // FL_WHEN_RELEASE|
		o->table_box(FL_NO_BOX);
		o->col_resize_min(10);
		o->row_resize_min(10);

		// ROWS
		o->row_header(0);
		// o->row_header_width(60);
		o->row_resize(1);
		o->rows(2*_SOUNDS/_TABLE_COLUMNS); // 2 cells per sound
		o->row_height_all(16);

		// COLS
		o->cols(_TABLE_COLUMNS);
		o->col_header(0);
		// o->col_header_height(25);
		o->col_resize(1);
		o->col_width_all(96);
		for(int c=0; c<_TABLE_COLUMNS; c+=2) o->col_width(c, 25);
		
		o->end(); 
		}
		// Context menu
		/*
		{ Fl_Menu_Button *m = new Fl_Menu_Button(0, 10, _INIT_WIDTH, _INIT_HEIGHT-30, tablabel[i].c_str());
			m->type(Fl_Menu_Button::POPUP3);         // pops menu on right click
			m->add("Import", "^I", soundMenuCallback, 0);
			m->add("Export", "^E", soundMenuCallback, 0);
			m->add("Copy", "^C", soundMenuCallback, 0);
			m->add("Paste", "", soundMenuCallback, 0);
			m->add("Load as voice 1", "^1", soundMenuCallback, 0);
			m->add("Load as voice 2", "^2", soundMenuCallback, 0);
			m->add("Load as voice 3", "^3", soundMenuCallback, 0);
			m->add("Load as voice 4", "^4", soundMenuCallback, 0);
			m->add("Load as voice 5", "^5", soundMenuCallback, 0);
			m->add("Load as voice 6", "^6", soundMenuCallback, 0);
			m->add("Load as voice 7", "^7", soundMenuCallback, 0);
			m->add("Load as voice 8", "^8", soundMenuCallback, 0);
			// m->add("Do thing#1", "^1", soundMenuCallback, 0);  // ctrl-1 hotkey
			// m->add("Do thing#2", "^2", soundMenuCallback, 0);  // ctrl-2 hotkey
			// m->add("Quit",       "^q", soundMenuCallback, 0);  // ctrl-q hotkey
			soundMenu=m;
		}
		*/

		int y=_INIT_HEIGHT-_LOGO_HEIGHT2-6;
		int h=_LOGO_HEIGHT2+1;
		{ Fl_Button* o = new Fl_Button(5, y, 60, h, _("save sounds"));
			o->tooltip(_("save all sound data"));
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			//o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
			o->callback((Fl_Callback*)soundssavebtnCallback);
			saveSoundsBtn=o;
		}
		int x=70;
		for(int v=0; v<_MULTITEMP; v++){
			char sv[10]="";
			snprintf(sv, 10, _("load %d"), v+1);
			Fl_Button* o = new Fl_Button(x+66*v, y, 35, h);
			o->copy_label(sv);
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			o->argument(v);
			o->callback((Fl_Callback*)soundLoadBtn2Callback);
			voiceLoadBtn[v]=o;
			Fl_Output* o2 = new Fl_Output(x+37+66*v, y, 22, h);
			o2->box(FL_BORDER_BOX);
			o2->color(_BGCOLOR);
			o2->textsize(_TEXT_SIZE);
			soundNoDisplay[v]=o2;
		}
		{ Fl_Output* o = new Fl_Output(606, y, 190, h);
			o->box(FL_BORDER_BOX);
			o->color(_BGCOLOR);
			o->textsize(_TEXT_SIZE);
			//o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
			soundNameDisplay=o;
		}

		d->end();

		tab[i]=d;
		i++;
	}// ==================================== end sounds tab
	// ==================================== multi librarian tab
	{ 
		tablabel[i]=_("Multis");
		Fl_Group* d = new Fl_Group(0, 10, _INIT_WIDTH, _INIT_HEIGHT, tablabel[i].c_str());
		groups[nGroups++]=d;
		d->labelsize(_TEXT_SIZE);
		d->color(_BGCOLOR);
		{
		MultiTable* o = new MultiTable(0, 10, _INIT_WIDTH, _INIT_HEIGHT-35);
		multiTable=o;
		o->selection_color(FL_YELLOW);
		o->color(_BGCOLOR);
		o->when(FL_WHEN_CHANGED|FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED); //FL_WHEN_RELEASE|
		o->table_box(FL_NO_BOX);
		o->col_resize_min(10);
		o->row_resize_min(10);

		// ROWS
		o->row_header(0);
		o->row_resize(1);
		o->rows(2*_MULTIS/_TABLE_COLUMNS); // 2 cells per sound
		o->row_height_all(16);

		// COLS
		o->cols(_TABLE_COLUMNS);
		o->col_header(0);
		o->col_resize(1);
		o->col_width_all(96);
		for(int c=0; c<_TABLE_COLUMNS; c+=2) o->col_width(c, 25);

		o->end(); 
		}
		{ Fl_Button* o = new Fl_Button(5, _INIT_HEIGHT-_LOGO_HEIGHT2-5, 60, _LOGO_HEIGHT2, _("save multis"));
			o->tooltip(_("save all multis data"));
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			//o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
			o->callback((Fl_Callback*)multissavebtnCallback);
			saveMultisBtn=o;
		}
		{ Fl_Button* o = new Fl_Button(70, _INIT_HEIGHT-_LOGO_HEIGHT2-5, 60, _LOGO_HEIGHT2, _("load multi"));
			o->tooltip(_("load (activate) currently selected multi"));
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			//o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
			o->callback((Fl_Callback*)loadmultibtn2Callback);
			loadMultiBtn2=o;
		}
		{ Fl_Button* o = new Fl_Button(135, _INIT_HEIGHT-_LOGO_HEIGHT2-5, 60, _LOGO_HEIGHT2, _("import multi"));
			o->tooltip(_("Import multi from file to selected location"));
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			//o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
			o->callback((Fl_Callback*)multiimportCallback, (void*) multiTable);
			importMultiBtn=o;
		}
		{ Fl_Button* o = new Fl_Button(200, _INIT_HEIGHT-_LOGO_HEIGHT2-5, 60, _LOGO_HEIGHT2, _("export multi"));
			o->tooltip(_("Export multi from selected location to file"));
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			//o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
			o->callback((Fl_Callback*)multiexportCallback, (void*) multiTable);
			exportMultiBtn=o;
		}
		{ Fl_Output* o = new Fl_Output(265, _INIT_HEIGHT-_LOGO_HEIGHT2-5, 180, _LOGO_HEIGHT2);
			o->box(FL_BORDER_BOX);
			o->textsize(_TEXT_SIZE);
			//o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
			multiNameDisplay=o;
		}

		d->end();

		tab[i]=d;
		i++;
	}// ==================================== end multi librarian tab

	// ==================================== about tab 
	{ 
		tablabel[i]=_("About");
		Fl_Group* o = new Fl_Group(1, 10, _INIT_WIDTH, _INIT_HEIGHT, tablabel[i].c_str());
		groups[nGroups++]=o;
		o->color((Fl_Color)_BGCOLOR);
		o->labelsize(_TEXT_SIZE);
		// o->callback((Fl_Callback*)tabCallback,&xtab);  
		o->box(FL_BORDER_FRAME);
		// draw logo
		/*
		{ Fl_Box* o = new Fl_Box(855, 450, 25, 25);
		  o->image(image_miniMini2);
		}
		*/ 
		{
		  Fl_Help_View* o=new Fl_Help_View(200, 50, 600, 300, _("About Minicomputer"));
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(12);
		  o->color((Fl_Color)_BGCOLOR);
		  //o->textcolor(FL_BACKGROUND2_COLOR); 
		  o->textfont(FL_HELVETICA_BOLD );
		  o->labelcolor(FL_BACKGROUND2_COLOR);
		  const char version[] = _VERSION;
		  const char *about=_("<html><body>"
			  "<i><center>version %s</center></i><br>"
			  "<p><br>a standalone industrial grade software synthesizer for Linux<br>"
			  "<p><br>originally developed by Malte Steiner 2007-2009"
			  "<p>contact:<br>"
			  "<center>steiner@block4.com"
			  "<br>http://www.block4.com"
			  "<br>http://minicomputer.sourceforge.net"
			  "</center>"
			  "<p>distributed as free open source software under GPL3 licence<br>"
			  "<p>additional bugs by Marc Périlleux 2018"
			  "<p>OSC currently using ports %s and %s"
			  "<p>contact:<br>"
			  "<center>marc.perilleux@laposte.net"
			  "<br>https://github.com/vloop/minicomputer-virtual-synth"
			  "</center>"
			  "</body></html>");
		  char *Textausgabe;
		  Textausgabe=(char *)malloc(strlen(about)+strlen(version)+strlen(oscPort1)+strlen(oscPort2));
		  sprintf(Textausgabe, about, version, oscPort1, oscPort2);
		  o->value(Textausgabe);
		}	
		o->end(); 
		tab[i]=o;
	} // ==================================== end about tab 
	  
	o->end();
	tabs = o;
	}
// ---------------------------------------------------------------- end of tabs
	// Change logo background
	// memcpy(idata_miniMini2, idata_miniMini, _LOGO_WIDTH*_LOGO_HEIGHT*3*sizeof(unsigned char));
	copy_frame(idata_miniMini1, _LOGO_WIDTH1, _LOGO_HEIGHT1, idata_miniMini, 113, 2, _LOGO_WIDTH, _LOGO_HEIGHT);
	replace_color(idata_miniMini1, _LOGO_WIDTH1*_LOGO_HEIGHT1, 190, 218, 255, _BGCOLOR_R, _BGCOLOR_G, _BGCOLOR_B);
	// idata_miniMini1[0]=255; idata_miniMini1[1]=0; idata_miniMini1[2]=0; // Red dot for debugging
	copy_frame(idata_miniMini2, _LOGO_WIDTH2, _LOGO_HEIGHT2, idata_miniMini, 0, _LOGO_HEIGHT-_LOGO_HEIGHT2, _LOGO_WIDTH, _LOGO_HEIGHT);
	replace_color(idata_miniMini2, _LOGO_WIDTH1*_LOGO_HEIGHT1, 190, 218, 255, _BGCOLOR_R, _BGCOLOR_G, _BGCOLOR_B);

	// draw logo
	{ Fl_Box* o = new Fl_Box(_INIT_WIDTH-_LOGO_WIDTH2-5, _INIT_HEIGHT-_LOGO_HEIGHT2-5, _LOGO_WIDTH2, _LOGO_HEIGHT2);
		o->image(image_miniMini2);
		logo2Box=o;
	}
	{ Fl_Group* d = new Fl_Group(_INIT_WIDTH-_LOGO_WIDTH1-5, _INIT_HEIGHT-_LOGO_HEIGHT1-_LOGO_HEIGHT2-10, _LOGO_WIDTH1, _LOGO_HEIGHT1);
		logoGroup=d;
		{ Fl_Box* o = new Fl_Box(_INIT_WIDTH-_LOGO_WIDTH1-5, _INIT_HEIGHT-_LOGO_HEIGHT1-_LOGO_HEIGHT2-10, _LOGO_WIDTH1, _LOGO_HEIGHT1);
			o->image(image_miniMini1);
			logo1Box=o;
		}
		// Sound indicators after the tabs, will appear on top
		{ for (int j=0; j<_MULTITEMP; j++)
			{
			Fl_Box* o = new Fl_Box(00+6*(j/4), 425+5*(j%4)+(j/4), 4, 4);
			o->box(FL_RFLAT_BOX);
			o->color(FL_DARK3, FL_DARK3); 
			sounding[j]=o;
			}
		}
		d->end();
	}
// ----------------------------------------- Multis
// This code must be outside of the tabs loop!
// There is only one instance of the multi section.
// It impacts resizing?
		int x0=12;
		{ Fl_Group* d = new Fl_Group(x0-7, 461, 300, 48, _("multi"));
			groups[nGroups++]=d;
			multiGroup=d;
			d->box(FL_ROUNDED_FRAME);
			d->color(FL_BACKGROUND2_COLOR);
			d->labelsize(_TEXT_SIZE);
			d->labelcolor(FL_BACKGROUND2_COLOR);
			d->align(FL_ALIGN_TOP);
		  { Fl_Button* o = new Fl_Button(x0, 471, 10, 30, "@<");
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
			o->argument(-1);
			o->callback((Fl_Callback*)multiincdecCallback);
			decMultiBtn=o;
		  }
		  { Fl_Int_Input* o = new Fl_Int_Input(x0+10, 471, 35, 30, _("multi #"));
			o->box(FL_BORDER_BOX);
			o->color(FL_BLACK);
			o->labelsize(_TEXT_SIZE);
			// o->maximum(127);
			o->align(FL_ALIGN_TOP_LEFT);
			o->textsize(16);
			o->textcolor(FL_RED);
			o->cursor_color(FL_RED);
			o->callback((Fl_Callback*)multiNoInputCallback);
			o->tooltip(_("Target slot number in the multi memory"));
			multiNoInput=o;
		  }
		  { Fl_Button* o = new Fl_Button(x0+45, 471, 10, 30, "@>");
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
			o->argument(1);
			o->callback((Fl_Callback*)multiincdecCallback);
			incMultiBtn=o;
		  }
		  { Fl_Input* o = new Fl_Input(x0+60, 471, 150, 14, _("name"));
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->align(FL_ALIGN_TOP_LEFT);
			o->tooltip(_("Enter the multi name here before storing it"));
			o->callback((Fl_Callback*)multiNameInputCallback);
			// multichoice = o;
			multiNameInput = o; // global ??
		  } 
		  // roller for the multis:
		  { Fl_Roller* o = new Fl_Roller(x0+60, 487, 150, 14);
			o->type(FL_HORIZONTAL);
			o->tooltip(_("roll the list of multis, press load button for loading or save button for storing"));
			o->minimum(0);
			o->maximum(127);
			o->value(0);
			o->step(1);
			o->box(FL_BORDER_FRAME);
			o->callback((Fl_Callback*)multiRollerCallback,NULL);
			multiRoller=o;
		  }
		  { Fl_Button* o = new Fl_Button(x0+217, 465, 60, 19, _("load multi"));
			o->tooltip(_("load (activate) selected multi"));
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
			o->callback((Fl_Callback*)loadmultiCallback, multiNameInput); // multichoice
			loadMultiBtn = o;
		  }
		  { Fl_Button* o = new Fl_Button(x0+217, 485, 60, 19, _("store multi"));
			o->tooltip(_("overwrite selected multi with current settings"));
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
			o->callback((Fl_Callback*)multistoreCallback, multiNameInput); // multichoice
			multistoreBtn = o;
		  }
			d->end(); // groups multi
			make_labeltip(d,
				_("These controls allow storing current multi parameters to the selected memory slot,\n"
				"or retrieving them from it. The name is displayed in red when the current settings\n"
				"do not match the selected memory slot."));
		}
	/*{ Fl_Chart * o = new Fl_Chart(600, 300, 70, 70, "eg");
		o->bounds(0.0,1.0);
		o->type(Fl::LINE_CHART);
		o->insert(0, 0.5, NULL, 0);
		o->insert(1, 0.5, NULL, 0);
		o->insert(2, 1, NULL, 0);
		o->insert(3, 0.5, NULL, 0);
		EG[0]=o;
		
	}*/

	// parameter tuning
	{ Fl_Value_Input* o = new MiniValue_Input(900, 390, 80, 20);
		o->box(FL_ROUNDED_BOX);
		o->labelsize(12);
		o->textsize(12);
		// o->menubutton()->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_LEFT);
		o->step(0.0001);
		o->callback((Fl_Callback*)parminputCallback);
		parmInput = o;
	}

	{ Fl_Toggle_Button* o = new Fl_Toggle_Button(679, 466, 60, 19, _("Audition"));
		o->tooltip(_("Hear the currently loaded sound"));
		// These borders won't prevent look change on focus
		o->box(FL_BORDER_BOX);
		// o->downbox(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->color(FL_LIGHT1, FL_YELLOW);
		o->callback((Fl_Callback*)do_audition);
		auditionBtn = o;
	}
	{ Fl_Toggle_Button* o = new Fl_Toggle_Button(679, 487, 60, 19, _("Compare"));
		o->tooltip(_("Compare edited sound to original"));
		// These borders won't prevent look change on focus
		o->box(FL_BORDER_BOX);
		// o->downbox(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->color(FL_LIGHT1, FL_YELLOW);
		o->callback((Fl_Callback*)do_compare);
		compareBtn = o;
	}
	{ Fl_Button* o = new Fl_Button(741, 466, 60, 40, _("PANIC"));
		o->tooltip(_("Instantly stop all voices (shortcut Esc)"));
		o->box(FL_BORDER_BOX);
		// o->labelsize(_TEXT_SIZE);
		o->color(FL_RED);
		// o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->callback((Fl_Callback*)do_all_off);
		panicBtn = o;
	}

	// parameters here would be common to all tabs

	o->end(); // End of main window
	o->size_range(o->w(), o->h());
	o->resizable(o);

#ifdef _DEBUG
	// Dump parm list (from voice 0, same for others)
	for(int parmnum=0; parmnum<_PARACOUNT; parmnum++){
		if(knobs[0][parmnum])
			printf("%3.3u : %s\n", parmnum, knobs[0][parmnum]->label());
			/* Parent Fl_Group has no label
			if(knobs[0][parmnum]->parent())
				printf("    %s\n", knobs[0][parmnum]->parent()->label());
			*/
		else
			printf("%3.3u : not defined\n", parmnum);
	}
	for(int choicenum=0; choicenum<_CHOICECOUNT; choicenum++){
		if(choices[0][choicenum])
			printf("#%2.2u : %s\n", choicenum, choices[0][choicenum]->label());
		else
			printf("#%2.2u : not defined\n", choicenum);
	}
#endif

	return o;
}

/**
 * constructor
 * calling straight the super class constructor
 */
MiniWindow::MiniWindow(int w, int h, const char* t): Fl_Double_Window(w,h,t)
{
}
/**
 * constructor
 * calling straight the super class constructor
 */
MiniWindow::MiniWindow(int w, int h): Fl_Double_Window(w,h)
{
	current_text_size = _TEXT_SIZE;
}
/**
 * using the destructor to shutdown synthcore
 */
MiniWindow::~MiniWindow()
{
	printf(_("gui quit\n"));
	fflush(stdout);
	if (transmit) lo_send(t, "/Minicomputer/close", "i",1);
	//~Fl_Double_Window();
}

/**
 * overload the resize method
 * @param x
 * @param y
 * @param w width
 * @param h height
 * 
 */
void MiniWindow::resize (int x, int y, int w, int h)
{
	Fl_Double_Window::resize(x, y, w, h);
	// Logo - re-align with bottom
	logo1Box->resize(w-_LOGO_WIDTH1-5, h-_LOGO_HEIGHT1-_LOGO_HEIGHT2-10, _LOGO_WIDTH1, _LOGO_HEIGHT1);
	logo2Box->resize(w-_LOGO_WIDTH2-5, h-_LOGO_HEIGHT2-5, _LOGO_WIDTH2, _LOGO_HEIGHT2);
	// Sound indicators - re-align with logo
	for (int j=0; j<_MULTITEMP; j++){
		sounding[j]->resize(logo1Box->x()+5+6*(j/4), logo1Box->y()+31+5*(j%4)+(j/4), 4, 4);
		voiceDisplayBox[j]->resize(w-_LOGO_WIDTH1-50, h-_LOGO_HEIGHT2-50, 40, 40);
	}
	// Multi groups
	multiGroup->position(multiGroup->x(), soundGroup->y());
	multiNameInput->resize(multiNameInput->x(), soundNameInput[0]->y(), multiNameInput->w(), multiNameInput->h());
	multiRoller->position(multiRoller->x(), soundRoller[0]->y());
	multiNoInput->position(multiNoInput->x(), soundNoInput[0]->y());
	loadMultiBtn->position(loadMultiBtn->x(), loadSoundBtn[0]->y());
	multistoreBtn->position(multistoreBtn->x(), soundstoreBtn[0]->y());
	decMultiBtn->position(decMultiBtn->x(), soundNoInput[0]->y());
	incMultiBtn->position(incMultiBtn->x(), soundNoInput[0]->y());

	auditionBtn->position(auditionBtn->x(), loadSoundBtn[0]->y());
	compareBtn->position(compareBtn->x(), soundstoreBtn[0]->y());
	panicBtn->position(panicBtn->x(), loadSoundBtn[0]->y());
	soundTable->resize_cols(w);
	multiTable->resize_cols(w);

	// Set text size for all widgets
	float minscale=min((float)this->w()/_INIT_WIDTH, (float)this->h()/_INIT_HEIGHT);
	int new_text_size = _TEXT_SIZE*minscale;
	if(new_text_size!=current_text_size){
		current_text_size=new_text_size;
		parmInput->textsize(12*minscale);
		parmInput->labelsize(12*minscale);
		compareBtn->labelsize(new_text_size);
		auditionBtn->labelsize(new_text_size);
		panicBtn->labelsize(new_text_size);
		loadMultiBtn->labelsize(new_text_size);
		loadMultiBtn2->labelsize(new_text_size);
		importMultiBtn->labelsize(new_text_size);
		exportMultiBtn->labelsize(new_text_size);
		multistoreBtn->labelsize(new_text_size);
		saveMultisBtn->labelsize(new_text_size);
		saveSoundsBtn->labelsize(new_text_size);
		multiNoInput->textsize(16*minscale);
		multiNoInput->labelsize(new_text_size);
		multiNameInput->textsize(new_text_size);
		multiNameInput->labelsize(new_text_size);
		soundNameDisplay->textsize(new_text_size);
		multiNameDisplay->textsize(new_text_size);

		for(unsigned int i=0; i<sizeof(menu_amod)/sizeof(menu_amod[0]); i++) {
			menu_amod[i].labelsize(new_text_size);
		}
		for(unsigned int i=0; i<sizeof(menu_fmod)/sizeof(menu_fmod[0]); i++) {
			menu_fmod[i].labelsize(new_text_size);
		}
		for(unsigned int i=0; i<sizeof(menu_wave)/sizeof(menu_wave[0]); i++) {
			menu_wave[i].labelsize(new_text_size);
		}

		for(int i=0; i<_MULTIPARMS; i++){
			multiParmInput[i]->labelsize(new_text_size);
			multiParmInput[i]->textsize(new_text_size);
		}

		for(int i=0; i<nGroups; i++)
			groups[i]->labelsize(new_text_size);

		for(int i=0; i<_MULTITEMP; i++){
			soundNoInput[i]->textsize(16*minscale);
			soundNoInput[i]->labelsize(new_text_size);
			soundNoDisplay[i]->textsize(new_text_size);
			soundNameInput[i]->textsize(new_text_size);
			soundNameInput[i]->labelsize(new_text_size);
			loadSoundBtn[i]->labelsize(new_text_size);
			soundstoreBtn[i]->labelsize(new_text_size);
			voiceLoadBtn[i]->labelsize(new_text_size);
			clearStateBtn[i]->labelsize(new_text_size);
			importSoundBtn[i]->labelsize(new_text_size);
			exportSoundBtn[i]->labelsize(new_text_size);
			for(int j=0; j<3; j++){
				noteInput[i][j]->labelsize(new_text_size);
				noteInput[i][j]->textsize(new_text_size);
			}
			for(int j=0; j<_MINICOUNT; j++){
				miniInput[i][j]->textsize(new_text_size);
				miniInput[i][j]->labelsize(new_text_size);
			}
			for(int j=0; j<_CHOICECOUNT; j++){
				choices[i][j]->textsize(new_text_size); // No effect
                // The displayed choice text size is set at the menu level
				choices[i][j]->labelsize(new_text_size);
			}
			for(int j=0; j<_PARACOUNT; j++){
				if(knobs[i][j])
					knobs[i][j]->labelsize(new_text_size);
			}
			for(int j=125; j<=130; j++)
				((Fl_Value_Input*)knobs[i][j])->textsize(new_text_size);
		}
	}
}
/**
 * overload the eventhandler for some custom shortcuts
 * @param event
 * @return 1 when all is right
 */
int MiniWindow::handle (int event)
{
#ifdef _DEBUG
	printf("event %u %s\n", event, fl_eventnames[event]);
#endif
	switch (event)
		{
#ifdef _DEBUG
	case FL_CLOSE:
		printf("About to close main window\n");
		return 0;
	case FL_DEACTIVATE:
		printf("About to deactivate main window\n");
		return 0;
	case FL_HIDE:
		printf("About to hide main window\n");
		return 0;
#endif
	case FL_KEYBOARD:
				if (tabs != NULL)
		{
		switch (Fl::event_key ())
				{
				case FL_F+1:
			tabs->value(tab[0]);
			tabCallback(tabs,NULL);
		break;
				case FL_F+2:
			tabs->value(tab[1]);
			tabCallback(tabs,NULL);
		break;
				case FL_F+3:
			tabs->value(tab[2]);
			tabCallback(tabs,NULL);
		break;
				case FL_F+4:
			tabs->value(tab[3]);
			tabCallback(tabs,NULL);
		break;
				case FL_F+5:
			tabs->value(tab[4]);
			tabCallback(tabs,NULL);
		break;
				case FL_F+6:
			tabs->value(tab[5]);
			tabCallback(tabs,NULL);
		break;
				case FL_F+7:
			tabs->value(tab[6]);
			tabCallback(tabs,NULL);
		break;
				case FL_F+8:
			tabs->value(tab[7]);
			tabCallback(tabs,NULL);
		break;
				case FL_F+9:
			tabs->value(tab[8]);
			tabCallback(tabs,NULL);
		break;
				case FL_F+10:
			tabs->value(tab[9]);
			tabCallback(tabs,NULL);
		break;
				case FL_F+11:
			tabs->value(tab[10]);
			tabCallback(tabs,NULL);
		break;
				case FL_F+12:
			tabs->value(tab[11]);
			tabCallback(tabs,NULL);
		break;
				case FL_Escape:
			if(noEscape==false) do_all_off(NULL, NULL);
		break;
				// case 32: // Space bar - Doesn't work
				case FL_Insert:
			if(currentTab<_MULTITEMP){
				auditionBtn->value(!auditionBtn->value());
				do_audition(auditionBtn, NULL);
			}
		break;
		
				} // end of switch
		} // end of if
				return 1;

		default:
				// pass other events to the base class...
				return Fl_Double_Window::handle(event);
		}
}
