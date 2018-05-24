// Adapted from http://membres-timc.imag.fr/Yves.Usson/personnel/mysofts.html
#ifndef MiniKnob_H
#define MiniKnob_H
#include <FL/Fl.H>
#include <Fl/Fl_Valuator.H>

class MiniKnob : public Fl_Valuator {
public:
  enum MiniKnobtype {DOTLIN=0,DOTLOG_1,DOTLOG_2,DOTLOG_3,LINELIN,LINELOG_1,LINELOG_2,LINELOG_3};
  enum MiniKnobDragtype {ANGLE=0, HORIZONTAL, VERTICAL};
private:
  int _type;
  float _percent;
  int _scaleticks;
  int _bgcolor;
  static int _defaultcolor;
  static int _defaultselectioncolor;
  static int _defaultbgcolor;
  short _dragtype;
  static short _defaultdragtype;
  int _margin;
  static int _defaultmargin;
  int _bevel;
  static int _defaultbevel;
  short a1, a2;
public:
  MiniKnob(int xx,int yy,int ww,int hh,const char *l=0);
  ~MiniKnob();
private:
  void draw();
  int handle(int  event);
public:
  void type(int ty);
private:
  void shadow(const int offs,const uchar r,uchar g,uchar b);
  void draw_scale(const int ox,const int oy,const int side);
  void draw_cursor(const int ox,const int oy,const int side);
public:
  void cursor(const int pc);
  void scaleticks(const int tck);
  void bgcolor(const int c);
  void dragtype(const short c);
  int margin();
  void margin(const int m);
  int bevel();
  void bevel(const int b);
};
#endif
