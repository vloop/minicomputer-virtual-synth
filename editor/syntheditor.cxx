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

#include "syntheditor.h"
#include "Memory.h"

#define _DEBUG
// TODO: try to move globs to class variables
// Problem: some UI items need to be accessible from window resize function
static Fl_RGB_Image image_miniMini(idata_miniMini, _LOGO_WIDTH, _LOGO_HEIGHT, 3, 0);
Fl_Box* logo1;
Fl_Box* logo2;

Fl_Widget* Knob[_MULTITEMP][_PARACOUNT];
int needs_finetune[_PARACOUNT];
int needs_multi[_PARACOUNT];
int is_button[_PARACOUNT];

Fl_Choice* auswahl[_MULTITEMP][_CHOICECOUNT];
Fl_Value_Input* miniDisplay[_MULTITEMP][_MINICOUNT];
Fl_Widget* tab[_TABCOUNT];
Fl_Tabs* tabs;
static const char* voicename[_MULTITEMP]={"1", "2", "3", "4", "5", "6", "7", "8"};
Fl_Box* voicedisplay[_MULTITEMP];
Fl_Group *soundgroup;
Fl_Int_Input *soundnumber[_MULTITEMP];
Fl_Input* soundname[_MULTITEMP];
Fl_Button *loadsoundBtn[_MULTITEMP], *storesoundBtn[_MULTITEMP];
Fl_Button *importsoundBtn[_MULTITEMP], *exportsoundBtn[_MULTITEMP];
Fl_Roller* soundRoller[_MULTITEMP];
bool sound_changed[_MULTITEMP];

int audition_state[_MULTITEMP];
Fl_Toggle_Button *auditionBtn;
Fl_Button *panicBtn;
Fl_Button *clearstateBtn[_MULTITEMP];
Fl_Value_Input* paramon;
Fl_Box* sounding[_MULTITEMP];

Fl_Group *multigroup;
Fl_Int_Input *multinumber;
Fl_Input* multiname;
Fl_Button *loadmulti, *storemulti, *multidecBtn, *multiincBtn;
Fl_Roller* multiRoller;
bool multi_changed;

Fl_Value_Input* multiparm[_MULTIPARMS];

Fl_Group *filtersgroup[_MULTITEMP];
 
int currentParameter=0;
char currentParameterName[32]="...............................";

unsigned int currentsound=0, currentmulti=0;
bool transmit;
bool sense=false; // Core not sensed yet

int groups=0;
Fl_Group *group[156];

patch compare_buffer;
Fl_Toggle_Button *compareBtn;

//Fl_Chart * EG[7];

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
void replace_color (unsigned char * bits, unsigned int pixcount, unsigned char r1, unsigned char g1, unsigned char b1, unsigned char r2, unsigned char g2, unsigned char b2){
	for(unsigned int i=0; i<pixcount*3; i+=3){
		if(bits[i]==r1 && bits[i+1]==g1 && bits[i+2]==b1){
			bits[i]=r2;
			bits[i+1]=g2;
			bits[i+2]=b2;
		}
	}
}

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

// This function probably belongs somewhere else
char *strnrtrim(char *dest, const char*source, size_t len){
	int last_letter=-1;
	for(unsigned int i=0; i<len && source[i]!=0; i++){
		if(!isspace(source[i])) last_letter=i;
		dest[i]=source[i];
	}
	// Beware terminating 0 is not added past end!
	if (unsigned(last_letter+1) < len)
		dest[last_letter+1]=0; // Should fill with zeros to the end?
	return dest;
}

static void setsound_changed(){
	if(!sound_changed[currentsound]){
		sound_changed[currentsound]=true;
		soundname[currentsound]->textcolor(FL_RED);
		soundname[currentsound]->redraw();
	}
}
static void clearsound_changed(){
	if(sound_changed[currentsound]){
		sound_changed[currentsound]=false;
		soundname[currentsound]->textcolor(FL_BLACK);
		soundname[currentsound]->redraw();
	}
}
static void setmulti_changed(){
	if(!multi_changed){
		multi_changed=true;
		multiname->textcolor(FL_RED);
		multiname->redraw();
	}
}
static void clearmulti_changed(){
	if(multi_changed){
		multi_changed=false;
		multiname->textcolor(FL_BLACK);
		multiname->redraw();
	}
}

static void choiceCallback(Fl_Widget* o, void*)
{
#ifdef _DEBUF
	printf("choiceCallback voice %u, parameter %u, value %u\n", currentsound, ((Fl_Choice*)o)->argument(), ((Fl_Choice*)o)->value());
#endif
	if (transmit) lo_send(t, "/Minicomputer/choice", "iii", currentsound, ((Fl_Choice*)o)->argument(), ((Fl_Choice*)o)->value());
	setsound_changed(); // All choices relate to the sound, none to the multi
}

static void do_audition(Fl_Widget* o, void*)
{
	if (transmit){
		Fl::lock();
		int note=((Fl_Valuator*)Knob[currentsound][125])->value();
		int velocity=((Fl_Valuator*)Knob[currentsound][126])->value();
		audition_state[currentsound]=((Fl_Toggle_Button*)o)->value();
		if ( ((Fl_Toggle_Button*)o)->value()){
			lo_send(t, "/Minicomputer/midi", "iiii", 0, 0x90+currentsound, note, velocity);
		}else{
			lo_send(t, "/Minicomputer/midi", "iiii", 0, 0x80+currentsound, note, velocity);
		}
		Fl::awake();
		Fl::unlock();
	}
}
static void all_off(Fl_Widget* o, void*)
{
	int i;
	// make sure audition is off
	auditionBtn->clear();
	if (transmit) for (i=0; i<_MULTITEMP; i++){
	  lo_send(t, "/Minicomputer/midi", "iiii", 0, 0xB0+i, 120, 0);
	  audition_state[i]=((Fl_Toggle_Button*)auditionBtn)->value();
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
			audition_state[voice]=0;
			if(voice==currentsound)
				((Fl_Toggle_Button*)auditionBtn)->value(0);
		}
		return 0;
	}
	return 1;
}

static void tabCallback(Fl_Widget* o, void* );
void EG_draw_all(){
	// Refresh EG label lights
	// Called by timer_handler
	int voice, EGnum, stage;
	Fl::lock(); // Doesn't help - tab label not redrawn
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
					Knob[voice][EG_parm_num[EGnum]+stage-1]->labelcolor(EG_stage[voice][EGnum]==stage?FL_RED:FL_FOREGROUND_COLOR);
					Knob[voice][EG_parm_num[EGnum]+stage-1]->redraw_label();
				}
			}
		}
	}
	// Fl::check(); // Doesn't help - tab label not redrawn
	Fl::awake(); // Doesn't help - tab label not redrawn
	Fl::unlock(); // Doesn't help - tab label not redrawn

}

/* // not good:
static void changemulti(Fl_Widget* o, void*)
{
	if (multiname != NULL)
	{
		int t = multiname->menubutton()->value();
		if ((t!=currentmulti) && (t>-1) && (t<128))
		{
			// ok, we are on somewhere other multi so we need to copy the settings
		//	for (int i=0;i<8;++i)
		//	{
		//		Speicher.multis[t].sound[i] = Speicher.multis[currentmulti].sound[i];
		//	}
			currentmulti = t;
		}
	}
	else
		printf("problems with the multichoice widgets!\n");
}
*/
/**
 * parmCallback when another tab is chosen
 * @param Fl_Widget the calling widget
 * @param defined by FLTK but not used
 */
static void tabCallback(Fl_Widget* o, void* )
{
Fl::lock();
	Fl_Widget* e =((Fl_Tabs*)o)->value();
	if (e==tab[9] ) // The "About" tab
	{
		// The controls below are displayed on all the tabs except "About"
		if (multinumber != NULL)
			multinumber->hide();
		else
			printf("there seems to be something wrong with multidisplay widget");
		
		if (multiRoller != NULL)
			multiRoller->hide();
		else
			printf("there seems to be something wrong with multiroller widget");

		if (multiname != NULL)
			multiname->hide();
		else
			printf("there seems to be something wrong with multichoice widget");
		if (storemulti != NULL)
			storemulti->hide();
		else
			printf("there seems to be something wrong with the store multi button widget");
		if (loadmulti != NULL)
			loadmulti->hide();
		else
			printf("there seems to be something wrong with the load multi button widget");
		if (multiincBtn!= NULL)
			multiincBtn->hide();
		else
			printf("there seems to be something wrong with the multi inc button widget");
		if (multidecBtn!= NULL)
			multidecBtn->hide();
		else
			printf("there seems to be something wrong with the multi dec button widget");
		if (multigroup!= NULL)
			multigroup->hide();
		else
			printf("there seems to be something wrong with the multi group widget");
	}else{
		if (multinumber != NULL)
			multinumber->show();
		else
			printf("there seems to be something wrong with multinumber widget");

		if (multiRoller != NULL)
			multiRoller->show();
		else
			printf("there seems to be something wrong with multiroller widget");
		
		if (multiname != NULL)
			multiname->show();
		else
			printf("there seems to be something wrong with multichoice widget");
		if (storemulti != NULL)
			storemulti->show();
		else
			printf("there seems to be something wrong with the store multi button widget");
		if (loadmulti != NULL)
			loadmulti->show();
		else
			printf("there seems to be something wrong with the load multi button widget");
		if (multiincBtn!= NULL)
			multiincBtn->show();
		else
			printf("there seems to be something wrong with the multi inc button widget");
		if (multidecBtn!= NULL)
			multidecBtn->show();
		else
			printf("there seems to be something wrong with the multi dec button widget");
		if (multigroup!= NULL)
			multigroup->show();
		else
			printf("there seems to be something wrong with the multi group widget");
	}
	if (e==tab[8] || e==tab[9]) // The "midi" and the "About" tabs
	{
		// The controls below are displayed on all the tabs except "About" and "midi"
		if (paramon != NULL)
			paramon->hide();
		else
			printf("there seems to be something wrong with paramon widget");
		if (auditionBtn != NULL)
			auditionBtn->hide();
		else
			printf("there seems to be something wrong with audition widget");
		if (compareBtn!= NULL)
			compareBtn->hide();
		else
			printf("there seems to be something wrong with the compare button widget");
	}
	else // Voice tabs
	{
		// Update currentsound
		for (int i=0; i<_MULTITEMP;++i)
		{
			if (e==tab[i]){ 
				currentsound=i;
				break;
			}
		}	

		if (paramon != NULL)
			paramon->show();
		else
			printf("there seems to be something wrong with paramon widget");
		if (auditionBtn != NULL){
			auditionBtn->show();
			((Fl_Toggle_Button*)auditionBtn)->value(audition_state[currentsound]);
		}else
			printf("there seems to be something wrong with audition widget");
		if (compareBtn!= NULL)
			compareBtn->show();
		else
			printf("there seems to be something wrong with the compare button widget");
#ifdef _DEBUG
	printf("sound %i\n", currentsound );
	fflush(stdout);
#endif
 	} // end of else
Fl::awake();
Fl::unlock();
}

static void paramon_flash(void *userdata){
	Fl::lock();
	paramon->labelcolor(FL_BLACK);
	paramon->redraw_label();
	Fl::awake();
	Fl::unlock();
}
/**
 * main parmCallback, called whenever a parameter has changed
 * @param Fl_Widget the calling widget
 * @param defined by FLTK but not used
 */
