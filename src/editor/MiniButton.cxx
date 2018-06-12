// Adapted from http://membres-timc.imag.fr/Yves.Usson/personnel/mysofts.html
/** Minicomputer
 * industrial grade digital synthesizer
 *
 * Copyright  Marc Périlleux 2018
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

#include "MiniButton.H"
#include <Fl/fl_draw.H>

Fl_Widget_Tracker *MiniButton::key_release_tracker = 0;

MiniButton::MiniButton(int xx,int yy,int ww,int hh,const char *l): Fl_Button(xx,yy,ww,hh,l) {
	_bgcolor = _defaultbgcolor;
	_bevel = _defaultbevel;
	_shadow = _defaultshadow;
	color(_defaultcolor); // 37
	selection_color(_defaultselectioncolor); // 7
}

MiniButton::~MiniButton() {
}

// Should find a way to avoid duplicate code with MiniKnob
void MiniButton::shadow(const int offs,const uchar r,uchar g,uchar b) {
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

void MiniButton::draw() {
	if (type() == FL_HIDDEN_BUTTON) return;
	int d1=2;
	int d2=d1+d1;
	int ox,oy,ww,hh;
	unsigned char rr,gg,bb;

	ox = x();
	oy = y();
	ww = w();
	hh = h();

	// Fl_Color col = value() ? selection_color() : color();
	Fl_Color col = (Fl::focus() == this) ? selection_color() : color();
	// draw_box(value() ? (down_box()?down_box():fl_down(box())) : box(), col);
	fl_color(col);
	fl_rectf(ox+d1,oy,ww-d2,hh);
	fl_rectf(ox,oy+d1,d1,hh-d2);
	fl_rectf(ox+ww-d1,oy+d1,d1,hh-d2);
	Fl::get_color(col,rr,gg,bb);
	// Shadow
	if(_shadow){
		shadow(-120,rr,gg,bb);
		fl_xyline(ox+d1, oy+hh-1, ox+ww-1-d1);
		fl_yxline(ox+ww-1, oy+d1, oy+hh-1-d1);
		fl_line(ox+ww-1-d1,oy+hh-1,ox+ww-1,oy+hh-1-d1);
		fl_line(ox+ww-2-d1+1,oy+hh-2,ox+ww-2,oy+hh-2-d1+1);
		ww--; hh--;
	}
	// Frame
	shadow(-90,rr,gg,bb);
	fl_line(ox,oy+d1,ox+d1,oy);
	fl_line(ox+ww-1-d1,oy,ox+ww-1,oy+d1);
	fl_line(ox,oy+hh-1-d1,ox+d1,oy+hh-1);
	fl_line(ox+ww-1-d1,oy+hh-1,ox+ww-1,oy+hh-1-d1);
	fl_xyline(ox+d1, oy, ox+ww-1-d1);
	fl_xyline(ox+d1, oy+hh-1, ox+ww-1-d1);
	fl_yxline(ox, oy+d1, oy+hh-1-d1);
	fl_yxline(ox+ww-1, oy+d1, oy+hh-1-d1);
	// Fake anti-aliasing
	shadow(-30,rr,gg,bb);
	fl_line(ox,oy+d1-1,ox+d1-1,oy);
	fl_line(ox+ww-1-d1+1,oy,ox+ww-1,oy+d1-1);
	fl_line(ox,oy+hh-1-d1+1,ox+d1-1,oy+hh-1);
	if(_shadow==0)
		fl_line(ox+ww-1-d1+1,oy+hh-1,ox+ww-1,oy+hh-1-d1+1);

	// draw_backdrop();
	if (labeltype() == FL_NORMAL_LABEL && value()) {
		Fl_Color c = labelcolor();
		labelcolor(fl_contrast(c, col));
		draw_label();
		labelcolor(c);
	} else draw_label();
	/*
	if (Fl::focus() == this){ 
		MiniButton::draw_focus();
	}
	*/
}

// draw_focus is not virtual, cannot override here
void MiniButton::draw_focus(Fl_Boxtype B, int X, int Y, int W, int H) const {
  if (!Fl::visible_focus()) return;
  printf("draw focus\n");
  switch (B) {
    case FL_DOWN_BOX:
    case FL_DOWN_FRAME:
    case FL_THIN_DOWN_BOX:
    case FL_THIN_DOWN_FRAME:
      X ++;
      Y ++;
    default:
      break;
  }

  // fl_color(fl_contrast(FL_BLACK, color()));
  Fl_Color col = selection_color();
  fl_color(col);
  fl_rect(X + Fl::box_dx(B), Y + Fl::box_dy(B),
          W - Fl::box_dw(B) - 1, H - Fl::box_dh(B) - 1);
  fl_rect(X + Fl::box_dx(B)+2, Y + Fl::box_dy(B)+2,
          W - Fl::box_dw(B) - 5, H - Fl::box_dh(B) - 5);

}

