#ifndef MiniTable_H
#define MiniTable_H
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
#include <FL/Fl_Table_Row.H>

class MiniTable : public Fl_Table_Row
{
protected:
	int _selected_row;
	int _selected_col;
	int _copied_row;
	int _copied_col;
	// int _copied_cell;
	bool _cell_is_cut;
	bool _odd_cols_only;
	bool _cols_first;
//	static void event_callback(Fl_Widget*, void*);
//	void event_callback2();	// callback for table events

public:
	MiniTable(int x, int y, int w, int h, const char *l=0) : Fl_Table_Row(x,y,w,h,l)
	{
		_odd_cols_only=true; // False not tested TODO accessor
		_cols_first=true; // Cell numbering is vertical first TODO accessor; False not tested
		_selected_row=0;
		_selected_col=_odd_cols_only?1:0;
		// _copied_cell=-2; // Still negative after halving means none
		_copied_row=-1;
		_copied_col=-1;
		_cell_is_cut=false;
		end(); // Fl_Table derives from Fl_Group, so end() it
	}
	~MiniTable() { }
	int handle(int e);
	void resize_cols(int W); // To be defined in GUI
	int selected_row() const ;
	int selected_col() const ;
	int copied_cell();
	void copied_cell(int c);
	void set_cell_is_cut();
	void clear_cell_is_cut();
	bool cell_is_cut() const;
	int selected_cell();
	int copied_row() const ;
	int copied_col() const ;
	int index(const unsigned int R, const unsigned int C);
	int selected_index();
	int copied_index();
	void copy();
	void cut();
};
#endif
