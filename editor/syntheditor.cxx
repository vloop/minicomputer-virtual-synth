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
static Fl_RGB_Image image_miniMini(idata_miniMini, _LOGO_WIDTH, _LOGO_HEIGHT, 3, 0);
 Fl_Widget* Knob[_MULTITEMP][_PARACOUNT];
 Fl_Choice* auswahl[_MULTITEMP][_CHOICECOUNT];
 Fl_Value_Input* miniDisplay[_MULTITEMP][13];
 Fl_Widget* tab[9];
 Fl_Input* schoice[_MULTITEMP];
 Fl_Roller* Rollers[_MULTITEMP];
 Fl_Roller* multiRoller;
 Fl_Tabs* tabs;
 Fl_Button* lm,*sm;
 Fl_Toggle_Button* audition;
 int audition_state[_MULTITEMP];
 Fl_Value_Input* paramon;
 Fl_Value_Input* note_number;
 Fl_Value_Input* note_velocity;
 Fl_Value_Output *memDisplay[_MULTITEMP];
 Fl_Value_Output *multiDisplay;
 Fl_Input* Multichoice;
 Fl_Box* sounding[_MULTITEMP];
 
 int currentParameter=0;

 unsigned int currentsound=0, currentmulti=0;
// unsigned int multi[128][8];
// string multiname[128];
 bool transmit;
 bool sense=false; // Core not sensed yet

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

static void choiceCallback(Fl_Widget* o, void*)
{
	// printf("choiceCallback %u %u %u\n",currentsound,((Fl_Choice*)o)->argument(),((Fl_Choice*)o)->value());
	if (transmit) lo_send(t, "/Minicomputer/choice", "iii",currentsound,((Fl_Choice*)o)->argument(),((Fl_Choice*)o)->value());
}
static void do_audition(Fl_Widget* o, void*)
{
	if (transmit){
		int note=((Fl_Valuator*)Knob[currentsound][125])->value();
		int velocity=((Fl_Valuator*)Knob[currentsound][126])->value();
		audition_state[currentsound]=((Fl_Toggle_Button*)o)->value();
		if ( ((Fl_Toggle_Button*)o)->value()){
			lo_send(t, "/Minicomputer/midi", "iiii", 0, 0x90+currentsound, note, velocity);
		}else{
			lo_send(t, "/Minicomputer/midi", "iiii", 0, 0x80+currentsound, note, velocity);
		}
	}
}
static void all_off(Fl_Widget* o, void*)
{
	int i;
	// make sure audition is off
	audition->clear();
	if (transmit) for (i=0; i<_MULTITEMP; i++){
	  lo_send(t, "/Minicomputer/midi", "iiii", 0, 0xB0+i, 120, 0);
	  audition_state[currentsound]=((Fl_Toggle_Button*)audition)->value();
	}
}

int EG_changed[_MULTITEMP];
int EG_stage[_MULTITEMP][_EGCOUNT];
int EG_parm_num[7]={102, 60, 65, 70, 75, 80, 85}; // First parameter for each EG (attack)
// ADSR controls are required to be numbered sequentially, i.e. if A is 60, D is 61...

int EG_draw(int voice, int EGnum, int stage){
	if ((voice < _MULTITEMP) && (EGnum < _EGCOUNT ))
	{
		// Schedule redraw for next run of timer_handler
		EG_changed[voice]=1;
		EG_stage[voice][EGnum]=stage;
		return 0;
	}
	return 1;
}

static void tabCallback(Fl_Widget* o, void* );
void EG_draw_all(){
	// Refresh EG label lights
	// Called by timer_handler
	int voice, EGnum, stage;
	// Fl::lock(); // Doesn't help
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
	// Fl::check(); // Doesn't help
	// Fl::awake(); // Doesn't help
	// Fl::unlock(); // Doesn't help

}

/* // not good:
static void changemulti(Fl_Widget* o, void*)
{
	if (Multichoice != NULL)
	{
		int t = Multichoice->menubutton()->value();
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
	if (e==tab[8]) // The "About" tab
	{
		if (multiDisplay != NULL)
			multiDisplay->hide();
		else
			printf("there seems to be something wrong with multidisplay widget");
		
		if (multiRoller != NULL)
			multiRoller->hide();
		else
			printf("there seems to be something wrong with multiroller widget");

		if (Multichoice != NULL)
			Multichoice->hide();
		else
			printf("there seems to be something wrong with multichoice widget");
		if (sm != NULL)
			sm->hide();
		else
			printf("there seems to be something wrong with storemultibutton widget");

		if (lm != NULL)
			lm->hide();
		else
			printf("there seems to be something wrong with loadmultibutton widget");

		if (paramon != NULL)
			paramon->hide();
		else
			printf("there seems to be something wrong with paramon widget");

		if (audition != NULL)
			audition->hide();
		else
			printf("there seems to be something wrong with audition widget");

		if (note_number != NULL)
			note_number->hide();
		else
			printf("there seems to be something wrong with note_number widget");

		if (note_velocity != NULL)
			note_velocity->hide();
		else
			printf("there seems to be something wrong with note_velocity widget");
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

		if (multiDisplay != NULL)
			multiDisplay->show();
		else
			printf("there seems to be something wrong with multiDisplay widget");

		if (multiRoller != NULL)
			multiRoller->show();
		else
			printf("there seems to be something wrong with multiroller widget");
		
		if (Multichoice != NULL)
			Multichoice->show();
		else
			printf("there seems to be something wrong with multichoice widget");
		if (sm != NULL)
			sm->show();
		else
			printf("there seems to be something wrong with storemultibutton widget");

		if (lm != NULL)
			lm->show();
		else
			printf("there seems to be something wrong with loadmultibutton widget");

		if (paramon != NULL)
			paramon->show();
		else
			printf("there seems to be something wrong with paramon widget");

		if (audition != NULL){
			audition->show();
			((Fl_Toggle_Button*)audition)->value(audition_state[currentsound]);
		}else
			printf("there seems to be something wrong with audition widget");

		if (note_number != NULL)
			note_number->show();
		else
			printf("there seems to be something wrong with note_number widget");

		if (note_velocity != NULL)
			note_velocity->show();
		else
			printf("there seems to be something wrong with note_velocity widget");

#ifdef _DEBUG
	printf("sound %i\n", currentsound );
	fflush(stdout);
#endif
 	} // end of else

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

		// show only parameter on finetune when its not a frequency
		switch (currentParameter)
		{
			case 1:
			case 3:
			case 16:
			case 18:
			case 30:
			case 33:
			case 40:
			case 43:
			case 50:
			case 53:
			case 90:
			case 123: // BPM
			case 125: // Audition note number
			case 126: // Audition note velocity
			 break; // do nothing
			default: 
				paramon->value(((Fl_Valuator*)o)->value());
			break;
		}

// now actually process parameter
switch (currentParameter)
{
	case 256:
	{
		lo_send(t, "/Minicomputer", "iif",currentsound,0,0);
		break;
	}
	case 1:
	{
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
		miniDisplay[currentsound][0]->value( ((Fl_Valuator* )Knob[currentsound][1])->value() );
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
#endif
	break;
	}
	case 2:
	{
		if (((Fl_Light_Button *)o)->value()==0)
		{
			Knob[currentsound][1]->deactivate();
			Knob[currentsound][3]->activate();
			miniDisplay[currentsound][1]->activate();
			miniDisplay[currentsound][0]->deactivate();
			if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),0.f);
		}
		else
		{
			Knob[currentsound][3]->deactivate();
			Knob[currentsound][1]->activate();
			miniDisplay[currentsound][0]->activate();
			miniDisplay[currentsound][1]->deactivate();
			if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),1.f);
		}
#ifdef _DEBUG
		printf("%li : %i     \r", ((Fl_Light_Button*)o)->argument(),((Fl_Light_Button*)o)->value());
#endif		
	break;	
	}
	case 17:
	{
		if (((Fl_Light_Button *)o)->value()==0)
		{
			Knob[currentsound][16]->deactivate();
			Knob[currentsound][18]->activate();
			miniDisplay[currentsound][3]->activate();
			miniDisplay[currentsound][2]->deactivate();
			if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),0.f);
		}
		else
		{
			Knob[currentsound][18]->deactivate();
			Knob[currentsound][16]->activate();
			miniDisplay[currentsound][2]->activate();
			miniDisplay[currentsound][3]->deactivate();
			if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),1.f);
		}
#ifdef _DEBUG
		printf("%li : %i     \r", ((Fl_Light_Button*)o)->argument(),((Fl_Light_Button*)o)->value());
#endif		
	break;	
	}
	case 3:
	{
		float f = ((Fl_Positioner*)o)->xvalue() + ((Fl_Positioner*)o)->yvalue();
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
		miniDisplay[currentsound][1]->value( f);
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Positioner*)o)->argument(),f);
#endif
	break;
	}
	case 16:
	{
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
		miniDisplay[currentsound][2]->value( ((Fl_Valuator*)o)->value() );
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
#endif
		break;
	}
	case 18:
	{ 
		float f = ((Fl_Positioner*)o)->xvalue() + ((Fl_Positioner*)o)->yvalue();
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
		miniDisplay[currentsound][3]->value(f);
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Positioner*)o)->argument(),f);
#endif
	break;
	}
	case 4: // boost button
	case 15:
	{
		if (((Fl_Light_Button *)o)->value()==0)
		{
			if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),1.f);
		}
		else
		{
			if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),100.f);
		}
#ifdef _DEBUG
		printf("%li : %i     \r", ((Fl_Light_Button*)o)->argument(),((Fl_Light_Button*)o)->value());
#endif
	break;	
	}
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
	case 119:
	case 120:
	{
		if (((Fl_Light_Button *)o)->value()==0)
		{
			if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),0.f);
		}
		else
		{
			if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Light_Button*)o)->argument(),1.f);
		}
#ifdef _DEBUG
		fprintf(stderr, "%li : %i     \r", ((Fl_Light_Button*)o)->argument(),((Fl_Light_Button*)o)->value());
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
		float tr=(((Fl_Valuator*)o)->value());///200.f;//exp(((Fl_Valuator*)o)->value())/200.f;
		tr*= tr*tr/2.f;// tr * tr*20.f;//48000.0f;//trtr*tr/2;
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Valuator*)o)->argument(),tr);
#ifdef _DEBUG
		printf("eg %li : %g     \r", ((Fl_Valuator*)o)->argument(),tr);
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
		if (transmit)lo_send(t, "/Minicomputer", "iif", currentsound, ((Fl_Valuator*)o)->argument(), tr);
#ifdef _DEBUG
		printf("eg %li : %f --> %f \r", ((Fl_Valuator*)o)->argument(), ((Fl_Valuator*)o)->value(), tr);
#endif
		break;
	}	
	
	//************************************ filter cuts *****************************
	case 30:{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Positioner*)o)->argument(),f);
#endif
		miniDisplay[currentsound][4]->value(f);
		break;
	}
	case 33:{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Positioner*)o)->argument(),f);
#endif		
		miniDisplay[currentsound][5]->value(f);
		break;
	}
	case 40:{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Positioner*)o)->argument(),f);
#endif
		miniDisplay[currentsound][6]->value(f);
		break;
	}
	case 43:{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Positioner*)o)->argument(),f);
#endif
		miniDisplay[currentsound][7]->value(f);
		break;
	}
	case 50:{
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
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,Argument,f);
#ifdef _DEBUG
		printf("%i : %g     \r", Argument,f);
		fflush(stdout);
#endif
		miniDisplay[currentsound][8]->value(f);
		break;
	}
	case 53:{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Positioner*)o)->argument(),f);
#endif
		miniDisplay[currentsound][9]->value(f);
		break;
	}
	case 90: // OSC 3 tune
	{	float f=((Fl_Positioner*)o)->xvalue()+((Fl_Positioner*)o)->yvalue();
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Positioner*)o)->argument(),f);
#endif
		miniDisplay[currentsound][10]->value(f);
		miniDisplay[currentsound][11]->value(round(f*6000)/100); // BPM
		break;
	}
	case 111: // Delay time
	{	float f=((Fl_Valuator*)o)->value();
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Positioner*)o)->argument(),f);
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Valuator*)o)->argument(),f);
#endif
		if (f!=0) f=round(6000/f)/100;
		miniDisplay[currentsound][12]->value(f); // BPM
		break;
	}
	default:
	{
		if (transmit)lo_send(t, "/Minicomputer", "iif",currentsound,((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
#ifdef _DEBUG
		printf("%li : %g     \r", ((Fl_Valuator*)o)->argument(),((Fl_Valuator*)o)->value());
#endif		
		break;
	}
	

}
	
#ifdef _DEBUG
fflush(stdout);
#endif		
} // end of o != NULL

Fl::awake();
Fl::unlock();
} // end of parmCallback

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
static void finetune(Fl_Widget* o, void*)
{
	if (currentParameter<_PARACOUNT)// range check
	{
		switch (currentParameter)
		{
			case 1:
			case 3:
			case 16:
			case 18:
			case 30:
			case 33:
			case 40:
			case 43:
			case 50:
			case 53:
			case 90:
			 break; // do nothin
			default: 
			Fl::lock();
				((Fl_Valuator* )Knob[currentsound][currentParameter])->value(((Fl_Valuator* )o)->value());
				parmCallback(Knob[currentsound][currentParameter],NULL);
			Fl::awake();
			Fl::unlock();
			break;
		}
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
	int Faktor = ((int)(((Fl_Valuator* )o)->value()/1000)*1000);
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
static void BPMCallback(Fl_Widget* o, void*)
{
	Fl::lock();
	float f = ((Fl_Valuator* )o)->value()/60.f;
	int Faktor = (int)f;
	float Rem = f-Faktor;
	// 90 is OSC 3 tune
	int Argument = 90; // ((Fl_Valuator* )o)->argument();
	((Fl_Positioner* )Knob[currentsound][Argument])->xvalue(Faktor);
	((Fl_Positioner* )Knob[currentsound][Argument])->yvalue(Rem);
	parmCallback(Knob[currentsound][Argument],NULL);
	Fl::awake();
	Fl::unlock();
}

static void BPMCallback2(Fl_Widget* o, void*)
{
	Fl::lock();
	float f = ((Fl_Valuator* )o)->value();
	if(f!=0) f=60.f/f;
	// 111 is delay time
	int Argument = 111; // ((Fl_Valuator* )o)->argument();
	((Fl_Valuator*)Knob[currentsound][Argument])->value(f);
	parmCallback(Knob[currentsound][Argument],NULL);
	Fl::awake();
	Fl::unlock();
}

static void rollerCallback(Fl_Widget* o, void*)
{
	Fl::lock();
		int Faktor = (int)((Fl_Valuator* )o)->value();
		schoice[currentsound]->value((Speicher.getName(0,Faktor)).c_str());
		// Speicher.multis[currentmulti].sound[currentsound]);// set gui
		schoice[currentsound]->position(0);
		memDisplay[currentsound]->value(Faktor);// set gui
	Fl::awake();
	Fl::unlock();
}
/*
static void chooseCallback(Fl_Widget* o, void*)
{
//	int Faktor = (int)((Fl_Valuator* )o)->value();
	Rollers[currentsound]->value(schoice[currentsound]->menubutton()->value());// set gui
	memDisplay[currentsound]->value(schoice[currentsound]->menubutton()->value());// set gui
}*/
static void multiRollerCallback(Fl_Widget* o, void*)
{
	Fl::lock();
		int Faktor = (int)((Fl_Valuator* )o)->value();
		Multichoice->value(Speicher.multis[Faktor].name);// set gui
		Multichoice->position(0);// put cursor in the beginning, otherwise the begin of the string might be invisible
		multiDisplay->value(Faktor);
	Fl::awake();
	Fl::unlock();
}
/*
static void chooseMultiCallback(Fl_Widget* o, void*)
{
	multiRoller->value(Multichoice->menubutton()->value());// set gui
}*/
/**
 * parmCallback from the export filedialog
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
/** parmCallback when export button was pressed, shows a filedialog
 */
