/* $Id: single.c,v 1.86 2006-08-19 23:41:47 stpohle Exp $ */
/* single player */

#include "basic.h"
#include "bomberclone.h"
#include "menu.h"
#include "player.h"
#include "single.h"

/* start a single player game and prepare everything for the game */
void
single_game_new ()
{
    int p;

    bman.players_nr_s = 1;
    bman.players_nr = 1;

    // set players variables, only the variables which depend on single games
    for (p = 0; p < MAX_PLAYERS; p++) {
        if (PS_IS_used (players[p].state)) {
            bman.players_nr_s++;
            bman.players_nr++;
            players[p].state |= PSF_used + PSF_alife + PSF_playing;
        }
        else
            players[p].state = 0;
    }

    players[bman.p_nr].state = PSFM_alife;
    player_set_gfx (&players[bman.p_nr], players[bman.p_nr].gfx_nr);
    bman.last_ex_nr = 1;

    bman.sock = -1;
    bman.state = GS_ready;
};



/*
 * the ai player will choose diffrent gfx for every
 * team another gfx, but not one which is selected by
 * human player.
 */
void ai_team_choosegfx () {
	struct _team_tmpdata_ {
		int ai_gfx;
	} teamdat[MAX_TEAMS];
	
	int tm_nr;
	int pl_nr;
	int i, used;
	_player *pl;
	
	/* find a ai player gfx for every team */
	for (i = 0, tm_nr = 0; (i < gfx.player_gfx_count && tm_nr < MAX_TEAMS); i++) {
		used = 0;
		for (pl_nr = 0; pl_nr < MAX_PLAYERS; pl_nr++) {
			if ((!PS_IS_aiplayer(players[pl_nr].state)) 
				&& (PS_IS_used (players[pl_nr].state)) 
				&& i == players[pl_nr].gfx_nr)
					used = 1;
		}
		if (!used && tm_nr < MAX_TEAMS) {
			teamdat[tm_nr].ai_gfx = i;
			tm_nr++;
		}
	}
	
	/*
	 * give all ai players in the teams the right gfx
	 */
	for (tm_nr = 0; tm_nr < MAX_TEAMS; tm_nr++) for (pl_nr = 0; pl_nr < MAX_PLAYERS; pl_nr++) {
		pl = teams[tm_nr].players[pl_nr];
		if (pl) {
			if (PS_IS_used(pl->state) && PS_IS_aiplayer(pl->state))
				player_set_gfx (pl, teamdat[tm_nr].ai_gfx);
		}
	}
};



inline int
ai_checkfield (int x, int y)
{
    return ((map.field[x][y].type == FT_nothing || map.field[x][y].type == FT_fire
             || map.field[x][y].type == FT_shoe || map.field[x][y].type == FT_bomb
             || map.field[x][y].type == FT_mixed || map.field[x][y].type == FT_tunnel)
            && map.bfield[x][y] == 0);
}


/* give the run away direction 
 * this function is even needed for the start of 
 * the game to place the players on a good position */
int
ai_easyrunaway (_point p, int range)
{
    int i,
	  dist = 0,
      done = 0,
        dir = 0;
    _point pos[4];

    for (i = 0; i < 4; i++) {
        pos[i] = p;
        pos[i].x += dir_change[i].x;
        pos[i].y += dir_change[i].y;
    }

    /* test the possible ways */
    while (!done) {
        done = 1;
		dist++;
        for (i = 0; i < 4; i++) {
            /* check if we are still in the game field */
            if (pos[i].x <= 0 || pos[i].y <= 0 || pos[i].x >= map.size.x - 1
                || pos[i].y >= map.size.y - 1)
                pos[i].x = pos[i].y = -1;

            if (pos[i].x != -1 && pos[i].y != -1) {
                /* check if this place is free to go to */
                if (ai_checkfield (pos[i].x, pos[i].y)) {
                    done = 0;
                    /* check the field left and right beside */
                    if (i == left || i == right) {
                        if (ai_findnearbombs (pos[i]) == 0
                            && (ai_checkfield (pos[i].x, pos[i].y - 1)
                                || ai_checkfield (pos[i].x, pos[i].y + 1)))
                            dir |= (1 << i);
                    }
                    else {
                        if (ai_findnearbombs (pos[i]) == 0
                            && (ai_checkfield (pos[i].x - 1, pos[i].y)
                                || ai_checkfield (pos[i].x + 1, pos[i].y)))
                            dir |= (1 << i);
                    }
                    pos[i].x += dir_change[i].x;
                    pos[i].y += dir_change[i].y;
					if (dist > range) dir |= (1 << i);
                }
            }
        }
    }

    return dir;
};


