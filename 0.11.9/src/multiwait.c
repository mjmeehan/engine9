/* $Id: multiwait.c,v 1.60 2008-07-27 11:24:37 stpohle Exp $
   multiwait.c - this manages only the network screen where
   everyone have to select it's players and where even the basic chat is inside
*/

#include "bomberclone.h"
#include "network.h"
#include "packets.h"
#include "gfx.h"
#include "chat.h"
#include "menu.h"
#include "single.h"
#include "keyb.h"

/****************** prototype declaration */
static void mw_init ();
static void mw_shutdown ();

static void mw_keys_loop ();
static void mw_draw_top ();
static void mw_draw_all_player ();
static void mw_draw_all_teams ();

/* static int mw_team_is_used (int t_nr); */
static int mw_check_players ();
static int mw_check_screenredraw ();

extern int UpdateRects_nr;
extern int blitdb_nr;


/* this will load some graphics and so other stuff */
static void mw_init ()
{
	team_update ();
	gfx_blitdraw ();
	draw_logo ();
	gfx_blitdraw ();
	bman.updatestatusbar = 1;
	d_playerdetail ("mw_init:\n");
	SDL_Flip (gfx.screen);
	chat_show (10, gfx.res.y / 2, gfx.res.x - 20, gfx.res.y / 2 - 10);
	chat_setactive (1, 1);
};


/* free all graphics */
static void mw_shutdown () {
	chat_show (-1, -1, -1, -1);
	gfx_blitdraw ();
	draw_logo ();

	SDL_Flip (gfx.screen);
};


/*
 * returns the number of players which are ready
 */
static int mw_check_players () {
	int readyplayers = 0;
	int i;

	if (GT_MP_PTPS)		/* only the master can start the game */
		return 0;
	
	for (i = 0; i < MAX_PLAYERS; i++) {
		if (players[i].gfx_nr >= 0 && players[i].gfx_nr < MAX_PLAYERS)
			readyplayers++;
	}
	
	return readyplayers;
}



/*
 * check if one key is pressed
 */
static void mw_keys_loop () {
	if (GT_MP_PTPM && mw_check_players () >= 2 && keyb_gamekeys.state[BCK_pause] && !keyb_gamekeys.old[BCK_pause]) {
		/* min 2 players are ready for the game and 
		 * the start key has pressed */
        bman.state = GS_ready;
        bman.updatestatusbar = 1; // force an update
    }

    if (GT_MP_PTPM && mw_check_players () < 2 && keyb_gamekeys.state[BCK_pause] && !keyb_gamekeys.old[BCK_pause]) {
		font_gfxdraw (20,20,"There are not enough configured players", 0, COLOR_brown, 0x1000);
		font_gfxdraw (20,40,"(each player needs to press Ctrl to select the role)", 0, COLOR_brown, 0x1000);
    }

	if (keyb_gamekeys.state[BCK_fullscreen]  && !keyb_gamekeys.old[BCK_fullscreen]) {
        /* Switch Fullscreen */
		SDL_WM_ToggleFullScreen(gfx.screen);
		gfx.fullscreen = !gfx.fullscreen;
        bman.updatestatusbar = 1; // force an update
		bman.updatestatusbar = 1;
	}

	if (keyb_gamekeys.state[BCK_esc] && !keyb_gamekeys.old[BCK_esc]) {
		/* ESCAPE key was pressed */
		net_game_send_delplayer (bman.p_nr);
        bman.state = GS_startup;
    }

	if (((IS_LPLAYER2) && keyb_gamekeys.state[BCPK_max + BCPK_drop] && !keyb_gamekeys.old[BCPK_max + BCPK_drop])
		|| ((!IS_LPLAYER2) && keyb_gamekeys.state[BCPK_drop] && !keyb_gamekeys.old[BCPK_drop])) {
		/* player 1 want to select a new gfx */
		playermenu_selgfx (bman.p_nr);
		bman.updatestatusbar = 1;
		keyb_loop (NULL);	// to reload the current keys
	}

	if (IS_LPLAYER2 && keyb_gamekeys.state[BCPK_max + BCPK_max + BCPK_drop] && !keyb_gamekeys.old[BCPK_max + BCPK_max + BCPK_drop]) {
		/* player 2 want to select a new gfx */
		playermenu_selgfx (bman.p2_nr);
		bman.updatestatusbar = 1;
		keyb_loop (NULL);	// to reload the current keys
	}

	if (keyb_gamekeys.state[BCK_mapmenu] && !keyb_gamekeys.old[BCK_mapmenu] && GT_MP_PTPM) {
		/* mapmenu */
		mapmenu ();
		bman.updatestatusbar = 1;
		keyb_loop (NULL);	// to reload the current keys
	}

	if (keyb_gamekeys.state[BCK_help] && !keyb_gamekeys.old[BCK_help]) {
		/* playermenu */
		help (HP_keyboard0);
		bman.updatestatusbar = 1;
		keyb_loop (NULL);	// to reload the current keys
	}

	
	if (keyb_gamekeys.state[BCK_playermenu] && !keyb_gamekeys.old[BCK_playermenu]) {
		/* playermenu */
		playermenu ();
		bman.updatestatusbar = 1;
		keyb_loop (NULL);	// to reload the current keys
	}
};


