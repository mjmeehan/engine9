/* $:ID $
 * keyboard handling */

#ifndef _KEYB_H_
#define _KEYB_H_

#include "menu.h"

enum _bcplayerkeys {
	BCPK_up = 0,
	BCPK_down,
	BCPK_left,
	BCPK_right,
	BCPK_drop,
	BCPK_special,
	
	BCPK_max 
};

enum _bckeys {
	BCK_help = BCPK_max * 3,
	BCK_esc,
	BCK_fullscreen,
	BCK_chat,
	BCK_pause,
	BCK_playermenu,
	BCK_mapmenu,

	BCK_max
};

struct {
	Uint8 state [BCK_max];	// current state
	Uint8 old [BCK_max];	// old state
	int keycode [BCK_max];	// keycode
} typedef BCGameKeys;

struct {
	int drop;
	int special;
} typedef BCGameJoystick;

extern BCGameKeys keyb_gamekeys;
extern BCGameJoystick joy_keys[2];

extern void keyb_config ();
extern void keyb_config_reset ();
extern void keyb_config_createkeymenu (_menu *menu, int key, int x, int y, int menu_nr);
extern void keyb_config_readkey (int key);
extern void keyb_init ();
extern void keyb_loop (SDL_Event *event);

#endif
