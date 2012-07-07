/* $Id: playermenu.c,v 1.18 2005-08-07 17:46:21 stpohle Exp $ 
 */

#include "bomberclone.h"
#include "menu.h"
#include "menugui.h"
#include "player.h"
#include "network.h"
#include "keyb.h"
#include "single.h"
#include "ogcache-client.h"


/*
 * prototype definitions
 */
// static int playermenu_gfxaviable (int gfx);
static void playermenu_selgfx_drawplayer (int selgfx, _menu *menu);

#define PLAYERMENU_GFXSEL_Y 130


/*
 * show the basis menu
 */
static void playermenu_selgfx_drawplayer (int selgfx, _menu *menu) {
	static int old_gfxsel = -1;
	static int changed = 0;
	SDL_Rect rect, srcrect;
	int i;
	int i_start;
	int i_end;
	
	/*
	 * delete old state, to force an update of all playergfx
	 */
	if (menu == NULL) {
		changed = 1;
		return;
	}
	if (selgfx != old_gfxsel)
		changed = 1;
	old_gfxsel = selgfx;

	/*
	 * 3. draw changes from the last loop
	 */
	if (changed) {
		rect.x = 0;
		rect.w = menu->oldscreenpos.w - 2 * menuimages[0]->w;
		rect.y = PLAYERMENU_GFXSEL_Y;
		rect.h = 4 * GFX_IMGSIZE;
		menu_draw_background (menu, &rect);
		
		/* calculate the first element on the screen */
		if (gfx.player_gfx_count < 12 || selgfx < 4) i_start = 0;
		else {
			i_start = (((selgfx / 4)-1) * 4);
			if (i_start < 0) i_start = 0;
		}
		
		if ((i_end = i_start + 12) > gfx.player_gfx_count)
			i_end = gfx.player_gfx_count;
		
		for (i = 0; i < i_end - i_start; i++) {
			srcrect.h = rect.h = gfx.players[i + i_start].menu_image->h;
			srcrect.w = rect.w = gfx.players[i + i_start].menu_image->w;
			rect.x = (2 * GFX_MENUPLAYERIMGSIZE_X) * (i % 4) + ((menu->oldscreenpos.w - 2 * menuimages[0]->w) - (8 * GFX_MENUPLAYERIMGSIZE_X)) / 2;
			rect.y = (GFX_MENUPLAYERIMGSIZE_X * 2) * (i / 4) + PLAYERMENU_GFXSEL_Y;
			srcrect.x = 0;
			srcrect.y = 0;
			rect.x += menu->oldscreenpos.x + menuimages[0]->w;
			rect.y += menu->oldscreenpos.y + menuimages[0]->h;
	        gfx_blit (gfx.players[i + i_start].menu_image, &srcrect, gfx.screen, &rect, 10002);
			
			/* draw the select border */
			if ((i + i_start) == selgfx) {
				srcrect.x = 0;
				srcrect.y = 0;
				srcrect.h = rect.h = GFX_IMGSIZE;
				srcrect.w = rect.w = GFX_IMGSIZE;
				rect.x = GFX_IMGSIZE * (i % 4) + ((menu->oldscreenpos.w - 2 * menuimages[0]->w) -(4 * GFX_IMGSIZE)) / 2;
				rect.y = GFX_IMGSIZE * (i / 4) + PLAYERMENU_GFXSEL_Y;
				rect.x += (gfx.players[i + i_start].menu_image->w - gfx.menuselect.image->w) / 2; // center the playergfx
				rect.x += menu->oldscreenpos.x + menuimages[0]->w;
				rect.y += menu->oldscreenpos.y + menuimages[0]->h;
	        	gfx_blit (gfx.menuselect.image, &srcrect, gfx.screen, &rect, 10001);
			}
		}
	}
}

/*
 * draw a small menu where the player has to select his gfx 
 */
