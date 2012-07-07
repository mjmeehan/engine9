/* $Id: keyb.c,v 1.7 2008-07-27 11:24:37 stpohle Exp $
 * keyb.c
 */

#include "bomberclone.h"
#include "network.h"
#include "ogcache-client.h"
#include "menu.h"
#include "keyb.h"
#include "SDL.h"

BCGameKeys keyb_gamekeys;
BCGameJoystick joy_keys[2];
SDL_Joystick *joy[2];

/*
 * Translation table for the keycodes 
 */
const struct _key_codes key_codetab [] = {
	{ SDLK_RSHIFT, "RSHIFT" },
	{ SDLK_LSHIFT, "LSHIFT" },
	{ SDLK_RCTRL, "RCTRL" },
	{ SDLK_LCTRL, "LCTRL" },
	{ SDLK_UP, "CurUP" },
	{ SDLK_DOWN, "CurDOWN" },
	{ SDLK_LEFT, "CurLEFT" },
	{ SDLK_RIGHT, "CurRIGHT" },
	{ SDLK_F1, "F1" },
	{ SDLK_F2, "F2" },
	{ SDLK_F3, "F3" },
	{ SDLK_F4, "F4" },
	{ SDLK_F5, "F5" },
	{ SDLK_F6, "F6" },
	{ SDLK_F7, "F7" },
	{ SDLK_F8, "F8" },
	{ SDLK_F9, "F9" },
	{ SDLK_F10, "F10" },
	{ SDLK_F11, "F11" },
	{ SDLK_F12, "F12" },
	{ ' ', "SPACE" },
	{ -1, "" }
};


/*
 * set the default keyboard settings
 */
void keyb_config_reset () {
	
	/* One Player on One Screen - player one */
	keyb_gamekeys.keycode[BCPK_up] = SDLK_UP;
	keyb_gamekeys.keycode[BCPK_down] = SDLK_DOWN;
	keyb_gamekeys.keycode[BCPK_left] = SDLK_LEFT;
	keyb_gamekeys.keycode[BCPK_right] = SDLK_RIGHT;
	keyb_gamekeys.keycode[BCPK_drop] = SDLK_LCTRL;
	keyb_gamekeys.keycode[BCPK_special] = SDLK_LSHIFT;

	/* Two Players on One Screen - player one */
	keyb_gamekeys.keycode[BCPK_max + BCPK_up] = SDLK_UP;
	keyb_gamekeys.keycode[BCPK_max + BCPK_down] = SDLK_DOWN;
	keyb_gamekeys.keycode[BCPK_max + BCPK_left] = SDLK_LEFT;
	keyb_gamekeys.keycode[BCPK_max + BCPK_right] = SDLK_RIGHT;
	keyb_gamekeys.keycode[BCPK_max + BCPK_drop] = SDLK_RCTRL;
	keyb_gamekeys.keycode[BCPK_max + BCPK_special] = SDLK_RSHIFT;
	
	/* Two Player on One Screen - player two */
	keyb_gamekeys.keycode[BCPK_max + BCPK_max + BCPK_up] = 'W';
	keyb_gamekeys.keycode[BCPK_max + BCPK_max + BCPK_down] = 'S';
	keyb_gamekeys.keycode[BCPK_max + BCPK_max + BCPK_left] = 'A';
	keyb_gamekeys.keycode[BCPK_max + BCPK_max + BCPK_right] = 'D';
	keyb_gamekeys.keycode[BCPK_max + BCPK_max + BCPK_drop] = SDLK_LCTRL;
	keyb_gamekeys.keycode[BCPK_max + BCPK_max + BCPK_special] = SDLK_LSHIFT;

	/* game keys */
	keyb_gamekeys.keycode[BCK_help] = SDLK_F1;
	keyb_gamekeys.keycode[BCK_playermenu] = SDLK_F2;
	keyb_gamekeys.keycode[BCK_mapmenu] = SDLK_F3;
	keyb_gamekeys.keycode[BCK_chat] = SDLK_F5;
	keyb_gamekeys.keycode[BCK_pause] = SDLK_F4;
	keyb_gamekeys.keycode[BCK_fullscreen] = SDLK_F8;
	keyb_gamekeys.keycode[BCK_esc] = SDLK_ESCAPE;
	
	joy_keys[0].drop = 0;
	joy_keys[0].special = 1;
	joy_keys[1].drop = 0;
	joy_keys[1].special = 1;
};



/*
 * only for debug reasons, print out the states of the keys
 */
