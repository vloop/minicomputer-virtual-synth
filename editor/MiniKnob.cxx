// Adapted from http://membres-timc.imag.fr/Yves.Usson/personnel/mysofts.html

#include "MiniKnob.H"
#include <Fl/fl_draw.H>
#include <math.h>

MiniKnob::MiniKnob(int xx,int yy,int ww,int hh,const char *l): Fl_Valuator(xx,yy,ww,hh,l) {
	a1 = 35;
	a2 = 325;
	_bgcolor = _defaultbgcolor;
	_type = DOTLIN;
	_percent = 0.3;
	_scaleticks = 0; // 10
	_dragtype = _defaultdragtype;
	_margin = _defaultmargin;
	_bevel = _defaultbevel;
	color(_defaultcolor); // 37
	selection_color(_defaultselectioncolor); // 7
}

MiniKnob::~MiniKnob() {
}

void MiniKnob::draw() {
	int ox,oy,ww,hh,side;
	// float sa,ca,a_r;
	unsigned char rr,gg,bb;

	ox = x();
	oy = y();
	ww = w();
	hh = h();
	draw_label();
	fl_clip(ox,oy,ww,hh);
	if (ww > hh)
	{
		side = hh;
		ox = ox + (ww - side) / 2; 
	}
	else
	{
		side = ww;
		oy = oy + (hh - side) / 2; 
	}
	side = w() > h () ? hh:ww;
	int dam = damage();
	int m=margin();
	int b=bevel();
	if (dam & FL_DAMAGE_ALL)
	{
		int col = _bgcolor; // parent()->color();
		fl_color(col);
		fl_rectf(ox,oy,side,side);
		// The actual shadow
		Fl::get_color((Fl_Color)col,rr,gg,bb);
		shadow(-60,rr,gg,bb);
		// fl_pie(ox+(3*m)/2,oy+(3*m)/2,side-2*m,side-2*m,0,360);
		fl_pie(ox+2*m,oy+2*m,side-2*m,side-2*m,0,360);
		
		draw_scale(ox,oy,side);
		
		col = color();
		Fl::get_color((Fl_Color)col,rr,gg,bb);

		// 6 shades for bevel on each side
		if(b){
			shadow(7,rr,gg,bb);
			fl_pie(ox+m,oy+m,side-2*m,side-2*m,40,50);
			fl_pie(ox+m,oy+m,side-2*m,side-2*m,260,270);

			shadow(15,rr,gg,bb);
			fl_pie(ox+m,oy+m,side-2*m,side-2*m,50,70);
			fl_pie(ox+m,oy+m,side-2*m,side-2*m,230,260);

			shadow(25,rr,gg,bb);
			fl_pie(ox+m,oy+m,side-2*m,side-2*m,70,80);
			fl_pie(ox+m,oy+m,side-2*m,side-2*m,220,230);

			shadow(30,rr,gg,bb);
			fl_pie(ox+m,oy+m,side-2*m,side-2*m,80,220);

			shadow(-7,rr,gg,bb);
			fl_pie(ox+m,oy+m,side-2*m,side-2*m,30,40);
			fl_pie(ox+m,oy+m,side-2*m,side-2*m,270,280);

			shadow(-15,rr,gg,bb);
			fl_pie(ox+m,oy+m,side-2*m,side-2*m,280,400);
			shadow(-25,rr,gg,bb);
			fl_pie(ox+m,oy+m,side-2*m,side-2*m,290,390);
		}

		// Outline
		fl_color(FL_BLACK);
		fl_arc(ox+m,oy+m,side-2*m,side-2*m,0,360);
		// Top
		fl_color(col);
		// fl_pie(ox+m+b,oy+m+b,side-2*(m+b),side-2*(m+b),0,360); // Centered
		fl_pie(ox+m+b+1,oy+m+b+1,side-2*(m+b)-1,side-2*(m+b)-1,0,360);
	}
	else
	{
		fl_color(color());
		fl_pie(ox+2*m-2,oy+2*m-2,side-4*m+4,side-4*m+4,0,360);
	}
	// 3 reflections on each side
	Fl::get_color((Fl_Color)color(),rr,gg,bb);
	shadow(10,rr,gg,bb);
	// fl_pie(ox+2*m-2,oy+2*m-2,side-4*m+4,side-4*m+4,110,150);
	fl_pie(ox+m+b,oy+m+b,side-2*(m+b),side-2*(m+b),110,150);
	fl_pie(ox+m+b,oy+m+b,side-2*(m+b),side-2*(m+b),290,330);
	shadow(17,rr,gg,bb);
	fl_pie(ox+m+b,oy+m+b,side-2*(m+b),side-2*(m+b),120,140);
	fl_pie(ox+m+b,oy+m+b,side-2*(m+b),side-2*(m+b),300,320);
	shadow(25,rr,gg,bb);
	fl_pie(ox+m+b,oy+m+b,side-2*(m+b),side-2*(m+b),127,133);
	fl_pie(ox+m+b,oy+m+b,side-2*(m+b),side-2*(m+b),307,313);
	draw_cursor(ox,oy,side);
	// Focus
	if (Fl::focus() == this){
		fl_line_style(FL_DOT, 1, 0);
		fl_arc(ox+m+2,oy+m+2,side-2*m-4,side-2*m-4,0,360);
		fl_line_style(0);
	}
	fl_pop_clip();
}

