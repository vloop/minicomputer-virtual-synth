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
// Adapted from fl_ask.cxx
// Nicer buttons and different enter handling

#include <stdio.h>
#include <stdarg.h>

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>

// #include "minichoice.H"
#include "MiniButton.H"

static Fl_Window *messageWindow;
static Fl_Box *messageBox;
static Fl_Box *iconBox;
static MiniButton *button[3];
static Fl_Input *input;

static int ret_val;

static const char *iconlabel = "?";
static const char *message_title_default;
Fl_Font fl_message_font_ = FL_HELVETICA;
Fl_Fontsize fl_message_size_ = -1;
static int enableHotspot = 1;

static char avoidRecursion = 0;

// Sets the global return value (ret_val) and closes the window.
// Note: this is used for the button callbacks and the window
// callback (closing the window with the close button or menu).
// The first argument (Fl_Widget *) can either be an Fl_Button*
// pointer to one of the buttons or an Fl_Window* pointer to the
// messageBox window (messageWindow).
static void button_cb(Fl_Widget *, long val) {
  ret_val = (int) val;
  // printf("button_cb: %u\n", ret_val);
  messageWindow->hide();
}

static Fl_Window *makeform() {
 if (messageWindow) {
   return messageWindow;
 }
 // make sure that the dialog does not become the child of some
 // current group
 Fl_Group *previously_current_group = Fl_Group::current();
 Fl_Group::current(0);
 // create a new top level window
 Fl_Window *w = messageWindow = new Fl_Window(410,103);
 messageWindow->callback(button_cb);
 // w->clear_border();
 // w->box(FL_UP_BOX);
 (messageBox = new Fl_Box(60, 25, 340, 20))
   ->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
 (input = new Fl_Input(60, 37, 340, 23))->hide();
 {Fl_Box* o = iconBox = new Fl_Box(10, 10, 50, 50);
  o->box(FL_THIN_UP_BOX);
  o->labelfont(FL_TIMES_BOLD);
  o->labelsize(34);
  o->color(FL_WHITE);
  o->labelcolor(FL_BLUE);
 }
 w->end(); // don't add the buttons automatically
 // create the buttons (right to left)
 {
   for (int b=0, x=310; b<3; b++, x -= 100) {
     // if (b==1)
     //  button[b] = new Fl_Return_Button(x, 70, 90, 23);
     // else
       button[b] = new MiniButton(x, 70, 90, 23);
     button[b]->align(FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
     button[b]->callback(button_cb, b+1); // 0 for Esc/close
   }
 }
 // button[0]->shortcut(FL_Escape);
 // add the buttons (left to right)
 {
   for (int b=2; b>=0; b--)
     w->add(button[b]);
 }
 w->begin();
 w->resizable(new Fl_Box(60,10,110-60,27));
 w->end();
 w->set_modal();
 Fl_Group::current(previously_current_group);
 return w;
}

static void resizeform() {
  int	i;
  int	message_w, message_h;
  int	text_height;
  int	button_w[3], button_h[3];
  int	x, w, h, max_w, max_h;
	const int icon_size = 50;

  messageWindow->size(410,103);

  fl_font(messageBox->labelfont(), messageBox->labelsize());
  message_w = message_h = 0;
  fl_measure(messageBox->label(), message_w, message_h);

  message_w += 10;
  message_h += 10;
  if (message_w < 340)
    message_w = 340;
  if (message_h < 30)
    message_h = 30;

  fl_font(button[0]->labelfont(), button[0]->labelsize());

  memset(button_w, 0, sizeof(button_w));
  memset(button_h, 0, sizeof(button_h));

  for (max_h = 25, i = 0; i < 3; i ++)
    if (button[i]->visible())
    {
      fl_measure(button[i]->label(), button_w[i], button_h[i]);

      // if (i == 1)
      //  button_w[1] += 20;

      button_w[i] += 30;
      button_h[i] += 10;

      if (button_h[i] > max_h)
        max_h = button_h[i];
    }

  if (input->visible())
	text_height = message_h + 25;
  else
	text_height = message_h;

  max_w = message_w + 10 + icon_size;
  w     = button_w[0] + button_w[1] + button_w[2] - 10;

  if (w > max_w)
    max_w = w;

  message_w = max_w - 10 - icon_size;

  w = max_w + 20;
  h = max_h + 30 + text_height;

  messageWindow->size(w, h);
  messageWindow->size_range(w, h, w, h);

  messageBox->resize(20 + icon_size, 10, message_w, message_h);
  iconBox->resize(10, 10, icon_size, icon_size);
  iconBox->labelsize(icon_size - 10);
  input->resize(20 + icon_size, 10 + message_h, message_w, 25);

  for (x = w, i = 0; i < 3; i ++)
    if (button_w[i])
    {
      x -= button_w[i];
      button[i]->resize(x, h - 10 - max_h, button_w[i] - 10, max_h);

//      printf("button %d (%s) is %dx%d+%d,%d\n", i, button[i]->label(),
//             button[i]->w(), button[i]->h(),
//	     button[i]->x(), button[i]->y());
    }
}

static int innards(const char* fmt, va_list ap,
  const char *b0,
  const char *b1,
  const char *b2)
{
  Fl::pushed(0); // stop dragging (STR #2159)

  avoidRecursion = 1;

  makeform();
  messageWindow->size(410,103);
  char buffer[1024];
  if (!strcmp(fmt,"%s")) {
    messageBox->label(va_arg(ap, const char*));
  } else {
    ::vsnprintf(buffer, 1024, fmt, ap);
    messageBox->label(buffer);
  }

  messageBox->labelfont(fl_message_font_);
  if (fl_message_size_ == -1)
    messageBox->labelsize(FL_NORMAL_SIZE);
  else
    messageBox->labelsize(fl_message_size_);
  if (b0) {button[0]->show(); button[0]->label(b0); button[1]->position(210,70);}
  else {button[0]->hide(); button[1]->position(310,70);}
  if (b1) {button[1]->show(); button[1]->label(b1);}
  else button[1]->hide();
  if (b2) {button[2]->show(); button[2]->label(b2);}
  else button[2]->hide();
  const char* prev_icon_label = iconBox->label();
  if (!prev_icon_label) iconBox->label(iconlabel);

  resizeform();

  if(button[1]->visible() && input->visible())
    button[1]->shortcut(FL_Enter); // Button still sees it first !!

  if (button[1]->visible() && !input->visible())
    button[1]->take_focus();
  if (enableHotspot)
    messageWindow->hotspot(button[0]);
  /*
  if (b0 && Fl_Widget::label_shortcut(b0))
    button[0]->shortcut(0);
  else
    button[0]->shortcut(FL_Escape);
  */
  // set default window title, if defined and a specific title is not set
  if (!messageWindow->label() && message_title_default)
    messageWindow->label(message_title_default);

  // deactivate Fl::grab(), because it is incompatible with modal windows
  Fl_Window* g = Fl::grab();
  if (g) Fl::grab(0);
  Fl_Group *current_group = Fl_Group::current(); // make sure the dialog does not interfere with any active group
  messageWindow->show();
  Fl_Group::current(current_group);
  while (messageWindow->shown()) Fl::wait();
  if (g) // regrab the previous popup menu, if there was one
    Fl::grab(g);
  iconBox->label(prev_icon_label);
  messageWindow->label(0); // reset window title

  avoidRecursion = 0;
  return ret_val;
}

int minichoice(const char*fmt,const char *b0,const char *b1,const char *b2, const char *title,...){

  if (avoidRecursion) return 0;

  va_list ap;

  // fl_beep(FL_BEEP_QUESTION);

  va_start(ap, title);
  makeform(); // Or window below might not exist
  messageWindow->label(title);
  int r = innards(fmt, ap, b0, b1, b2);
  va_end(ap);
  // printf("minichoice: %u\n", r);
  return r;
}

static const char* input_innards(const char* fmt, va_list ap,
				 const char* defstr, uchar type) {
  makeform();
  messageWindow->size(410,103);
  messageBox->position(60,10);
  input->type(type);
  input->show();
  input->value(defstr);
  input->take_focus();

  // int r = innards(fmt, ap, fl_cancel, fl_ok, 0);
  int r = innards(fmt, ap, _("Cancel"), _("OK"), 0);
  input->hide();
  messageBox->position(60,25);
  return r==2 ? input->value() : 0;
}

const char* miniinput(const char *fmt, const char *defstr, const char *title, ...) {

  if (avoidRecursion) return 0;

  // fl_beep(FL_BEEP_QUESTION);

  va_list ap;
  va_start(ap, title);
  makeform(); // Or window below might not exist
  messageWindow->label(title);
  const char* r = input_innards(fmt, ap, defstr, FL_NORMAL_INPUT);
  va_end(ap);
  return r;
}