int playermenu_selgfx (int pl_nr) {
	_menu *menu;
	int selgfx, eventstate;
    SDL_Event event;
    Uint8 *keys;
	int done = 0;

	if (pl_nr < 0 || pl_nr >= MAX_PLAYERS)
		return -1;

	selgfx = players[pl_nr].gfx_nr;
	if (selgfx < 0) 
		selgfx = 1;

	playermenu_selgfx_drawplayer (-1, NULL);
	player_set_gfx (&players[pl_nr], -1);
	
	if (gfx.player_gfx_count > 8)
		menu = menu_new ("Player Selection", 400, 330);
	else 
		menu = menu_new ("Player Selection", 400, 270);
	menu_create_text (menu, "playergfxsel", -1, 50, 40, 5, COLOR_yellow, "%s, please select your Player and press ENTER/RETURN or press ESCAPE for no player (that means you will only watch the next game).", players[pl_nr].name);

    menu->looprunning = 1;
	menu_draw (menu);
	

	while (!done && bman.state != GS_quit) {
        /* do the network loop if we have to */
        if (bman.sock > 0) {
            network_loop ();
            if (bman.notifygamemaster)
                ogc_loop ();
        }
		
        eventstate = s_fetchevent (&event);

        if (eventstate) {
            switch (event.type) {
            case (SDL_QUIT):
                bman.state = GS_quit;
                done = 1;
				menu_delete (menu);
				return -1;
                break;
            case (SDL_KEYDOWN): 
				/* 
				 * go to the next gfx or the preview one 
				 */
                if (event.key.keysym.sym == SDLK_TAB) {
                    keys = SDL_GetKeyState (NULL);
                    if (keys[SDLK_LSHIFT] || keys[SDLK_RSHIFT]) {
                        if ((--selgfx) < 0)
							selgfx = gfx.player_gfx_count-1;
					}
                    else {
                        if ((++selgfx) >= gfx.player_gfx_count)
							selgfx = 0;
					}
                    break;
                }
				/*
				 * cursor keys for gfx selection
				 */
                if (event.key.keysym.sym == SDLK_UP && selgfx >= 4)
					selgfx -= 4;
                if (event.key.keysym.sym == SDLK_DOWN && selgfx < gfx.player_gfx_count-4)
					selgfx += 4;
                if (event.key.keysym.sym == SDLK_RIGHT && selgfx < gfx.player_gfx_count-1)
					selgfx++;
                if (event.key.keysym.sym == SDLK_LEFT && selgfx > 0)
					selgfx--;
				/*
				 * do not select any gfx
				 */
                else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    selgfx = -1;
					done = 2;
					break;
                }
				/*
				 * select the current gfx if aviable
				 */
				else if (event.key.keysym.sym == SDLK_RETURN
					|| event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL 
					|| event.key.keysym.sym == keyb_gamekeys.keycode[BCPK_drop] || event.key.keysym.sym == keyb_gamekeys.keycode[BCPK_special] 
					|| event.key.keysym.sym == keyb_gamekeys.keycode[BCPK_max + BCPK_drop] || event.key.keysym.sym == keyb_gamekeys.keycode[BCPK_max + BCPK_special]) {
						done = 1;
					break;
				}
            }
        }

		playermenu_selgfx_drawplayer (selgfx, menu);
		gfx_blitdraw ();

        s_calctimesync ();
	};
	menu_delete (menu);
	player_set_gfx (&players[pl_nr], selgfx);
	if (done == 2)
		return -1;
	
	return 0;
};




/*

+----------------------------------------------------+
| List of Players    Details                         |
| |6)           |    Name: Playername        Team    |
| |             |    IP: 10.10.10.10                 |
| |             |    Port: 11000                     |
| |             |    Flags: Firewall, Second Player  |
| |             |                                    |
| +-------------+                                    |
|                 [1) Teammenu ]                     |
|   [2) Add AI    ]            [3)Add 2. Player]     |
|   [4)Kick Player]            [5)    Close    ]     |
+----------------------------------------------------+

 */