int MiniKnob::handle(int  event) {
	int ox,oy,ww,hh;
	int delta;
	int mx = Fl::event_x_root();
	int my = Fl::event_y_root();
	static int ix, iy, drag;
	double val, minval, maxval, step;

	ox = x() + 10; oy = y() + 10;
	ww = w() - 20;
	hh = h()-20;
	val=this->value();
	minval=this->minimum();
	maxval=this->maximum();
	step=this->step();
	if (minval>maxval){
		minval=maxval;
		maxval=this->minimum();
	}
	if (step<=0.0f) step=(maxval-minval)/100.0f;
	switch (event) 
	{
		// case FL_KEYBOARD: // Not working unless focused ?
		case FL_KEYUP: // It seems every knob is getting that until handled ?
			if (Fl::focus() == this) {
				// printf("key %u on %lu\n", Fl::event_key (), this->argument());
				// printf("min %f max %f step %f\n", minval, maxval, step);
				switch(Fl::event_key ()){
					case FL_Home:
						this->value(this->minimum());
						break;
					case FL_End:
						this->value(this->maximum());
						break;
					case FL_KP+'+':
						val+=step;
						if (val>maxval) val=maxval;
						this->value(val);
						break;
					case FL_KP+'-':
						val-=step;
						if (val<minval) val=minval;
						this->value(val);
						break;
					case FL_KP+'.':
						val=0;
						if (val>maxval) val=maxval;
						if (val<minval) val=minval;
						this->value(val);
						break;
					case FL_Page_Up:
						val+=step*10;
						if (val>maxval) val=maxval;
						this->value(val);
						break;
					case FL_Page_Down:
						val-=step*10;
						if (val<minval) val=minval;
						this->value(val);
						break;
					default:
						return 0;
				}
				this->callback()((Fl_Widget*)this, 0);
			}else{
				return 0;
				// printf("key %u on %lu (not focused)\n", Fl::event_key (), this->argument());
			}
			return 1;
		case FL_FOCUS:
			// printf("Focus %lu!\n", this->argument());
			Fl::focus(this);
			this->callback()((Fl_Widget*)this, 0);
			if (visible()) damage(FL_DAMAGE_ALL);
			return 1;
		case FL_UNFOCUS:
			// printf("Unfocus %lu!\n", this->argument());
			this->callback()((Fl_Widget*)this, 0);
			if (visible()) damage(FL_DAMAGE_ALL);
			return 1;
		case FL_ACTIVATE: // Not needed?
			// printf("Activate %lu!\n", this->argument());
			if (visible()) damage(FL_DAMAGE_ALL);
			return 1;
		case FL_ENTER: // Needed for tooltip
			// printf("FL_ENTER\n");
			return 1;
		case FL_LEAVE: // Needed for tooltip
			// printf("FL_LEAVE\n");
			return 1;
		case FL_MOUSEWHEEL:
			// printf("FL_MOUSEWHEEL %d\n",Fl::event_dy());
			val+=step*Fl::event_dy();
			if (val>maxval) val=maxval;
			if (val<minval) val=minval;
			this->value(val);
			this->callback()((Fl_Widget*)this, 0);
			return 1;
		case FL_PUSH:
			if(Fl::event_clicks()){ // Double click or more
				val=0;
				if (val>maxval) val=maxval;
				if (val<minval) val=minval;
				this->value(val);
			}
			if (Fl::visible_focus()) handle(FL_FOCUS);
			ix = mx;
			iy = my;
			drag = Fl::event_button();
			handle_push();
			// Originally fall through ??
			return 1;
		case FL_DRAG:
			{
				switch (_dragtype)
				{
					case MiniKnob::HORIZONTAL:
						delta = mx-ix;
						if (delta > 5) delta -= 5;
						else if (delta < -5) delta += 5;
						else delta = 0;
						switch (drag) {
							case 3: val = increment(previous_value(), delta*100); break;
							case 2: val = increment(previous_value(), delta*10); break;
							default:val = increment(previous_value(), delta); break;
						}
						break;
					case MiniKnob::VERTICAL:
						delta = my-iy;
						if (delta > 5) delta -= 5;
						else if (delta < -5) delta += 5;
						else delta = 0;
						switch (drag) {
							case 3: val = increment(previous_value(), -delta*100); break;
							case 2: val = increment(previous_value(), -delta*10); break;
							default:val = increment(previous_value(), -delta); break;
						}
						break;
					default:
						int mx = Fl::event_x()-ox-ww/2;
						int my = Fl::event_y()-oy-hh/2;
						if (!mx && !my) return 1;
						double angle = 270-atan2((float)-my, (float)mx)*180/M_PI;
						double oldangle = (a2-a1)*(value()-minimum())/(maximum()-minimum()) + a1;
						while (angle < oldangle-180) angle += 360;
						while (angle > oldangle+180) angle -= 360;
						if ((a1<a2) ? (angle <= a1) : (angle >= a1))
						{
							val = minimum();
						} 
						else 
						if ((a1<a2) ? (angle >= a2) : (angle <= a2)) 
						{
							val = maximum();
						} 
						else
						{
							val = minimum() + (maximum()-minimum())*(angle-a1)/(a2-a1);
						}
						break;
				}
				// val = round(val);
				// handle_drag(soft()?softclamp(val):clamp(val));;
				handle_drag(clamp(round(val)));
			} 
			return 1;
		case FL_RELEASE:
			handle_release();
			return 1;
		default:
			return 0;
	}
	return 0;
}

