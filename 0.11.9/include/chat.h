
#ifndef _CHAT_H_
#define _CHAT_H_

#include "keybinput.h"

#define CHAT_MAX_LINES 255
#define CHAT_BG_SHADE_DARK -64
#define CHAT_BG_SHADE_BRIGHT 64
#define CHAT_TEXTCOLOR COLOR_gray
#define CHAR_NETCOLOR COLOR_blue

struct __chat {
	SDL_Rect window;
	signed char changed;	// if the chat windows has to redarwn after chat_loop
	SDL_Surface *oldscreen;	// old screen
	short int curline;		// current line
	short int active;		// if the chat window is active
	short int keepactive;	// keep chat active after pressing enter
	struct {
		char text[KEYBI_LINE_LEN];
		int color;			// color of the line
		int end;			// mark the end of one line
	} lines[CHAT_MAX_LINES];
	_keybinput input;
} typedef _chat;

extern _chat chat;

extern void chat_show (int x, int y, int w, int h);
extern void chat_addline (char *text, int color);
extern void chat_loop (SDL_Event *event);
extern void chat_setactive (int active, int keepactive);
extern void chat_draw ();

#endif