/* give the run away direction 
  the return value:				*/
_airunaway
ai_runawayfrom (_point p, int nearbomb, int range, signed char norecursive)
{
    int i,
      done = 0,
        nbomb,
        tdir,
        _i,
        bdirpoints = 10,
        j,
        c,
		dist = 0;
    _airunaway res;
    _point pos[4],
      tpos;

    res.dir = 0;
    res.bestdir = -1;

    for (i = 0; i < 4; i++) {
        pos[i] = p;
        pos[i].x += dir_change[i].x;
        pos[i].y += dir_change[i].y;
    }

    /* test if we just have to move to the side */
    if (!norecursive)
        for (i = 0; i < 4; i++)
            if (ai_checkfield (pos[i].x, pos[i].y) && ai_findnearbombs (pos[i]) == 0) {
                bdirpoints = 0;
                res.bestdir = i;
                res.dir |= (1 << i);
            }

    /* test the possible ways */
    while (!done) {
        done = 1;
		dist++;
        for (i = 0; i < 4; i++) {
            /* check if we are still in the game field */
            if (pos[i].x <= 0 || pos[i].y <= 0 || pos[i].x >= map.size.x - 1
                || pos[i].y >= map.size.y - 1)
                pos[i].x = pos[i].y = -1;

            if (pos[i].x != -1 && pos[i].y != -1) {
                /* check if this place is free to go to */
                if (ai_checkfield (pos[i].x, pos[i].y)) {
                    done = 0;
                    /* check the field left and right beside */
                    for (j = 0; j < 4; j++)
                        if (((i == left || i == right) && (j == up || j == down)) ||
                            ((j == left || j == right) && (i == up || i == down))) {
                            c = 10;
                            tpos.x = pos[i].x + dir_change[j].x;
                            tpos.y = pos[i].y + dir_change[j].y;
                            if (ai_checkfield (tpos.x, tpos.y)) {
                                nbomb = ai_findnearbombs (tpos);
                                c = s_countbits (nbomb, 5);
                                if (!norecursive) {
                                    tdir = ai_runawayfrom (tpos, nbomb, range, 1).dir;
                                    _i = ai_invertdir (i);
                                    if (tdir != (1 << _i)) { // usefull direction
                                        res.dir |= (1 << i); // add this one
                                    }
                                    else {
                                        c = -1;
                                    }
                                }
                                else
                                    res.dir |= (1 << i);

                                /* check for the best direction */
                                if (c != -1 && !norecursive) {
                                    if (c < bdirpoints) {
                                        bdirpoints = c;
                                        res.bestdir = i;
                                    }
                                    else if (bdirpoints != 0 && c == bdirpoints && c < 5
                                             && s_random (2) == 0)
                                        res.bestdir = i; // random if the points are equal
                                }
                            }
                        }
                    pos[i].x += dir_change[i].x;
                    pos[i].y += dir_change[i].y;
					if (dist > range && res.bestdir == -1) {
						res.dir |= (1 << i);
						res.bestdir = i;
					}
                }
            }
        }
    }
    return res;
};


/* count the points for dropping a bomb here 
   this will even count if there is any powerup*/