void MiniKnob::type(int ty) {
	_type = ty;
}

void MiniKnob::shadow(const int offs,const uchar r,uchar g,uchar b) {
	int rr,gg,bb;

	rr = r + offs; 
	rr = rr > 255 ? 255:rr;
	rr = rr < 0 ? 0:rr;
	gg = g + offs; 
	gg = gg > 255 ? 255:gg;
	gg = gg < 0 ? 0:gg;
	bb = b + offs; 
	bb = bb > 255 ? 255:bb;
	bb = bb < 0 ? 0:bb;
	fl_color((uchar)rr,(uchar)gg,(uchar)bb);
}

void MiniKnob::draw_scale(const int ox,const int oy,const int side) {
  float x1,y1,x2,y2,rds,cx,cy,ca,sa;
// float minv,maxv,curv;
	int m=margin();
	rds = side / 2;
	cx = ox + side / 2;
	cy = oy + side / 2;
	if (!(_type & DOTLOG_3)) // Must be xxxLIN
	{
		if (_scaleticks == 0) return;
		double a_step = (10.0*3.14159/6.0) / _scaleticks;
		double a_orig = -(3.14159/3.0);
		for (int a = 0; a <= _scaleticks; a++)
		{
			double na = a_orig + a * a_step;
			ca = cos(na);
			sa = sin(na);
			x1 = cx + rds * ca;
			y1 = cy - rds * sa;
			x2 = cx + (rds-m) * ca;
			y2 = cy - (rds-m) * sa;
			fl_color(FL_BLACK);
			fl_line(x1,y1,x2,y2);
			fl_color(FL_WHITE);
			if (sa*ca >=0)
				fl_line(x1+1,y1+1,x2+1,y2+1);
			else
				fl_line(x1+1,y1-1,x2+1,y2-1);
		}
	}
	else // xxxLOG_n
	{
		int nb_dec = (_type & DOTLOG_3);
		for (int k = 0; k < nb_dec; k++)
		{
			double a_step = (10.0*3.14159/6.0) / nb_dec;
			double a_orig = -(3.14159/3.0) + k * a_step;
			for (int a = (k) ? 2:1; a <= 10; )
			{
				double na = a_orig + log10((double)a) * a_step;
				ca = cos(na);
				sa = sin(na);
				x1 = cx - rds * ca;
				y1 = cy - rds * sa;
				x2 = cx - (rds-m) * ca;
				y2 = cy - (rds-m) * sa;
				fl_color(FL_BLACK);
				fl_line(x1,y1,x2,y2);
				fl_color(FL_WHITE);
				if (sa*ca <0)
					fl_line(x1+1,y1+1,x2+1,y2+1);
				else
					fl_line(x1+1,y1-1,x2+1,y2-1);
				if ((a == 1) || (nb_dec == 1))
					a += 1;
				else
					a += 2;
			}
		}
	}
}

