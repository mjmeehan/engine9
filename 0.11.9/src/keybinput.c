/* keyborad handling for text fields */

#include <SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "keybinput.h"

static int keybinput_oldkey = 0;

void keybinput_new (_keybinput *ki, int type, int maxlen) {
	int i;
	
	for (i = 0; i < 255; i++)
		ki->text[i] = 0;
	ki->curpos = 0;
	ki->len = 0;
	ki->type = type;
	ki->maxlen = maxlen;
}


int keybinput_loop (_keybinput *ki, SDL_Event *event) {
	int key = 0, keyu = 0;

	ki->changed = 0;
	
	if (event->type == SDL_KEYDOWN && keybinput_oldkey != event->key.keysym.sym) {
		key = keybinput_oldkey = event->key.keysym.sym;
		keyu = event->key.keysym.unicode;
	
		if (key == 8) { // BACKSPACE
			if (ki->curpos > 0) {
				ki->curpos--;
				ki->text[ki->curpos] = 0;
				ki->changed = 1;
			}
		}
		else if (ki->type == KEYBI_text && ((keyu >= 32 && keyu <= 126) || (keyu >= 128 && keyu <= 255))) {
			/* text keys will be read */
			if (ki->curpos < ki->maxlen) {
				ki->text[ki->curpos++] = event->key.keysym.unicode;
				ki->text[ki->curpos] = 0;
				ki->changed = 1;
			}
		}
		else if (ki->type == KEYBI_int && (keyu == '-' || (keyu >= '0' && keyu <= '9'))) {
			/* only integers will be read */
			if (ki->curpos < 255) {
				ki->text[ki->curpos] = event->key.keysym.unicode;
				if (atoi(ki->text) <= ki->maxlen)
					ki->curpos++;
				ki->text[ki->curpos] = 0;
				ki->changed = 1;
			}
		}
		else if (ki->type == KEYBI_float && (keyu == '-' || keyu == '.' || (keyu >= '0' && keyu <= '9'))) {
			/* only floats will be read */
			if (ki->curpos < 255) {
				ki->text[ki->curpos++] = event->key.keysym.unicode;
				ki->text[ki->curpos] = 0;
				ki->changed = 1;
			}
		}
		ki->len = strlen (ki->text);
	}
	
	if (keybinput_oldkey == SDLK_RETURN && event->type == SDL_KEYUP)
		keyu = 1;
	else if (keybinput_oldkey == SDLK_ESCAPE && event->type == SDL_KEYUP)
		keyu = -1;
	else
		keyu = 0;
	
	if (event->type == SDL_KEYUP)
		keybinput_oldkey = 0;
	
	return keyu;
}