int
ai_bombpoints (_point pos, int range)
{
    int points = 0,
        d,
        r;
    _point p;

	if (pos.x < 0 || pos.x >= MAX_FIELDSIZE_X || pos.y < 0 || pos.y >= MAX_FIELDSIZE_Y) {
		printf ("WARNING: ai_bombpoints: pos out of range.\n");
		printf ("         pos.x=%d pos.y=%d range=%d\n", pos.x, pos.y, range);
		return 0;
	}
		
    if (map.field[pos.x][pos.y].type != FT_block && map.field[pos.x][pos.y].type != FT_stone
        && ai_checkfield (pos.x, pos.y)) {
        for (d = 0; d < 4; d++) {
            p = pos;

            for (r = 0;
                 (r < range
                  && (p.x >= 0 && p.x < map.size.x && p.y >= 0 && p.y < map.size.y) 
				  && (map.field[p.x][p.y].type == FT_nothing || map.field[p.x][p.y].type == FT_tunnel)); r++) {
                p.x += dir_change[d].x;
                p.y += dir_change[d].y;
            }

            if ((p.x >= 0 && p.x < map.size.x && p.y >= 0 && p.y < map.size.y)
				&& map.field[p.x][p.y].type != FT_nothing && map.field[p.x][p.y].type != FT_tunnel
                && (map.field[p.x][p.y].type != FT_block || map.field[p.x][p.y].type == FT_shoe
                    || map.field[p.x][p.y].type == FT_bomb || map.field[p.x][p.y].type == FT_fire))
                points++;
            if ((p.x >= 0 && p.x < map.size.x && p.y >= 0 && p.y < map.size.y)
				&& (map.field[p.x][p.y].type == FT_shoe || map.field[p.x][p.y].type == FT_bomb ||
                   map.field[p.x][p.y].type == FT_fire))
                points += 2;
        }
    }

    if (ai_easyrunaway (pos, range) == 0 || ai_findnearbombs (pos) != 0)
        points = 0;

    return points;
};


/* find the best position to go for dropping a bomb */
int
ai_findbestbombdir (_point pos, int dir, int range)
{
    int points[5] = { 0, 0, 0, 0, 0 };
    int d,
      done = 0,
        j,
        maxpoints = 0,
        bestd;
    _point p[4];

    points[4] = ai_bombpoints (pos, range);

    /* fill in the positions */
    for (d = 0; d < 4; d++) {
        p[d] = pos;
    }

    while (!done) {
        done = 1;

        for (d = 0; d < 5; d++) {

            if (d < 4) {
                p[d].x += dir_change[d].x;
                p[d].y += dir_change[d].y;
                if (p[d].x > 0 && p[d].y > 0 && p[d].x < map.size.x - 1 && p[d].y < map.size.y - 1) {
                    if (ai_checkfield (p[d].x, p[d].y)) {
                        /* we are opn a empty field go on with the test */
                        done = 0;
                        j = ai_bombpoints (p[d], range);
                        if (points[d] < j)
                            points[d] = j;
                    }
                    else        /* no empty field */
                        p[d].x = p[d].y = -1;
                }
            }

            if (maxpoints < points[d])
                maxpoints = points[d];
        }
    }

    bestd = 0;
    if (maxpoints > 2)
        maxpoints--;

    for (d = 0; d < 5; d++)
        if (points[d] >= maxpoints)
            bestd |= (1 << d);

    /* prevent from turning around */
    if (dir != -1 && (bestd & (0xFF - (1 << ai_invertdir (dir)))))
        bestd &= (0xFF - (1 << ai_invertdir (dir)));

    return bestd;
}


/* check if is there is a bom in the near
   returns the directions (bits 0(left)-3(down) bit4 bomb under us) where a bomb is */