void MiniKnob::draw_cursor(const int ox,const int oy,const int side) {
	float rds,cur,cx,cy;
	double angle;
	int m=margin();
	rds = (side - 3*m) / 2.0; // 20
	cur = _percent * rds / 2;
	cx = ox + side / 2;
	cy = oy + side / 2;
	angle = (a2-a1)*(value()-minimum())/(maximum()-minimum()) + a1;	
	fl_push_matrix();
	fl_scale(1,1);
	fl_translate(cx,cy);
	fl_rotate(-angle);
	fl_translate(0,rds-cur-2.0);
	if (_type<LINELIN)
	{
		fl_begin_polygon();
		fl_color(selection_color());
		fl_circle(0.0,0.0,cur);
		fl_end_polygon();

		fl_begin_loop();
		fl_color(FL_WHITE);
		fl_circle(0.0,0.0,cur);
		fl_end_loop();
	}
	else
	{
		fl_begin_polygon();
		fl_color(selection_color());
		fl_vertex(-1.5,-cur);
		fl_vertex(-1.5,cur);
		fl_vertex(1.5,cur);
		fl_vertex(1.5,-cur);
		fl_end_polygon();
		fl_begin_loop();
		fl_color(FL_BLACK);
		fl_vertex(-1.5,-cur);
		fl_vertex(-1.5,cur);
		fl_vertex(1.5,cur);
		fl_vertex(1.5,-cur);
		fl_end_loop();
	}
	fl_pop_matrix();
}

void MiniKnob::cursor(const int pc) {
	_percent = (float)pc/100.0;

	if (_percent < 0.05) _percent = 0.05;
	if (_percent > 1.0) _percent = 1.0;
	if (visible()) damage(FL_DAMAGE_CHILD);
}

void MiniKnob::scaleticks(const int tck) {
	_scaleticks = tck;
	if (_scaleticks < 0) _scaleticks = 0;
	if (_scaleticks > 31) _scaleticks = 31;
	if (visible()) damage(FL_DAMAGE_ALL);
}

void MiniKnob::bgcolor(const int c) {
	_bgcolor=c;
	if (visible()) damage(FL_DAMAGE_ALL);
}
void MiniKnob::dragtype(const short c) {
	_dragtype=c;
}
/*
void MiniKnob::defaultbgcolor(const int c) {
	_defaultbgcolor=c;
}
void MiniKnob::defaultdragtype(const short c) {
	_defaultdragtype=c;
}
*/
int MiniKnob::margin(){ return _margin; }
void MiniKnob::margin(int m){ _margin=m; }
int MiniKnob::bevel(){ return _bevel; }
void MiniKnob::bevel(int b){ _bevel=b; }
