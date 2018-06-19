#ifndef MiniPositioner_H
#define MiniPositioner_H
/*! \file MiniPositioner.H
 *  \brief MiniPositioner class declaration
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
#include <FL/Fl.H>
#include <Fl/fl_draw.H>
#include <FL/Fl_Positioner.H>

/**
 * \brief a replacement for Fl_Positioner
 * 
 * A version of Fl_Positioner with focus and keyboard events handling
 */
class MiniPositioner : public Fl_Positioner
{
protected:
/*
	static void event_callback(Fl_Widget*, void*);
	void event_callback2();	// callback for table events
*/
private:
	void draw(int X, int Y, int W, int H);
	void draw();

	int handle(int event);
public:
	MiniPositioner(int x, int y, int w, int h, const char *l=0) : Fl_Positioner(x,y,w,h,l)
	{
//		callback(&event_callback, (void*)this);
//		end(); // ??
	}
	~MiniPositioner() { }
};
#endif
