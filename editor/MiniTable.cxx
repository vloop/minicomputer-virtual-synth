#include "MiniTable.H"

// Keyboard and mouse events
int MiniTable::handle(int e) {
	// printf("MiniTable::handle(%u)\n", e);
	int ret=1; // Optimistic, we'll handle it!
	switch (e) {
		case FL_KEYDOWN:
			// printf("MiniTable::handle FL_KEYDOWN\n");
			// Handle selected cell
			switch (Fl::event_key()) {
				case FL_Down:
					_selected_row++;
					if(_selected_row>=rows()) _selected_row=rows()-1;
					break;
				case FL_Up:
					_selected_row--;
					if(_selected_row<0) _selected_row=0;
					break;
				case FL_Tab:
					if (Fl::event_shift ()){
						_selected_col--;
						if(_odd_cols_only && (_selected_col&1)==0) _selected_col--;
						if(_selected_col<0){
							_selected_row--;
							if(_selected_row<0) {
								_selected_row=0;
								_selected_col=0;
								if(_odd_cols_only) _selected_col|=1;
							}else{
								_selected_col=cols()-1;
								if(_odd_cols_only && (_selected_col&1)==0) _selected_col--;
							}
						}
					}else{
						_selected_col++;
						if(_odd_cols_only) _selected_col|=1;
						if(_selected_col>=cols()){
							_selected_row++;
							if(_selected_row>=rows()){
								_selected_row=rows()-1;
								_selected_col=cols()-1;
								if(_odd_cols_only && (_selected_col&1)==0) _selected_col--;
							}else{
								_selected_col=0;
								if(_odd_cols_only) _selected_col|=1;
							}
						}
					}
					break;
				case FL_Right:
					_selected_col++;
					if(_odd_cols_only && (_selected_col&1)==0) _selected_col++;
					if(_selected_col>=cols()){
						_selected_col=cols();
						if(_odd_cols_only && (_selected_col&1)==0) _selected_col--;
					}
					break;
				case FL_Left:
					_selected_col--;
					if(_odd_cols_only && (_selected_col&1)==0) _selected_col--;
					if(_selected_col<0) _selected_col=_odd_cols_only?1:0;
					break;
				case FL_Home:
					_selected_row=0;
					_selected_col=_odd_cols_only?1:0;
					break;
				case FL_End:
					_selected_row=rows()-1;
					_selected_col=cols()-1;
					if(_odd_cols_only && (_selected_col&1)==0) _selected_col--;
					break;
				// we need a when(FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED) to propagate enter to derived class
				case FL_Enter:
					// printf("MiniTable::handle FL_Enter\n");
					Fl_Table_Row::handle(e);
					return 0; // unsure??
					break;
				default:
					// Leave Fn keys to parent window
					if(Fl::event_key()>=FL_F+1 && Fl::event_key()<=FL_F+12) return 0; // Not handled
					return Fl_Table_Row::handle(e);
			}
			// Common to movement keys
			// Handle scrolling
			int r1, r2, c1, c2;
			visible_cells(r1, r2, c1, c2);
			// printf("row %u %u..%u\n", _selected_row, r1, r2);
			if(_selected_row<=r1) row_position (_selected_row);
			// Fl_Table scrolls down on page before we get the event ??
			// Still needed for FL_End
			else if(_selected_row>=r2) row_position(_selected_row-r2+r1+1);
			
			// Propagate event
			Fl_Table_Row::handle(e); // Give children a chance to update name display
			redraw();
			ret = 1; // Handled
			break;
		case FL_KEYUP:
		case FL_PUSH:
		case FL_RELEASE:
		case FL_DRAG:
			// ret = 1;		// *don't* indicate we 'handled' these, just update ('handling' prevents e.g. tab nav)
			// printf("MiniTable::handle\n");
			ret = Fl_Table_Row::handle(e);
			redraw();
			break;
		case FL_FOCUS:		// tells FLTK we're interested in keyboard events
		case FL_UNFOCUS:
			ret = 1;
			break;
		/* Not working
		case FL_SHOW:
			printf("MiniTable::handle(%u) FL_SHOW\n", e);
			ret = 0;
			Fl_Table_Row::handle(e); // Give children a chance to update name display
			break;
		*/
		default:
			ret = Fl_Table_Row::handle(e);
	} // end of switch
return(ret);
} // end of handle