static void parmCallback(Fl_Widget* o, void*) {
Fl::lock();
if (o != NULL)
{
	currentParameter = ((Fl_Valuator*)o)->argument();
	if(needs_multi[currentParameter]){
		setmulti_changed();
	}else{
		setsound_changed();
	}

	// show parameter on fine tune only when relevant (not a frequency...)
	if(needs_finetune[currentParameter]){
// #ifdef _DEBUG
// if(currentsound==0) // Don't clutter output with all voices
//	printf("parmCallback voice %u parameter %3.3u", currentsound, currentParameter);
// #endif
		paramon->value(((Fl_Valuator*)o)->value());
		paramon->minimum(((Fl_Valuator*)o)->minimum());
		paramon->maximum(((Fl_Valuator*)o)->maximum());
		snprintf(currentParameterName, 32, "%31s", ((Fl_Valuator*)o)->label());
		paramon->label(currentParameterName);
		paramon->labelcolor(FL_YELLOW);
		Fl::add_timeout(0.5, paramon_flash, 0);
		paramon->show();
// #ifdef _DEBUG
// if(currentsound==0)
//	printf("%s =%f done.\n", currentParameterName, paramon->value());
// #endif
	}else{
		paramon->hide();
	}

// now actually process parameter
if(is_button[currentParameter]){
	float val;
	if(currentParameter==2){ // Fixed frequency button Osc 1
		if (((Fl_Light_Button *)o)->value())
		{
			Knob[currentsound][1]->activate();
			Knob[currentsound][3]->deactivate();
			miniDisplay[currentsound][0]->activate();
			miniDisplay[currentsound][1]->deactivate();
		}
		else
		{
			Knob[currentsound][1]->deactivate();
			Knob[currentsound][3]->activate();
			miniDisplay[currentsound][0]->deactivate();
			miniDisplay[currentsound][1]->activate();
		}
	}else if(currentParameter==17){ // Fixed frequency button Osc 2
		if (((Fl_Light_Button *)o)->value())
		{
			Knob[currentsound][16]->activate();
			Knob[currentsound][18]->deactivate();
			miniDisplay[currentsound][2]->activate();
			miniDisplay[currentsound][3]->deactivate();
		}
		else
		{
			Knob[currentsound][16]->deactivate();
			Knob[currentsound][18]->activate();
			miniDisplay[currentsound][2]->deactivate();
			miniDisplay[currentsound][3]->activate();
		}
	}else if(currentParameter==137){ // Filter bypass
		if (((Fl_Button *)o)->value())
		{
			filtersgroup[currentsound]->deactivate();
		}else{
			filtersgroup[currentsound]->activate();
		}
	}
	if(currentParameter==4 || currentParameter==19){ // Boost buttons, transmit 1 or 100
		val=((Fl_Light_Button*)o)->value()?100.0f:1.0f;
	}else { // Regular buttons, transmit plain value as float
		val=((Fl_Light_Button*)o)->value()?1.0f:0.0f;
	}
	if (transmit) lo_send(t, "/Minicomputer", "iif", currentsound, ((Fl_Light_Button*)o)->argument(), val);
#ifdef _DEBUG
	printf("parmCallback button %li : %f\n", ((Fl_Light_Button*)o)->argument(),val);
#endif
}else{ // Not a button
	switch (currentParameter)
	{
		case 256: // ??
		{
			lo_send(t, "/Minicomputer", "iif", currentsound, 0, 0);
			break;
		}
		case 1: // Osc 1 fixed frequency dial
		{
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
			miniDisplay[currentsound][0]->value( ((Fl_Valuator* )Knob[currentsound][1])->value() );
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
	#endif
			break;
		}
		/*
		case 2: // Osc 1 fixed frequency button
		{
			if (((Fl_Light_Button *)o)->value()==0)
			{
				Knob[currentsound][1]->deactivate();
				Knob[currentsound][3]->activate();
				miniDisplay[currentsound][1]->activate();
				miniDisplay[currentsound][0]->deactivate();
				if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),0.0f);
			}
			else
			{
				Knob[currentsound][3]->deactivate();
				Knob[currentsound][1]->activate();
				miniDisplay[currentsound][0]->activate();
				miniDisplay[currentsound][1]->deactivate();
				if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),1.0f);
			}
	#ifdef _DEBUG
			printf("parmCallback %li : %i\n", ((Fl_Light_Button*)o)->argument(),((Fl_Light_Button*)o)->value());
	#endif
			break;
		}
		case 17: // Osc 2 fixed frequency button
		{
			if (((Fl_Light_Button *)o)->value()==0)
			{
				Knob[currentsound][16]->deactivate();
				Knob[currentsound][18]->activate();
				miniDisplay[currentsound][3]->activate();
				miniDisplay[currentsound][2]->deactivate();
				if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),0.f);
			}
			else
			{
				Knob[currentsound][18]->deactivate();
				Knob[currentsound][16]->activate();
				miniDisplay[currentsound][2]->activate();
				miniDisplay[currentsound][3]->deactivate();
				if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),1.f);
			}
	#ifdef _DEBUG
			printf("parmCallback %li : %i\n", ((Fl_Light_Button*)o)->argument(),((Fl_Light_Button*)o)->value());
	#endif
		break;
		}
		*/
		case 3: // Osc 1 tune
		{
			float f = ((Fl_Positioner*)o)->xvalue() + ((Fl_Positioner*)o)->yvalue();
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
			miniDisplay[currentsound][1]->value( f);
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Positioner*)o)->argument(),f);
	#endif
		break;
		}
		case 16: // Osc 2 fixed frequency dial
		{
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
			miniDisplay[currentsound][2]->value( ((Fl_Valuator*)o)->value() );
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
	#endif
			break;
		}
		case 18: // Osc 2 tune
		{ 
			float f = ((Fl_Positioner*)o)->xvalue() + ((Fl_Positioner*)o)->yvalue();
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
			miniDisplay[currentsound][3]->value(f);
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Positioner*)o)->argument(),f);
	#endif
		break;
		}
		/*
		case 4: // boost button
		case 19:
		{
			if (((Fl_Light_Button *)o)->value()==0)
			{
				if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),1.f);
			}
			else
			{
				if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),100.f);
			}
	#ifdef _DEBUG
			printf("parmCallback %li : %i\n", ((Fl_Light_Button*)o)->argument(),((Fl_Light_Button*)o)->value());
	#endif
		break;
		}
		*/
		/*
		// the repeat buttons of the mod egs + sync button
		case 64:
		case 69:
		case 74:
		case 79:
		case 84:
		case 89:
		case 115: // Sync
		// mult. buttons
		case 117:
		case 118:
		case 132:
		case 140:
		case 121:
		case 136:
		case 133:
		case 120:
		case 135:
		case 137: // Bypass filters
		case 139: // Legato
		case 143: // Time modulator 2 mult.
		{
			if (((Fl_Light_Button *)o)->value()==0)
			{
				if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),0.f);
			}
			else
			{
				if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),1.f);
			}
	#ifdef _DEBUG
			printf("parmCallback %li : %i\n", ((Fl_Light_Button*)o)->argument(),((Fl_Light_Button*)o)->value());
	#endif
			break;
		}
		*/
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
			float tr=(((Fl_Valuator*)o)->value()); // 0.5..0.01
			tr*= tr*tr/2.f;
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Valuator*)o)->argument(),tr);
	#ifdef _DEBUG
			printf("parmCallback eg %li : %g\n", ((Fl_Valuator*)o)->argument(), tr);
	#endif
			break;
		}
		case 116: // Glide
		{
			float tr=(((Fl_Valuator*)o)->value());// 0..1
			// .9999 is slow .99995 is slower, must stay <1
			tr= 1-pow(10, -tr*5);
			if(tr>=1) tr = 0.99995f;
			if(tr<0) tr = 0.f;
			if (transmit) lo_send(t, "/Minicomputer", "iif", currentsound, ((Fl_Valuator*)o)->argument(), tr);
	#ifdef _DEBUG
			printf("parmCallback glide %li : %f --> %f \n", ((Fl_Valuator*)o)->argument(), ((Fl_Valuator*)o)->value(), tr);
	#endif
			break;
		}

		//************************************ filter cuts *****************************
		case 30:{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Positioner*)o)->argument(),f);
	#endif
			miniDisplay[currentsound][4]->value(f);
			break;
		}
		case 33:{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Positioner*)o)->argument(),f);
	#endif		
			miniDisplay[currentsound][5]->value(f);
			break;
		}
		case 40:{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Positioner*)o)->argument(),f);
	#endif
			miniDisplay[currentsound][6]->value(f);
			break;
		}
		case 43:{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Positioner*)o)->argument(),f);
	#endif
			miniDisplay[currentsound][7]->value(f);
			break;
		}
		case 50: // Filter 3 a cut
		{
			float f=0;
			int Argument=0;

			//if (!isFine)
			//{
				f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
				Argument=((Fl_Positioner*)o)->argument();
			//}
			//else 
			//{
			//	f=(((Fl_Valuator*)o)->value());
			//	isFine = false;
			//	Argument=((Fl_Valuator*)o)->argument();
			//}*/
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,Argument,f);
	#ifdef _DEBUG
			printf("parmCallback %i : %g\n", Argument,f);
			fflush(stdout);
	#endif
			miniDisplay[currentsound][8]->value(f);
			break;
		}
		case 53: // Filter 3 b cut
		{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Positioner*)o)->argument(),f);
	#endif
			miniDisplay[currentsound][9]->value(f);
			break;
		}
		case 90: // OSC 3 tune
		{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Positioner*)o)->argument(),f);
	#endif
			miniDisplay[currentsound][10]->value(f);
			miniDisplay[currentsound][11]->value(round(f*6000)/100); // BPM
			break;
		}
		case 111: // Delay time
		{	float f=((Fl_Valuator*)o)->value();
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Valuator*)o)->argument(),f);
	#endif
			if (f!=0) f=round(6000/f)/100;
			miniDisplay[currentsound][12]->value(f); // BPM
			break;
		}
		default:
		{
			if (transmit) lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
	#ifdef _DEBUG
			printf("parmCallback %li : %g\n", ((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
	#endif
			break;
		}
	} // end of switch
} // end of if is_button
#ifdef _DEBUG
fflush(stdout);
#endif
} // end of o != NULL

Fl::awake();
Fl::unlock();
} // end of parmCallback

static void midiparmCallback(Fl_Widget* o, void*) {
	int voice=(((Fl_Valuator*)o)->argument())>>8;
	int parm=(((Fl_Valuator*)o)->argument())&0xFF;
	double val=((Fl_Valuator*)o)->value();
	val=min(val,((Fl_Valuator*)o)->maximum());
	val=max(val,((Fl_Valuator*)o)->minimum());
	((Fl_Valuator*)o)->value(val);
	if (transmit) lo_send(t, "/Minicomputer", "iif", voice, parm, val);
#ifdef _DEBUG
	printf("%li : %g     \r", ((Fl_Valuator*)o)->argument(), val);
#endif
	setmulti_changed(); // All midi parameters belong to the multi
}

/**
 * copybutton parmCallback, called whenever the user wants to copy filterparameters
 * @param Fl_Widget the calling widget
 * @param defined by FLTK but not used
 */
/*
static void copyparmCallback(Fl_Widget* o, void*) {

	int filter = ((Fl_Valuator*)o)->argument();// what to copy there
	switch (filter)
	{
	case 21:
	{
		
		((Fl_Valuator* )Knob[currentsound][33])->value(	((Fl_Valuator* )Knob[currentsound][30])->value());
		parmCallback(Knob[currentsound][33],NULL);
		((Fl_Valuator* )Knob[currentsound][34])->value(	((Fl_Valuator* )Knob[currentsound][31])->value());
		parmCallback(Knob[currentsound][34],NULL);
		((Fl_Valuator* )Knob[currentsound][35])->value(	((Fl_Valuator* )Knob[currentsound][32])->value());
		parmCallback(Knob[currentsound][35],NULL);
	}
	break;
	}
}*/
/**
 * parmCallback for finetuning the current parameter
 * @param Fl_Widget the calling widget
 * @param defined by FLTK but not used
 */
static void finetuneCallback(Fl_Widget* o, void*)
{
	if (currentParameter<_PARACOUNT) // range check
	{
		if(needs_finetune[currentParameter]){
			Fl::lock();
// #ifdef _DEBUG
			printf("finetuneCallback voice %u parameter %u", currentsound, currentParameter);
// #endif
			double val=((Fl_Valuator* )o)->value();
			double val_min = ((Fl_Valuator* )Knob[currentsound][currentParameter])->minimum();
			double val_max = ((Fl_Valuator* )Knob[currentsound][currentParameter])->maximum();
			if(val_max<val_min){ // Reversed range
				val=max(val, val_max);
				val=min(val, val_min);
			}else{
				val=max(val, val_min);
				val=min(val, val_max);
			}
			((Fl_Valuator* )Knob[currentsound][currentParameter])->value(val);
// #ifdef _DEBUG
			printf("=%f\n", val);
			parmCallback(Knob[currentsound][currentParameter], NULL);
			Fl::awake();
			Fl::unlock();
// #ifdef _DEBUG
		}else{
			printf("finetuneCallback - ignoring %i\n", currentParameter);
// #endif
		}
	}else{
		fprintf(stderr, "finetuneCallback - Error: unexpected current parameter %i\n", currentParameter);
	}
}
/*
static void lfoCallback(Fl_Widget* o, void*)
{
	int Faktor = (int)((Fl_Valuator* )o)->value();
	float Rem = ((Fl_Valuator* )o)->value()-Faktor;
	int Argument = ((Fl_Valuator* )o)->argument();
	((Fl_Positioner* )Knob[currentsound][Argument])->xvalue(Faktor);
	((Fl_Positioner* )Knob[currentsound][Argument])->yvalue(Rem);
	parmCallback(Knob[currentsound][Argument],NULL);
}
*/
/** parmCallback when a cutoff has changed
 * these following two parmCallbacks are specialized
 * for the Positioner widget which is 2 dimensional
 * @param Fl_Widget the calling widget
 * @param defined by FLTK but not used
 */
static void cutoffCallback(Fl_Widget* o, void*)
{
	Fl::lock();
	int Faktor = ((int)(((Fl_Valuator* )o)->value()/500)*500);
	float Rem = ((Fl_Valuator* )o)->value()-Faktor;
	int Argument = ((Fl_Valuator* )o)->argument();
	((Fl_Positioner* )Knob[currentsound][Argument])->xvalue(Faktor);
	((Fl_Positioner* )Knob[currentsound][Argument])->yvalue(Rem);
	parmCallback(Knob[currentsound][Argument],NULL);
	Fl::awake();
	Fl::unlock();
}
/** parmCallback for frequency positioners in the oscillators
 * which are to be treated a bit different
 *
 * @param Fl_Widget the calling widget
 * @param defined by FLTK but not used
 */
static void tuneCallback(Fl_Widget* o, void*)
{
	Fl::lock();
	int Faktor = (int)((Fl_Valuator* )o)->value();
	float Rem = ((Fl_Valuator* )o)->value()-Faktor;
	int Argument = ((Fl_Valuator* )o)->argument();
	((Fl_Positioner* )Knob[currentsound][Argument])->xvalue(Faktor);
	((Fl_Positioner* )Knob[currentsound][Argument])->yvalue(Rem);
	parmCallback(Knob[currentsound][Argument],NULL);
	Fl::awake();
	Fl::unlock();
}

static void fixedfrequencyCallback(Fl_Widget* o, void*)
{
	Fl::lock();
	float f = ((Fl_Valuator* )o)->value();
	// No conversion needed
	int Argument = ((Fl_Valuator* )o)->argument();
	((Fl_Valuator*)Knob[currentsound][Argument])->value(f);
	parmCallback(Knob[currentsound][Argument],NULL);
	Fl::awake();
	Fl::unlock();
}

static void BPMtuneCallback(Fl_Widget* o, void*)
{ // Used for BPM to tune conversion
// like for osc 3 tune (90)
	Fl::lock();
	float f = ((Fl_Valuator* )o)->value()/60.f;
	int Faktor = (int)f;
	float Rem = f-Faktor;
	int Argument = ((Fl_Valuator* )o)->argument();
	((Fl_Positioner* )Knob[currentsound][Argument])->xvalue(Faktor);
	((Fl_Positioner* )Knob[currentsound][Argument])->yvalue(Rem);
	parmCallback(Knob[currentsound][Argument],NULL);
	Fl::awake();
	Fl::unlock();
}

static void BPMtimeCallback(Fl_Widget* o, void*)
{ // Used for BPM to time conversion
// like for delay time (111)
	Fl::lock();
	float f = ((Fl_Valuator* )o)->value();
	if(f!=0) f=60.f/f;
	int argument = ((Fl_Valuator* )o)->argument();
	((Fl_Valuator*)Knob[currentsound][argument])->value(f);
	parmCallback(Knob[currentsound][argument], NULL);
	Fl::awake();
	Fl::unlock();
}