void playermenu () {
	_menu *menu;
	_menuitem *btn_SecondPlayer;
	_menuitem *list_PlayerList;
	_menuitem *detail_Name;
	_menuitem *detail_Addr;
	_menuitem *detail_Flags;
	_charlist playerlist[MAX_PLAYERS + 1];
	_charlist *playerlist_sel = &playerlist[0];
	int i, done = 0, eventstate, pl_nr, sel_pl_nr, old_2pl_btn = -1;
	SDL_Event event;

	/* delete the playerlist */
	playerlist[0].text[0] = 0;
	playerlist[0].next = NULL;
	
	/* create the window */
	menu = menu_new ("Playermenu", 400, 350);
	
	menu_create_label (menu, "Players", 20, 50, 0, COLOR_brown); 
	list_PlayerList = menu_create_list (menu, "playerlist", 15, 70, 170, 160, playerlist, &playerlist_sel, 6);

	menu_create_label (menu, "Details", 240, 50, 0, COLOR_brown);
	
	detail_Name = menu_create_label (menu, "Name", 220, 100, 0, COLOR_yellow);
	detail_Addr = menu_create_label (menu, "10.10.10.1:6666", 220, 120, 0, COLOR_yellow);
	detail_Flags = menu_create_label (menu, "firewall", 220, 140, 0, COLOR_yellow);
	
    if (GT_SP || GT_MP_PTPM)
		menu_create_button (menu, "Add AI Player", 20, 280, 150, 2);
	btn_SecondPlayer = menu_create_button (menu, "2 Player", 250, 280, 150, 3);
	if (IS_LPLAYER2)
		sprintf (btn_SecondPlayer->label, "Del 2 Player");
	else
		sprintf (btn_SecondPlayer->label,"Add 2 Player");
	if (GT_SP || GT_MP_PTPM)
		menu_create_button (menu, "Kick Player", 20, 315, 150, 4); 
	menu_create_button (menu, "Close", 250, 315, 150, 5); 
	
	if (bman.gametype == GT_team && (GT_SP || GT_MP_PTPM))
		menu_create_button (menu, "Teammenu", -1, 245, 150, 1);
	
	/* prepare everything for the menu_loop */
	menu_focus_id (menu, 5);
	menu->looprunning = 1;
	menu_draw (menu);
	
	/* the menu loop */
	do {
		detail_Name->label[0] = 0;
		detail_Addr->label[0] = 0;
		detail_Flags->label[0] = 0;
		sel_pl_nr = -1;
		for (i = 0, pl_nr = 0; pl_nr < MAX_PLAYERS; pl_nr++) {
			if (PS_IS_used(players[pl_nr].state)) {
				if (i > 0)
					playerlist[i-1].next = &playerlist[i];
				playerlist[i].next = NULL;
				strncpy (playerlist[i].text, players[pl_nr].name, LEN_PLAYERNAME);
				
				/* get detail information */
				if (i == (playerlist_sel - &playerlist[0])) {
					if (players[pl_nr].team_nr >= 0 && players[pl_nr].team_nr < MAX_TEAMS)
						sprintf (detail_Name->label, "%s(%s)", players[pl_nr].name, teams[players[pl_nr].team_nr].name);
					else
						sprintf (detail_Name->label, "%s", players[pl_nr].name);
					sprintf (detail_Addr->label, "%-32s:%s",players[pl_nr].net.addr.host, players[pl_nr].net.addr.port);
					sprintf (detail_Flags->label, "FIX ME");
					
					playermenu_getflags (detail_Flags->label, &players[pl_nr]);
					
					sel_pl_nr = pl_nr;
				}
				
				i++;
			}
		}
		list_PlayerList->changed = 1;
		detail_Name->changed = 1;
		detail_Addr->changed = 1;
		detail_Flags->changed = 1;
		
		eventstate = s_fetchevent (&event);
		if (bman.sock != -1)
			network_loop ();

		menu_draw (menu);
		gfx_blitdraw ();
		done = menu_event_loop (menu, &event, eventstate);
		
		/* 
		 * check if one of the buttons was pressed
		 */
		if (done == 1 && menu->focus->id == btn_SecondPlayer->id) {	/* second local player want to join */
			if (IS_LPLAYER2)
				player_delete (bman.p2_nr);
			else
				player2_join ();
		}

		if (old_2pl_btn != IS_LPLAYER2) {
			if (IS_LPLAYER2)
				sprintf (btn_SecondPlayer->label, "Del 2 Player");
			else
				sprintf (btn_SecondPlayer->label,"Add 2 Player");
			btn_SecondPlayer->changed = 1;
			old_2pl_btn = IS_LPLAYER2;
		}
		
		if (done == 1 && menu->focus->id == 2) {	/* create ai player */
			net_send_playerid (single_create_ai (1));
			done = 0;
		}

		if (done == 1 && menu->focus->id == 1) {	/* Teammenu */
			teammenu ();
			done = 0;
		}

		if (done == 1 && menu->focus->id == list_PlayerList->id) { /* team selection */
			done = 0;
			players[sel_pl_nr].team_nr = teammenu_select (&players[sel_pl_nr]);
			bman.updatestatusbar = 1;
			if (GT_MP)
				net_send_playerid (sel_pl_nr);
		}
		
		if (done == 1 && menu->focus->id == 4) {	/* kick player */
			if (sel_pl_nr >= 0 && sel_pl_nr < MAX_PLAYERS && sel_pl_nr != bman.p_servnr)
				player_delete (sel_pl_nr);
			else
				menu_displaymessage ("No", "You can't kick yourself from the game.\n");
			done = 0;
		}
		
		s_calctimesync ();
	} while ((done == 0 || menu->focus->id != 5) && done != -1);

	team_update ();
	if (GT_SP || GT_MP_PTPM) 
		ai_team_choosegfx ();

	menu_delete (menu);
};