static void exportPressed(Fl_Widget* o, void*)
{
char warn[256];
sprintf (warn,"export %s:",schoice[currentsound]->value());
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
		
		Speicher.exportSound(fc->value(),(unsigned int)memDisplay[currentsound]->value());
		
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
sprintf (warn,"overwrite %s:",schoice[currentsound]->value());
Fl_File_Chooser *fc = new Fl_File_Chooser(".","TEXT Files (*.txt)\t",Fl_File_Chooser::SINGLE,warn);
	fc->textsize(9);
	fc->show();
	while(fc->shown()) Fl::wait(); // block until choice is done
	if ((fc->value() != NULL))
	{
	#ifdef _DEBUG
		//printf("currentsound %i,roller %f, importon %i to %i : %s\n",currentsound,Rollers[currentsound]->value(),((Fl_Input_Choice*)e)->menubutton()->value(),(int)memDisplay[currentsound]->value(),fc->value());//Speicher.multis[currentmulti].sound[currentsound]
		fflush(stdout);
	#endif
		Fl::lock();
		fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
		Fl::check();
  		
		Speicher.importSound(fc->value(),(int)memDisplay[currentsound]->value());//schoice[currentsound]->menubutton()->value());	
		// ok, now we have a new sound saved but we should update the userinterface
		schoice[currentsound]->value(Speicher.getName(0,(int)memDisplay[currentsound]->value()).c_str());
	  	/*
		int i;
		for (i = 0;i<8;++i)
	  	{
  			schoice[i]->clear();
	  	} 
  
	  	for (i=0;i<512;++i) 
  		{
			 if (i==230)
			 {
			 	printf("%s\n",Speicher.getName(0,229).c_str());
			 	printf("%s\n",Speicher.getName(0,230).c_str());
			 	printf("%s\n",Speicher.getName(0,231).c_str());
			 }
  			schoice[0]->add(Speicher.getName(0,i).c_str());
		  	schoice[1]->add(Speicher.getName(0,i).c_str());
		  	schoice[2]->add(Speicher.getName(0,i).c_str());
	  		schoice[3]->add(Speicher.getName(0,i).c_str());
		  	schoice[4]->add(Speicher.getName(0,i).c_str());
		  	schoice[5]->add(Speicher.getName(0,i).c_str());
  			schoice[6]->add(Speicher.getName(0,i).c_str());
  			schoice[7]->add(Speicher.getName(0,i).c_str());
	  	}
		*/
		fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
		Fl::check();
		Fl::awake();
		Fl::unlock();
	}

}
/** parmCallback when the storebutton is pressed
 * @param Fl_Widget the calling widget
 * @param defined by FLTK but not used
 */