/*
 * return 1 if we have to redraw the screen or 0 if everything is still normal
 */
static int mw_check_screenredraw () {
	static int old_gametype = -1;
	int ret = 0;

	if (old_gametype != bman.gametype || bman.updatestatusbar)
		ret = 1;
	
	bman.updatestatusbar = 0;
	old_gametype = bman.gametype;
	return ret;
}


/*
 * draw the top of the screen, the keys you can press and so
 */
static void mw_draw_top () {
	int step = gfx.res.x / 4;
	int x = 0;
	
	font_gfxdraw (x,0,"F1", 0, COLOR_brown, 0x1000);
	font_gfxdraw (x+1,1,"F1", 0, COLOR_yellow, 0x1001);
	font_gfxdraw (x+32,0,"Help", 0, COLOR_brown, 0x1000);
	
	x += step;
	
	font_gfxdraw (x,0,"F2", 0, COLOR_brown, 0x1000);
	font_gfxdraw (x+1,0,"F2", 0, COLOR_yellow, 0x1001);
	font_gfxdraw (x+32,0,"Playermenu", 0, COLOR_brown, 0x1000);

	x += step;
	if (GT_MP_PTPM) {
		font_gfxdraw (x,0,"F3", 0, COLOR_brown, 0x1000);
		font_gfxdraw (x+1,0,"F3", 0, COLOR_yellow, 0x1001);
		font_gfxdraw (x+32,0,"Mapmenu", 0, COLOR_brown, 0x1000);

		x += step;

		font_gfxdraw (x,0,"F4", 0, COLOR_brown, 0x1000);
		font_gfxdraw (x+1,0,"F4", 0, COLOR_yellow, 0x1001);
		font_gfxdraw (x+32,0,"Start", 0, COLOR_brown, 0x1000);
	}
	else {
		x += step/2;
		font_gfxdraw (x+32,0,"Wait for the Server", 0, COLOR_brown, 0x1000);
	}
};


/*
 * draw all player informations
 */
static void mw_draw_all_player () {
	mw_draw_top ();
	game_showresultnormal (20, 50, gfx.res.x-40, (gfx.res.y / 2) - 60);
	chat_draw ();
};


/*
 * draw all team informations
 */
static void mw_draw_all_teams () {
	mw_draw_top ();
	game_showresultteam (20, 50, gfx.res.x-40, (gfx.res.y / 2) - 60);
	chat_draw ();
};



/* the loop itself */
void wait_for_players () {
    SDL_Event event;

	mw_init ();

	do {
        if (s_fetchevent (&event) != 0)
            switch (event.type) {
	            case (SDL_QUIT):
    	            bman.state = GS_quit;
					break;
            }

		/*
		 * check some little things and do the network stuff
		 */
		network_loop ();

		/*
		 * the drawing stuff
		 */
		if (mw_check_screenredraw()) {
			d_printf ("Draw Status\n");
			gfx_blitdraw ();
	    		draw_logo ();
			if (bman.gametype==GT_team) mw_draw_all_teams(); 
			else mw_draw_all_player ();
		}
		gfx_blitdraw ();
		
		/*
		 *	input handling 
		 */
		keyb_loop (&event);
		mw_keys_loop ();

		chat_loop (&event);
		chat.active = 1;
		
		s_calctimesync ();
	} while (bman.state == GS_wait && bman.sock != -1);

    mw_shutdown ();
};
