#ifndef _FONT_H_

#define _FONT_H_

#include "basic.h"

enum _color {
	COLOR_white = 0,
	COLOR_red,
	COLOR_green,
	COLOR_blue,
	COLOR_yellow,
	COLOR_brown,
	COLOR_gray,
	COLOR_black,
	
	COLOR_max
};


struct __font {
	SDL_Surface *image[COLOR_max];
	_point size;
} typedef _font;


struct _key_codes { 
		int code; 
		char *text;
};

extern _point font_lastsize;
extern _font font[3];
extern char *color_map[COLOR_max];

extern void font_draw (int x, int y, char *text, int size, int color);
extern void font_drawbold (int x, int y, char *text, int size, int color, int bold);
extern void font_load ();
extern void font_free ();
extern void font_gfxdraw (int x, int y, char *text, int size, int color, int ypos);
extern void font_gfxdrawbold (int x, int y, char *text, int size, int color, int bold, int ypos);

#endif
