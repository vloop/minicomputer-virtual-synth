/*! \file MiniPositioner.cxx
 *  \brief replacement for Fl_Positioner
 * 
 * can take focus and handle keyboard events
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

#include "MiniPositioner.H"
void MiniPositioner::draw(int X, int Y, int W, int H){
//	printf("MiniPositioner::Draw!\n");
	Fl_Positioner::draw(X, Y, W, H);
	if (Fl::focus() == this){
//		printf("MiniPositioner::Focus!\n");
		// fl_line_style(FL_DOT, 1, 0);
		fl_color(FL_WHITE); //83 // selection_color());
		fl_xyline(X+1, Y+1, X+W-1);
		fl_xyline(X+1, Y+H-1, X+W-1);
		fl_yxline(X+1, Y+1, Y+H-1);
		fl_yxline(X+W-1, Y+1, Y+H-1);
		// fl_line_style(0);
	}
}
void MiniPositioner::draw() {
	draw(x(), y(), w(), h());
	draw_label();
}

int MiniPositioner::handle(int event){
//	printf("MiniPositioner::Handle!\n");
	double xval, xminval, xmaxval, xstep;
	double yval, yminval, ymaxval, ystep;
	xval=this->xvalue();
	xminval=this->xminimum();
	xmaxval=this->xmaximum();
	if (xminval>xmaxval){
		xminval=xmaxval;
		xmaxval=this->xminimum();
	}
	yval=this->yvalue();
	yminval=this->yminimum();
	ymaxval=this->ymaximum();
	if (yminval>ymaxval){
		yminval=ymaxval;
		ymaxval=this->yminimum();
	}
	ystep=(ymaxval-yminval)/100.0f;
	xstep=(ymaxval-yminval); // (xmaxval-xminval)/100.0f;
	// printf("key %u on %lu\n", Fl::event_key (), this->argument());
	// printf("xmin %f xmax %f xstep %f\n", xminval, xmaxval, xstep);
	// printf("ymin %f ymax %f ystep %f\n", yminval, ymaxval, ystep);
	switch (event) 
	{
		// case FL_KEYUP: // It seems every knob is getting that until handled ?
		case FL_KEYBOARD: // It seems every knob is getting that until handled ?
			if (Fl::focus() == this) {
				switch(Fl::event_key ()){
					case FL_Home:
						this->xvalue(xminval);
						this->yvalue(yminval);
						break;
					case FL_End:
						this->xvalue(xmaxval);
						this->yvalue(ymaxval);
						break;
					case FL_KP+'+':
					case FL_Right:
						xval+=xstep;
						if (xval>xmaxval) xval=xmaxval;
						this->xvalue(xval);
						break;
					case FL_KP+'-':
					case FL_Left:
						xval-=xstep;
						if (xval<xminval) xval=xminval;
						this->xvalue(xval);
						break;
					case FL_KP+'.':
						xval=0;
						if (xval>xmaxval) xval=xmaxval;
						if (xval<xminval) xval=xminval;
						this->xvalue(xval);
						yval=0;
						if (yval>ymaxval) yval=ymaxval;
						if (yval<yminval) yval=yminval;
						this->yvalue(yval);
						break;
					case FL_Page_Up:
						xval+=xstep*5;
						if (xval>xmaxval) xval=xmaxval;
						this->xvalue(xval);
						break;
					case FL_Page_Down:
						xval-=xstep*5;
						if (xval<xminval) xval=xminval;
						this->xvalue(xval);
						break;
					case FL_Up:
						yval+=ystep;
						if (yval>ymaxval) yval=ymaxval;
						this->yvalue(yval);
						break;
					case FL_Down:
						yval-=ystep;
						if (yval<yminval) yval=yminval;
						this->yvalue(yval);
						break;
					default:
						return Fl_Positioner::handle(event);
				}
				this->set_changed(); // TODO Not always changed
				this->callback()((Fl_Widget*)this, 0);
				return 1;
			}
			break;
		case FL_FOCUS:
//			printf("MiniPositioner::Focus %lu!\n", this->argument());
			Fl::focus(this);
			this->callback()((Fl_Widget*)this, 0);
			if (visible()) damage(FL_DAMAGE_ALL);
			return 1;
		case FL_UNFOCUS:
			// printf("MiniPositioner::Unfocus %lu!\n", this->argument());
			this->callback()((Fl_Widget*)this, 0);
			if (visible()) damage(FL_DAMAGE_ALL);
			return 1;
		case FL_PUSH:
			if(Fl::event_clicks()){ // Double click or more
				// printf("MiniPositioner::double click %lu!\n", this->argument());
				this->xvalue(xminval);
				this->yvalue(yminval);
				this->callback()((Fl_Widget*)this, 0);
				return 1; // Don't call base class handler !!
			}
			if (Fl::visible_focus()) handle(FL_FOCUS);
			return Fl_Positioner::handle(event);
		case FL_RELEASE:
			return 1; // Don't call base class handler !!
			// It would prevent double click from working
		case FL_ENTER: // Needed for tooltip
			// printf("FL_ENTER\n");
			return 1;
		case FL_LEAVE: // Needed for tooltip
			// printf("FL_LEAVE\n");
			return 1;
		case FL_MOUSEWHEEL:
			// printf("FL_MOUSEWHEEL %d\n",Fl::event_dy());
			yval-=ystep*Fl::event_dy();
			if (yval>ymaxval){
				yval=yminval;
				xval+=xstep;
				if (xval>xmaxval){
					xval=xmaxval;
					yval=ymaxval;
				}
			}
			if (yval<yminval){
				yval=ymaxval;
				xval-=xstep;
				if (xval<xminval){
					xval=xminval;
					yval=yminval;
				}
			}
			this->yvalue(yval);
			this->xvalue(xval);
			this->callback()((Fl_Widget*)this, 0);
			return 1;
	}
	return Fl_Positioner::handle(event);
}
