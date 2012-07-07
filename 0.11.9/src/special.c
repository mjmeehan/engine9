/* $Id: special.c,v 1.39 2007-01-12 22:42:31 stpohle Exp $ */
/* special.c - procedues to control the specials */

#include "bomberclone.h"
#include "basic.h"
#include "player.h"
#include "bomb.h"

void special_trigger (int p_nr) {
    int i,
      z = 0,
      ex_nr = bman.last_ex_nr;

    _player *p = &players[p_nr];

    // all triggered bombs will explode
    for (i = 0; i < MAX_BOMBS; i++)
        if (p->bombs[i].state == BS_trigger) {
            p->bombs[i].ex_nr = ex_nr + 5; // we take the next 5 number to be shure 
            bomb_explode (&p->bombs[i], 0); // no other explosion interfear with it.
            z++;                // count the bombs which will explode
        }

    if ((p_nr == bman.p_nr || p_nr == bman.p2_nr) && GT_MP && z)
        net_game_send_special (p_nr, ex_nr, p->special.type);

    if (z) {
        bman.last_ex_nr = ex_nr + 6;
        p->special.numuse--;
        if (!p->special.numuse)
            special_clear (p_nr);
    }
}


void special_row (int p_nr) {
    _bomb *b = NULL;
    _player *p = &players[p_nr];
    int x = (int) p->pos.x,
        y = (int) p->pos.y,
        dx = 0,
        dy = 0,
        t = 0,
        i;

    dx = dir_change[p->d].x;
    dy = dir_change[p->d].y;
	
    x += dx;
    y += dy;
    while (map.bfield[x][y]) {
        x += dx;
        y += dy;
        /* add one time tick to each bomb found to ensure that the explosion is infacted by the previous bomb
           otherwise powerups will disappear due to explosion of previous bomb */
        t++;
    }
    if (map.field[x][y].type == FT_nothing) {
        for (i = 0; ((i < p->bombs_n) && (p->bombs[i].state != BS_off)); i++);
        if (i < p->bombs_n) {
            b = &p->bombs[i];
            b->state = BS_ticking;
            b->r = p->range;
            b->ex_nr = -1;
            b->pos.x = x;
            b->pos.y = y;
            b->to = bman.bomb_tickingtime + t; // 5 Secs * 200
            map.bfield[x][y] = 1;
            if (GT_MP) {
                net_game_send_bomb (p_nr, i);
                if (GT_MP_PTPS)
                    b->to = b->to + 2 * RESENDCACHE_RETRY;
            }
        }
    }
}


void special_liquidmoved (int p_nr) {
    _bomb *b = NULL;
    _player *p = &players[p_nr];
    _point bombs[MAX_PLAYERS * MAX_BOMBS];

    int x = (int) p->pos.x,
        y = (int) p->pos.y,
        dx = 0,
        dy = 0,
        x1,
        y1,
        i;

    if ((CUTINT (p->pos.x) != 0.0f) || (CUTINT (p->pos.y) != 0.0f))
        return;

    dx = dir_change[p->d].x;
    dy = dir_change[p->d].y;
	x += dx;
	y += dy;
	
    // check that player is beside a bomb
    if (!map.bfield[x][y])
        return;

    x1 = x + dx;
    y1 = y + dy;

    // check the field behind the bomb
    if (map.bfield[x1][y1]
        || (map.field[x1][y1].type != FT_nothing && map.field[x1][y1].type != FT_tunnel))
        return;

    get_bomb_on ((float) x, (float) y, bombs);
    // move all bombs on that field (there should be only 1)
    for (i = 0; (bombs[i].x != -1) && (i < MAX_PLAYERS * MAX_BOMBS); i++) {
        b = &players[bombs[i].x].bombs[bombs[i].y];
        if (b->state != BS_exploding) {
            b->dest.x = dx;
            b->dest.y = dy;
            b->fdata = p->speed;
            if (p->special.type == SP_liquid)
                b->mode = BM_liquid;
            else
                b->mode = BM_moving;
            map.bfield[x][y] = 0;
            map.bfield[x1][y1] = 1;
            stonelist_add (x, y);
            if (GT_MP) {
                net_game_send_bomb (bombs[i].x, bombs[i].y);
            }
        }
    }
};


void
special_push (int p_nr)
{
    _bomb *b = NULL;
    _player *p = &players[p_nr];
    _point bombs[MAX_PLAYERS * MAX_BOMBS];

    int x = (int) p->pos.x,
        y = (int) p->pos.y,
        dx = 0,
        dy = 0,
        x1,
        y1,
        i;

    if ((CUTINT (p->pos.x) != 0.0f) || (CUTINT (p->pos.y) != 0.0f))
        return;

    dx = dir_change[p->d].x;
    dy = dir_change[p->d].y;
	
    x += dx;
    y += dy;

    // check that player is beside a bomb
    if (!map.bfield[x][y])
        return;

    x1 = x + dx;
    y1 = y + dy;

    // check the field behind the bomb
    if (map.bfield[x1][y1]
        || (map.field[x1][y1].type != FT_nothing && map.field[x1][y1].type != FT_tunnel))
        return;

    get_bomb_on (x, y, bombs);
    // move all bombs on that field (there should be only 1)
    for (i = 0; bombs[i].x != -1; i++) {
        b = &players[bombs[i].x].bombs[bombs[i].y];
        if (b->state != BS_exploding) {
            b->dest.x = dx;
            b->dest.y = dy;
            b->fdata = p->speed;
            b->mode = BM_pushed;
            map.bfield[x][y] = 0;
            map.bfield[x1][y1] = 1;
            stonelist_add (x, y);
            if (GT_MP) {
                net_game_send_bomb (bombs[i].x, bombs[i].y);
            }
        }
    }
}