int MiniButton::handle(int event) {
  int newval;
  switch (event) {
  case FL_ENTER: /* FALLTHROUGH */
  case FL_LEAVE:
//  if ((value_?selection_color():color())==FL_GRAY) redraw();
    return 1;
  case FL_PUSH:
    if (Fl::visible_focus() && handle(FL_FOCUS)) Fl::focus(this);
    /* FALLTHROUGH */
  case FL_DRAG:
    if (Fl::event_inside(this)) {
      if (type() == FL_RADIO_BUTTON) newval = 1;
      else newval = !oldval;
    } else {
      clear_changed();
      newval = oldval;
    }
    if (newval != value_) {
      value_ = newval;
      set_changed();
      redraw();
      if (when() & FL_WHEN_CHANGED) do_callback();
    }
    return 1;
  case FL_RELEASE:
    if (value_ == oldval) {
      if (when() & FL_WHEN_NOT_CHANGED) do_callback();
      return 1;
    }
    set_changed();
    if (type() == FL_RADIO_BUTTON) setonly();
    else if (type() == FL_TOGGLE_BUTTON) oldval = value_;
    else {
      value(oldval);
      set_changed();
      if (when() & FL_WHEN_CHANGED) {
	Fl_Widget_Tracker wp(this);
        do_callback();
        if (wp.deleted()) return 1;
      }
    }
    if (when() & FL_WHEN_RELEASE) do_callback();
    return 1;
  case FL_SHORTCUT:
    if (!(shortcut() ?
	  Fl::test_shortcut(shortcut()) : test_shortcut())) return 0;
    if (Fl::visible_focus() && handle(FL_FOCUS)) Fl::focus(this);
    goto triggered_by_keyboard;
  case FL_FOCUS :
  case FL_UNFOCUS :
    if (Fl::visible_focus()) {
      if (box() == FL_NO_BOX) {
	// Widgets with the FL_NO_BOX boxtype need a parent to
	// redraw, since it is responsible for redrawing the
	// background...
	int X = x() > 0 ? x() - 1 : 0;
	int Y = y() > 0 ? y() - 1 : 0;
	if (window()) window()->damage(FL_DAMAGE_ALL, X, Y, w() + 2, h() + 2);
      } else redraw();
      return 1;
    } else return 0;
    /* NOTREACHED */
  case FL_KEYBOARD :
    if (Fl::focus() == this &&
		(Fl::event_key() == ' ' || Fl::event_key() == FL_Enter)&&
        !(Fl::event_state() & (FL_SHIFT | FL_CTRL | FL_ALT | FL_META))) {
      set_changed();
    triggered_by_keyboard:
      Fl_Widget_Tracker wp(this);
      if (type() == FL_RADIO_BUTTON) {
        if (!value_) {
	  setonly();
	  if (when() & FL_WHEN_CHANGED) do_callback();
        }
      } else if (type() == FL_TOGGLE_BUTTON) {
	value(!value());
	if (when() & FL_WHEN_CHANGED) do_callback();
      } else {
        simulate_key_action();
      }
      if (wp.deleted()) return 1;
      if (when() & FL_WHEN_RELEASE) do_callback();
      return 1;
    }
  default:
    return 0;
  }
}

void MiniButton::simulate_key_action()
{
  if (key_release_tracker) {
    Fl::remove_timeout(key_release_timeout, key_release_tracker);
    key_release_timeout(key_release_tracker);
  }
  value(1);
  redraw();
  key_release_tracker = new Fl_Widget_Tracker(this);
  Fl::add_timeout(0.15, key_release_timeout, key_release_tracker);
}

void MiniButton::key_release_timeout(void *d)
{
  Fl_Widget_Tracker *wt = (Fl_Widget_Tracker*)d;
  if (!wt)
    return;
  if (wt==key_release_tracker)
    key_release_tracker = 0L;
  MiniButton *btn = (MiniButton*)wt->widget();
  if (btn) {
    btn->value(0);
    btn->redraw();
  }
  delete wt;
}


