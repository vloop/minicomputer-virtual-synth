/*! \file MiniButton.H
 *  \brief MiniButton class declaration
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

#ifndef MiniButton_H
#define MiniButton_H
#include <FL/Fl.H>
#include <FL/Fl_Button.H>

// See http://www.labri.fr/perso/fleury/posts/programming/a-quick-gettext-tutorial.html
#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)

/**
 * \brief a replacement for Fl_Button
 */
class MiniButton : public Fl_Button {
	public:
	MiniButton(int xx,int yy,int ww,int hh,const char *l=0);
	~MiniButton();
	int handle(int event);

	protected:
	void draw();
	static Fl_Widget_Tracker *key_release_tracker;
	static void key_release_timeout(void*);
	void simulate_key_action();


	private:
	void shadow(const int offs,const uchar r,uchar g,uchar b);
	void draw_focus(Fl_Boxtype B, int X, int Y, int W, int H) const;
	void draw_focus() {draw_focus(box(),x(),y(),w(),h());}
	int _bgcolor;
	static int _defaultcolor;
	static int _defaultselectioncolor;
	static int _defaultbgcolor;
	int _bevel;
	static int _defaultbevel;
	int _shadow;
	static int _defaultshadow;
	char oldval;
	char value_;


};
#endif