void keyb_print () {
	int i;
	
	printf ("keyb_gamekeys.state/old : ");
	for (i = 0; i < BCK_max; i++)
		printf ("%1d%1d ", keyb_gamekeys.state[i], keyb_gamekeys.old[i]);
	printf ("\n");
};


/*
 * convert a keycode to a text
 */
void keyb_code2text (int keycode, char *keytext) {
	int i;
	
	for (i = 0; (key_codetab [i].code != -1 && key_codetab[i].code != keycode); i++);

	if (key_codetab[i].code == keycode)
		strcpy (keytext, key_codetab[i].text);
	else if ((keycode >= 'a' && keycode <= 'z')
			 || (keycode >= 'A' && keycode <= 'Z')
			 || (keycode >= '0' && keycode <= '9'))
		sprintf (keytext, "%c", keycode);
	else 
		sprintf (keytext, "%x", keycode);
};


/*
 * create a small menu with all informations
 */
void keyb_config_createkeymenu (_menu *menu, int key, int x, int y, int menu_nr) {
	int key_id;
	char keyname [32];
	char keytext [50];

	for (key_id = key; key_id >= BCPK_max && key_id < BCPK_max * 3; key_id = key_id - BCPK_max);

	switch (key_id) {
		case (BCPK_up):
			strcpy (keyname, "Up");
			break;
		case (BCPK_down):
			strcpy (keyname, "Down");
			break;
		case (BCPK_left):
			strcpy (keyname, "Left");
			break;
		case (BCPK_right):
			strcpy (keyname, "Right");
			break;
		case (BCPK_drop):
			strcpy (keyname, "Drop");
			break;
		case (BCPK_special):
			strcpy (keyname, "Special");
			break;
/*		case (BCK_chat):
			strcpy (keyname, "Chat");
			break;
		case (BCK_fullscreen):
			strcpy (keyname, "Fullscreen");
			break;
		case (BCK_help):
			strcpy (keyname, "Help");
			break;
		case (BCK_mapmenu):
			strcpy (keyname, "Mapmenu");
			break;
		case (BCK_playermenu):
			strcpy (keyname, "Playermenu");
			break;
		case (BCK_pause):
			strcpy (keyname, "Pause/Start");
			break; */
	}
	
	menu_create_label (menu, keyname, x, y + 2, 0, COLOR_brown);
	keyb_code2text (keyb_gamekeys.keycode[key], keytext);
	menu_create_button (menu, keytext, x + 70, y, 100, menu_nr + key);
}

/*
 * select a new key for the function, 
 */
void keyb_config_joypad (int key) {
	unsigned int n = 0;
	SDL_Event event;
	Uint8 *keys;
	int keypressed = 0,	done = 0, eventstate = 0, reorder = 0, i, j;

	if (joy[0] == NULL || key < 0 || key >= BCK_max) return;
	
	SDL_JoystickUpdate ();
	
	menu_displaytext ("Joypad Config", "Please press the new key\nfor this function.");
	
	keys = SDL_GetKeyState (NULL);
	if (keys[SDLK_RETURN] || keys[SDLK_ESCAPE])
		keypressed = 1;
	
	timestamp = SDL_GetTicks (); // needed for time sync.
	
	while (!reorder && !done && bman.state != GS_quit) {
		/* do the network loop if we have to */
		if (bman.sock > 0) {
			network_loop ();
			if (bman.notifygamemaster)
				reorder = ogc_loop ();
			else
				reorder = 0;
		}
		
		// eventstate = s_fetchevent (&event);
		SDL_JoystickEventState ( SDL_QUERY ); // js
		SDL_JoystickUpdate ();
		
		for ( j = 0; j < 2; j++)
			for ( i=0; i < SDL_JoystickNumButtons ( joy[j] ); ++i ) {
				n = SDL_JoystickGetButton ( joy[j], i );
				// 2 .. bomb
				/* Sadly every other controller enumberates different */
				if (n != 0) {
					printf("keyb keyb_config_joypad: JS %d: %d \n", j, i);
					if (key == BCPK_drop || key == BCPK_drop + BCPK_max + BCPK_max)
						joy_keys[j].drop = i;
					if (key == BCPK_special || key == BCPK_special + BCPK_max + BCPK_max)
						joy_keys[j].special = i;
					eventstate = 1;
					done = 1;
				}
			}
		
		if (eventstate >= 1) {
			switch (event.type) {
				case (SDL_QUIT):
					bman.state = GS_quit;
					done = 1;
					break;
			}
		}
		
		s_calctimesync ();
	}
};


/*
 * select a new key for the function, 
 */