static void storesound(Fl_Widget* o, void* e)
{
	Fl::lock();
	fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
	Fl::check();
	int Speicherplatz = (int) memDisplay[currentsound]->value();
#ifdef _DEBUG
	printf("choice %i\n",Speicherplatz);//((Fl_Input_Choice*)e)->menubutton()->value());
	fflush(stdout);
#endif	
	Speicher.setChoice(currentsound,Speicherplatz);//(Fl_Input_Choice*)e)->menubutton()->value());
	// clean first the name string
	sprintf(Speicher.sounds[Speicher.getChoice(currentsound)].name,"%s",((Fl_Input*)e)->value());
#ifdef _DEBUG
	printf("input choice %s\n",((Fl_Input*)e)->value());
#endif	
	//((Fl_Input_Choice*)e)->menubutton()->replace(Speicher.getChoice(currentsound),((Fl_Input_Choice*)e)->value());
	
	//Schaltbrett.soundchoice-> add(Speicher.getName(i).c_str());
	int i;
	for (i=0;i<_PARACOUNT;++i)// go through all parameters
	{
	if (Knob[currentsound][i] != NULL)
	{
		//int j=-1024;
		Speicher.sounds[Speicher.getChoice(currentsound)].parameter[i]=((Fl_Valuator*)Knob[currentsound][i])->value();
		
		switch (i)
{
	
	case 2:
	case 4: // boost button
	case 15:
	case 17:
		// the repeat buttons of the mod egs
	case 64:
	case 69:
	case 74:
	case 79:
	case 84:
	case 89:
	case 115:
	// mult. buttons
	case 117:
	case 118:
	case 119:
	case 120:
	{
		if (((Fl_Light_Button *)Knob[currentsound][i])->value()==0)
		{
			Speicher.sounds[Speicher.getChoice(currentsound)].parameter[i]=0;
		}
		else
		{
			Speicher.sounds[Speicher.getChoice(currentsound)].parameter[i]=1;
		}
		#ifdef _DEBUG
		printf("button %d = %f\n",i,Speicher.sounds[Speicher.getChoice(currentsound)].parameter[i]);
		#endif
	break;	
	}
	case 3:
		{
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[0][0]=((Fl_Positioner*)Knob[currentsound][i])->xvalue();
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[0][1]=((Fl_Positioner*)Knob[currentsound][i])->yvalue();
	break;
	}
	case 18:
			{
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[1][0]=((Fl_Positioner*)Knob[currentsound][i])->xvalue();
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[1][1]=((Fl_Positioner*)Knob[currentsound][i])->yvalue();
	break;
	}
	//************************************ filter cuts *****************************
	case 30:
			{
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[2][0]=((Fl_Positioner*)Knob[currentsound][i])->xvalue();
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[2][1]=((Fl_Positioner*)Knob[currentsound][i])->yvalue();
	break;
	}
	case 33:
			{
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[3][0]=((Fl_Positioner*)Knob[currentsound][i])->xvalue();
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[3][1]=((Fl_Positioner*)Knob[currentsound][i])->yvalue();
	break;
	}
	case 40:
			{
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[4][0]=((Fl_Positioner*)Knob[currentsound][i])->xvalue();
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[4][1]=((Fl_Positioner*)Knob[currentsound][i])->yvalue();
	break;
	}
	case 43:
			{
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[5][0]=((Fl_Positioner*)Knob[currentsound][i])->xvalue();
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[5][1]=((Fl_Positioner*)Knob[currentsound][i])->yvalue();
	break;
	}
	case 50:
			{
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[6][0]=((Fl_Positioner*)Knob[currentsound][i])->xvalue();
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[6][1]=((Fl_Positioner*)Knob[currentsound][i])->yvalue();
	break;
	}
	case 53:
			{
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[7][0]=((Fl_Positioner*)Knob[currentsound][i])->xvalue();
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[7][1]=((Fl_Positioner*)Knob[currentsound][i])->yvalue();
	break;
	}
	case 90:
	{
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[8][0]=((Fl_Positioner*)Knob[currentsound][i])->xvalue();
		Speicher.sounds[Speicher.getChoice(currentsound)].freq[8][1]=((Fl_Positioner*)Knob[currentsound][i])->yvalue();
	break;
	}
	// special treatment for the mix knobs, they are saved in the multisetting
	case 101:
	case 106:
	case 107:
	case 108:
	case 109:
	{
		// do nothing
	}
	break;
	
//	{
//		if (((Fl_Light_Button *)Knob[i])->value()==0)
//		{
//			Speicher.sounds[Speicher.getChoice()].parameter[i]=1;
//		}
//		else
//		{
//			Speicher.sounds[Speicher.getChoice()].parameter[i]=1000;
//		}
//	break;	
//	}
	/*
	case 60:
	//case 61:
	case 63:
	case 65:
	//case 66: 
	case 68:
	
	case 70:
	//case 71:
	case 73:
	case 75:
	//case 76: 
	case 78:
	
	case 80:
	//case 81:
	case 83:
	case 85:
	//case 86: 
	case 88:
	
	
	case 102:
	case 105:
	{
		float tr=(((Fl_Valuator*)o)->value());///200.f;//exp(((Fl_Valuator*)o)->value())/200.f;
		tr *= tr*tr/2.f;// tr * tr*20.f;//48000.0f;//trtr*tr/2;
		Speicher.sounds[Speicher.getChoice()].parameter[i]=tr;
		break;
	}	
	*/
	
	default:
	{
		Speicher.sounds[Speicher.getChoice(currentsound)].parameter[i]=((Fl_Valuator*)Knob[currentsound][i])->value();
		break;
	}
	

	} // end of switch
	} // end of if
	} // end of for

	for (i=0;i<_CHOICECOUNT;++i)
	{
		if (auswahl[currentsound][i] != NULL)
		{
			Speicher.sounds[Speicher.getChoice(currentsound)].choice[i]=auswahl[currentsound][i]->value();
#ifdef _DEBUG
			printf("f:%i:%i ",i,auswahl[currentsound][i]->value());
#endif
		}
	}
#ifdef _DEBUG
	printf("\n");
	fflush(stdout);
#endif
	Speicher.save();
	/*	
	// ok, now we have saved but we should update the userinterface
  	for (i = 0;i<8;++i)
  	{
  		schoice[i]->clear();
  	} 
  
  	for (i=0;i<512;++i) 
  	{
  		schoice[0]->add(Speicher.getName(0,i).c_str());
	  	schoice[1]->add(Speicher.getName(0,i).c_str());
	  	schoice[2]->add(Speicher.getName(0,i).c_str());
	  	schoice[3]->add(Speicher.getName(0,i).c_str());
	  	schoice[4]->add(Speicher.getName(0,i).c_str());
	  	schoice[5]->add(Speicher.getName(0,i).c_str());
  		schoice[6]->add(Speicher.getName(0,i).c_str());
  		schoice[7]->add(Speicher.getName(0,i).c_str());
  	}
	*/
	fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
	Fl::check();
	Fl::awake();
	Fl::unlock();
}
/**
 * recall a single sound into current tab
 * by calling each parameter's parmCallback
 * which should handle both display update
 * and OSC transmission
 */
static void recall(unsigned int preset)
{
	int i;//,j=-1024;
#ifdef _DEBUG
	printf("recall: voice %u preset %u\n", currentsound, preset);
	fflush(stdout);
#endif
	Speicher.setChoice(currentsound,preset);
	for(i=0;i<_PARACOUNT;++i)
	{
		if (Knob[currentsound][i] != NULL)
		{
#ifdef _DEBUG
			printf("i == %i \n",i);
			fflush(stdout);
#endif
		switch (i)
		{

	case 2:
	case 4: // boost button
	case 15:
	case 17:
	// the repeat buttons of the mod egs
	case 64:
	case 69:
	case 74:
	case 79:
	case 84:
	case 89:
	case 115:
	// modulation mult. buttons
	case 117:
	case 118:
	case 119:
	case 120:

	{
		#ifdef _DEBUG
		printf("handle: %d\n",i);
		#endif
		if (Speicher.sounds[Speicher.getChoice(currentsound)].parameter[i]==0.0f)
		{
			((Fl_Light_Button*)Knob[currentsound][i])->value(0);
		}
		else
		{
			((Fl_Light_Button*)Knob[currentsound][i])->value(1);
		}
		parmCallback(Knob[currentsound][i],NULL);
		
		break;	
	}
	//{
	//	((Fl_Light_Button*)Knob[i])->value((int)Speicher.sounds[Speicher.getChoice()].parameter[i]);
		
	//	break;	
	//}
	
	case 3:
			{
		((Fl_Positioner*)Knob[currentsound][i])->xvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[0][0]);
		((Fl_Positioner*)Knob[currentsound][i])->yvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[0][1]);
		parmCallback(Knob[currentsound][i],NULL);
		
	break;
	}
	case 18:
			{
		((Fl_Positioner*)Knob[currentsound][i])->xvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[1][0]);
		((Fl_Positioner*)Knob[currentsound][i])->yvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[1][1]);
		parmCallback(Knob[currentsound][i],NULL);
	break;
	}
	//************************************ filter cuts *****************************
	case 30:
			{
		((Fl_Positioner*)Knob[currentsound][i])->xvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[2][0]);
		((Fl_Positioner*)Knob[currentsound][i])->yvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[2][1]);
		parmCallback(Knob[currentsound][i],NULL);
		
	break;
	}
	case 33:
			{
		((Fl_Positioner*)Knob[currentsound][i])->xvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[3][0]);
		((Fl_Positioner*)Knob[currentsound][i])->yvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[3][1]);
		parmCallback(Knob[currentsound][i],NULL);
		
	break;
	}
	case 40:
			{
		((Fl_Positioner*)Knob[currentsound][i])->xvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[4][0]);
		((Fl_Positioner*)Knob[currentsound][i])->yvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[4][1]);
		parmCallback(Knob[currentsound][i],NULL);
	break;
	}
	case 43:
			{
		((Fl_Positioner*)Knob[currentsound][i])->xvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[5][0]);
		((Fl_Positioner*)Knob[currentsound][i])->yvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[5][1]);
		parmCallback(Knob[currentsound][i],NULL);
		
	break;
	}
	case 50:
			{
		((Fl_Positioner*)Knob[currentsound][i])->xvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[6][0]);
		((Fl_Positioner*)Knob[currentsound][i])->yvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[6][1]);
		parmCallback(Knob[currentsound][i],NULL);
						
	break;
	}
	case 53:
	{
		((Fl_Positioner*)Knob[currentsound][i])->xvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[7][0]);
		((Fl_Positioner*)Knob[currentsound][i])->yvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[7][1]);
		parmCallback(Knob[currentsound][i],NULL);
		
	break;
	}
	case 90:
	{
		((Fl_Positioner*)Knob[currentsound][i])->xvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[8][0]);
		((Fl_Positioner*)Knob[currentsound][i])->yvalue(Speicher.sounds[Speicher.getChoice(currentsound)].freq[8][1]);
		parmCallback(Knob[currentsound][i],NULL);
	break;
	}
	
	// special treatment for the mix knobs, they are saved in the multisetting
	case 101:
	case 106:
	case 107:
	case 108:
	case 109:
	{
		// do nothing
	}
	break;
	default:
	{
		((Fl_Valuator*)Knob[currentsound][i])->value(Speicher.sounds[Speicher.getChoice(currentsound)].parameter[i]);
		parmCallback(Knob[currentsound][i],NULL);
		break;
	}
	

}
		}
	}

#ifdef _DEBUG 
	printf("so weit so gut");
#endif
	for (i=0;i<_CHOICECOUNT;++i)
	{
		if (auswahl[currentsound][i] != NULL)
		{
		auswahl[currentsound][i]->value(Speicher.sounds[Speicher.getChoice(currentsound)].choice[i]);
		choiceCallback(auswahl[currentsound][i],NULL);
#ifdef _DEBUG 
		printf("l:%i:%i ",i,Speicher.sounds[Speicher.getChoice(currentsound)].choice[i]);
#endif
		}
	}
	// send a reset (clears filters and delay buffer)
	if (transmit) lo_send(t, "/Minicomputer", "iif", currentsound, 0, 0.0f);
#ifdef _DEBUG 
	printf("\n");
	fflush(stdout);
#endif
}
/**
 * parmCallback when the load button is pressed
 * @param pointer to the calling widget
 * @param optional data, this time the entry id of which the sound 
 * should be loaded
 */
static void loadsound(Fl_Widget* o, void* )
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
	recall((unsigned int)((int) memDisplay[currentsound]->value()));//(Fl_Input_Choice*)e)->menubutton()->value());
	//fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
	Fl::awake();
	Fl::unlock();
}
/**
 * parmCallback when the load multi button is pressed
 * recall a multitemperal setup
 * @param pointer to the calling widget
 * @param optional data, this time the entry id of which the sound 
 * should be loaded
 */
