/* $Id: main.c,v 1.37 2008-07-27 11:24:37 stpohle Exp $ */

#include "basic.h"
#include "bomberclone.h"
#include "network.h"
#include "gfx.h"
#include "menu.h"
#include "player.h"
#include "keyb.h"
#include "single.h"

_bomberclone bman;              // Holds GameData
_player *players;				// holds all Playerdata
_team *teams;					// team stuff

_point dir_change[] ={	{ -1,  0 },		// changes for directions
						{  1,  0 },
						{  0, -1 },
						{  0,  1 } }; 

Uint32 timestamp;				// timestamp
float timefactor = 0.0f;		/* factor for the time time of the last loop
								   1.0f would be 20ms */
float timediff = 0.0f;			/* last loop timedifference in seconds */
int firstrun = 1;				// flag for the first menuloop

int
main (int argc, char **argv)
{
	int menuselect = 0;
	_menu *menu;

    printf ("Bomberclone version %s\n", VERSION);
	
	players = malloc (sizeof (_player) * MAX_PLAYERS);
	teams = malloc (sizeof (_team) * MAX_TEAMS);
	gfxengine_init ();
	
    if (SDL_Init (SDL_INIT_VIDEO| SDL_INIT_NOPARACHUTE) != 0) {
        d_printf ("Unable to init SDL: %s\n", SDL_GetError ());
        return (1);
    }

    SDL_InitSubSystem ( SDL_INIT_JOYSTICK );
	SDL_EnableUNICODE(1);
	
	config_init (argc, argv);
	keyb_init ();

	while (menuselect != -1 && bman.state != GS_quit) {

		menu = menu_new ("Bomberclone", 400, 250);
		menu_create_label (menu, VERSION, 300, 240, 0, COLOR_yellow);
		menu_create_button (menu, "Single Game", -1, 70, 200, 0); 
		menu_create_button (menu, "Multiplayer Game", -1, 100, 200, 1); 
		menu_create_button (menu, "Options", -1, 130, 200, 2);
		menu_create_button (menu, "Manual", -1, 160, 200, 3);
		menu_create_button (menu, "Quit Game", -1, 190, 200, 4); 
		menuselect = menu_loop (menu);
		menu_delete (menu);		
		switch (menuselect) {
			case (0) : // Singleplayer
				single_menu ();
				break;
			case (1) : // Multiplayer
				netmenu();
				break;
			case (2) : // Options
				config_menu ();
				break;
			case (3) : // Manual
				help (0);
				break;
			case (4) : // Quit
				bman.state = GS_quit;
				break;
		}
	}

    gfx_shutdown ();

    return 0;
};