void keyb_config_readkey (int key) {
	int newkey;
    SDL_Event event;
    Uint8 *keys;
    int keypressed = 0,
        done = 0,
        eventstate = 0,
        reorder = 0;
		newkey = 0;

	
	if (key < 0 || key >= BCK_max) 
		return;
	
	menu_displaytext ("Keyboard Config", "Please press the new key\nfor this function.");

    keys = SDL_GetKeyState (NULL);
    if (keys[SDLK_RETURN] || keys[SDLK_ESCAPE])
        keypressed = 1;

    timestamp = SDL_GetTicks (); // needed for time sync.

    while (!reorder && !done && bman.state != GS_quit) {
        /* do the network loop if we have to */
        if (bman.sock > 0) {
            network_loop ();
            if (bman.notifygamemaster)
                reorder = ogc_loop ();
            else
                reorder = 0;
        }

        eventstate = s_fetchevent (&event);

        if (eventstate >= 1) {
            switch (event.type) {
            case (SDL_QUIT):
                bman.state = GS_quit;
                done = 1;
                break;
            case (SDL_KEYDOWN): /* focus next element */
				newkey = event.key.keysym.sym;
				done = 1;
				break;
            }
        }

        s_calctimesync ();
    }
	
	if (newkey != 0 && newkey != SDLK_ESCAPE)
		keyb_gamekeys.keycode[key] = newkey;
};

/*
 * joypad configuration screen
 */
void joypad_config () {
	int menuselect = 1;
	_menu *menu;
	
	do {
		menu = menu_new ("Joypad Config", 420, 400);
		
		if ( joy[0] != NULL ) {   
			char text[32];
			
			menu_create_label (menu, "Player 1 Joypad", 20, 105, 1, COLOR_yellow);
			// keyb_config_createkeymenu (menu, BCPK_max + BCPK_drop, 25, 250, 10);
			menu_create_label (menu, "Drop", 25, 250 + 2, 0, COLOR_brown);
			sprintf (text, "%d", joy_keys[0].drop);
			menu_create_button (menu, text, 25 + 70, 250, 100, 10 + BCPK_drop);
			// keyb_config_createkeymenu (menu, BCPK_max + BCPK_special, 25, 280, 10);
			menu_create_label (menu, "Special", 25, 280 + 2, 0, COLOR_brown);
			sprintf (text, "%d", joy_keys[0].special);
			menu_create_button (menu, text, 25 + 70, 280, 100, 10 + BCPK_special);
		}
		
		if ( joy[1] != NULL ) {
			char text[32];
			
			menu_create_label (menu, "Player 2 Joypad", 210, 105, 1, COLOR_yellow);
			// keyb_config_createkeymenu (menu, BCPK_max + BCPK_max + BCPK_drop, 225, 250, 10);
			menu_create_label (menu, "Drop", 225, 250 + 2, 0, COLOR_brown);
			sprintf (text, "%d", joy_keys[1].drop);
			menu_create_button (menu, text, 225 + 70, 250, 100, 10 + BCPK_max + BCPK_max + BCPK_drop);
			// keyb_config_createkeymenu (menu, BCPK_max + BCPK_max + BCPK_special, 225, 280, 10); 
			menu_create_label (menu, "Special", 225, 280 + 2, 0, COLOR_brown);
			sprintf (text, "%d", joy_keys[1].special);
			menu_create_button (menu, text, 225 + 70, 280, 100, 10 + BCPK_max + BCPK_max + BCPK_special);
		}
		
		menu_create_button (menu, "OK", 250, 330, 150, 1);
		menu_focus_id (menu, menuselect);
		menuselect = menu_loop (menu);
		menu_delete (menu);
		if (menuselect >= 10 && menuselect < 10+BCK_max)
			keyb_config_joypad (menuselect - 10);
	} while (menuselect != 1 && menuselect != -1);
};



/*
 * keyboard configuration screen
 */
