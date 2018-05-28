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
		_odd_cols_only=true; // False not tested
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
	int selected_row(){return _selected_row;}
	int selected_col(){return _selected_col;}
	int copied_cell(){return _copied_cell;}
	void copied_cell(int c){_copied_cell=c;}
	void set_cell_is_cut(){_cell_is_cut=true;}
	void clear_cell_is_cut(){_cell_is_cut=false;}
	bool cell_is_cut(){return _cell_is_cut;}
	int selected_cell(){return _cols_first ? _selected_col*rows()+_selected_row : _selected_row*cols()+_selected_col;}
	int copied_row(){return _cols_first ? _copied_cell % rows() : _copied_cell / cols();}
	int copied_col(){return _cols_first ? _copied_cell / rows() : _copied_cell % cols();}
	int selected_index(){
		return _odd_cols_only ?
			(_cols_first ? _selected_row+rows()*(_selected_col/2) : _selected_row*cols()+(_selected_col/2))
			:selected_cell();
	}
	int copied_index(){
		return _odd_cols_only ?
			(_cols_first ?
				_copied_cell % rows() + rows() * ((_copied_cell / rows()) / 2) : 
				(_copied_cell / cols()) * cols() + ((_copied_cell % cols()) / 2)
			):
			_copied_cell;
	}
};
#endif