/*
 * put all flags into the givin text like:
 *  NET_FW, NET_2P, AI...
 */
void playermenu_getflags (char *text, _player *player) {
	text[0] = 0;
	if (player->net.flags & NETF_firewall)
		sprintf (text, "%sNET_FW ", text);
	if (player->net.flags & NETF_local2)
		sprintf (text, "%sNET_2P ", text);
	if (player->state & PSF_net)
		sprintf (text, "%sPSF_NET ", text);
	if (player->state & PSF_ai)
		sprintf (text, "%sPSF_AI ", text);
};


/*
 * teammenu: Returns a Teamnumber for this player.
 */
int teammenu_select (_player *pl) {
	_menu *menu;
	int i;	
	_charlist teamlist[MAX_TEAMS];
	_charlist *teamlistsel=NULL;
	
	if (pl == NULL) return -1;
	
	/* create the list of teams */
	for (i = 0; i < MAX_TEAMS; i++)
		strncpy (teamlist[i].text, teams[i].name, LEN_PLAYERNAME);
	charlist_fillarraypointer (teamlist, MAX_TEAMS);
	
	teamlistsel = &teamlist[pl->team_nr];
	
	menu = menu_new ("Teamselection", 250, 250);
	menu_create_text (menu, "text", -1, 50, 200/font[0].size.x, 5, COLOR_yellow,
						"Please select the team where you want to put in Player '%s'.", pl->name);
	menu_create_list (menu, "teams", -1, 120, 200, 100, teamlist, &teamlistsel, 1);
	menu_focus_id (menu, 1);
	menu_loop (menu);
	
	menu_delete (menu);
	
	i = (teamlistsel - teamlist);

	d_printf ("teammenu_select: Player %s is in Team %s (%d)\n", pl->name, teams[i].name);
	
	return i;
};


/*
 * with the team menu you can select your own teamnames
 */
void teammenu () {
	_menu *menu;
	
	int i, col, y;
	struct _s_team {
		_charlist collist[COLOR_max];
		_charlist *collistsel;
	} tdata[MAX_TEAMS];
	
	menu = menu_new ("Teamsettings", 420, 100 + (MAX_TEAMS * 30));
	
	y = 60;
		
	for (i = 0; i < MAX_TEAMS; i++) {
		for (col = 0; col < COLOR_max; col++)
			strncpy (tdata[i].collist[col].text, color_map[col], LEN_CHARENTRY);
		charlist_fillarraypointer (tdata[i].collist, COLOR_max);
		tdata[i].collistsel = &tdata[i].collist[teams[i].col];
		menu_create_entry (menu, "Name :", 25, y, 200, teams[i].name, LEN_PLAYERNAME, MENU_entrytext, 2*i + 1);
		menu_create_label (menu, "Color:", 250, y, 0, COLOR_yellow);
		menu_create_list (menu, "Liste", 320, y, 90, 20, tdata[i].collist, &tdata[i].collistsel, 2*i + 2);
		
		y += 30;
	}

	menu_create_button (menu, "Close", -1, y + 10, 150, 2*i+1);
	
	menu_loop (menu);
	
	for (i = 0; i < MAX_TEAMS; i++) {
		teams[i].col = tdata[i].collistsel - &tdata[i].collist[0];
		if (GT_MP_PTPM)
			net_send_teamdata (i);
	}
	
	menu_delete (menu);
	
	return;
}


/*
 * create a menu where we can change the playernames
 */
void playernamemenu () {
	_menu *menu;

	menu = menu_new ("Playernames", 350, 150);
	
	menu_create_entry (menu, "Player 1:", -1, 50, 300, &bman.playername, LEN_PLAYERNAME, MENU_entrytext, 1);
	menu_create_entry (menu, "Player 2:", -1, 80, 300, &bman.player2name, LEN_PLAYERNAME, MENU_entrytext, 2);
	
	menu_create_button (menu, "Close", -1, 110, 100, 3); 
	
	menu_loop (menu);
	
	menu_delete (menu);
}