static void loadmulti(Fl_Widget*, void*)
{
	Fl::lock();
	//fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
	//Fl::awake();
	currentmulti = (unsigned int)multiRoller->value();
#ifdef _DEBUG
	printf("loadmulti #%u transmit %u\n", currentmulti, transmit);
#endif
	//multi[currentmulti][currentsound]=(unsigned int)((Fl_Input_Choice*)e)->menubutton()->value();
	for (int i=0;i<_MULTITEMP;++i)
	{
		currentsound = i;
		if ((Speicher.multis[currentmulti].sound[i]>=0) && (Speicher.multis[currentmulti].sound[i]<512))
		{
			recall(Speicher.multis[currentmulti].sound[i]);// actual recall Bug
			#ifdef _DEBUG
				printf("i ist %i Speicher ist %i\n",i,Speicher.multis[currentmulti].sound[i]);
				fflush(stdout);
			#endif
			// schoice[i]->value(Speicher.getName(0,Speicher.multis[currentmulti].sound[i]).c_str());// set gui
			char temp_name[128];
			strnrtrim(temp_name, Speicher.getName(0,Speicher.multis[currentmulti].sound[i]).c_str(), 128);
			schoice[i]->value(temp_name);
			#ifdef _DEBUG
			printf("loadmulti voice %u: \"%s\"\n", i, temp_name);
			#endif
			#ifdef _DEBUG
				printf("schoice gesetzt\n");
				fflush(stdout);
			#endif
			Rollers[i]->value(Speicher.multis[currentmulti].sound[i]);// set gui
			#ifdef _DEBUG
				printf("Roller gesetzt\n");
				fflush(stdout);
			#endif
			memDisplay[i]->value(Speicher.multis[currentmulti].sound[i]);
			#ifdef _DEBUG
				printf("memDisplay gesetzt\n");
				fflush(stdout);
			#endif
		}
		// set the knobs of the mix
		int MULTI_parm_num[]={101, 106, 107, 108, 109};
		for (unsigned int j=0; j<sizeof(MULTI_parm_num)/sizeof(int); j++)
		{
			#ifdef _DEBUG
			printf("i:%i j:%i knob:%i\n",i,j,actualknob);
			fflush(stdout);
			#endif
			((Fl_Valuator*)Knob[i][MULTI_parm_num[j]])->value(Speicher.multis[currentmulti].settings[i][j]);
			parmCallback(Knob[i][MULTI_parm_num[j]],NULL);
		}
	}
	currentsound = 0;
	// we should go to a defined state, means tab
	tabs->value(tab[currentsound]);
	tabCallback(tabs,NULL);

#ifdef _DEBUG
	printf("multi choice %s\n",((Fl_Input*)e)->value());
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
static void storemulti(Fl_Widget* o, void* e)
{
	Fl::lock();
	fl_cursor(FL_CURSOR_WAIT ,FL_WHITE, FL_BLACK);
	Fl::check();
	/*printf("choice %i\n",((Fl_Input_Choice*)e)->menubutton()->value());
	fflush(stdout);
	*/
	int i;

	if (Multichoice != NULL)
	{
		unsigned int t = (unsigned int) multiRoller->value();//Multichoice->menubutton()->value();
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
	
	for (i=0;i<8;++i)
	{
		Speicher.multis[currentmulti].sound[i]=Speicher.getChoice(i);
#ifdef _DEBUG
		printf("sound slot: %d = %d\n",i,Speicher.getChoice(i));
#endif
		Speicher.multis[currentmulti].settings[i][0]=((Fl_Valuator*)Knob[i][101])->value();
		Speicher.multis[currentmulti].settings[i][1]=((Fl_Valuator*)Knob[i][106])->value();
		Speicher.multis[currentmulti].settings[i][2]=((Fl_Valuator*)Knob[i][107])->value();
		Speicher.multis[currentmulti].settings[i][3]=((Fl_Valuator*)Knob[i][108])->value();
		Speicher.multis[currentmulti].settings[i][4]=((Fl_Valuator*)Knob[i][109])->value();
	}
	// write to disk
	Speicher.saveMulti();
	
	fl_cursor(FL_CURSOR_DEFAULT,FL_WHITE, FL_BLACK);
	Fl::check();
	Fl::awake();
	Fl::unlock();
}
/*
static void voiceparmCallback(Fl_Widget* o, void* e)
{
	transmit=false;
	currentsound=((unsigned int)((Fl_Valuator*)o)->value());
	recall(multi[currentmulti][currentsound]);
	transmit=true;
}*/
/**
 * change the multisetup, should be called from a Midi Program Change event on Channel 9
 * @param int the Program number between 0 and 127
 */
void UserInterface::changeMulti(int pgm)
{
	char temp_name[128];
	Fl::lock();
	strnrtrim(temp_name, Speicher.multis[pgm].name, 128);
	multichoice->value(temp_name);
	#ifdef _DEBUG
	printf("UserInterface::changeMulti # %u: \"%s\"\n", pgm, temp_name);
	#endif
	//multichoice->damage(FL_DAMAGE_ALL);
	multichoice->redraw();
	multiRoller->value(pgm);// set gui
	multiRoller->redraw();
	multiDisplay->value(pgm);
	multiDisplay->redraw();
	loadmulti(NULL,multichoice);
	//	Fl::redraw();
	//	Fl::flush();
	Fl::awake();
	Fl::unlock();
}

/**
 * change the sound for a certain voice,should be called from a Midi Program Change event on Channel 1 - 8
 * @param int the voice between 0 and 7 (it is not clear if the first Midi channel is 1 (which is usually the case in the hardware world) or 0)
 * @param int the Program number between 0 and 127
 */
void UserInterface::changeSound(int channel,int pgm)
{
	if ((channel >-1) && (channel < 8) && (pgm>-1) && (pgm<128))
	{
	Fl::lock();
		int t = currentsound;
		currentsound = channel;
		schoice[channel]->value(Speicher.getName(0,pgm).c_str());
		//schoice[channel]->damage(FL_DAMAGE_ALL);
		//schoice[channel]->redraw();
		Rollers[channel]->value(pgm);// set gui
		Rollers[channel]->redraw();
		memDisplay[channel]->value(pgm);
		loadsound(NULL,schoice[channel]);
//		Fl::redraw();
//		Fl::flush();
		currentsound = t;
	Fl::awake();
	Fl::unlock();
	}
}
/**
 * predefined menue with all modulation sources
 */
Fl_Menu_Item UserInterface::menu_amod[] = {
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
Fl_Menu_Item UserInterface::menu_fmod[] = {
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
 * waveform list for menue
 */
Fl_Menu_Item UserInterface::menu_wave[] = {
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
Fl_Menu_Item UserInterface::menu_pitch[] = {
 {"EG1", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"velocity", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};


Fl_Menu_Item UserInterface::menu_pitch1[] = {
 {"EG1", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"velocity", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

Fl_Menu_Item UserInterface::menu_morph[] = {
 {"EG1", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"velocity", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};
*/
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
// 			screen initialization
// ---------------------------------------------------------------

// These should be members of UserInterface?
static unsigned char idata_miniMini2[_LOGO_WIDTH*_LOGO_HEIGHT*3];
Fl_RGB_Image image_miniMini2(idata_miniMini2, _LOGO_WIDTH, _LOGO_HEIGHT, 3, 0);

Fenster* UserInterface::make_window(const char* title) {    
 // Fl_Double_Window* w;
 // {
	currentsound=0;
	currentmulti=0;
	transmit=true;
	Fenster* o = new Fenster(995, 515, title);
	// w = o;
	o->color((Fl_Color)_BGCOLOR);
	o->user_data((void*)(this));
	for (int i=0;i<_CHOICECOUNT;++i) {
		auswahl[currentsound][i]=NULL;
	}
	for (int i=0;i<_PARACOUNT;++i) {
		Knob[currentsound][i]=NULL;
	}

	// Change logo background
	memcpy(idata_miniMini2, idata_miniMini, _LOGO_WIDTH*_LOGO_HEIGHT*3*sizeof(unsigned char));
	replace_color(idata_miniMini2, _LOGO_WIDTH*_LOGO_HEIGHT, 190, 218, 255, _BGCOLOR_R, _BGCOLOR_G, _BGCOLOR_B);

// tabs beginning ------------------------------------------------------------
	{ Fl_Tabs* o = new Fl_Tabs(0,0,995, 515);
		o->callback((Fl_Callback*)tabCallback);
	 int i;
	for (i=0;i<_MULTITEMP;++i)// generate 8 tabs for the 8 voices
	{
		{ 
		ostringstream oss;
		oss<<"sound "<<(i+1);// create name for tab
		tablabel[i]=oss.str();
		Fl_Group* o = new Fl_Group(1, 10, 995, 515, tablabel[i].c_str());
		o->color((Fl_Color)_BGCOLOR);
		o->labelsize(8);
		// o->labelcolor(FL_BACKGROUND2_COLOR); 
		o->box(FL_BORDER_FRAME);

	// draw logo
	{ Fl_Box* o = new Fl_Box(855, 450, 25, 25);
	  o->image(image_miniMini2);
	}
	{ Fl_Group* o = new Fl_Group(5, 17, 300, 212);
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	  {
	  Fl_Box* d = new Fl_Box(145, 210, 30, 22,"oscillator 1");
	  d->labelsize(8);
	  d->labelcolor(FL_BACKGROUND2_COLOR);
	  }
	  { Fl_Dial* o= new Fl_Dial(21, 20, 34, 34, "frequency");
		o->labelsize(8);
		o->maximum(1000); 
		o->argument(1);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][1] = o;
	  }
	  { Fl_Value_Input* o = new Fl_Value_Input(16, 66, 46, 15);// frequency display of oscillator 1
		o->box(FL_ROUNDED_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->maximum(10000);
		o->step(0.001);
		o->argument(1);
		o->callback((Fl_Callback*)parmCallback);
		miniDisplay[i][0]=o;
	  }
	  { Fl_Light_Button* o = new Fl_Light_Button(20, 92, 66, 19, "fix frequency");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(8);
		o->argument(2);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][2] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(79, 179, 25, 25, "fm output  vol");
		o->labelsize(8);
		o->argument(13);
		//o->maximum(1000);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Light_Button* o = new Fl_Light_Button(80, 27, 40, 15, "boost");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(8);
		o->argument(4);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  /*{ Fl_Dial* o = new Fl_SteinerKnob(20, 121, 34, 34, "tune");
		o->labelsize(8);
		o->minimum(0.5);
		o->maximum(16);
		o->argument(3);
		o->callback((Fl_Callback*)parmCallback);
		Knob[3] = o;
	  }*/
	  
	  { Fl_Positioner* o = new Fl_Positioner(15,121,40,80,"tune");
		o->xbounds(0,16);
		o->ybounds(1,0);
		o->box(FL_BORDER_BOX);
		o->xstep(1);
		o->labelsize(8);
		o->argument(3); o->selection_color(0);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][3]=o;
	  }
	  { Fl_Value_Input* o = new Fl_Value_Input(62, 160, 46, 15);
		o->box(FL_ROUNDED_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->maximum(10000);
		o->argument(3);
		o->step(0.001);
		o->callback((Fl_Callback*)tuneCallback);
		miniDisplay[i][1]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(260, 97, 25, 25, "amount");
		o->labelsize(8);
		o->argument(9); 
		o->minimum(-1);
		o->maximum(1);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(134, 102, 120, 15, "amp modulator 1");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(2);
		o->callback((Fl_Callback*)choiceCallback);
		 o->menu(menu_amod);
		 auswahl[i][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(260, 133, 25, 25, "amount");
		o->labelsize(8);
		o->argument(11);  
		o->minimum(-1);
		o->maximum(1);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
		
	  }
	  { Fl_Choice* o = new Fl_Choice(134, 138, 120, 15, "amp modulator 2");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(3);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_amod);
		auswahl[i][o->argument()]=o;
	   
	  }
	  { Fl_Dial* o = new Fl_Dial(247, 23, 25, 25, "amount");
		o->labelsize(8);
		o->argument(5);  
		o->minimum(-1000);
		o->maximum(1000);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(122, 28, 120, 15, "freq modulator 1");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(0);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_fmod);
		auswahl[i][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(248, 59, 25, 25, "amount");
		o->labelsize(8);
		o->argument(7); 
		 o->minimum(-1000);
		o->maximum(1000);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(122, 64, 120, 15, "freq modulator 2");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(1);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_fmod);
		auswahl[i][o->argument()]=o;
	  }
	  
	  
	  { Fl_Choice* j = new Fl_Choice(134, 167, 120, 15, "waveform");
		j->box(FL_BORDER_BOX);
		j->down_box(FL_BORDER_BOX);
		j->labelsize(8);
		j->textsize(8);
		j->align(FL_ALIGN_TOP_LEFT);
		j->argument(4);
		auswahl[i][j->argument()] = j;
		j->callback((Fl_Callback*)choiceCallback);
		j->menu(menu_wave);
	  }
	  { Fl_Choice* j = new Fl_Choice(134, 196, 120, 15, "sub waveform");
		j->box(FL_BORDER_BOX);
		j->down_box(FL_BORDER_BOX);
		j->labelsize(8);
		j->textsize(8);
		j->align(FL_ALIGN_TOP_LEFT);
		j->argument(15);
		auswahl[i][j->argument()] = j;
		j->callback((Fl_Callback*)choiceCallback);
		j->menu(menu_wave);
	  }
	  o->end();
	}
   
	//}
   { Fl_Group* o = new Fl_Group(5, 238, 300, 212);
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	  {Fl_Box* d = new Fl_Box(145, 431, 30, 22, "oscillator 2");
	  	d->labelsize(8);
	  	d->labelcolor(FL_BACKGROUND2_COLOR);
	  }
	  { Fl_Dial* o = new Fl_Dial(21, 244, 34, 34, "frequency");
		o->labelsize(8); 
		o->argument(16);
		 o->maximum(1000); 
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][16] = o;
	  }
	  { Fl_Value_Input* o = new Fl_Value_Input(16, 290, 46, 15);
		o->box(FL_ROUNDED_BOX);
		o->labelsize(8);
		o->textsize(8); 
		o->maximum(10000);
	o->step(0.001);
		o->argument(16);
		o->callback((Fl_Callback*)parmCallback);
		miniDisplay[i][2]=o;
	  }
	  { Fl_Light_Button* o = new Fl_Light_Button(20, 316, 66, 19, "fix frequency");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(8); 
		o->argument(17);
		o->callback((Fl_Callback*)parmCallback);
	Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(79, 403, 25, 25, "fm output  vol");
		o->labelsize(8); 
		o->argument(28);
	  //  o->maximum(1000);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][28] = o;
	  }
	  { Fl_Light_Button* o = new Fl_Light_Button(80, 251, 40, 15, "boost");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(8);
		 o->argument(15);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Positioner* o = new Fl_Positioner(15,345,40,80,"tune");
		o->xbounds(0,16);
		o->ybounds(1,0);
		o->box(FL_BORDER_BOX);
		o->xstep(1); o->selection_color(0);
		o->labelsize(8);
		o->argument(18);
		o->callback((Fl_Callback*)parmCallback);
		
		/*Fl_Dial* o = new Fl_SteinerKnob(20, 345, 34, 34, "tune");
		o->labelsize(8);
		o->minimum(0.5);
		o->maximum(16);   
		o->argument(18);
		o->callback((Fl_Callback*)parmCallback);*/
		Knob[i][18] = o;
	  }
	  { Fl_Value_Input* o = new Fl_Value_Input(62, 384, 46, 15);
		o->box(FL_ROUNDED_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->maximum(10000);   
		o->argument(18);
	o->step(0.001);
		o->callback((Fl_Callback*)tuneCallback);
		miniDisplay[i][3]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(260, 321, 25, 25, "amount");
		o->labelsize(8);
		o->argument(23);
		o->minimum(-1);
		o->maximum(1);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(134, 326, 120, 15, "amp modulator");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		 o->argument(8);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_amod);
		 auswahl[i][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(260, 357, 25, 25, "amount");
		o->labelsize(8);
		o->argument(25);  o->minimum(-1);
		o->maximum(1);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(134, 362, 120, 15, "fm out amp modulator");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(9);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_amod);
		auswahl[i][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(247, 247, 25, 25, "amount");
		o->labelsize(8);
		o->argument(19);  o->minimum(-1000);
		o->maximum(1000);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(122, 252, 120, 15, "freq modulator 1");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		o->argument(6);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_fmod);
		auswahl[i][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(248, 283, 25, 25, "amount");
		o->labelsize(8);
		o->argument(21);  o->minimum(-1000);
		o->maximum(1000);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(122, 288, 120, 15, "freq modulator 2");
		  o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		 o->argument(7);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_fmod);
		auswahl[i][o->argument()]=o;
	  }
	  { Fl_Choice* o = new Fl_Choice(120, 408, 120, 15, "waveform");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		 o->argument(5);
		o->callback((Fl_Callback*)choiceCallback);
		o->menu(menu_wave);auswahl[i][o->argument()]=o;
	  }
		{ Fl_Light_Button* o = new Fl_Light_Button(220, 430, 65, 15, "sync to osc1");
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(8);
	  o->argument(115);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[i][o->argument()] = o;
		}
	  o->end();
	}
  // ----------------- knobs  for the filters -------------------------------------- 
  { Fl_Group* o = new Fl_Group(312, 17, 277, 433);
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
  	{ Fl_Box* d = new Fl_Box(312, 225, 277, 435, "filters");
	  	d->labelsize(8);
	  	d->labelcolor(FL_BACKGROUND2_COLOR);
	}
	  { Fl_Group* o = new Fl_Group(330, 28, 239, 92, "filter 1");
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		{Fl_Positioner* o = new Fl_Positioner(340,31,70,79,"cut");
		o->xbounds(0,9000);
		o->ybounds(499,0); o->selection_color(0);
		o->box(FL_BORDER_BOX);
		o->xstep(500);
		o->labelsize(8);
		o->argument(30);
		o->yvalue(200.5);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
		
		/* Fl_Dial* o = f1cut1 = new Fl_SteinerKnob(344, 51, 34, 34, "cut");
		  o->labelsize(8);
		  o->argument(30);
			o->maximum(10000);
		o->value(50);
		o->callback((Fl_Callback*)parmCallback);
		*/
		}
		{ Fl_Dial* o = f1q1 = new Fl_Dial(415, 33, 25, 25, "q");
		  o->labelsize(8);
		  o->argument(31);
		  o->minimum(0.9);
		  o->value(0.9);
		  o->maximum(0.01);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = f1vol1 = new Fl_Dial(425, 70, 20, 20, "vol");
		  o->labelsize(8);
		   o->argument(32);
		o->callback((Fl_Callback*)parmCallback);o->minimum(-1);
		  o->value(0.5);
		  o->maximum(1);Knob[i][o->argument()] = o;
		}
		
		
		{ Fl_Positioner* o = new Fl_Positioner(456,31,70,79,"cut");
		o->xbounds(0,9000);
		o->ybounds(499,0);
		o->box(FL_BORDER_BOX);
		o->xstep(500); o->selection_color(0);
		o->labelsize(8);
	o->yvalue(20);
		o->callback((Fl_Callback*)parmCallback);
	   /* Fl_Dial* o = f1cut2 = new Fl_SteinerKnob(481, 50, 34, 34, "cut");
		  o->labelsize(8); 
		  o->value(50);
		   o->maximum(10000);
		o->callback((Fl_Callback*)parmCallback);*/
		  o->argument(33);
	  Knob[i][o->argument()] = o;
		
		}
		{ Fl_Dial* o = f1q2 = new Fl_Dial(531, 32, 25, 25, "q");
		  o->labelsize(8); 
		  
		  o->argument(34);
		  o->minimum(0.9);
		  o->value(0.5);
		  o->maximum(0.01);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = f1vol2 = new Fl_Dial(541, 70, 20, 20, "vol");
		  o->labelsize(8); 
		  o->labelsize(8);
		  o->minimum(-1);
		  o->value(0.5);
		  o->maximum(1);
		   o->argument(35);
	   o->callback((Fl_Callback*)parmCallback);
	   Knob[i][o->argument()] = o;
		}
		{ Fl_Value_Input* o = new Fl_Value_Input(412, 100, 38, 15);
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(8);
		  o->textsize(8);
	  o->maximum(10000);
	  o->step(0.01);
	o->value(200);
		  o->argument(30);
	   o->callback((Fl_Callback*)cutoffCallback);
		  miniDisplay[i][4]=o;
		}
		{ Fl_Value_Input* o = new Fl_Value_Input(528, 100, 38, 15);
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(8);
		  o->textsize(8);
	  o->maximum(10000);
	  o->step(0.01);
	o->value(20);
		  o->argument(33);
	   o->callback((Fl_Callback*)cutoffCallback);
		   miniDisplay[i][5]=o;
		}
	/*
		{ Fl_Button* o = new Fl_Button(426, 35, 45, 15, "copy ->");
		  o->box(FL_BORDER_BOX);
		  o->labelsize(8);
		}
		{ Fl_Button* o = new Fl_Button(426, 59, 45, 15, "<- copy");
		  o->box(FL_BORDER_BOX);
		  o->labelsize(8);
		}*/
		o->end();
	  }
	  { Fl_Dial* o = new Fl_Dial(418, 360, 60, 57, "morph");
		o->type(1);
		o->labelsize(8);
		o->maximum(0.5f);
		o->argument(56);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(326, 392, 25, 25, "amount");
		o->labelsize(8);
		o->minimum(-2);
		o->maximum(2);
		o->argument(38);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(325, 366, 85, 15, "morph mod 1");
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(10);
		o->callback((Fl_Callback*)choiceCallback);auswahl[i][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(551, 392, 25, 25, "amount");
		o->labelsize(8);
		o->argument(48);
		o->minimum(-2);
		o->maximum(2);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(492, 366, 85, 15, "morph mod 2");
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(11);
		o->callback((Fl_Callback*)choiceCallback);auswahl[i][o->argument()]=o;
	  }
	  { Fl_Group* o = new Fl_Group(330, 132, 239, 92, "filter 2");
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		{ Fl_Positioner* o = new Fl_Positioner(340,135,70,79,"cut");
		o->xbounds(0,7000);
		o->ybounds(499,0); o->selection_color(0);
		o->box(FL_BORDER_BOX);
		o->xstep(500);
		o->labelsize(8);
		/*Fl_Dial* o = f1cut1 = new Fl_SteinerKnob(344, 155, 34, 34, "cut");
		  o->labelsize(8);
			o->labelsize(8);
		   o->value(50);
		  o->maximum(10000);*/
		  o->argument(40);Knob[i][o->argument()] = o;
		o->callback((Fl_Callback*)parmCallback);
		}
		{ Fl_Dial* o = f1q1 = new Fl_Dial(415, 137, 25, 25, "q");
		  o->labelsize(8);
		   o->argument(41);
		  o->minimum(0.9);
		  o->maximum(0.001);
		  o->value(0.5);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = f1vol1 = new Fl_Dial(425, 174, 20, 20, "vol");
		  o->labelsize(8);
		   o->argument(42);
	 o->callback((Fl_Callback*)parmCallback);
	 o->minimum(-1);
		  o->value(0);
		  o->maximum(1);Knob[i][o->argument()] = o;
		}
		{ Fl_Positioner* o = new Fl_Positioner(456,135,70,79,"cut");
		o->xbounds(0,7000);
		o->ybounds(499,0); o->selection_color(0);
		o->box(FL_BORDER_BOX);
		o->xstep(500);
		o->labelsize(8);
		
		/*Fl_Dial* o = f1cut2 = new Fl_SteinerKnob(481, 154, 34, 34, "cut");
		  o->labelsize(8);
		   o->labelsize(8);
		   o->value(50);
		  o->maximum(10000);*/
		  o->argument(43);Knob[i][o->argument()] = o;
		o->callback((Fl_Callback*)parmCallback);
		}
		{ Fl_Dial* o = f1q2 = new Fl_Dial(531, 136, 25, 25, "q");
		  o->labelsize(8);
		   o->labelsize(8);
		  o->argument(44);
		  o->minimum(0.9);
		  o->value(0.5);
		  o->maximum(0.001);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = f1vol2 = new Fl_Dial(541, 174, 20, 20, "vol");
		  o->labelsize(8);
		   o->labelsize(8);
		  o->argument(45);
	  o->maximum(1);
	 o->callback((Fl_Callback*)parmCallback);
	  o->minimum(-1);
		  o->value(0);
		  Knob[i][o->argument()] = o;
		}
		{ Fl_Value_Input* o = new Fl_Value_Input(412, 204, 38, 15);
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(8);
	  o->maximum(10000);
		  o->argument(40);
	  o->step(0.01);
	   o->callback((Fl_Callback*)cutoffCallback);
		  o->textsize(8);
	  miniDisplay[i][6]=o;
		}
		{ Fl_Value_Input* o = new Fl_Value_Input(528, 204, 38, 15);
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(8);
		  o->argument(43);
	  o->maximum(10000);
	  o->step(0.01);
	   o->callback((Fl_Callback*)cutoffCallback);
		  o->textsize(8);
	  miniDisplay[i][7]=o;
		}
	/*
		{ Fl_Button* o = new Fl_Button(426, 139, 45, 15, "copy ->");
		  o->box(FL_BORDER_BOX);
		  o->labelsize(8);
		  o->argument(21);
		//o->callback((Fl_Callback*)copyparmCallback);
		}
		{ Fl_Button* o = new Fl_Button(426, 163, 45, 15, "<- copy");
		  o->box(FL_BORDER_BOX);
		  o->labelsize(8);
		  o->argument(22);
	  //o->callback((Fl_Callback*)copyparmCallback);
		}*/
		o->end();
	  }
	  { Fl_Group* o = new Fl_Group(330, 238, 239, 92, "filter 3");
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		{Fl_Positioner* o = new Fl_Positioner(340,241,70,79,"cut");
		o->xbounds(0,7000);
		o->ybounds(499,0); o->selection_color(0);
		o->box(FL_BORDER_BOX);
		o->xstep(500);
		o->labelsize(8);
		/* Fl_Dial* o = f1cut1 = new Fl_SteinerKnob(344, 261, 34, 34, "cut");
		  o->labelsize(8); 
		  o->value(50);
		  o->maximum(10000);*/
		  o->argument(50);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = f1q1 = new Fl_Dial(415, 243, 25, 25, "q");
		  o->labelsize(8); 
		   o->argument(51);
		  o->minimum(0.9);
		  o->maximum(0.001);
		  o->value(0.5);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = f1vol1 = new Fl_Dial(425, 280, 20, 20, "vol");
		  o->labelsize(8);
		   o->argument(52);
		   o->maximum(1);o->minimum(-1);
		  o->value(0);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Positioner* o = new Fl_Positioner(456,241,70,79,"cut");
		o->xbounds(0,7000);
		o->ybounds(499,0); o->selection_color(0);
		o->box(FL_BORDER_BOX);
		o->xstep(500);
		o->labelsize(8);
		/*Fl_Dial* o = f1cut2 = new Fl_SteinerKnob(481, 260, 34, 34, "cut");
		  o->labelsize(8);
		  o->value(50);
		  o->maximum(10000);*/
		   o->argument(53);
		   Knob[i][o->argument()] = o;
		o->callback((Fl_Callback*)parmCallback);
		}
		{ Fl_Dial* o = f1q2 = new Fl_Dial(531, 242, 25, 25, "q");
		  o->labelsize(8);
		  o->argument(54);
		  o->minimum(0.9);
		  o->value(0.5);
		  o->maximum(0.001);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = f1vol2 = new Fl_Dial(541, 280, 20, 20, "vol");
		  o->labelsize(8); 
		  o->labelsize(8);
		  o->argument(55);o->maximum(1);o->minimum(-1);
		  o->value(0);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Value_Input* o = new Fl_Value_Input(412, 310, 38, 15);
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(8);
	  o->maximum(10000);
	  o->step(0.01);
		  o->argument(50);
	   o->callback((Fl_Callback*)cutoffCallback);
		  o->textsize(8);
	  miniDisplay[i][8]=o;
		}
		{ Fl_Value_Input* o = new Fl_Value_Input(528, 310, 38, 15);
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(8);
	  o->maximum(10000);
		  o->argument(53);
	  o->step(0.01);
	   o->callback((Fl_Callback*)cutoffCallback);
		  o->textsize(8);
	  miniDisplay[i][9]=o;
		}
	/*
		{ Fl_Button* o = new Fl_Button(426, 245, 45, 15, "copy ->");
		  o->box(FL_BORDER_BOX);
		  o->labelsize(8);
		}
		{ Fl_Button* o = new Fl_Button(426, 269, 45, 15, "<- copy");
		  o->box(FL_BORDER_BOX);
		  o->labelsize(8);
		}*/
		o->end();
	  } 
	  { Fl_Button* o = new Fl_Button(486, 430, 50, 15, "clear filter");
		o->tooltip("reset the filter");
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->argument(0);
		o->callback((Fl_Callback*)parmCallback);
	  }
	  o->end();
	}
	{ Fl_Group* o = new Fl_Group(595, 17, 225, 433);
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	{ Fl_Box* d = new Fl_Box(595, 225, 210, 432, "modulators");
	  d->labelsize(8);
	  d->labelcolor(FL_BACKGROUND2_COLOR);
	  }
	  // ----------- knobs for envelope generator 1 ---------------
	  { Fl_Group* o = new Fl_Group(608, 31, 200, 45, "EG 1");
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		{ Fl_Dial* o = new Fl_Dial(618, 37, 25, 25, "A");
		  o->labelsize(8); 
		  o->argument(60);  
		  o->minimum(0.2);
		  o->maximum(0.01);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(648, 37, 25, 25, "D");
		  o->labelsize(8);
		  o->argument(61);
		  o->minimum(0.15);
		  o->maximum(0.01);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[i][o->argument()] = o;
		}
		
		{ Fl_Dial* o = new Fl_Dial(678, 37, 25, 25, "S");
		  o->labelsize(8);
		  o->argument(62);
		 // o->minimum(0);
		 // o->maximum(0.001);
		  o->callback((Fl_Callback*)parmCallback);
		  Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(708, 37, 25, 25, "R");
		  o->labelsize(8);
		  o->argument(63);
		  o->minimum(0.15);
		  o->maximum(0.02);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Light_Button* o = new Fl_Light_Button(744, 42, 55, 15, "repeat");
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(8);
		  o->argument(64);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		o->end();
	  }
	  // ----------- knobs for envelope generator 2 ---------------
	  { Fl_Group* o = new Fl_Group(608, 90, 200, 45, "EG 2");
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		{ Fl_Dial* o = new Fl_Dial(618, 96, 25, 25, "A");
		  o->labelsize(8);o->argument(65);
	  o->minimum(0.2);
		  o->maximum(0.01);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(648, 96, 25, 25, "D");
		  o->labelsize(8);o->argument(66);
	  o->minimum(0.15);
		  o->maximum(0.01);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(678, 96, 25, 25, "S");
		  o->labelsize(8);o->argument(67);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(708, 96, 25, 25, "R");
		  o->labelsize(8);o->argument(68);
	  o->minimum(0.15);
		  o->maximum(0.02);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Light_Button* o = new Fl_Light_Button(744, 101, 55, 15, "repeat");
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(8);o->argument(69);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		o->end();
	  }
	  // ----------- knobs for envelope generator 3 ---------------
	  { Fl_Group* o = new Fl_Group(608, 147, 200, 45, "EG 3");
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		{ Fl_Dial* o = new Fl_Dial(618, 153, 25, 25, "A");
		  o->labelsize(8);o->argument(70);
	  o->minimum(0.2);
		  o->maximum(0.01);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(648, 153, 25, 25, "D");
		  o->labelsize(8);o->argument(71);
	  o->minimum(0.15);
		  o->maximum(0.01);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(678, 153, 25, 25, "S");
		  o->labelsize(8);o->argument(72);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(708, 153, 25, 25, "R");
		  o->labelsize(8);o->argument(73);
	  o->minimum(0.15);
		  o->maximum(0.02);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Light_Button* o = new Fl_Light_Button(744, 158, 55, 15, "repeat");
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(8);o->argument(74);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		o->end();
	  }
	  // ----------- knobs for envelope generator 4 ---------------
	  { Fl_Group* o = new Fl_Group(608, 204, 200, 45, "EG 4");
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		{ Fl_Dial* o = new Fl_Dial(618, 210, 25, 25, "A");
		  o->labelsize(8);o->argument(75);
	  o->minimum(0.2);
		  o->maximum(0.01);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(648, 210, 25, 25, "D");
		  o->labelsize(8);o->argument(76);
	  o->minimum(0.15);
		  o->maximum(0.01);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(678, 210, 25, 25, "S");
		  o->labelsize(8);o->argument(77);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(708, 210, 25, 25, "R");
		  o->labelsize(8);o->argument(78);
	  o->minimum(0.15);
		  o->maximum(0.02);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Light_Button* o = new Fl_Light_Button(744, 215, 55, 15, "repeat");
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(8);o->argument(79);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		o->end();
	  }
	  // ----------- knobs for envelope generator 5 ---------------
	  { Fl_Group* o = new Fl_Group(608, 263, 200, 45, "EG 5");
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		{ Fl_Dial* o = new Fl_Dial(618, 269, 25, 25, "A");
		  o->labelsize(8);o->argument(80);
	  o->minimum(0.2);
		  o->maximum(0.01);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(648, 269, 25, 25, "D");
		  o->labelsize(8);o->argument(81);
	  o->minimum(0.15);
		  o->maximum(0.01);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(678, 269, 25, 25, "S");
		  o->labelsize(8);o->argument(82);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(708, 269, 25, 25, "R");
		  o->labelsize(8);o->argument(83);
	  o->minimum(0.15);
		  o->maximum(0.02);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Light_Button* o = new Fl_Light_Button(744, 274, 55, 15, "repeat");
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(8);o->argument(84);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		o->end();
	  }
	  // ------ knobs for envelope generator 6 -----
	  { Fl_Group* o = new Fl_Group(608, 324, 200, 45, "EG 6");
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		{ Fl_Dial* o = new Fl_Dial(618, 330, 25, 25, "A");
		  o->labelsize(8);o->argument(85); 
		  o->minimum(0.2);
		  o->maximum(0.01);
		  Knob[i][o->argument()] = o;
		  
		  o->callback((Fl_Callback*)parmCallback);
		}
		{ Fl_Dial* o = new Fl_Dial(648, 330, 25, 25, "D");
		  o->labelsize(8);o->argument(86);
	  o->minimum(0.15);
		  o->maximum(0.01);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(678, 330, 25, 25, "S");
		  o->labelsize(8);o->argument(87);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Dial* o = new Fl_Dial(708, 330, 25, 25, "R");
		  o->labelsize(8);o->argument(88); 
	  o->minimum(0.15);
		  o->maximum(0.02);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		{ Fl_Light_Button* o = new Fl_Light_Button(744, 335, 55, 15, "repeat");
		  o->box(FL_BORDER_BOX);
		  o->selection_color((Fl_Color)89);
		  o->labelsize(8);o->argument(89);
		  o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
		}
		o->end();
	  }
	  { Fl_Group* o = new Fl_Group(608, 380, 200, 54, "mod osc");
		o->box(FL_ROUNDED_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		
		{  Fl_Positioner* o = new Fl_Positioner(620,384,50,40,"tune");
		o->xbounds(0,128);
		o->ybounds(0.99,0);
		o->box(FL_BORDER_BOX);
		o->xstep(1);
		o->labelsize(8);
		o->argument(90);
		   o->selection_color(0);
		o->callback((Fl_Callback*)parmCallback);
		 Knob[i][o->argument()] = o;
	  /*Fl_Dial* o = new Fl_SteinerKnob(627, 392, 34, 34, "frequency");
		  o->labelsize(8);o->argument(90);
		  o->callback((Fl_Callback*)parmCallback);
		  o->maximum(500); */
		}
		{ Fl_Choice* o = new Fl_Choice(680, 397, 120, 15, "waveform");
		  o->box(FL_BORDER_BOX);
		  o->down_box(FL_BORDER_BOX);
		  o->labelsize(8);
		  o->textsize(8);
		  o->align(FL_ALIGN_TOP_LEFT);
		  o->menu(menu_wave);
		  o->argument(12);
		  o->callback((Fl_Callback*)choiceCallback);
		  auswahl[i][o->argument()]=o;
		} 
		{ Fl_Value_Input* o = new Fl_Value_Input(680, 415, 38, 15, "Hz");// frequency display for modulation oscillator
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(8);
		  o->align(FL_ALIGN_RIGHT);
		  o->maximum(10000);
		  o->step(0.001);
		  o->textsize(8);
		  o->callback((Fl_Callback*)tuneCallback);
		  o->argument(90);
		  miniDisplay[i][10]=o;
		}
		{ Fl_Value_Input* o = new Fl_Value_Input(740, 415, 38, 15, "BPM");// BPM display for modulation oscillator
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(8);
		  o->align(FL_ALIGN_RIGHT);
		  o->maximum(10000);
		  o->step(0.001);
		  o->textsize(8);
		  o->callback((Fl_Callback*)BPMCallback);
		  o->argument(123);
		  miniDisplay[i][11]=o;
		}
		o->end();
	  }
	  o->end();
	}
	
   
	{ Fl_Group* d = new Fl_Group(5, 461, 680, 45, "memory");
	  d->box(FL_ROUNDED_FRAME);
	  d->color(FL_BACKGROUND2_COLOR);
	  d->labelsize(8);
	  d->labelcolor(FL_BACKGROUND2_COLOR);
	  d->begin();
	 /* { Fl_Button* o = new Fl_Button(191, 473, 50, 19, "create bank");
		o->tooltip("create a new bank after current one");
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
	  }
	  { Fl_Button* o = new Fl_Button(26, 476, 53, 14, "delete bank");
		o->tooltip("delete a whole bank of sounds!");
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
		o->labelcolor((Fl_Color)1);
	  }
	  { Fl_Button* o = new Fl_Button(732, 475, 59, 14, "delete sound");
		o->tooltip("delete current sound");
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
		o->labelcolor((Fl_Color)1);
	  }*/
	  { Fl_Input* o = new Fl_Input(274, 471, 150, 14, "Sound");
		o->box(FL_BORDER_BOX);
		//o->down_box(FL_BORDER_FRAME);
		//o->color(FL_FOREGROUND_COLOR);
		//o->selection_color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		o->textsize(8);
		//o->menubutton()->textsize(8);
		//o->menubutton()->type(Fl_Menu_Button::POPUP1);
		o->align(FL_ALIGN_TOP_LEFT);
		soundchoice[i] = o;
		schoice[i] = o;
		d->add(o);
		//o->callback((Fl_Callback*)chooseCallback,NULL);
	  }
	  { Fl_Roller* o = new Fl_Roller(274, 487, 150, 14);
	  	o->type(FL_HORIZONTAL);
		o->tooltip("roll the list of sounds");
		o->minimum(0);
	o->maximum(511);
	o->step(1);
	//o->slider_size(0.04);
	o->box(FL_BORDER_FRAME);
	Rollers[i]=o;
		o->callback((Fl_Callback*)rollerCallback,NULL);
	  }

	  { Fl_Button* o = new Fl_Button(516, 465, 55, 19, "store sound");
		o->tooltip("store this sound on current entry");
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
		o->callback((Fl_Callback*)storesound,soundchoice[i]);
	  }
	  { Fl_Button* o = new Fl_Button(436, 465, 70, 19, "load sound");
		o->tooltip("actually load the chosen sound");
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->callback((Fl_Callback*)loadsound,soundchoice[i]);
	  }
		{ Fl_Value_Output* o = new Fl_Value_Output(490, 488, 20, 15,"memory");
		  o->box(FL_ROUNDED_BOX);
		  o->color(FL_BLACK);
		  o->labelsize(8);
		  o->maximum(512);
		  o->textsize(8);
		  o->textcolor(FL_RED);
		  //char tooltip [64];
		  //sprintf(tooltip,"accept Midi Program Change on channel %i",i);
		  //o->tooltip(tooltip);
		  memDisplay[i]=o;
		}
	  { Fl_Button* o = new Fl_Button(600, 469, 70, 12, "import sound");
		o->tooltip("import single sound to dialed memory slot, you need to load it for playing");
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
		//o->labelcolor((Fl_Color)1);
		o->callback((Fl_Callback*)importPressed,soundchoice[i]);
	  }

	  { Fl_Button* o = new Fl_Button(600, 485, 70, 12, "export sound");
		o->tooltip("export sound data of dialed memory slot");
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
		//o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->callback((Fl_Callback*)exportPressed,soundchoice[i]);
	  }
	  /*{ Fl_Input_Choice* o = new Fl_Input_Choice(83, 476, 105, 14, "bank");
		o->box(FL_BORDER_FRAME);
		o->down_box(FL_BORDER_FRAME);
		o->color(FL_FOREGROUND_COLOR);
		o->selection_color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
	  }*/
	  d->end();
	}
	{ Fl_Group* o = new Fl_Group(825, 17, 160, 212);
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	{ Fl_Box* d = new Fl_Box(825, 210, 160, 22, "amp");
	  d->labelsize(8);
	  d->labelcolor(FL_BACKGROUND2_COLOR);
	}
	// amplitude envelope
	{ Fl_Dial* o = new Fl_Dial(844, 83, 25, 25, "A");
		o->labelsize(8);o->argument(102); 
		o->minimum(0.20);
		o->maximum(0.01);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(874, 83, 25, 25, "D");
		o->labelsize(8);o->argument(103); 
		o->minimum(0.15);
		o->maximum(0.01);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(904, 83, 25, 25, "S");
		o->labelsize(8);o->argument(104);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(934, 83, 25, 25, "R");
		o->labelsize(8);o->argument(105); 
		o->minimum(0.15);
		o->maximum(0.02);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(934, 29, 25, 25, "amount");
		o->labelsize(8);
		o->argument(100);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(844, 35, 85, 15, "amp modulator");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(13);
		o->callback((Fl_Callback*)choiceCallback);
		auswahl[i][o->argument()]=o;
	  }
	  /*
	  { Fl_Counter* o = new Fl_Counter(844, 151, 115, 14, "sound");
		o->type(1);
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
		o->minimum(0);
		o->maximum(7);
		o->step(1);
		o->value(0);
		o->textsize(8);
	  //  o->callback((Fl_Callback*)voiceparmCallback,soundchoice[0]);
	  }
	  { Fl_Counter* o = new Fl_Counter(844, 181, 115, 14, "midichannel");
		o->type(1);
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
		o->minimum(1);
		o->maximum(16);
		o->step(1);
		o->value(1);
		o->textsize(8);
	  }*/
	  { Fl_Dial* o = new Fl_Dial(844, 120, 25, 25, "id vol");
		o->labelsize(8); 
		o->argument(101);
		o->minimum(0);
		o->maximum(2);
		o->color(fl_rgb_color(190,160,255));
		o->value(0);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(874, 120, 25, 25, "aux 1");
		o->labelsize(8); 
		o->argument(108);
		o->minimum(0);
		o->maximum(2);
		o->color(fl_rgb_color(140,140,255));
		o->value(0);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(904, 120, 25, 25, "aux 2");
		o->labelsize(8); 
		o->argument(109);
		o->minimum(0);
		o->color(fl_rgb_color(140,140,255));
		o->maximum(2);
		o->value(0);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(934, 120, 25, 25, "mix vol");
		o->labelsize(8); 
		o->argument(106);
		o->minimum(0);
		o->maximum(2);
		o->color(fl_rgb_color(170,140,255));
		o->value(1);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Slider* o = new Fl_Slider(864, 160, 80, 10, "mix pan");
		o->labelsize(8); 
		o->box(FL_BORDER_BOX);
		o->argument(107);
		o->minimum(0);
		o->maximum(1);
		o->color(fl_rgb_color(170,140,255));
		o->value(0.5);
		o->type(FL_HORIZONTAL);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* j = new Fl_Choice(844, 190, 85, 15, "pan modulator");
		j->box(FL_BORDER_BOX);
		j->down_box(FL_BORDER_BOX);
		j->labelsize(8);
		j->textsize(8);
		j->align(FL_ALIGN_TOP_LEFT);
		j->argument(16);
		auswahl[i][j->argument()] = j;
		j->callback((Fl_Callback*)choiceCallback);
		j->menu(menu_amod);
	  }
	  { Fl_Dial* o = new Fl_Dial(934, 185, 25, 25, "amount");
		o->labelsize(8);
		o->argument(122);
		o->minimum(-1);
		o->maximum(1);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  o->end();
	}
	{ Fl_Group* o = new Fl_Group(825, 238, 160, 149);
	  o->box(FL_ROUNDED_FRAME);
	  o->color(FL_BACKGROUND2_COLOR);
	{ Fl_Box* d = new Fl_Box(825, 307, 160, 135, "delay");
	  d->labelsize(8);
	  d->labelcolor(FL_BACKGROUND2_COLOR);
	}
	  { Fl_Dial* o = new Fl_Dial(934, 260, 25, 25, "amount");
		o->labelsize(8);
		o->argument(110);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Choice* o = new Fl_Choice(844, 265, 85, 15, "time modulator");
		o->box(FL_BORDER_BOX);
		o->down_box(FL_BORDER_BOX);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		o->menu(menu_amod);
		o->argument(14);
		o->callback((Fl_Callback*)choiceCallback);
		auswahl[i][o->argument()]=o;
	  }
	  { Fl_Dial* o = new Fl_Dial(844, 293, 25, 25, "delay time");
		o->labelsize(8);
		o->argument(111);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
		{ Fl_Value_Input* o = new Fl_Value_Input(889, 298, 38, 15, "BPM");// BPM display for delay time
		  o->box(FL_ROUNDED_BOX);
		  o->labelsize(8);
		  o->align(FL_ALIGN_RIGHT);
		  o->maximum(10000);
		  o->step(0.01);
		  o->textsize(8);
		  o->callback((Fl_Callback*)BPMCallback2);
		  o->argument(124);
		  miniDisplay[i][12]=o;
		}
	  { Fl_Dial* o = new Fl_Dial(874, 330, 25, 25, "feedback");
		o->labelsize(8);
		o->argument(112);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(934, 330, 25, 25, "volume");
		o->labelsize(8);
		o->argument(113);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	  o->end();
	}
	{ Fl_Dial* o = new Fl_Dial(295, 151, 25, 25, "osc1  vol");
	  o->labelsize(8);
	  o->align(FL_ALIGN_TOP);
		o->argument(14);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(295, 252, 25, 25, "osc2  vol");
		o->labelsize(8);
		o->argument(29);
		o->callback((Fl_Callback*)parmCallback);Knob[i][o->argument()] = o;
	  }
	  { Fl_Dial* o = new Fl_Dial(950, 221, 25, 25, "to delay");
		o->labelsize(8);
		o->argument(114);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	  }
	{ Fl_Dial* o = new Fl_Dial(52, 221, 25, 25, "glide");
		o->labelsize(8);
		o->argument(116);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
	// Mult buttons OSC 1
	{ Fl_Light_Button* o = new Fl_Light_Button(80, 64, 40, 15, "mult.");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(8);
		o->argument(117);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
	{ Fl_Light_Button* o = new Fl_Light_Button(80, 138, 40, 15, "mult.");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(8);
		o->argument(118);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
	// Mult button OSC 2
	{ Fl_Light_Button* o = new Fl_Light_Button(80, 288, 40, 15, "mult.");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(8);
		o->argument(119);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
	// Osc 2 amp mod 2 is for FM only, mult. wouldn't make sense
	// Mult button morph
	{ Fl_Light_Button* o = new Fl_Light_Button(492, 397, 40, 15, "mult.");
		o->box(FL_BORDER_BOX);
		o->selection_color((Fl_Color)89);
		o->labelsize(8);
		o->argument(120);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
	{ Fl_Dial* o = new Fl_Dial(295, 191, 25, 25, "sub vol");
		o->labelsize(8);
		o->argument(121);
		o->align(FL_ALIGN_TOP);
		o->callback((Fl_Callback*)parmCallback);
		Knob[i][o->argument()] = o;
	}
   	{ Fl_Value_Input* o = new Fl_Value_Input(760, 465, 45, 14, "note");
	  o->box(FL_ROUNDED_BOX);
	  o->labelsize(8);
	  o->textsize(8);
	  o->range(0,127);
	  o->align(FL_ALIGN_TOP_LEFT);
	  o->step(1);
	  o->value(63);
	  // o->callback((Fl_Callback*)parmCallback);
	  o->argument(125);
	  Knob[i][o->argument()] = o;
	  note_number = o;
	}
	{ Fl_Value_Input* o = new Fl_Value_Input(815, 465, 45, 14, "velocity");
	  o->box(FL_ROUNDED_BOX);
	  o->labelsize(8);
	  o->textsize(8);
	  o->range(0,127);
	  o->align(FL_ALIGN_TOP_LEFT);
	  o->step(1);
	  o->value(80);
	  // o->callback((Fl_Callback*)parmCallback);
	  o->argument(126);
	  Knob[i][o->argument()] = o;
	  note_velocity = o;
	}

	o->end(); 
	tab[i]=o;
	} // ==================================== end single tab

	} // end of for

		{ 
		tablabel[i]="about";
		Fl_Group* o = new Fl_Group(1, 10, 995, 515, tablabel[i].c_str());
		o->color((Fl_Color)_BGCOLOR);
		o->labelsize(8);
		// o->callback((Fl_Callback*)tabCallback,&xtab);  
		o->box(FL_BORDER_FRAME);
	// draw logo
	{ Fl_Box* o = new Fl_Box(855, 450, 25, 25);
	  o->image(image_miniMini2);
	}
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

	// Sound indicators after everything else, will appear on top
	{ for (int j=0; j<_MULTITEMP; j++)
		{
		Fl_Box* o = new Fl_Box(930+6*(j/4), 425+5*(j%4)+(j/4), 4, 4);
		o->box(FL_RFLAT_BOX);
		o->color(FL_DARK3, FL_DARK3); 
		sounding[j]=o;
		}
	}

	tabs = o;
	}
// ---------------------------------------------------------------- end of tabs
	/*{ Fl_Chart * o = new Fl_Chart(600, 300, 70, 70, "eg");
		o->bounds(0.0,1.0);
		o->type(Fl::LINE_CHART);
		o->insert(0, 0.5, NULL, 0);
		o->insert(1, 0.5, NULL, 0);
		o->insert(2, 1, NULL, 0);
		o->insert(3, 0.5, NULL, 0);
		EG[0]=o;
		
	}*/

// ----------------------------------------- Multi
	  { Fl_Input* o = new Fl_Input(10, 476, 106, 14, "Multi");
		o->box(FL_BORDER_BOX);
		//o->color(FL_FOREGROUND_COLOR);
		o->labelsize(8);
		o->textsize(8);
		o->align(FL_ALIGN_TOP_LEFT);
		//o->callback((Fl_Callback*)changemulti,NULL);
		//o->callback((Fl_Callback*)chooseMultiCallback,NULL); // for the roller
		o->tooltip("enter name for multisetup before storing it");
		multichoice = o;
		Multichoice = o;
	   
	  }
	  // roller for the multis:
	  { Fl_Roller* o = new Fl_Roller(20, 492, 80, 10);
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
	  { Fl_Value_Output* o = new Fl_Value_Output(180, 488, 20, 15,"multiset");
		o->box(FL_ROUNDED_BOX);
		o->color(FL_BLACK);
		o->labelsize(8);
		o->maximum(512);
		o->textsize(8);
		o->textcolor(FL_RED);
		multiDisplay=o;
	  }
	  { Fl_Button* o = new Fl_Button(206, 465, 55, 19, "store multi");
		o->tooltip("overwrite this multi");
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR2);
		o->callback((Fl_Callback*)storemulti,multichoice);
		sm = o;
	  }
	  { Fl_Button* o = new Fl_Button(126, 465, 70, 19, "load multi");
		o->tooltip("load current multi");
		o->box(FL_BORDER_BOX);
		o->labelsize(8);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->callback((Fl_Callback*)loadmulti,multichoice);
		lm = o;
	  }
	  // parameter tuning
	{ Fl_Value_Input* o = new Fl_Value_Input(844, 397, 106, 14, "current parameter");
	  //o->box(FL_BORDER_FRAME);
	  o->box(FL_ROUNDED_BOX);
	  //o->color(FL_FOREGROUND_COLOR);
	  //o->selection_color(FL_FOREGROUND_COLOR);
	  o->labelsize(8);
	  o->textsize(8);
	  o->range(-2,2);
	  //o->menubutton()->textsize(8);
	  o->align(FL_ALIGN_TOP_LEFT);
	  o->step(0.0001);
	  o->callback((Fl_Callback*)finetune);
	  paramon = o;
	}
	{ Fl_Toggle_Button* o = new Fl_Toggle_Button(700, 465, 50, 39, "Audition");
		// These borders won't prevent look change on focus
		o->box(FL_BORDER_BOX);
		// o->downbox(FL_BORDER_BOX);
		o->labelsize(8);
		o->labelcolor((Fl_Color)_BTNLBLCOLOR1);
		o->color(FL_LIGHT1, FL_RED);
		o->callback((Fl_Callback*)do_audition);
		audition = o;
	}
	/* parameters here would be common to all tabs
	{ Fl_Value_Input* o = new Fl_Value_Input(760, 465, 45, 14, "note");
	  o->box(FL_ROUNDED_BOX);
	  o->labelsize(8);
	  o->textsize(8);
	  o->range(0,127);
	  o->align(FL_ALIGN_TOP_LEFT);
	  o->step(1);
	  o->value(63);
	  // o->callback((Fl_Callback*)parmCallback);
	  o->argument(125);
	  note_number = o;
	}
	{ Fl_Value_Input* o = new Fl_Value_Input(815, 465, 45, 14, "velocity");
	  o->box(FL_ROUNDED_BOX);
	  o->labelsize(8);
	  o->textsize(8);
	  o->range(0,127);
	  o->align(FL_ALIGN_TOP_LEFT);
	  o->step(1);
	  o->value(80);
	  o->argument(126);
	  // o->callback((Fl_Callback*)parmCallback);
	  note_velocity = o;
	}
	*/
	o->end();
	// too early, multi not loaded, loadmulti segfaults
	// multichoice->value(Speicher.multis[1].name);
	// multiRoller->value(0);
	// multiDisplay->value(1);
	// loadmulti(NULL, NULL);
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
}
/**
 * using the destructor to shutdown synthcore
 */
Fenster::~Fenster()
{
	
	printf("guittt");
	fflush(stdout);
	if (transmit) lo_send(t, "/Minicomputer/close", "i",1);
	//~Fl_Double_Window();
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
				case FL_Escape:
			all_off(NULL, NULL);
		break;
				// case 32: // Space bar - Doesn't work
				case FL_Insert:
			audition->value(!audition->value());
			do_audition(audition, NULL);
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

