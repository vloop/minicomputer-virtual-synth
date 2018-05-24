#ifndef MiniPositioner_H
#define MiniPositioner_H
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