int
ai_findnearbombs (_point pos)
{
    int d,
      res = 0,                  // result if there is a bomb
        done = 0;
    _point dist[4];             // to check every direction (on three ways)

    for (d = 0; d < 4; d++) {
        dist[d].x = pos.x + dir_change[d].x;
        dist[d].y = pos.y + dir_change[d].y;
	}
	
    while (!done) {
        done = 1;
        /* check every direction again */
        for (d = 0; d < 4; d++)
            if (dist[d].x >= 0 && dist[d].x < map.size.x &&
                dist[d].y >= 0 && dist[d].y < map.size.y) {
                if (map.bfield[dist[d].x][dist[d].y] != 0) {
                    res |= (1 << d); // set the bit for the direction;
                    dist[d].x = dist[d].y = -1; // don't check no more.
                }
                if (map.field[dist[d].x][dist[d].y].type != FT_nothing)
                    dist[d].x = dist[d].y = -1; // don't check no more.
                else if (dist[d].x != -1 && dist[d].y != -1) {
                    dist[d].x += dir_change[d].x;
                    dist[d].y += dir_change[d].y;
                    done = 0;
                }
            }
    }

    if (map.bfield[pos.x][pos.y] != 0)
        res |= 16;              // set the 4th. bit

    return res;
}


/* check if we are still running and fill out the position 
	return == 0 we're still walking ... else we have reached a point */
inline int
ai_checkpos (_player * pl, _point * pos)
{
    _pointf _p;

    _p.x = CUTINT (pl->pos.x);
    _p.y = CUTINT (pl->pos.y);

    pos->x = rintf (pl->pos.x);
    pos->y = rintf (pl->pos.y);

    return ((_p.x < 0.15f || _p.x > 0.85f) && (_p.y < 0.15f || _p.y > 0.85f));
};


/* random choose direction */
int
ai_choosedir (int dir, int nearbomb, int oldpos)
{
    int rdir[4];
    int bdir[4 * 3];
    int i,
      rnr,
      bnr;

    for (rnr = bnr = i = 0; i < 4; i++) {
        if ((dir & (1 << i)) != 0) {
            rdir[rnr] = i;
            rnr++;
        }
        if (((nearbomb & (DIRM_up + DIRM_down)) != 0) && ((dir & (1 << i)) != 0)
            && (i == left || i == right) && i != ai_invertdir (oldpos)) {
            bdir[bnr] = i;
            bnr++;
        }
        if (((nearbomb & (DIRM_left + DIRM_right)) != 0) && ((dir & (1 << i)) != 0)
            && (i == down || i == up) && i != ai_invertdir (oldpos)) {
            bdir[bnr] = i;
            bnr++;
        }
    }

    if (rnr == 0)
        return (s_random (4));

    if (bnr != 0)
        i = bdir[s_random (bnr)];
    else
        i = rdir[s_random (rnr)];

    return i;
};


inline int
ai_invertdir (int dir)
{
    int idir;
    switch (dir) {
    case (left):
        idir = right;
        break;
    case (right):
        idir = left;
        break;
    case (down):
        idir = up;
        break;
    default:
        idir = down;
        break;
    }
    return idir;
};


inline int
ai_checknewpos (_point pos, int d)
{
    _point m;
	
	m.x = pos.x + dir_change[d].x;
	m.y = pos.y + dir_change[d].y;

    return (ai_findnearbombs (m) == 0);
};


/* create a giving number of ai players */
int
single_create_ai (int num_players)
{
    int p,
      count,
	  try,
      gfx_sel,
      i = 0;
    _player *pl = NULL;

    for (count = 0; count < num_players; count++) {
        /* find free players */
        for (pl = NULL, p = 0; (pl == NULL && p < MAX_PLAYERS);p++)
            if (p < MAX_PLAYERS && !(PS_IS_used (players[p].state))) {
                pl = &players[p];
                sprintf (pl->name, "AI %d", p + 1);
                pl->state |= PSF_used + PSF_alife + PSF_playing + PSF_ai;
                pl->net.flags = NETF_firewall;
                sprintf (pl->net.addr.host, "localhost");
                sprintf (pl->net.addr.port, "11000");
				try = 0;
                do {
                    gfx_sel = s_random (gfx.player_gfx_count);
                    MW_IS_GFX_SELECT (gfx_sel, i);
					try++;
                } while (try < 100 && i != -1);
				pl->wins = 0;
				pl->points = 0;
				pl->team_nr = -1;
                player_set_gfx (pl, gfx_sel);
				team_choose (pl);
			}

        if (pl == NULL) {
			d_printf ("single_create_ai: No free Player found.\n"); 
            return -1;
		}
		else
			d_printf ("single_create_ai: Player %d Created  Name:%s\n", pl - players, pl->name); 
	}
	
	if (pl != NULL)
		return pl - players;
	return -1;	
};


