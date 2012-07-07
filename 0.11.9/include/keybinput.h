/* $Id: keybinput.h,v 1.6 2004-05-25 22:22:27 stpohle Exp $ */

#ifndef _KEYBINPUT_H_
#define _KEYBINPUT_H_

#define KEYBI_LINE_LEN 255

enum _keybinputtype {
	KEYBI_text = 0,
	KEYBI_int,
	KEYBI_float
};

struct __keybinput {
	char text[KEYBI_LINE_LEN];
	short int curpos;
	short int len;
	char changed;
	int type;
	int maxlen;
} typedef _keybinput;


extern void keybinput_new (_keybinput *ki, int type, int maxlen);
extern int keybinput_loop (_keybinput *ki, SDL_Event *event);

#endif