/* 
 * kick the bomb over the field
 */
#define KICK_MAXTRY 20
void special_kick (int p_nr) {
    _bomb *b = NULL;
    _player *p = &players[p_nr];
    _point bombs[MAX_PLAYERS * MAX_BOMBS];

    int x = (int) p->pos.x,
        y = (int) p->pos.y,
        dx = 0,
        dy = 0,
        x1,y1,					// new position
		i,
        trycnt = KICK_MAXTRY,	// maximum number of trys to kick the bomb.
        r;

    if ((CUTINT (p->pos.x) != 0.0f) || (CUTINT (p->pos.y) != 0.0f))
        return;

    dx = dir_change[p->d].x;
    dy = dir_change[p->d].y;
	
    x += dx;
    y += dy;

    // check that player is beside a bomb
    if (!map.bfield[x][y])
        return;
    /* calculate a new destination for the bomb
       (the new dest has to be in the direction of that bomb
       with max angle of 45 degree and distance SPECIAL_KICK_MAXDIST
       if the bomb kickt to the border of maze, nothing happens.)        */
    do {
        trycnt--;
        r = s_random (SPECIAL_KICK_MAXDIST) + 1;
        if (dx != 0) {
            x1 = x + dx * r;
            y1 = y + s_random (r * 2 + 1) - r;
        }
        else {
            y1 = y + dy * r;
            x1 = x + s_random (r * 2 + 1) - r;
        }
        // check if within maze
        if ((x1 >= 0) && (x1 < map.size.x) && (y1 >= 0) && (y1 < map.size.y)) {
            // check if that field is emty
            if (!map.bfield[x1][y1]
                && (map.field[x1][y1].type == FT_nothing || map.field[x1][y1].type == FT_tunnel)) {
                // move bomb to new destination
			    get_bomb_on (x, y, bombs);
				for (i = 0; bombs[i].x != -1; i++) {
                    b = &players[bombs[i].x].bombs[bombs[i].y];
                    if (b->state != BS_exploding) {              
						b->dest.x = x1;
                        b->dest.y = y1;
                        b->fdata = 0.0f;
                        b->mode = BM_kicked;
                        b->source.x = x;
                        b->source.y = y;
                        map.bfield[x][y] = 0;
                        stonelist_add (x, y);
						
                        if (GT_MP) net_game_send_bomb (bombs[i].x, bombs[i].y);
					}
                }
                trycnt = 0;
            }
        }
    } while (trycnt > 0);
}
#undef KICK_MAXTRY


void
special_pickup (int p_nr, int s_nr)
{
    _special *s = &players[p_nr].special;

	if (s->type != s_nr)
		special_clear (p_nr);
    s->to = 0;
    s->numuse = 0;
    s->type = s_nr;
    switch (s_nr) {
    case SP_trigger:
        s->numuse = SPECIAL_TRIGGER_NUMUSE;
        s->to = SPECIAL_TRIGGER_TIME;
        break;
    case SP_row:
        s->to = SPECIAL_ROW_TIME;
        break;
    case SP_push:
    case SP_moved:
    case SP_liquid:
        s->to = SPECIAL_PUSH_TIME;
        break;
    case SP_kick:
        s->to = SPECIAL_KICK_TIME;
        break;
    }

    bman.updatestatusbar = 1;
}


void special_clear (int p_nr) {
    if (players[p_nr].special.type == SP_trigger) {
        _bomb *bomb;
        int i;
        /* put all bombs to normal and if the timeout is higher as usual 
           set it to normal */
        for (i = 0; i < MAX_BOMBS; i++) {
            bomb = &players[p_nr].bombs[i];
            if (bomb->state == BS_trigger && !players[p_nr].special.use) {
                bomb->state = BS_ticking;
                if (bomb->to > bman.bomb_tickingtime)
                    bomb->to = bman.bomb_tickingtime;
            }
        }
    }

    players[p_nr].special.type = 0;
    bman.updatestatusbar = 1;
	if (bman.p_nr == p_nr || bman.p2_nr == p_nr)
		net_game_send_special (p_nr, -1, SP_clear);
}


void special_loop () {
    _special *s;
    int p_nr;

    for (p_nr = 0; p_nr < MAX_PLAYERS; p_nr++) {
        s = &players[p_nr].special;

        if (s->use) {
            switch (s->type) {
            case SP_trigger:
                special_trigger (p_nr);
                break;
            case SP_row:
                if (players[p_nr].m)
                    special_row (p_nr);
                break;
            case SP_push:
                if (players[p_nr].m)
                    special_push (p_nr);
                break;
            case SP_liquid:
            case SP_moved:
                if (players[p_nr].m)
                    special_liquidmoved (p_nr);
                break;
            case SP_kick:
                if (players[p_nr].m)
                    special_kick (p_nr);
                break;
            }
        	s->use = 0;
    	}

 	   if (s->type && (s->to > 0.0f)) {
    	    s->to -= timediff;
        	if (s->to <= 0.0f) 		s->clear = 1;
    	}
		
		if (s->clear) {
			s->clear = 0;
           	special_clear (p_nr);
 		}			
	}
}


void special_use (int p_nr) {
    players[p_nr].special.use = 1;
}