/* single player game win/point screen 
 * 1. setup all player data
 * 2. setup second local player and ai players
 * 3. setup all other game related data
 */
void
single_playergame (int second_player, int ai_players)
{
    int p,
      done = 0;

	bman.p_nr = -1;
	bman.p2_nr = -1;
	
    /* delete player and teams from the game */
    for (p = 0; p < MAX_PLAYERS; p++) {
        players[p].points = 0;
        players[p].wins = 0;
        players[p].state = 0;
		players[p].team_nr = -1;
        players[p].gfx_nr = -1;
        players[p].gfx = NULL;
		memset (&players[p].net, 0x0, sizeof (_net_player));
    }

	for (p = 0; p < MAX_TEAMS; p++) {
		teams[p].wins = 0;
		teams[p].points = 0;
		
		for (done = 0; done < MAX_PLAYERS; done++)
			teams[p].players[done] = NULL;
	}
	
	done = 0;
    for (bman.p_nr = -1, p = 0; (bman.p_nr == -1 && p < MAX_PLAYERS); p++)
        if (!(PS_IS_used (players[p].state)))
            bman.p_nr = p;
	players[bman.p_nr].team_nr = 0;

    if (bman.p_nr >= MAX_PLAYERS) {
        printf ("ERROR in function (single_game_new): couldn't find any free player\n");
        exit (1);
    }

	strncpy (players[bman.p_nr].name, bman.playername, LEN_PLAYERNAME);
	do {
		done = playermenu_selgfx (bman.p_nr);
	} while (players[bman.p_nr].gfx_nr == -1 && done != -1);
		
    players[bman.p_nr].state = PSF_used + PSF_alife + PSF_playing;
    strncpy (players[bman.p_nr].name, bman.playername, LEN_PLAYERNAME);

	if (done != -1 && second_player) {
		player2_join ();
		do {
			done = playermenu_selgfx (bman.p2_nr);
		} while (players[bman.p2_nr].gfx_nr == -1 && done != -1);
		players[bman.p2_nr].team_nr = 0;
	}

	if (done == -1)
		return;
	
    single_create_ai (ai_players);
	
	if (bman.gametype == GT_team) {
		playermenu ();
		team_update ();
		ai_team_choosegfx ();
	}
	
    bman.state = GS_ready;

    while (!done && bman.state != GS_quit && bman.state != GS_startup) {
        single_game_new ();
        game_start ();
		bman.state = GS_running;
        game_loop ();
        game_end ();
    }
	
	gfx_blitdraw ();
	draw_logo ();
	gfx_blitdraw ();
};


