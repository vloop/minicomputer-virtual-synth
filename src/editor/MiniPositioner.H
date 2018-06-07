#ifndef MiniPositioner_H
#define MiniPositioner_H
/** Minicomputer
 * industrial grade digital synthesizer
 *
 * Copyright Marc Périlleux 2018
 * This file is part of Minicomputer, which is free software:
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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