/**
 * multiparmCallback for global and per note fine tuning
 * @param Fl_Widget the calling widget
 * @param defined by FLTK but not used
 */
static void multiparmCallback(Fl_Widget* o, void*)
{
	Fl::lock();
	int argument = ((Fl_Valuator* )o)->argument();
	if(argument<_MULTIPARMS){
		double value = ((Fl_Valuator*)o)->value();
		value=min(value, max(((Fl_Valuator*)o)->minimum(), ((Fl_Valuator*)o)->maximum()));
		value=max(value, min(((Fl_Valuator*)o)->minimum(), ((Fl_Valuator*)o)->maximum()));
		((Fl_Valuator*)o)->value(value);
		if (transmit) lo_send(t, "/Minicomputer/multiparm", "if", argument, value);
	}else{
		fprintf(stderr, "ERROR: multiparmCallback - unexpected argument %i\n", argument);
	}
	setmulti_changed();
	Fl::awake();
	Fl::unlock();
}

static void soundRollerCallback(Fl_Widget* o, void*)
{
	Fl::lock();
	int soundnum = (int)((Fl_Valuator* )o)->value();
	soundname[currentsound]->value((Speicher.getName(0,soundnum)).c_str());
	soundname[currentsound]->position(0);
	char ssoundnum[]="***";
	snprintf(ssoundnum, 4, "%i", soundnum);
	soundnumber[currentsound]->value(ssoundnum);// set gui
	setsound_changed();
	Fl::awake();
	Fl::unlock();
}
static void soundincdecCallback(Fl_Widget* o, void*)
{
	Fl::lock();
	int soundnum=soundRoller[currentsound]->value() + o->argument();
	if(soundnum<0) soundnum=0;
	if(soundnum>511) soundnum=511;
	// printf("soundincdecCallback sound %+ld # %u\n", o->argument(), soundnum);
	soundRoller[currentsound]->value(soundnum);
	Fl::awake();
	Fl::unlock();
	soundRollerCallback(soundRoller[currentsound], NULL);
}
static void soundnumberCallback(Fl_Widget* o, void*)
{
	Fl::lock();
	int soundnum=atoi(soundnumber[currentsound]->value());
	if(soundnum<0) soundnum=0;
	if(soundnum>511) soundnum=511;
	char ssoundnum[]="***";
	snprintf(ssoundnum, 4, "%i", soundnum);
	soundnumber[currentsound]->value(ssoundnum);
	if(soundRoller[currentsound]->value()!=soundnum){
		soundRoller[currentsound]->value(soundnum);
		Fl::awake();
		Fl::unlock();
		soundRollerCallback(soundRoller[currentsound], NULL);
	}else{
		Fl::awake();
		Fl::unlock();
	}
}
/*
static void chooseCallback(Fl_Widget* o, void*)
{
//	int Faktor = (int)((Fl_Valuator* )o)->value();
	soundRoller[currentsound]->value(soundname[currentsound]->menubutton()->value());// set gui
	soundnumber[currentsound]->value(soundname[currentsound]->menubutton()->value());// set gui
}*/
static void multiRollerCallback(Fl_Widget* o, void*)
{
	Fl::lock();
	int pgm = (int)((Fl_Valuator* )o)->value();
	char spgm[]="***";
	snprintf(spgm, 4, "%i", pgm);
	multiname->value(Speicher.multis[pgm].name);// set gui
	multiname->position(0);// put cursor at the beginning, make sure the start of the string is visible
	multinumber->value(spgm);
// printf("Multi %i %s\n", pgm, Speicher.multis[pgm].name);
	setmulti_changed();
	Fl::awake();
	Fl::unlock();
}
static void multiincdecCallback(Fl_Widget* o, void*)
{
	Fl::lock();
	int multinum=multiRoller->value() + o->argument();
	if(multinum<0) multinum=0;
	if(multinum>127) multinum=127;
	// printf("multiincdecCallback multi %+ld # %u\n", o->argument(), multinum);
	multiRoller->value(multinum);
	Fl::awake();
	Fl::unlock();
	multiRollerCallback(multiRoller, NULL);
}
static void multinumberCallback(Fl_Widget* o, void*)
{
	Fl::lock();
	int multinum=atoi(multinumber->value());
	if(multinum<0) multinum=0;
	if(multinum>127) multinum=127;
	char smultinum[]="***";
	snprintf(smultinum, 4, "%i", multinum);
	multinumber->value(smultinum);
	if(multiRoller->value()!=multinum){
		multiRoller->value(multinum);
		Fl::awake();
		Fl::unlock();
		multiRollerCallback(multiRoller, NULL);
	}else{
		Fl::awake();
		Fl::unlock();
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
/** parmCallback when export button was pressed, shows a file dialog
 */
static void exportPressed(Fl_Widget* o, void*)
{
char warn[256];
sprintf (warn,"export %s:",soundname[currentsound]->value());
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
		
		Speicher.exportSound(fc->value(),atoi(soundnumber[currentsound]->value()));
		
		fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
		Fl::check();
		Fl::awake();
		Fl::unlock();
	}

}
/** parmCallback when import button was pressed, shows a filedialog
 */
static void importPressed(Fl_Widget* o, void* )
{
char warn[256];
sprintf (warn,"overwrite %s:",soundname[currentsound]->value());
Fl_File_Chooser *fc = new Fl_File_Chooser(".","TEXT Files (*.txt)\t",Fl_File_Chooser::SINGLE,warn);
	fc->textsize(9);
	fc->show();
	while(fc->shown()) Fl::wait(); // block until choice is done
	if ((fc->value() != NULL))
	{
	#ifdef _DEBUG
		//printf("currentsound %i,roller %f, importon %i to %i : %s\n",currentsound,soundRoller[currentsound]->value(),((Fl_Input_Choice*)e)->menubutton()->value(),(int)soundnumber[currentsound]->value(),fc->value());//Speicher.1s[currentmulti].sound[currentsound]
		fflush(stdout);
	#endif
		Fl::lock();
		fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
		Fl::check();
  		
		Speicher.importSound(fc->value(),atoi(soundnumber[currentsound]->value()));//soundname[currentsound]->menubutton()->value());	
		// ok, now we have a new sound saved but we should update the userinterface
		soundname[currentsound]->value(Speicher.getName(0,atoi(soundnumber[currentsound]->value())).c_str());
	  	/*
		int i;
		for (i = 0;i<8;++i)
	  	{
  			soundname[i]->clear();
	  	} 
  
	  	for (i=0;i<512;++i) 
  		{
			 if (i==230)
			 {
			 	printf("%s\n",Speicher.getName(0,229).c_str());
			 	printf("%s\n",Speicher.getName(0,230).c_str());
			 	printf("%s\n",Speicher.getName(0,231).c_str());
			 }
  			soundname[0]->add(Speicher.getName(0,i).c_str());
		  	soundname[1]->add(Speicher.getName(0,i).c_str());
		  	soundname[2]->add(Speicher.getName(0,i).c_str());
	  		soundname[3]->add(Speicher.getName(0,i).c_str());
		  	soundname[4]->add(Speicher.getName(0,i).c_str());
		  	soundname[5]->add(Speicher.getName(0,i).c_str());
  			soundname[6]->add(Speicher.getName(0,i).c_str());
  			soundname[7]->add(Speicher.getName(0,i).c_str());
	  	}
		*/
		fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
		Fl::check();
		Fl::awake();
		Fl::unlock();
	}

}
/** Store given sound # parameters into given patch
 */
static void storesound(unsigned int srcsound, patch *destpatch)
{
	Fl::lock();
	fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
	Fl::check();
#ifdef _DEBUG
	printf("storesound: source sound %i\n",srcsound);
	fflush(stdout);
#endif	
	// clean first the name string
	sprintf(destpatch->name, "%s", soundname[srcsound]->value());

	for (int i=0; i<_PARACOUNT; ++i) // go through all parameters
	{
		if (Knob[srcsound][i] != NULL && needs_multi[i]==0)
		{
			if (is_button[i]){
				destpatch->parameter[i]=((Fl_Valuator*)Knob[srcsound][i])->value()?1.0f:0.0f;
#ifdef _DEBUG
				printf("storesound: button %d = %f\n", i, destpatch->parameter[i]);
#endif
			}else{
				destpatch->parameter[i]=((Fl_Valuator*)Knob[srcsound][i])->value();
#ifdef _DEBUG
				printf("storesound: parameter %d = %f\n", i, destpatch->parameter[i]);
#endif
				switch (i)
				{
				case 3:
					{
					destpatch->freq[0][0]=((Fl_Positioner*)Knob[srcsound][i])->xvalue();
					destpatch->freq[0][1]=((Fl_Positioner*)Knob[srcsound][i])->yvalue();
				break;
				}
				case 18:
						{
					destpatch->freq[1][0]=((Fl_Positioner*)Knob[srcsound][i])->xvalue();
					destpatch->freq[1][1]=((Fl_Positioner*)Knob[srcsound][i])->yvalue();
				break;
				}
				//************************************ filter cuts *****************************
				case 30:
						{
					destpatch->freq[2][0]=((Fl_Positioner*)Knob[srcsound][i])->xvalue();
					destpatch->freq[2][1]=((Fl_Positioner*)Knob[srcsound][i])->yvalue();
				break;
				}
				case 33:
						{
					destpatch->freq[3][0]=((Fl_Positioner*)Knob[srcsound][i])->xvalue();
					destpatch->freq[3][1]=((Fl_Positioner*)Knob[srcsound][i])->yvalue();
				break;
				}
				case 40:
						{
					destpatch->freq[4][0]=((Fl_Positioner*)Knob[srcsound][i])->xvalue();
					destpatch->freq[4][1]=((Fl_Positioner*)Knob[srcsound][i])->yvalue();
				break;
				}
				case 43:
						{
					destpatch->freq[5][0]=((Fl_Positioner*)Knob[srcsound][i])->xvalue();
					destpatch->freq[5][1]=((Fl_Positioner*)Knob[srcsound][i])->yvalue();
				break;
				}
				case 50:
						{
					destpatch->freq[6][0]=((Fl_Positioner*)Knob[srcsound][i])->xvalue();
					destpatch->freq[6][1]=((Fl_Positioner*)Knob[srcsound][i])->yvalue();
				break;
				}
				case 53:
						{
					destpatch->freq[7][0]=((Fl_Positioner*)Knob[srcsound][i])->xvalue();
					destpatch->freq[7][1]=((Fl_Positioner*)Knob[srcsound][i])->yvalue();
				break;
				}
				case 90:
				{
					destpatch->freq[8][0]=((Fl_Positioner*)Knob[srcsound][i])->xvalue();
					destpatch->freq[8][1]=((Fl_Positioner*)Knob[srcsound][i])->yvalue();
				break;
				}
				} // end of switch
			} // end of if not button
		} // end of if not null & not multi
	} // end of for

	for (int i=0; i<_CHOICECOUNT; ++i)
	{
		if (auswahl[srcsound][i] != NULL)
		{
			destpatch->choice[i]=auswahl[srcsound][i]->value();
#ifdef _DEBUG
			printf("storesound: choice #%i: %i\n", i, auswahl[srcsound][i]->value());
#endif
		}
	}
#ifdef _DEBUG
	printf("\n");
	fflush(stdout);
#endif
	/* Build list for selection??
	// ok, now we have saved but we should update the userinterface
  	for (i = 0;i<8;++i)
  	{
  		soundname[i]->clear();
  	} 
  
  	for (i=0;i<512;++i) 
  	{
  		soundname[0]->add(Speicher.getName(0,i).c_str());
	  	soundname[1]->add(Speicher.getName(0,i).c_str());
	  	soundname[2]->add(Speicher.getName(0,i).c_str());
	  	soundname[3]->add(Speicher.getName(0,i).c_str());
	  	soundname[4]->add(Speicher.getName(0,i).c_str());
	  	soundname[5]->add(Speicher.getName(0,i).c_str());
  		soundname[6]->add(Speicher.getName(0,i).c_str());
  		soundname[7]->add(Speicher.getName(0,i).c_str());
  	}
	*/
	fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
	Fl::check();
	Fl::awake();
	Fl::unlock();
}
/** parmCallback when the storebutton is pressed
 * @param Fl_Widget the calling widget
 * @param defined by FLTK but not used
 */
static void storesoundCallback(Fl_Widget* o, void* e)
{
	int preset=atoi(soundnumber[currentsound]->value());
#ifdef _DEBUG
	printf("storesoundCallback: source sound %i\n", preset);
	fflush(stdout);
#endif	
	storesound(currentsound, &Speicher.sounds[preset]);
	Speicher.save();
}
/**
 * recall a single sound into given tab
 * by calling each parameter's parmCallback
 * which should handle both display update
 * and OSC transmission
 */
// static void recall(unsigned int preset)
static void recall0(unsigned int destsound, patch *srcpatch)
{
/*
#ifdef _DEBUG
	printf("recall: voice %u preset %u\n", currentsound, preset);
	fflush(stdout);
#endif
	Speicher.setChoice(currentsound, preset);
	patch *srcpatch;
	int destsound;
	destsound=currentsound;
	srcpatch=&Speicher.sounds[Speicher.getChoice(destsound)];
	*/

	printf("recall0 dest %u\n", destsound);
	for(int i=0;i<_PARACOUNT;++i)
	{
		if (Knob[destsound][i] != NULL && needs_multi[i]==0)
		{
			if(is_button[i]){
#ifdef _DEBUG
				printf("recall0 voice %u button # %u\n", destsound, i);
				fflush(stdout);
#endif
				if (srcpatch->parameter[i]==0.0f)
				{
					((Fl_Light_Button*)Knob[destsound][i])->value(0);
				}
				else
				{
					((Fl_Light_Button*)Knob[destsound][i])->value(1);
				}
			}else{
#ifdef _DEBUG
				printf("recall0 voice %u parameter # %u\n", destsound, i);
				fflush(stdout);
#endif
				switch (i)
				{
				case 3:
						{
					((Fl_Positioner*)Knob[destsound][i])->xvalue(srcpatch->freq[0][0]);
					((Fl_Positioner*)Knob[destsound][i])->yvalue(srcpatch->freq[0][1]);
					break;
				}
				case 18:
						{
					((Fl_Positioner*)Knob[destsound][i])->xvalue(srcpatch->freq[1][0]);
					((Fl_Positioner*)Knob[destsound][i])->yvalue(srcpatch->freq[1][1]);
					break;
				}
				//************************************ filter cuts *****************************
				case 30:
						{
					((Fl_Positioner*)Knob[destsound][i])->xvalue(srcpatch->freq[2][0]);
					((Fl_Positioner*)Knob[destsound][i])->yvalue(srcpatch->freq[2][1]);
				break;
				}
				case 33:
						{
					((Fl_Positioner*)Knob[destsound][i])->xvalue(srcpatch->freq[3][0]);
					((Fl_Positioner*)Knob[destsound][i])->yvalue(srcpatch->freq[3][1]);
					parmCallback(Knob[destsound][i],NULL);
				break;
				}
				case 40:
						{
					((Fl_Positioner*)Knob[destsound][i])->xvalue(srcpatch->freq[4][0]);
					((Fl_Positioner*)Knob[destsound][i])->yvalue(srcpatch->freq[4][1]);
				break;
				}
				case 43:
						{
					((Fl_Positioner*)Knob[destsound][i])->xvalue(srcpatch->freq[5][0]);
					((Fl_Positioner*)Knob[destsound][i])->yvalue(srcpatch->freq[5][1]);
				break;
				}
				case 50:
						{
					((Fl_Positioner*)Knob[destsound][i])->xvalue(srcpatch->freq[6][0]);
					((Fl_Positioner*)Knob[destsound][i])->yvalue(srcpatch->freq[6][1]);
				break;
				}
				case 53:
				{
					((Fl_Positioner*)Knob[destsound][i])->xvalue(srcpatch->freq[7][0]);
					((Fl_Positioner*)Knob[destsound][i])->yvalue(srcpatch->freq[7][1]);
				break;
				}
				case 90:
				{
					((Fl_Positioner*)Knob[destsound][i])->xvalue(srcpatch->freq[8][0]);
					((Fl_Positioner*)Knob[destsound][i])->yvalue(srcpatch->freq[8][1]);
				break;
				}
				default:
				{
					((Fl_Valuator*)Knob[destsound][i])->value(srcpatch->parameter[i]);
				break;
				}
				} // End of switch
			} // End of if is_button 
			parmCallback(Knob[destsound][i], NULL);
#ifdef _DEBUG
		}else{
			printf("recall0 voice %i parameter %i : null or multi\n", destsound, i);
#endif
		} // End of if not null & not multi
	} // End of loop for parameters

	for (int i=0; i<_CHOICECOUNT; ++i)
	{
		if (auswahl[destsound][i] != NULL)
		{
			auswahl[destsound][i]->value(srcpatch->choice[i]);
			choiceCallback(auswahl[destsound][i],NULL);
#ifdef _DEBUG 
			printf("recall0 voice %i choice # %i : %i\n", destsound, i,Speicher.sounds[Speicher.getChoice(destsound)].choice[i]);
		}else{
			printf("recall0 voice %i choice %i : null or multi\n", destsound, i);
#endif
		}
	}
	// send a reset (clears filters and delay buffer)
	if (transmit) lo_send(t, "/Minicomputer", "iif", destsound, 0, 0.0f);
#ifdef _DEBUG 
	fflush(stdout);
#endif
}

/**
 * recall a single sound into current tab
 */
static void recall(unsigned int preset)
{
#ifdef _DEBUG
	printf("recall: voice %u preset %u\n", currentsound, preset);
	fflush(stdout);
#endif
	Speicher.setChoice(currentsound, preset);
	// In case name has been edited
	soundname[currentsound]->value(Speicher.sounds[preset].name);
	soundname[currentsound]->position(0);
	recall0(currentsound, &Speicher.sounds[preset]);
	clearsound_changed();
}

static void do_compare(Fl_Widget* o, void* ){
	if(((Fl_Valuator*)o)->value()){
		storesound(currentsound, &compare_buffer);
		int preset=atoi(soundnumber[currentsound]->value());
		recall0(currentsound, &Speicher.sounds[preset]);
	}else{
		recall0(currentsound, &compare_buffer);
	}
}
/**
 * parmCallback when the load button is pressed
 * @param pointer to the calling widget
 * @param optional data, this time the entry id of which the sound 
 * should be loaded
 */
static void loadsoundCallback(Fl_Widget* o, void* )
{
	Fl::lock();
	//fl_cursor(FL_CURSOR_WAIT,FL_WHITE, FL_BLACK);
	//Fl::awake();
#ifdef _DEBUG
	//printf("maybe %i choice %i\n",Speicher.getChoice(currentsound),((Fl_Input_Choice*)e)->menubutton()->value());
	fflush(stdout);
#endif
	//Speicher.multis[currentmulti].sound[currentsound]=(unsigned int)((Fl_Input_Choice*)e)->menubutton()->value();
	//recall(Speicher.multis[currentmulti].sound[currentsound]);
	recall((unsigned int)(atoi(soundnumber[currentsound]->value())));//(Fl_Input_Choice*)e)->menubutton()->value());
	//fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
	Fl::awake();
	Fl::unlock();
}
static void soundnameCallback(Fl_Widget* o, void* ){
	setsound_changed();
}
static void multinameCallback(Fl_Widget* o, void* ){
	setmulti_changed();
}

/**
 * parmCallback when the load multi button is pressed
 * recall a multitemperal setup
 * @param pointer to the calling widget
 * @param optional data, this time the entry id of which the sound 
 * should be loaded
 */
static void loadmultiCallback(Fl_Widget*, void*)
{
	Fl::lock();
	//fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
	//Fl::awake();
	currentmulti = (unsigned int)multiRoller->value();
	// In case name has been edited
	multiname->value(Speicher.multis[currentmulti].name);
	multiname->position(0);
#ifdef _DEBUG
	printf("loadmultiCallback #%u transmit %u\n", currentmulti, transmit);
#endif
	//multi[currentmulti][currentsound]=(unsigned int)((Fl_Input_Choice*)e)->menubutton()->value();
	for (int i=0;i<_MULTITEMP;++i)
	{
		currentsound = i;
		if ((Speicher.multis[currentmulti].sound[i]>=0) && (Speicher.multis[currentmulti].sound[i]<512))
		{
			recall(Speicher.multis[currentmulti].sound[i]);
			#ifdef _DEBUG
				printf("i ist %i Speicher ist %i\n",i,Speicher.multis[currentmulti].sound[i]);
				fflush(stdout);
			#endif
			char temp_name[128];
			strnrtrim(temp_name, Speicher.getName(0,Speicher.multis[currentmulti].sound[i]).c_str(), 128);
			soundname[i]->value(temp_name);
			#ifdef _DEBUG
			printf("loadmultiCallback voice %u: \"%s\"\n", i, temp_name);
			#endif
			#ifdef _DEBUG
				printf("soundname gesetzt\n");
				fflush(stdout);
			#endif
			soundRoller[i]->value(Speicher.multis[currentmulti].sound[i]);// set gui
			#ifdef _DEBUG
				printf("Roller gesetzt\n");
				fflush(stdout);
			#endif
			char ssoundnum[]="***";
			snprintf(ssoundnum, 4, "%i", Speicher.multis[currentmulti].sound[i]);
			soundnumber[i]->value(ssoundnum);
			#ifdef _DEBUG
				printf("soundnumber gesetzt\n");
				fflush(stdout);
			#endif
		}
		// set the knobs of the mix, 0..4
		int MULTI_parm_num[]={101, 106, 107, 108, 109};
		for (unsigned int j=0; j<sizeof(MULTI_parm_num)/sizeof(int); j++)
		{
			((Fl_Valuator*)Knob[i][MULTI_parm_num[j]])->value(Speicher.multis[currentmulti].settings[i][j]);
			parmCallback(Knob[i][MULTI_parm_num[j]],NULL);
		}
		// set the midi parameters, 5..10
		int MIDI_parm_num[]={125, 126, 127, 128, 129, 130};
		int start=5; // sizeof(MULTI_parm_num)/sizeof(int);
		for (unsigned int j=0; j<sizeof(MIDI_parm_num)/sizeof(int); j++)
		{
			// printf("loadmultiCallback: %u %u %f\n", j, MIDI_parm_num[j], Speicher.multis[currentmulti].settings[i][j+start]);
			((Fl_Valuator*)Knob[i][MIDI_parm_num[j]])->value(Speicher.multis[currentmulti].settings[i][j+start]);
			midiparmCallback(Knob[i][MIDI_parm_num[j]],NULL);
		}
	}
	for (int i=0;i<_MULTIPARMS;++i){
		((Fl_Valuator*)multiparm[i])->value(Speicher.multis[currentmulti].parms[i]);
		multiparmCallback(multiparm[i], NULL);
	}
	clearmulti_changed();

	currentsound = 0;
	// we should go to a defined state, means tab
	tabs->value(tab[currentsound]);
	tabCallback(tabs,NULL);

#ifdef _DEBUG
	printf("multi choice %u\n", currentmulti);
	fflush(stdout);
#endif
	//fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
	//Fl::check();
	Fl::awake();
	Fl::unlock();
}

/**
 * parmCallback when the store multi button is pressed
 * store a multitemperal setup
 * @param pointer to the calling widget
 */
static void storemultiCallback(Fl_Widget* o, void* e)
{
	Fl::lock();
	fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
	Fl::check();
	/*printf("choice %i\n",((Fl_Input_Choice*)e)->menubutton()->value());
	fflush(stdout);
	*/
	int i;

	if (multiname != NULL)
	{
		unsigned int t = (unsigned int) multiRoller->value();//multiname->menubutton()->value();
#ifdef _DEBUG
		printf("was:%d is:%d\n",currentmulti,t);
#endif
		if ((t!=currentmulti) && (t>=0) && (t<128))
		{
			currentmulti = t;
		}
	}
	else
		printf("problems with the multichoice widgets!\n");

	strcpy(Speicher.multis[currentmulti].name,((Fl_Input*)e)->value());
	//printf("input choice %s\n",((Fl_Input_Choice*)e)->value());

	//((Fl_Input_Choice*)e)->menubutton()->replace(currentmulti,((Fl_Input_Choice*)e)->value());

	//Schaltbrett.soundchoice-> add(Speicher.getName(i).c_str());
	// get the knobs of the mix
	
	for (i=0;i<_MULTITEMP;++i)
	{
		Speicher.multis[currentmulti].sound[i]=Speicher.getChoice(i);
#ifdef _DEBUG
		printf("sound slot: %d = %d\n",i,Speicher.getChoice(i));
#endif
		// Indices must match those in loadmultiCallback
		Speicher.multis[currentmulti].settings[i][0]=((Fl_Valuator*)Knob[i][101])->value();
		Speicher.multis[currentmulti].settings[i][1]=((Fl_Valuator*)Knob[i][106])->value();
		Speicher.multis[currentmulti].settings[i][2]=((Fl_Valuator*)Knob[i][107])->value();
		Speicher.multis[currentmulti].settings[i][3]=((Fl_Valuator*)Knob[i][108])->value();
		Speicher.multis[currentmulti].settings[i][4]=((Fl_Valuator*)Knob[i][109])->value();
		Speicher.multis[currentmulti].settings[i][5]=((Fl_Valuator*)Knob[i][125])->value();
		Speicher.multis[currentmulti].settings[i][6]=((Fl_Valuator*)Knob[i][126])->value();
		Speicher.multis[currentmulti].settings[i][7]=((Fl_Valuator*)Knob[i][127])->value();
		Speicher.multis[currentmulti].settings[i][8]=((Fl_Valuator*)Knob[i][128])->value();
		Speicher.multis[currentmulti].settings[i][9]=((Fl_Valuator*)Knob[i][129])->value();
		Speicher.multis[currentmulti].settings[i][10]=((Fl_Valuator*)Knob[i][130])->value();
	}
	for (i=0;i<_MULTIPARMS;++i){
		Speicher.multis[currentmulti].parms[i]=((Fl_Valuator*)multiparm[i])->value();
	}
	// write to disk
	Speicher.saveMulti();
	
	fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
	Fl::check();
	Fl::awake();
	Fl::unlock();
}
/**
 * change the multisetup, should be called from a Midi Program Change event on Channel 9
 * @param int the Program number between 0 and 127
 */
void UserInterface::changeMulti(int pgm)
{
	char temp_name[128];
	char spgm[]="***";
	snprintf(spgm, 4, "%i", pgm);
	Fl::lock();
	strnrtrim(temp_name, Speicher.multis[pgm].name, 128);
	multiname->value(temp_name); // multichoice
	#ifdef _DEBUG
	printf("UserInterface::changeMulti # %u: \"%s\"\n", pgm, temp_name);
	#endif
	//multichoice->damage(FL_DAMAGE_ALL);
	multiname->redraw(); // multichoice
	multiRoller->value(pgm);// set gui
	multiRoller->redraw();
	multinumber->value(spgm);
	multinumber->redraw();
	loadmultiCallback(NULL,multiname); // multichoice
	//	Fl::redraw();
	//	Fl::flush();
	Fl::awake();
	Fl::unlock();
}

/**
 * change the sound for a certain voice, should be called from a Midi Program Change event on Channel 1 - 8
 * @param int the voice between 0 and 7 (it is not clear if the first Midi channel is 1 (which is usually the case in the hardware world) or 0)
 * @param int the Program number between 0 and 127
 */
void UserInterface::changeSound(int channel, int pgm)
{
	if ((channel >-1) && (channel < 8) && (pgm>-1) && (pgm<512))
	{
	Fl::lock();
		int t = currentsound;
        char temp_name[128];
		currentsound = channel;
        strnrtrim(temp_name, Speicher.getName(0,pgm).c_str(),128);
#ifdef _DEBUG
        printf("Change to sound %u \"%s\"\n", pgm, temp_name);
#endif
		soundname[channel]->value(temp_name);
		//soundname[channel]->damage(FL_DAMAGE_ALL);
		//soundname[channel]->redraw();
		soundRoller[channel]->value(pgm);// set gui
		soundRoller[channel]->redraw();
		char spgm[]="***";
		snprintf(spgm, 4, "%i", pgm);
		soundnumber[channel]->value(spgm);
		loadsoundCallback(NULL,soundname[channel]);
//		Fl::redraw();
//		Fl::flush();
		currentsound = t;
	Fl::awake();
	Fl::unlock();
	}
}
/**
 * predefined menu with all modulation sources
 */
Fl_Menu_Item menu_amod[] = { // UserInterface::
 {"none", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"velocity", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"pitch bend", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"osc 1 fm out", 0,  0, 0,0 , FL_NORMAL_LABEL , 0, 8, 0},
 {"osc 2 fm out", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"osc 1", 0,  0, 0, FL_MENU_INVISIBLE, FL_NORMAL_LABEL, 0, 8, 0},
 {"osc 2", 0,  0, 0, FL_MENU_INVISIBLE, FL_NORMAL_LABEL, 0, 8, 0},
 {"filter", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 1", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 2", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 3", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 4", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 5", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 6", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"modulation osc", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"touch", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"mod wheel", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 12", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"delay", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"midi note", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 2", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 4", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 5", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 6", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 14", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 15", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 16", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 17", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {0,0,0,0,0,0,0,0,0}
};
// redundant for now...
Fl_Menu_Item menu_fmod[] = { // UserInterface::
 {"none", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"velocity", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"pitch bend", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"osc 1 fm out", 0,  0, 0,0 , FL_NORMAL_LABEL , 0, 8, 0},
 {"osc 2 fm out", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"osc 1", 0,  0, 0, FL_MENU_INVISIBLE, FL_NORMAL_LABEL, 0, 8, 0},
 {"osc 2", 0,  0, 0, FL_MENU_INVISIBLE, FL_NORMAL_LABEL, 0, 8, 0},
 {"filter", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 1", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 2", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 3", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 4", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 5", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"eg 6", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"modulation osc", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"touch", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"mod wheel", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 12", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"delay", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"midi note", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 2", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 4", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 5", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 6", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 14", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 15", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 16", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"cc 17", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {0,0,0,0,0,0,0,0,0}
};
/**
 * waveform list for menu
 */
Fl_Menu_Item menu_wave[] = { // UserInterface::
 {"sine", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"ramp up", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"ramp down", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"tri", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"square", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"bit", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"spike", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"comb", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0}, 
 {"add Saw", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"add Square", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"Microcomp 1", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"Microcomp 8", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"Microcomp 9", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"Microcomp 10", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"Microcomp 11", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"Microcomp 12", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"Microcomp 13", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {"Microcomp 14", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 8, 0},
 {0,0,0,0,0,0,0,0,0}
};
/*
Fl_SteinerKnob::Fl_SteinerKnob(int x, int y, int w, int h, const char *label)
: Fl_Dial(x, y, w, h, label) {
	
}
int Fl_SteinerKnob::handle(int event) {
	if (event==FL_PUSH)
	{
		if (Fl::event_button2())
		{
			altx=Fl::event_x();
			
			this->set_changed();
			//return 0;
		}
		else if (Fl::event_button3())
		{
			this->step(0.001);
		}
		else this->step(0);
	}
	if (event ==FL_DRAG)
	{
	if (Fl::event_button2())
		{
			if (Fl::event_x()<altx )
			 this->value(this->value()+1);
			else   this->value(this->value()-1);
			altx=Fl::event_x();
			this->set_changed();
			return 0;
		}
		if (Fl::event_button3())
		{
			if (Fl::event_x()<altx )
			 this->value(this->value()+0.01);
			else   this->value(this->value()-0.01);
			altx=Fl::event_x();
			this->set_changed();
			return 0;
		}
	}
	  return Fl_Dial::handle(event);
}
*/
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
	  { Fl_Group* o = new Fl_Group(x, y, 200, 45, EG_label); // 608 31
		group[groups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(_TEXT_SIZE);
		// Attack
		{ Fl_Dial* o = new Fl_Dial(x+10, y+6, 25, 25, "A");
		  o->labelsize(_TEXT_SIZE); 
		  o->argument(EG_base);  
		  o->minimum(EG_rate_min);
		  o->maximum(EG_rate_max);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[voice][o->argument()] = o;
		}
		// Decay
		{ Fl_Dial* o = new Fl_Dial(x+40, y+6, 25, 25, "D");
		  o->labelsize(_TEXT_SIZE);
		  o->argument(EG_base+1);
		  o->minimum(EG_rate_min);
		  o->maximum(EG_rate_max);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[voice][o->argument()] = o;
		}
		// Sustain
		{ Fl_Dial* o = new Fl_Dial(x+70, y+6, 25, 25, "S");
		  o->labelsize(_TEXT_SIZE);
		  o->argument(EG_base+2);
		 // o->minimum(0);
		 // o->maximum(0.001);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[voice][o->argument()] = o;
		}
		// Release
		{ Fl_Dial* o = new Fl_Dial(x+100, y+6, 25, 25, "R");
		  o->labelsize(_TEXT_SIZE);
		  o->argument(EG_base+3);
		  o->minimum(EG_rate_min);
		  o->maximum(EG_rate_max);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[voice][o->argument()] = o;
		}
		// Repeat
		{ Fl_Light_Button* o = new Fl_Light_Button(x+136, y+11, 55, 15, "repeat");
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(_TEXT_SIZE);
		  o->argument(EG_base+4);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[voice][o->argument()] = o;
		}
		o->end();
	  }
}

void UserInterface::make_filter(int voice, int filter_base, int minidisplay, int x, int y){
	{Fl_Positioner* o = new Fl_Positioner(x, y, 70, 79, "cut");
	o->xbounds(0,9000);
	o->ybounds(499,0); o->selection_color(0);
	o->xstep(500);
	o->box(FL_BORDER_BOX);
	o->labelsize(_TEXT_SIZE);
	o->argument(filter_base);
	o->yvalue(200);
	o->callback((Fl_Callback*)parmCallback);
	Knob[voice][o->argument()] = o;
	/* Fl_Dial* o = f1cut1 = new Fl_SteinerKnob(344, 51, 34, 34, "cut");
	  o->labelsize(_TEXT_SIZE);
	  o->argument(30);
	  o->maximum(10000);
	  o->value(50);
	  o->callback((Fl_Callback*)parmCallback);
	*/
	}
	{ Fl_Dial* o = new Fl_Dial(x+75, y+2, 25, 25, "q");
	  o->labelsize(_TEXT_SIZE);
	  o->argument(filter_base+1);
	  o->minimum(0.9);
	  o->value(0.9);
	  o->maximum(0.01);
	  o->callback((Fl_Callback*)parmCallback);
	  Knob[voice][o->argument()] = o;
	}
	{ Fl_Dial* o = new Fl_Dial(x+85, y+39, 20, 20, "vol");
	  o->labelsize(_TEXT_SIZE);
	  o->argument(filter_base+2);
	  o->callback((Fl_Callback*)parmCallback);
	  o->minimum(-1);
	  o->value(0.5);
	  o->maximum(1);
	  Knob[voice][o->argument()] = o;
	}
	{ Fl_Value_Input* o = new Fl_Value_Input(x+72, y+69, 38, 15);
	  o->box(FL_ROUNDED_BOX);
	  o->labelsize(_TEXT_SIZE);
	  o->textsize(_TEXT_SIZE);
	  o->maximum(9500);
	  o->step(0.01);
	  o->value(200);
	  o->argument(filter_base);
	  o->callback((Fl_Callback*)cutoffCallback);
	  miniDisplay[voice][minidisplay]=o;
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
	  { Fl_Dial* o= new Fl_Dial(x+5, y, 34, 34, "frequency");
		o->labelsize(_TEXT_SIZE);
		o->maximum(1000); 
		o->argument(osc_base);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	  }
	  { Fl_Value_Input* o = new Fl_Value_Input(x, y+46, 46, 15); // Fixed frequency display for oscillator 1
		o->box(FL_ROUNDED_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->maximum(1000);
		o->step(0.001);
		o->argument(osc_base);
		o->callback((Fl_Callback*)fixedfrequencyCallback);
		miniDisplay[voice][minidisplay_base]=o;
	  }
	  { Fl_Light_Button* o = new Fl_Light_Button(x, y+72, 66, 19, "fix frequency");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+1);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	  }
	  { Fl_Positioner* o = new Fl_Positioner(x, y+95, 40, 80, "tune");
		o->xbounds(0,16);
		o->ybounds(1,0);
		o->box(FL_BORDER_BOX);
		o->xstep(1);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+2);
		o->selection_color(0);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()]=o;
	  }
	  { Fl_Value_Input* o = new Fl_Value_Input(x, y+185, 46, 15); // Tune display for oscillator 1
		o->box(FL_ROUNDED_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->maximum(10000);
		o->argument(osc_base+2);
		o->step(0.001);
		o->callback((Fl_Callback*)tuneCallback);
		miniDisplay[voice][minidisplay_base+1]=o;
	  }
	  { Fl_Light_Button* o = new Fl_Light_Button(x+65, y+7, 40, 15, "boost");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+3);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	  } 
	  { Fl_Choice* o = new Fl_Choice(x+107, y+7, 120, 15, "freq modulator 1");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(choice_base);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_fmod);
		auswahl[voice][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(x+233, y+3, 25, 25, "amount");
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+4);  
		o->minimum(-1000);
		o->maximum(1000);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	  }
	{ Fl_Light_Button* o = new Fl_Light_Button(x+65, y+44, 40, 15, "mult.");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+116);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	}
	{ Fl_Choice* o = new Fl_Choice(x+107, y+44, 120, 15, "freq modulator 2");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(choice_base+1);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_fmod);
		auswahl[voice][o->argument()]=o;
	}
	{ Fl_Dial* o = new Fl_Dial(x+232, y+39, 25, 25, "amount");
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+6); 
		o->minimum(-1000);
		o->maximum(1000);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	}
	{ Fl_Light_Button* o = new Fl_Light_Button(x+77, y+82, 40, 15, "mult.");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+120);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	}
	  { Fl_Choice* o = new Fl_Choice(x+119, y+82, 120, 15, "amp modulator 1");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(choice_base+2);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_amod);
		auswahl[voice][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(x+245, y+77, 25, 25, "amount");
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+8); 
		o->minimum(-1);
		o->maximum(1);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	  }
	{ Fl_Light_Button* o = new Fl_Light_Button(x+77, y+116, 40, 15, "mult.");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+117);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
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
		auswahl[voice][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(x+245, y+111, 25, 25, "amount");
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+9);  
		o->minimum(-1);
		o->maximum(1);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(x+77, y+144, 20, 20, "start phase");
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+118); 
		o->minimum(0);
		o->maximum(360);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	  }
	  { Fl_Light_Button* o = new Fl_Light_Button(x+102, y+147, 10, 15); // Phase reset enable
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+119);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	  }
	  { Fl_Choice* j = new Fl_Choice(x+119, y+147, 120, 15, "waveform");
		j->box(FL_BORDER_BOX);
		j->down_box(FL_BORDER_BOX);
		j->labelsize(_TEXT_SIZE);
		j->textsize(_TEXT_SIZE);
		j->align(FL_ALIGN_TOP_LEFT);
		j->argument(choice_base+4);
		auswahl[voice][j->argument()] = j;
		j->callback((Fl_Callback*)choiceCallback);
		j->menu(menu_wave);
	  }
	  { Fl_Dial* o = new Fl_Dial(x+245, y+144, 20, 20, "fm out vol");
		o->labelsize(_TEXT_SIZE);
		o->argument(osc_base+12);
		o->callback((Fl_Callback*)parmCallback);
		Knob[voice][o->argument()] = o;
	  }
	  /*{ Fl_Dial* o = new Fl_SteinerKnob(20, 121, 34, 34, "tune");
		o->labelsize(_TEXT_SIZE);
		o->minimum(0.5);
		o->maximum(16);
		o->argument(3);
		o->callback((Fl_Callback*)parmCallback);
		Knob[3] = o;
	  }*/
}
    