void keyb_config () {
	int menuselect = 1;
	_menu *menu;
	
	do {
		menu = menu_new ("Keyboard Config", 420, 400);

		
		menu_create_label (menu, "One Screen Keys", -1, 50, 1, COLOR_yellow);
		keyb_config_createkeymenu (menu, BCPK_drop, 25, 75, 10);
		keyb_config_createkeymenu (menu, BCPK_special, 225, 75, 10);
				
		menu_create_label (menu, "Player 1", 20, 105, 1, COLOR_yellow);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_up, 25, 130, 10);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_down, 25, 160, 10);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_left, 25, 190, 10);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_right, 25, 220, 10);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_drop, 25, 250, 10);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_special, 25, 280, 10);
	
		menu_create_label (menu, "Player 2", 210, 105, 1, COLOR_yellow);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_max + BCPK_up, 225, 130, 10);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_max + BCPK_down, 225, 160, 10);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_max + BCPK_left, 225, 190, 10);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_max + BCPK_right, 225, 220, 10);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_max + BCPK_drop, 225, 250, 10);
		keyb_config_createkeymenu (menu, BCPK_max + BCPK_max + BCPK_special, 225, 280, 10);
	
		/*menu_create_label ("Other Keys", -1, 270, 2, COLOR_brown);
		keyb_config_createkeymenu (BCK_help, 25, 300, 10);
		keyb_config_createkeymenu (BCK_chat, 25, 330, 10);
		keyb_config_createkeymenu (BCK_fullscreen, 25, 360, 10);
		keyb_config_createkeymenu (BCK_mapmenu, 225, 300, 10);
		keyb_config_createkeymenu (BCK_playermenu, 225, 330, 10);
		keyb_config_createkeymenu (BCK_pause, 225, 360, 10); */
		
		menu_create_button (menu, "Default", 50, 330, 150, 2);
		menu_create_button (menu, "OK", 250, 330, 150, 1);
		menu_focus_id (menu, menuselect);
		menuselect = menu_loop (menu);
		menu_delete (menu);
		if (menuselect ==2)
			keyb_config_reset ();
		if (menuselect >= 10 && menuselect < 10+BCK_max)
			keyb_config_readkey (menuselect - 10);
	} while (menuselect != 1 && menuselect != -1);
};


/*
 * delete all old data of the keyb_gamekeys
 */
void keyb_init () {	
	memset (keyb_gamekeys.state, 0, sizeof (Uint8) * BCK_max);
	joy[0] = joy[1] = NULL;

	joy[0] = SDL_JoystickOpen (0);
	if (joy[0])
		joy[1] = SDL_JoystickOpen (1);
};


/*
 * read all keys and set the keyb_gamekeys
 */
void keyb_loop (SDL_Event *event) {
	int j, i, offset = 0;

	Uint8 *keys = SDL_GetKeyState (NULL);

	if (joy[0]) {
		SDL_JoystickEventState ( SDL_QUERY ); // js
		SDL_JoystickUpdate ();
	}

	/* copy the state into the old state */
	memcpy (keyb_gamekeys.old, keyb_gamekeys.state, sizeof (Uint8) * BCK_max);
	memset (keyb_gamekeys.state, 0, sizeof (Uint8) * BCK_max);
	
	for (j = 0; j < 2; j++) {
		if (joy[j]) {
			for ( i=0; i < SDL_JoystickNumButtons (joy[j]); ++i ) {
				unsigned int n = SDL_JoystickGetButton (joy[j], i);
				/* Sadly every other controller enumberates different */
				if (n != 0 && i == joy_keys[j].drop)
					keyb_gamekeys.state[offset + BCPK_drop] |= 1;
				if (n != 0 && i == joy_keys[j].special)
					keyb_gamekeys.state[offset + BCPK_special] |= 1;
			}

			for (  i=0; i < SDL_JoystickNumAxes ( joy[j] ); ++i )  {
				signed short a = SDL_JoystickGetAxis ( joy[j], i );
				/*
				 X -> Axis 0
				 Y -> Axis 1
				 There are only the values -32786 .. 32768 available
				 */
				if ( i == 0 && a < (-16000) ) {
					keyb_gamekeys.state[offset + BCPK_left] |= 1;
				}
				if (i == 0 && a > 16000 ) {
					keyb_gamekeys.state[offset + BCPK_right] |= 1;
				}
				if ( i == 1 && a < -16000 ) {
					keyb_gamekeys.state[offset + BCPK_up] |= 1;
				}
				if (i == 1 && a > 16000 ) {
					keyb_gamekeys.state[offset + BCPK_down] |= 1;
				}
			}
		}
		offset = BCPK_max + BCPK_max;
	}
	
	/* read the new state of the pressed keys */
	for (i = 0; i < BCK_max; i++) {
		if (keyb_gamekeys.keycode[i] >= 'A' && keyb_gamekeys.keycode[i] <= 'Z') {
			if (keys[keyb_gamekeys.keycode[i]] || keys[tolower (keyb_gamekeys.keycode[i])])
				keyb_gamekeys.state[i] |= 1;
		}
		else if (keys[keyb_gamekeys.keycode[i]])
			keyb_gamekeys.state[i] |= 1;
	}
};