/* single player loop for calculating the ai players */
void
single_loop ()
{
    int p;
    _player *pl;
    _point plpos;
    int nearbomb = 0,
        bestbdir,
        i;
    _airunaway rawdir;

    if (GT_MP_PTPS)             // we are not the master so no need for this.
        return;

    for (p = 0; p < MAX_PLAYERS; p++)
        if (p != bman.p_nr && PS_IS_aiplayer (players[p].state)) {
            if (PS_IS_alife (players[p].state)) {
                pl = &players[p];

                i = ai_checkpos (pl, &plpos);

                if (!i)
                    /* we're still moving */
                    pl->m = 1;
                else {
                    nearbomb = ai_findnearbombs (plpos);
                    if (nearbomb == 0) { // no bombs found
                        bestbdir = ai_findbestbombdir (plpos, pl->d, pl->range);
                        if (bestbdir & DIRM_under) {
                            if (ai_easyrunaway (plpos, pl->range) != 0)
                                player_drop_bomb (p);
                        }
                        else if (bestbdir == 0) {
                            pl->d = s_random (4);
                            pl->m = 1;
                        }
                        else {
                            pl->d = ai_choosedir (bestbdir, 0, pl->d);
                            pl->m = 1;
                        }
                        if (!ai_checknewpos (plpos, pl->d))
                            pl->m = 0;
                    }
                    else {
                        // bombs in the near found
                        rawdir = ai_runawayfrom (plpos, nearbomb, pl->range, 0);
                        if (rawdir.dir != 0 && rawdir.bestdir == -1) {
                            pl->d = ai_choosedir (rawdir.dir, nearbomb, pl->d); // we have to make a choice.. do it
                            pl->m = 1;
                        }
                        else if (rawdir.bestdir != -1) {
                            pl->d = rawdir.bestdir;
                            pl->m = 1;
                        } 
						else if (rawdir.bestdir == -1 && rawdir.dir == 0) {
							/* no good ways found, just run in the opposite direction of the bomb */
							if (map.field[(int) pl->pos.x][(int) pl->pos.y].type == FT_nothing ||
								map.field[(int) pl->pos.x][(int) pl->pos.y].type >= FT_tunnel)
									pl->m = 1;
						}
                    }

                    if (pl->m == 0 && map.field[(int) pl->pos.x][(int) pl->pos.y].type == FT_tunnel)
                        pl->m = 1;
                }
                player_ilness_loop (p);
                player_move (p);
            }
			else 
				player_checkdeath (p);
        }

};


/* singleplayer menü with some options you can make */
void
single_menu ()
{
    int i,
		p,
		eventstate = 0,
        done = 0,
		second_player = 0;
    _charlist nrplayerlist[MAX_PLAYERS + 1];
    _charlist *selnrplayer = &nrplayerlist[bman.ai_players];
	_menu *menu;
	_menuitem *aiplayer = NULL;
	SDL_Event event;

    /* fill in the nrplayerlist */
    for (p = 0, i = 0; p < MAX_PLAYERS + 1; i++) {
        sprintf (nrplayerlist[i].text, "%d", p);
        if (p < MAX_PLAYERS - 1)
            nrplayerlist[i].next = &nrplayerlist[i + 1];
        else
            nrplayerlist[i].next = NULL;
        p++;
    }
	
	game_resetdata ();
	
	menu = menu_new ("Single Game", 380,240);
	
	menu_create_text (menu, "numpl", 20, 50, 12, 2, COLOR_yellow, "Number of\nAI Players");
	aiplayer = menu_create_list (menu, "AI Players", 40, 90, 50, 100, nrplayerlist, &selnrplayer, 3);
	
	menu_create_button (menu,"Change Playernames" ,160, 50, 210, 4);

	menu_create_bool (menu, "Use Second Player", 160, 90, 210, &second_player, 5);

	menu_create_button (menu, "Game Options", 180, 130, 150, 6);
	menu_create_button (menu, "Map Options", 180, 170, 150, 7);
	menu_create_button (menu, "Main Menu", 30, 220, 150, 1);
	menu_create_button (menu, "Start", 220, 220, 150, 2);
	
	menu_focus_id (menu, 2);
	menu->looprunning = 1;
	menu_draw (menu);
	
	do {
		gfx_blitdraw ();
		eventstate = s_fetchevent (&event);

		done = menu_event_loop (menu, &event, eventstate);

		if (done > 0 && menu->focus->id == 6) {
			done = 0;
			mapgamesetting ();
		}
		
		if (done > 0 && menu->focus->id == 7) {
			done = 0;
			mapmenu ();
		}
		
		if (done > 0 && menu->focus->id == 4) {
			done = 0;
			playernamemenu ();
		}
		
		if (done > 0 && menu->focus->id == 3)
			done = 0;
		
		s_calctimesync ();
	} while (!done); 
	
	bman.ai_players = selnrplayer - &nrplayerlist[0];
	
	if (menu->focus->id == 2)
		single_playergame (second_player, bman.ai_players);

	menu_delete (menu);
};
