#ifndef MiniTable_H
#define MiniTable_H

#include <FL/Fl.H>
#include <FL/Fl_Table_Row.H>

class MiniTable : public Fl_Table_Row
{
protected:
	int _selected_row;
	int _selected_col;
	int _copied_cell;
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
		_copied_cell=-2; // Still negative after halving means none
		_cell_is_cut=false;
		end(); // Fl_Table derives from Fl_Group, so end() it
	}
	~MiniTable() { }
	int handle(int e);
	void resize_cols(int W); // To be defined in GUI
	int selected_row();
	int selected_col();
	int copied_cell();
	void copied_cell(int c);
	void set_cell_is_cut();
	void clear_cell_is_cut();
	bool cell_is_cut();
	int selected_cell();
	int copied_row();
	int copied_col();
	int index(const unsigned int R, const unsigned int C);
	int selected_index();
	int copied_index();
	void copy();
	void cut();
};
#endif