Fenster* UserInterface::make_window(const char* title) {    
 // Fl_Double_Window* w;
 // {
	currentsound=0;
	currentmulti=0;
	
	// special treatment for the mix knobs and MIDI settings
	// they are saved in the multi, not in the sound
	needs_multi[101]=1; // Id vol
	needs_multi[106]=1; // Mix vol
	needs_multi[107]=1; // Pan
	needs_multi[108]=1; // Aux 1
	needs_multi[109]=1; // Aux 2
	needs_multi[125]=1; // Note
	needs_multi[126]=1; // Velocity
	needs_multi[127]=1; // Channel
	needs_multi[128]=1; // Note min
	needs_multi[129]=1; // Note max
	needs_multi[130]=1; // Transpose

	// show parameter on fine tune only when relevant (not a frequency...)
	for (int i=0;i<_PARACOUNT;++i) needs_finetune[i]=1;
	needs_finetune[0]=0; // clear state
	needs_finetune[1]=0; // Osc 1 fixed frequency
	needs_finetune[3]=0; // Osc 1 tune
	needs_finetune[16]=0; // Osc 2 fixed frequency
	needs_finetune[18]=0; // Osc 2 tune
	needs_finetune[30]=0; // Filter 1 A cut
	needs_finetune[33]=0; // Filter 1 B cut
	needs_finetune[40]=0; // Filter 2 A cut
	needs_finetune[43]=0; // Filter 2 B cut
	needs_finetune[50]=0; // Filter 3 A cut
	needs_finetune[53]=0; // Filter 3 B cut
	needs_finetune[90]=0; // Osc 3 tune
	needs_finetune[125]=0; // Audition note number
	needs_finetune[126]=0; // Audition note velocity
	needs_finetune[127]=0; // Midi channel
	needs_finetune[128]=0; // Min note
	needs_finetune[129]=0; // Max note
	needs_finetune[130]=0; // Transpose
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

	for (int i=0;i<_PARACOUNT;++i) is_button[i]=0;
	is_button[2]=1; // Fixed frequency osc 1
	is_button[4]=1; // boost button osc 1
	is_button[17]=1; // Fixed frequency osc 2
	is_button[19]=1; // boost button osc 2
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
	is_button[120]=1;
	is_button[135]=1;
	is_button[137]=1; // Bypass filters
	is_button[139]=1; // Legato
	is_button[143]=1; // Time modulator 2 mult.


	transmit=true;

	Fenster* o = new Fenster(_INIT_WIDTH, _INIT_HEIGHT, title);
	o->color((Fl_Color)_BGCOLOR);
	o->user_data((void*)(this));

	for (int i=0;i<_CHOICECOUNT;++i) {
		auswahl[currentsound][i]=NULL;
	}
	for (int i=0;i<_PARACOUNT;++i) {
		Knob[currentsound][i]=NULL;
	}

// tabs beginning ------------------------------------------------------------
	{ Fl_Tabs* o = new Fl_Tabs(0, 0, _INIT_WIDTH, _INIT_HEIGHT);
		o->callback((Fl_Callback*)tabCallback);
	int i;
	for (i=0; i<_MULTITEMP; ++i)// generate a tab for each voice
	{
		{ 
		ostringstream oss;
		oss<<"voice "<<(i+1);// create name for tab
		tablabel[i]=oss.str();
		Fl_Group* o = new Fl_Group(1, 10, 995, 515, tablabel[i].c_str());
		group[groups++]=o;
		o->color((Fl_Color)_BGCOLOR);
		o->labelsize(_TEXT_SIZE);
		// o->labelcolor(FL_BACKGROUND2_COLOR); 
		o->box(FL_BORDER_FRAME);

	// Show voice number next to the logo
	{ Fl_Box* d = new Fl_Box(845, 455, 40, 40, voicename[i]);
		d->labelsize(48);
		d->labelcolor(FL_RED);
		voicedisplay[i]=d;
	}

	// Oscillator 1
	{ Fl_Group* o = new Fl_Group(5, 17, 300, 212, "oscillator 1");
	  group[groups++]=o;
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	  o->labelsize(_TEXT_SIZE);
	  o->labelcolor(FL_BACKGROUND2_COLOR);
	  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
	  {
		make_osc(i, 1, 0, 0, 16, 20, "oscillator 1", "amp modulator 2");
		{ Fl_Choice* j = new Fl_Choice(134, 196, 120, 15, "sub waveform");
			j->box(FL_BORDER_BOX);
			j->down_box(FL_BORDER_BOX);
			j->labelsize(_TEXT_SIZE);
			j->textsize(_TEXT_SIZE);
			j->align(FL_ALIGN_TOP_LEFT);
			j->argument(15);
			auswahl[i][j->argument()] = j;
			j->callback((Fl_Callback*)choiceCallback);
			j->menu(menu_wave);
		}
	  o->end();
	  } 
	}
   { Fl_Group* o = new Fl_Group(5, 238, 300, 212, "oscillator 2");
		group[groups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_BACKGROUND2_COLOR);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor(FL_BACKGROUND2_COLOR);
		o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
		make_osc(i, 16, 2, 6, 16, 244, "oscillator 2", "fm out amp modulator");
		{ Fl_Light_Button* o = new Fl_Light_Button(134, 416, 65, 15, "sync to osc1");
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(_TEXT_SIZE);
		  o->argument(115);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[i][o->argument()] = o;
		}
		/*Fl_Dial* o = new Fl_SteinerKnob(20, 345, 34, 34, "tune");
		o->labelsize(_TEXT_SIZE);
		o->minimum(0.5);
		o->maximum(16);   
		o->argument(18);
		o->callback((Fl_Callback*)parmCallback);*/
	  o->end();
	}
	// ----------------- knobs  for the filters -------------------------------------- 
	{ Fl_Group* o = new Fl_Group(312, 17, 277, 433, "filters");
	  group[groups++]=o;
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	  o->labelsize(_TEXT_SIZE);
	  o->labelcolor(FL_BACKGROUND2_COLOR);
	  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
	  { Fl_Group* o = new Fl_Group(330, 28, 239, 92, "filter 1");
		group[groups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(_TEXT_SIZE);
		make_filter(i, 30, 4, 340, 31);
		make_filter(i, 33, 5, 456, 31);
		o->end();
	  }
	  { Fl_Group* o = new Fl_Group(330, 132, 239, 92, "filter 2");
		group[groups++]=o;
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
	  { Fl_Group* o = new Fl_Group(330, 238, 239, 92, "filter 3");
		group[groups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(_TEXT_SIZE);
		make_filter(i, 50, 8, 340, 241);
		make_filter(i, 53, 9, 456, 241);
		o->end();
	  } 
	  { Fl_Dial* o = new Fl_Dial(418, 360, 60, 57, "morph");
		o->type(1);
		o->labelsize(_TEXT_SIZE);
		o->maximum(0.5f);
		o->argument(56);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(326, 392, 25, 25, "amount");
		o->labelsize(_TEXT_SIZE);
		o->minimum(-2);
		o->maximum(2);
		o->argument(38);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(325, 366, 85, 15, "morph mod 1");
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(5);
		o->callback((Fl_Callback*)choiceCallback);
		auswahl[i][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(551, 392, 25, 25, "amount");
		o->labelsize(_TEXT_SIZE);
		o->argument(48);
		o->minimum(-2);
		o->maximum(2);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(492, 366, 85, 15, "morph mod 2");
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(11);
		o->callback((Fl_Callback*)choiceCallback);
		auswahl[i][o->argument()]=o;
	  }
	  // Mult button morph mod 2
	  { Fl_Light_Button* o = new Fl_Light_Button(492, 397, 40, 15, "mult.");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(_TEXT_SIZE);
		o->argument(140);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  o->end(); // Group "filters"
	  filtersgroup[i]=o;
	  // The two following buttons are not really part of the filter parameters
	  { Fl_Light_Button* o = new Fl_Light_Button(325, 430, 85, 15, "bypass filters");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		// o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->selection_color(FL_RED);
		// o->color(FL_LIGHT1, FL_RED);
		o->argument(137);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Button* o = new Fl_Button(492, 430, 85, 15, "clear state");
		o->tooltip("reset the filter and delay");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->argument(0);
		o->callback((Fl_Callback*)parmCallback);
		clearstateBtn[i]=o;
	  }
	}
	{ Fl_Group* o = new Fl_Group(595, 17, 225, 433, "modulators");
	  group[groups++]=o;
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	  o->labelsize(_TEXT_SIZE);
	  o->labelcolor(FL_BACKGROUND2_COLOR);
	  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
/*
	{ Fl_Box* d = new Fl_Box(595, 225, 210, 432, "modulators");
	  d->labelsize(_TEXT_SIZE);
	  d->labelcolor(FL_BACKGROUND2_COLOR);
	  }
	  */
	  // ----------- knobs for envelope generators ---------------
	  make_EG(i, 60, 608, 31, "EG 1");
	  make_EG(i, 65, 608, 90, "EG 2");
	  make_EG(i, 70, 608, 147, "EG 3");
	  make_EG(i, 75, 608, 204, "EG 4");
	  make_EG(i, 80, 608, 263, "EG 5");
	  make_EG(i, 85, 608, 324, "EG 6");
	  
	  // ----------- knobs for mod oscillator ---------------
	  { Fl_Group* o = new Fl_Group(608, 380, 200, 54, "mod osc");
		group[groups++]=o;
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(_TEXT_SIZE);

		{  Fl_Positioner* o = new Fl_Positioner(620,384,50,40,"tune");
		o->xbounds(0,128);
		o->ybounds(0.99,0);
		o->box(FL_BORDER_BOX);
		o->xstep(1);
		o->labelsize(_TEXT_SIZE);
		o->argument(90);
		o->selection_color(0);
		o->callback((Fl_Callback*)parmCallback);
		 Knob[i][o->argument()] = o;
	  /*Fl_Dial* o = new Fl_SteinerKnob(627, 392, 34, 34, "frequency");
		  o->labelsize(_TEXT_SIZE);o->argument(90);
		  o->callback((Fl_Callback*)parmCallback);
		  o->maximum(500); */
		}
		{ Fl_Choice* o = new Fl_Choice(680, 397, 120, 15, "waveform");
		  o->box(FL_BORDER_BOX);
		  o->down_box(FL_BORDER_BOX);
		  o->labelsize(_TEXT_SIZE);
		  o->textsize(_TEXT_SIZE);
		  o->align(FL_ALIGN_TOP_LEFT);
		  o->menu(menu_wave);
		  o->argument(12);
		  o->callback((Fl_Callback*)choiceCallback);
		  auswahl[i][o->argument()]=o;
		} 
		{ Fl_Value_Input* o = new Fl_Value_Input(680, 415, 38, 15, "Hz"); // frequency display for modulation oscillator
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(_TEXT_SIZE);
		  o->align(FL_ALIGN_RIGHT);
		  o->maximum(10000);
		  o->step(0.001);
		  o->textsize(_TEXT_SIZE);
		  o->callback((Fl_Callback*)tuneCallback);
		  o->argument(90);
		  miniDisplay[i][10]=o;
		}
		{ Fl_Value_Input* o = new Fl_Value_Input(740, 415, 38, 15, "BPM"); // BPM display for modulation oscillator
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(_TEXT_SIZE);
		  o->align(FL_ALIGN_RIGHT);
		  o->maximum(10000);
		  o->step(0.001);
		  o->textsize(_TEXT_SIZE);
		  o->callback((Fl_Callback*)BPMtuneCallback);
		  o->argument(90);
		  miniDisplay[i][11]=o;
		}
		o->end();
	  }
	  o->end();
	}
//------------------------------------- AMP group
	{ Fl_Group* o = new Fl_Group(825, 17, 160, 212, "amp");
	  group[groups++]=o;
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	  o->labelsize(_TEXT_SIZE);
	  o->labelcolor(FL_BACKGROUND2_COLOR);
	  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);
	/*
	{ Fl_Box* d = new Fl_Box(825, 210, 160, 22, "amp");
	  d->labelsize(_TEXT_SIZE);
	  d->labelcolor(FL_BACKGROUND2_COLOR);
	}
	*/
	// amplitude envelope
	{ Fl_Dial* o = new Fl_Dial(844, 83, 25, 25, "A");
		o->labelsize(_TEXT_SIZE);o->argument(102); 
		o->minimum(EG_rate_min);
		o->maximum(EG_rate_max);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(874, 83, 25, 25, "D");
		o->labelsize(_TEXT_SIZE);o->argument(103); 
		o->minimum(EG_rate_min);
		o->maximum(EG_rate_max);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(904, 83, 25, 25, "S");
		o->labelsize(_TEXT_SIZE);o->argument(104);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(934, 83, 25, 25, "R");
		o->labelsize(_TEXT_SIZE);o->argument(105); 
		o->minimum(EG_rate_min);
		o->maximum(EG_rate_max);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(934, 29, 25, 25, "amount");
		o->labelsize(_TEXT_SIZE);
		o->argument(100);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(844, 35, 85, 15, "amp modulator");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(13);
		o->callback((Fl_Callback*)choiceCallback);
		auswahl[i][o->argument()]=o;
	  }
		{ Fl_Light_Button* o = new Fl_Light_Button(844, 55, 40, 15, "legato");
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(_TEXT_SIZE);
		  o->argument(139);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[i][o->argument()] = o;
		}

	  /*
	  { Fl_Counter* o = new Fl_Counter(844, 151, 115, 14, "sound");
		o->type(1);
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->minimum(0);
		o->maximum(7);
		o->step(1);
		o->value(0);
		o->textsize(_TEXT_SIZE);
	  //  o->callback((Fl_Callback*)voiceparmCallback,soundchoice[0]);
	  }
	  { Fl_Counter* o = new Fl_Counter(844, 181, 115, 14, "midichannel");
		o->type(1);
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->minimum(1);
		o->maximum(16);
		o->step(1);
		o->value(1);
		o->textsize(_TEXT_SIZE);
	  }*/
	  { Fl_Dial* o = new Fl_Dial(844, 120, 25, 25, "id vol");
		o->labelsize(_TEXT_SIZE); 
		o->argument(101);
		o->minimum(0);
		o->maximum(2);
		o->color(fl_rgb_color(190,160,255));
		o->value(0);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(874, 120, 25, 25, "aux 1");
		o->labelsize(_TEXT_SIZE); 
		o->argument(108);
		o->minimum(0);
		o->maximum(2);
		o->color(fl_rgb_color(140,140,255));
		o->value(0);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(904, 120, 25, 25, "aux 2");
		o->labelsize(_TEXT_SIZE); 
		o->argument(109);
		o->minimum(0);
		o->color(fl_rgb_color(140,140,255));
		o->maximum(2);
		o->value(0);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(934, 120, 25, 25, "mix vol");
		o->labelsize(_TEXT_SIZE); 
		o->argument(106);
		o->minimum(0);
		o->maximum(2);
		o->color(fl_rgb_color(170,140,255));
		o->value(1);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Slider* o = new Fl_Slider(864, 160, 80, 10, "mix pan");
		o->labelsize(_TEXT_SIZE); 
		o->box(FL_BORDER_BOX);
		o->argument(107);
		o->minimum(0);
		o->maximum(1);
		o->color(fl_rgb_color(170,140,255));
		o->value(0.5);
		o->type(FL_HORIZONTAL);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* j = new Fl_Choice(844, 190, 85, 15, "pan modulator");
		j->box(FL_BORDER_BOX);
		j->down_box(FL_BORDER_BOX);
		j->labelsize(_TEXT_SIZE);
		j->textsize(_TEXT_SIZE);
		j->align(FL_ALIGN_TOP_LEFT);
		j->argument(16);
		auswahl[i][j->argument()] = j;
		j->callback((Fl_Callback*)choiceCallback);
		j->menu(menu_amod);
	  }
	  { Fl_Dial* o = new Fl_Dial(934, 185, 25, 25, "amount");
		o->labelsize(_TEXT_SIZE);
		o->argument(122);
		o->minimum(-1);
		o->maximum(1);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  o->end();
	}
// ------------------------------ Delay group
	{ Fl_Group* o = new Fl_Group(825, 238, 160, 149, "delay");
	  group[groups++]=o;
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	  o->labelsize(_TEXT_SIZE);
	  o->labelcolor(FL_BACKGROUND2_COLOR);
	  o->align(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE);

	/*  
	{ Fl_Box* d = new Fl_Box(825, 307, 160, 135, "delay");
	  d->labelsize(_TEXT_SIZE);
	  d->labelcolor(FL_BACKGROUND2_COLOR);
	}
	*/
	  { Fl_Choice* o = new Fl_Choice(844, 265, 85, 15, "time modulator 1");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(14);
		o->callback((Fl_Callback*)choiceCallback);
		auswahl[i][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(934, 260, 25, 25, "amount");
		o->labelsize(_TEXT_SIZE);
		o->argument(110);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
		{ Fl_Light_Button* o = new Fl_Light_Button(844, 285, 40, 15, "mult.");
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(_TEXT_SIZE);
		  o->argument(143);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[i][o->argument()] = o;
		}
	  { Fl_Choice* o = new Fl_Choice(844, 310, 85, 15, "time modulator 2");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(17);
		o->callback((Fl_Callback*)choiceCallback);
		auswahl[i][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(934, 305, 25, 25, "amount");
		o->labelsize(_TEXT_SIZE);
		o->argument(144);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(844, 340, 25, 25, "delay time");
		o->labelsize(_TEXT_SIZE);
		o->argument(111);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
		{ Fl_Value_Input* o = new Fl_Value_Input(879, 345, 38, 15, "BPM");// BPM display for delay time
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(_TEXT_SIZE);
		  o->align(FL_ALIGN_BOTTOM);
		  o->maximum(10000);
		  o->step(0.01);
		  o->textsize(_TEXT_SIZE);
		  o->callback((Fl_Callback*)BPMtimeCallback);
		  o->argument(111);
		  miniDisplay[i][12]=o;
		}
	  { Fl_Dial* o = new Fl_Dial(934, 340, 25, 25, "feedback");
		o->labelsize(_TEXT_SIZE);
		o->argument(112);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  o->end();
	}


// ----------------------------------------- Sounds
	  int x0=319;
	{ Fl_Group* d = new Fl_Group(x0-7, 461, 364, 48, "sound");
	  group[groups++]=d;
	  soundgroup=d;
	  d->box(FL_ROUNDED_FRAME);
	  d->color(FL_BACKGROUND2_COLOR);
	  d->labelsize(_TEXT_SIZE);
	  d->labelcolor(FL_BACKGROUND2_COLOR);
	  d->begin();
	 /* { Fl_Button* o = new Fl_Button(191, 473, 50, 19, "create bank");
		o->tooltip("create a new bank after current one");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
	  }
	  { Fl_Button* o = new Fl_Button(26, 476, 53, 14, "delete bank");
		o->tooltip("delete a whole bank of sounds!");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)1);
	  }
	  { Fl_Button* o = new Fl_Button(732, 475, 59, 14, "delete sound");
		o->tooltip("delete current sound");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)1);
	  }*/
	  { Fl_Input* o = new Fl_Input(x0+60, 471, 150, 14, "name");
		o->box(FL_BORDER_BOX);
		o->tooltip("Enter the sound name here before storing it");
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		//o->menubutton()->textsize(_TEXT_SIZE);
		//o->menubutton()->type(Fl_Menu_Button::POPUP1);
		o->align(FL_ALIGN_TOP_LEFT);
		soundchoice[i] = o;
		soundname[i] = o; // ?? global
		o->callback((Fl_Callback*)soundnameCallback,NULL);
	  }
	  { Fl_Roller* o = new Fl_Roller(x0+60, 487, 150, 14);
		o->type(FL_HORIZONTAL);
		o->tooltip("roll the list of sounds");
		o->minimum(0);
		o->maximum(511);
		o->step(1);
	//o->slider_size(0.04);
		o->box(FL_BORDER_FRAME);
		soundRoller[i]=o;
		o->callback((Fl_Callback*)soundRollerCallback, NULL);
	  }
	  { Fl_Button* o = new Fl_Button(x0+217, 466, 60, 19, "load sound");
		o->tooltip("actually load the dialed sound");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->callback((Fl_Callback*)loadsoundCallback,soundchoice[i]);
		loadsoundBtn[i]=o;
	  }
	  { Fl_Button* o = new Fl_Button(x0+217, 487, 60, 19, "store sound");
		o->tooltip("store current sound in dialed memory");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
		o->callback((Fl_Callback*)storesoundCallback,soundchoice[i]);
		storesoundBtn[i]=o; // For resize, no need for array here, any will do
	  }
	  { Fl_Button* o = new Fl_Button(x0, 471, 10, 30, "@<");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
		o->argument(-1);
		o->callback((Fl_Callback*)soundincdecCallback);
	  }
//	  { Fl_Value_Output* o = new Fl_Value_Output(x0+10, 471, 35, 30,"sound #");
	  { Fl_Int_Input* o = new Fl_Int_Input(x0+10, 471, 35, 30,"sound #");
		o->box(FL_BORDER_BOX);
		o->color(FL_BLACK);
		o->labelsize(_TEXT_SIZE);
		// o->maximum(512);
		o->align(FL_ALIGN_TOP_LEFT);
		o->textsize(16);
		o->textcolor(FL_RED);
		o->cursor_color(FL_RED);
		soundnumber[i]=o;
		o->callback((Fl_Callback*)soundnumberCallback);
	  }
	  { Fl_Button* o = new Fl_Button(x0+45, 471, 10, 30, "@>");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
		o->argument(+1);
		o->callback((Fl_Callback*)soundincdecCallback);
	  }

	  { Fl_Button* o = new Fl_Button(x0+284, 466, 60, 19, "import sound");
		o->tooltip("import single sound to dialed memory slot, you need to load it for playing");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		//o->labelcolor((Fl_Color)1);
		o->callback((Fl_Callback*)importPressed,soundchoice[i]);
		importsoundBtn[i]=o;
	  }

	  { Fl_Button* o = new Fl_Button(x0+284, 487, 60, 19, "export sound");
		o->tooltip("export sound data of dialed memory slot");
		o->box(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		//o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->callback((Fl_Callback*)exportPressed,soundchoice[i]);
		exportsoundBtn[i]=o;
	  }
	  /*{ Fl_Input_Choice* o = new Fl_Input_Choice(83, 476, 105, 14, "bank");
		o->box(FL_BORDER_FRAME);
		o->down_box(FL_BORDER_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->selection_color(FL_FOREGROUND_COLOR);
		o->labelsize(_TEXT_SIZE);
		o->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_TOP_LEFT);
	  }*/
	  d->end();
	}
	{ Fl_Dial* o = new Fl_Dial(295, 151, 25, 25, "osc1 vol");
		o->labelsize(_TEXT_SIZE);
		// o->align(FL_ALIGN_TOP);
		o->argument(14);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
	{ Fl_Dial* o = new Fl_Dial(295, 191, 25, 25, "sub vol");
		o->labelsize(_TEXT_SIZE);
		o->argument(141);
		// o->align(FL_ALIGN_TOP);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
	{ Fl_Dial* o = new Fl_Dial(295, 252, 25, 25, "osc2 vol");
		o->labelsize(_TEXT_SIZE);
		o->argument(29);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
	{ Fl_Dial* o = new Fl_Dial(295, 292, 25, 25, "ring vol");
		o->labelsize(_TEXT_SIZE);
		o->argument(138);
		// o->align(FL_ALIGN_TOP);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
	{ Fl_Dial* o = new Fl_Dial(844, 221, 25, 25, "to delay");
		o->labelsize(_TEXT_SIZE);
		o->argument(114);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
	{ Fl_Dial* o = new Fl_Dial(934, 221, 25, 25, "from delay");
		o->labelsize(_TEXT_SIZE);
		o->argument(113);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}

	{ Fl_Dial* o = new Fl_Dial(52, 221, 25, 25, "glide");
		o->labelsize(_TEXT_SIZE);
		o->argument(116);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
	{ Fl_Dial* o = new Fl_Dial(92, 221, 25, 25, "bender");
		o->tooltip("Pitch bend range (semitones)");
		o->minimum(0);
		o->maximum(12);
		o->labelsize(_TEXT_SIZE);
		o->argument(142);
		o->align(FL_ALIGN_TOP);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}

	o->end(); 
	tab[i]=o;
	} // ==================================== end single voice tab

	} // end of voice tabs loop

	// ==================================== midi config tab
	{ 
		tablabel[i]="MIDI";
		Fl_Group* o = new Fl_Group(1, 10, 995, 515, tablabel[i].c_str());
		group[groups++]=o;
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
			sprintf(voice_label[voice], "Voice %u", voice+1); // ?? trailing \0
			{ Fl_Group* d = new Fl_Group(11+60*voice, 30, 50, 240, voice_label[voice]);
				group[groups++]=d;
				d->box(FL_ROUNDED_FRAME);
				d->color(FL_BACKGROUND2_COLOR);
				d->labelsize(_TEXT_SIZE);
				d->labelcolor(FL_BACKGROUND2_COLOR);
				d->align(FL_ALIGN_TOP);
		  // Channel
		  // Can't get Fl_Int_Input to work
		  // "If step() is non-zero and integral, then the range of numbers is limited to integers"
		  // range and bounds don't seem to be enforced when typing a value like 12345
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 46, 40, 15, "Channel");
			o->box(FL_ROUNDED_BOX);
			o->align(FL_ALIGN_TOP_LEFT);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(1, 16);
			o->bounds(1, 16);
			o->step(1);
			o->value(voice);
			o->argument(127+(voice<<8)); // Special encoding for voice
			o->callback((Fl_Callback*)midiparmCallback);
			o->value(voice);
			Knob[voice][127] = o; // NOT argument!!
		  }
		  // Note min
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 86, 40, 15, "Note min");
			o->box(FL_ROUNDED_BOX);
			o->align(FL_ALIGN_TOP_LEFT);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(0, 127);
			o->step(1);
			o->argument(128+(voice<<8)); // Special encoding for voice
			o->callback((Fl_Callback*)midiparmCallback);
			Knob[voice][128] = o; // NOT argument!!
		  }
		  // Note max
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 126, 40, 15, "Note max");
			o->box(FL_ROUNDED_BOX);
			o->align(FL_ALIGN_TOP_LEFT);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(0, 127);
			o->step(1);
			o->value(127);
			o->argument(129+(voice<<8)); // Special encoding for voice
			o->callback((Fl_Callback*)midiparmCallback);
			Knob[voice][129] = o; // NOT argument!!
		  }
		  // Transpose
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 166, 40, 14, "Transpose");
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
			Knob[voice][130] = o; // NOT argument!!
		  }
		  // Test note
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 206, 40, 14, "Test note");
			o->box(FL_ROUNDED_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(0,127);
			o->align(FL_ALIGN_TOP_LEFT);
			o->step(1);
			o->value(69);
			// o->callback((Fl_Callback*)parmCallback);
			o->argument(125);
			Knob[voice][o->argument()] = o;
		  }
		  // Test velocity
		  { Fl_Value_Input* o = new Fl_Value_Input(16+60*voice, 246, 40, 14, "Test vel.");
			o->box(FL_ROUNDED_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(0, 127);
			o->align(FL_ALIGN_TOP_LEFT);
			o->step(1);
			o->value(80);
			// o->callback((Fl_Callback*)parmCallback);
			o->argument(126);
			Knob[voice][o->argument()] = o;
		  }
		d->end();
			}

		} // End of for voice

		// Note detune, common to all voices
		const char *notenames[12]={"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
		{
			int y=310;
			Fl_Group* d = new Fl_Group(11, y, 471, 45, "Detune");
			group[groups++]=d;
			d->box(FL_ROUNDED_FRAME);
			d->color(FL_BACKGROUND2_COLOR);
			d->labelsize(_TEXT_SIZE);
			d->labelcolor(FL_BACKGROUND2_COLOR);
			d->begin();
			for(int notenum=0, x=0; notenum<12; notenum++){
				{ Fl_Value_Input* o = new Fl_Value_Input(46+60*x, y+26, 40, 14, notenames[notenum]);
					o->box(FL_ROUNDED_BOX);
					o->labelsize(_TEXT_SIZE);
					o->textsize(_TEXT_SIZE);
					o->range(-0.5, 0.5);
					o->align(FL_ALIGN_LEFT);
					o->step(.01);
					o->value(0);
					o->callback((Fl_Callback*)multiparmCallback);
					o->argument(notenum);
					multiparm[notenum]=o;
				}
				if(notenum<10 && notenum!=4 && strlen(notenames[notenum])==1){
					// We need a sharp version of this note
					notenum++;
					{
						Fl_Value_Input* o = new Fl_Value_Input(76+60*x, y+6, 40, 14, notenames[notenum]);
						o->box(FL_ROUNDED_BOX);
						o->labelsize(_TEXT_SIZE);
						o->textsize(_TEXT_SIZE);
						o->range(-0.5, 0.5);
						o->align(FL_ALIGN_LEFT);
						o->step(.01);
						o->value(0);
						o->callback((Fl_Callback*)multiparmCallback);
						o->argument(notenum);
						multiparm[notenum]=o;
					}
				}
				x++;
			}
			d->end();
		}
		{ Fl_Value_Input* o = new Fl_Value_Input(76, 280, 40, 14, "Master tune");
			o->box(FL_ROUNDED_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->range(-0.5, 0.5);
			o->align(FL_ALIGN_LEFT);
			o->step(.01);
			o->value(0);
			o->callback((Fl_Callback*)multiparmCallback);
			o->argument(12);
			multiparm[12]=o;
		}

		o->end(); 
		tab[i]=o;
		i++;
	} // ==================================== end midi config tab

	// ==================================== about tab 
	{ 
		tablabel[i]="about";
		Fl_Group* o = new Fl_Group(1, 10, 995, 515, tablabel[i].c_str());
		group[groups++]=o;
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
		  Fl_Help_View* o=new Fl_Help_View(200, 50, 600, 300, "About Minicomputer");
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(12);
		  o->color((Fl_Color)_BGCOLOR);
		  //o->textcolor(FL_BACKGROUND2_COLOR); 
		  o->textfont(FL_HELVETICA_BOLD );
		  o->labelcolor(FL_BACKGROUND2_COLOR);
		  const char version[] = _VERSION;
		  const char *about="<html><body>"
			  "<i><center>version %s</center></i><br>"
			  "<p><br>a standalone industrial grade softwaresynthesizer for Linux<br>"
			  "<p><br>developed by Malte Steiner 2007-2009"
			  "<p>distributed as free open source software under GPL3 licence<br>"
			  "<p>additional bugs by Marc Périlleux 2018"
			  "<p>OSC currently using ports %s and %s"
			  "<p>contact:<br>"
			  "<center>steiner@block4.com"
			  "<br>http://www.block4.com"
			  "<br>http://minicomputer.sourceforge.net"
			  "</center>"
			  "</body></html>";
		  char *Textausgabe;
		  Textausgabe=(char *)malloc(strlen(about)+strlen(version)+strlen(oport)+strlen(oport2));
		  sprintf(Textausgabe, about, version, oport, oport2);
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
	{ Fl_Box* o = new Fl_Box(_INIT_WIDTH-_LOGO_WIDTH1-5, _INIT_HEIGHT-_LOGO_HEIGHT1-_LOGO_HEIGHT2-10, _LOGO_WIDTH1, _LOGO_HEIGHT1);
		o->image(image_miniMini1);
		logo1=o;
	}
	{ Fl_Box* o = new Fl_Box(_INIT_WIDTH-_LOGO_WIDTH2-5, _INIT_HEIGHT-_LOGO_HEIGHT2-5, _LOGO_WIDTH2, _LOGO_HEIGHT2);
		o->image(image_miniMini2);
		logo2=o;
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
// ----------------------------------------- Multis
// This code must be outside of the tabs loop!
// It impacts resizing?
		int x0=12;
		{ Fl_Group* d = new Fl_Group(x0-7, 461, 300, 48, "multi");
			group[groups++]=d;
			multigroup=d;
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
			multidecBtn=o;
		  }
//		  { Fl_Value_Output* o = new Fl_Value_Output(x0+10, 471, 35, 30,"multi #");
		  { Fl_Int_Input* o = new Fl_Int_Input(x0+10, 471, 35, 30, "multi #");
			o->box(FL_BORDER_BOX);
			o->color(FL_BLACK);
			o->labelsize(_TEXT_SIZE);
			// o->maximum(127);
			o->align(FL_ALIGN_TOP_LEFT);
			o->textsize(16);
			o->textcolor(FL_RED);
			o->cursor_color(FL_RED);
			o->callback((Fl_Callback*)multinumberCallback);
			multinumber=o;
		  }
		  { Fl_Button* o = new Fl_Button(x0+45, 471, 10, 30, "@>");
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
			o->argument(1);
			o->callback((Fl_Callback*)multiincdecCallback);
			multiincBtn=o;
		  }
		  { Fl_Input* o = new Fl_Input(x0+60, 471, 150, 14, "name");
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			o->textsize(_TEXT_SIZE);
			o->align(FL_ALIGN_TOP_LEFT);
			o->tooltip("Enter the multi name here before storing it");
			o->callback((Fl_Callback*)multinameCallback);
			// multichoice = o;
			multiname = o; // global ??
		  } 
		  // roller for the multis:
		  { Fl_Roller* o = new Fl_Roller(x0+60, 487, 150, 14);
			o->type(FL_HORIZONTAL);
			o->tooltip("roll the list of multis, press load button for loading or save button for storing");
			o->minimum(0);
			o->maximum(127);
			o->value(0);
			o->step(1);
			o->box(FL_BORDER_FRAME);
			o->callback((Fl_Callback*)multiRollerCallback,NULL);
			multiRoller=o;
		  }
		  { Fl_Button* o = new Fl_Button(x0+217, 465, 60, 19, "load multi");
			o->tooltip("load current multi");
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
			o->callback((Fl_Callback*)loadmultiCallback, multiname); // multichoice
			loadmulti = o;
		  }
		  { Fl_Button* o = new Fl_Button(x0+217, 485, 60, 19, "store multi");
			o->tooltip("overwrite this multi");
			o->box(FL_BORDER_BOX);
			o->labelsize(_TEXT_SIZE);
			o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
			o->callback((Fl_Callback*)storemultiCallback, multiname); // multichoice
			storemulti = o;
		  }
			d->end();
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
	{ Fl_Value_Input* o = new Fl_Value_Input(895, 390, 80, 20, "current parameter");
		o->box(FL_ROUNDED_BOX);
		o->labelsize(12);
		o->textsize(12);
		// o->menubutton()->textsize(_TEXT_SIZE);
		o->align(FL_ALIGN_LEFT);
		o->step(0.0001);
		o->callback((Fl_Callback*)finetuneCallback);
		paramon = o;
	}

	{ Fl_Toggle_Button* o = new Fl_Toggle_Button(679, 466, 60, 19, "Audition");
		o->tooltip("Hear the currently loaded sound");
		// These borders won't prevent look change on focus
		o->box(FL_BORDER_BOX);
		// o->downbox(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->color(FL_LIGHT1, FL_YELLOW);
		o->callback((Fl_Callback*)do_audition);
		auditionBtn = o;
	}
	{ Fl_Toggle_Button* o = new Fl_Toggle_Button(679, 487, 60, 19, "Compare");
		o->tooltip("Compare edited sound to original");
		// These borders won't prevent look change on focus
		o->box(FL_BORDER_BOX);
		// o->downbox(FL_BORDER_BOX);
		o->labelsize(_TEXT_SIZE);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->color(FL_LIGHT1, FL_YELLOW);
		o->callback((Fl_Callback*)do_compare);
		compareBtn = o;
	}
	{ Fl_Button* o = new Fl_Button(741, 466, 60, 40, "PANIC");
		o->tooltip("Instantly stop all voices (shortcut Esc)");
		o->box(FL_BORDER_BOX);
		// o->labelsize(_TEXT_SIZE);
		o->color(FL_RED);
		// o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->callback((Fl_Callback*)all_off);
		panicBtn = o;
	}

	// parameters here would be common to all tabs

	o->end(); // End of main window
	o->size_range(o->w(), o->h());
	o->resizable(o);
	// too early, multi not loaded, loadmultiCallback segfaults
	// multichoice->value(Speicher.multis[1].name);
	// multiRoller->value(0);
	// multinumber->value(1);
	// loadmultiCallback(NULL, NULL);

#ifdef _DEBUG
	// Dump parm list (from voice 0, same for others)
	for(int parmnum=0; parmnum<_PARACOUNT; parmnum++){
		if(Knob[0][parmnum])
			printf("%3.3u : %s\n", parmnum, Knob[0][parmnum]->label());
			/* Parent Fl_Group has no label
			if(Knob[0][parmnum]->parent())
				printf("    %s\n", Knob[0][parmnum]->parent()->label());
			*/
		else
			printf("%3.3u : not defined\n", parmnum);
	}
	for(int choicenum=0; choicenum<_CHOICECOUNT; choicenum++){
		if(auswahl[0][choicenum])
			printf("#%2.2u : %s\n", choicenum, auswahl[0][choicenum]->label());
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
Fenster::Fenster(int w, int h, const char* t): Fl_Double_Window(w,h,t)
{
}
/**
 * constructor
 * calling straight the super class constructor
 */
Fenster::Fenster(int w, int h): Fl_Double_Window(w,h)
{
	current_text_size = _TEXT_SIZE;
}
/**
 * using the destructor to shutdown synthcore
 */
Fenster::~Fenster()
{
	printf("gui quit\n");
	fflush(stdout);
	if (transmit) lo_send(t, "/Minicomputer/close", "i",1);
	//~Fl_Double_Window();
}

/**
 * overload the resize method
 * @param event
 * @return 1 when all is right
 */
void Fenster::resize (int x, int y, int w, int h)
{
	Fl_Double_Window::resize(x, y, w, h);
	// Logo - re-align with bottom
	logo1->resize(w-_LOGO_WIDTH1-5, h-_LOGO_HEIGHT1-_LOGO_HEIGHT2-10, _LOGO_WIDTH1, _LOGO_HEIGHT1);
	logo2->resize(w-_LOGO_WIDTH2-5, h-_LOGO_HEIGHT2-5, _LOGO_WIDTH2, _LOGO_HEIGHT2);
	// Sound indicators - re-align with logo
	for (int j=0; j<_MULTITEMP; j++){
		sounding[j]->resize(logo1->x()+5+6*(j/4), logo1->y()+31+5*(j%4)+(j/4), 4, 4);
		voicedisplay[j]->resize(w-_LOGO_WIDTH1-50, h-_LOGO_HEIGHT2-50, 40, 40);
	}
	// Multi group
	multigroup->position(multigroup->x(), soundgroup->y());
	multiname->resize(multiname->x(), soundname[0]->y(), multiname->w(), multiname->h());
	multiRoller->position(multiRoller->x(), soundRoller[0]->y());
	multinumber->position(multinumber->x(), soundnumber[0]->y());
	loadmulti->position(loadmulti->x(), loadsoundBtn[0]->y());
	storemulti->position(storemulti->x(), storesoundBtn[0]->y());
	multidecBtn->position(multidecBtn->x(), soundnumber[0]->y());
	multiincBtn->position(multiincBtn->x(), soundnumber[0]->y());

	auditionBtn->position(auditionBtn->x(), loadsoundBtn[0]->y());
	compareBtn->position(compareBtn->x(), storesoundBtn[0]->y());
	panicBtn->position(panicBtn->x(), loadsoundBtn[0]->y());

	// Set text size for all widgets
	/* Would need to scan recursively
	 * How do we know whether we have a widget with text/label?
	Fl_Group *g = this->as_group();
	if (g){
		printf ("This group has %d children\n",g->children());
		for(int j=0; j<g->children(); j++){
			Fl_Group *g2=g->child(j)->as_group();
			if(g2==0){ // Leaf
				// g2->color(FL_GREEN); // segfault
				printf("Child %u %u\n", j, g->child(j)->type()); // Type is 0 or 1
			}
			// printf("Child %u %s\n", g->child(j)->as_group(), g->child(j)->type());
		}
	}
	*/
	float minscale=min((float)this->w()/_INIT_WIDTH, (float)this->h()/_INIT_HEIGHT);
	// printf("resize: scale width=%u/%u, height=%u/%u\n", this->w(), _INIT_WIDTH, this->h(), _INIT_HEIGHT);
	int new_text_size = _TEXT_SIZE*minscale;
	// printf("resize: scale=%f, text size=%u\n", minscale, new_text_size);
	if(new_text_size!=current_text_size){
		// printf("groups %u\n", groups);
		current_text_size=new_text_size;
		paramon->textsize(12*minscale);
		paramon->labelsize(12*minscale);
		compareBtn->labelsize(new_text_size);
		auditionBtn->labelsize(new_text_size);
		panicBtn->labelsize(new_text_size);
		loadmulti->labelsize(new_text_size);
		storemulti->labelsize(new_text_size);
		multinumber->textsize(16*minscale);
		multinumber->labelsize(new_text_size);
		multiname->textsize(new_text_size);
		multiname->labelsize(new_text_size);
		
		for(unsigned int i=0; i<sizeof(menu_amod)/sizeof(menu_amod[0]);i++){
			menu_amod[i].labelsize(new_text_size);
		}
		for(unsigned int i=0; i<sizeof(menu_fmod)/sizeof(menu_fmod[0]);i++){
			menu_fmod[i].labelsize(new_text_size);
		}
		for(unsigned int i=0; i<sizeof(menu_wave)/sizeof(menu_wave[0]);i++){
			menu_wave[i].labelsize(new_text_size);
		}

		for(int i=0; i<_MULTIPARMS; i++){
			multiparm[i]->labelsize(new_text_size);
			multiparm[i]->textsize(new_text_size);
		}

		for(int i=0; i<groups; i++)
			group[i]->labelsize(new_text_size);

		for(int i=0; i<_MULTITEMP; i++){
			soundnumber[i]->textsize(16*minscale);
			soundnumber[i]->labelsize(new_text_size);
			soundname[i]->textsize(new_text_size);
			soundname[i]->labelsize(new_text_size);
			loadsoundBtn[i]->labelsize(new_text_size);
			storesoundBtn[i]->labelsize(new_text_size);
			clearstateBtn[i]->labelsize(new_text_size);
			importsoundBtn[i]->labelsize(new_text_size);
			exportsoundBtn[i]->labelsize(new_text_size);
			for(int j=0; j<_MINICOUNT; j++){
				miniDisplay[i][j]->textsize(new_text_size);
				miniDisplay[i][j]->labelsize(new_text_size);
			}
			for(int j=0; j<_CHOICECOUNT; j++){
				auswahl[i][j]->textsize(new_text_size); // No effect??
				// auswahl[i][j]->redraw(); // Doesn't help
				// auswahl[i][j]->menu()->labelsize(new_text_size);
				// auswahl[i][j]->selection_color(FL_RED); // When selected only
				auswahl[i][j]->labelsize(new_text_size);
			}
			for(int j=0; j<_PARACOUNT; j++){
				if(Knob[i][j])
					Knob[i][j]->labelsize(new_text_size);
			}
			for(int j=125; j<=130; j++)
				((Fl_Value_Input*)Knob[i][j])->textsize(new_text_size);
		}
	}
}
/**
 * overload the eventhandler for some custom shortcuts
 * @param event
 * @return 1 when all is right
 */
int Fenster::handle (int event)
{
	switch (event)
		{
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
				case FL_Escape:
			all_off(NULL, NULL);
		break;
				// case 32: // Space bar - Doesn't work
				case FL_Insert:
			auditionBtn->value(!auditionBtn->value());
			do_audition(auditionBtn, NULL);
		break;
		
				}// end of switch
		}// end of if
				return 1;

		default:
				// pass other events to the base class...

		return Fl_Double_Window::handle(event);
		}
}
/*
void close_cb( Fl_Widget* o, void*) {
	printf("stop editor");
	fflush(stdout);
   exit(0);
}*/
