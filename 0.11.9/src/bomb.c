/* $Id: bomb.c,v 1.71 2009-05-11 20:51:25 stpohle Exp $ */
/* everything what have to do with the bombs */

#include "bomberclone.h"
#include "player.h"
#include "bomb.h"
#include <math.h>

void draw_bomb (_bomb * bomb) {
    SDL_Rect src, dest;
    int x = floorf (bomb->oldpos.x), y = floorf (bomb->oldpos.y);

    if (bomb->pos.x < 0 || bomb->pos.y < 0 || bomb->pos.x >= map.size.x || bomb->pos.y >= map.size.y) {
        d_printf ("FATAL: Draw Bomb out of range [%f,%f]\n", bomb->pos.x, bomb->pos.y);
        return;
    }

    if (bomb->state != BS_trigger || ((bomb->state == BS_trigger) && (bomb->to < bman.bomb_tickingtime))) {
        /*
		 * check the framenumber
		 */
        bomb->frame += (timefactor / 3.0);
        if (bomb->frame < 0 || bomb->frame >= gfx.bomb.frames)
            bomb->frame = 0.0f;
    }

    dest.w = src.w = gfx.bomb.image->w;
    dest.h = src.h = gfx.block.y;

    dest.x = gfx.offset.x + bomb->pos.x * gfx.block.x;
    dest.y = gfx.offset.y + bomb->pos.y * gfx.block.y;

    src.x = 0;
    src.y = src.h * (int) bomb->frame;
	
	/* delete the old position of the bomb */
	if (bomb->oldpos.x >= 0.0f && bomb->oldpos.y >= 0.0f) {
		stonelist_add (x, y);
	    if (bomb->mode != BM_normal) {
    	    stonelist_add (x + 1, y);
	        stonelist_add (x, y + 1);
    	    stonelist_add (x + 1, y + 1);
		}
	}
	else 
		stonelist_add (bomb->pos.x, bomb->pos.y);
	
	/* save the current position */
	bomb->oldpos = bomb->pos;

	if (bomb->mode == BM_kicked)  y++;
    gfx_blit (gfx.bomb.image, &src, gfx.screen, &dest, (y * 256) + 2);
};



/*
 * the bomb is going to explode, prepare all values,
 * set: ex_nr    - explosion number for the bomb, and all resulting explosions
 *      to       - timeout
 *      firer[d] - range of the bomb. 
 */
void bomb_explode (_bomb *bomb, int net) {
    int d;

    d_printf ("Bomb Explode p:%d, b:%d, pI:%d, exe_nr:%d [%f,%f]\n", bomb->id.p, bomb->id.b, bomb->id.pIgnition, bomb->ex_nr, bomb->pos.x, bomb->pos.y);

    if (bomb->ex_nr == -1)
        bomb->ex_nr = bman.last_ex_nr++; // set bomb explosion id
	
    players[bomb->id.p].bomb_lastex = bomb->id.b;
    bomb->to = EXPLOSIONTIMEOUT; /* set the timeout for the fireexplosion */
    bomb->state = BS_exploding;

	explosion_check_field ((int)bomb->pos.x, (int)bomb->pos.y, bomb);

    for (d = 0; d < 4; d++) {
		bomb->firer[d] = 0.0f;
		bomb->firemaxr[d] = 0;
		map.field[(int)bomb->pos.x][(int)bomb->pos.y].ex[d].count++;
   		map.field[(int)bomb->pos.x][(int)bomb->pos.y].ex[d].frame = 0.0f;
		map.field[(int)bomb->pos.x][(int)bomb->pos.y].ex_nr = bomb->ex_nr;
		map.field[(int)bomb->pos.x][(int)bomb->pos.y].ex[d].bomb_p = bomb->id.p;
		map.field[(int)bomb->pos.x][(int)bomb->pos.y].ex[d].bomb_b = bomb->id.b;
		stonelist_add ((int)bomb->pos.x, (int)bomb->pos.y);
    }

    if (GT_MP_PTPM && net)      /* from now on only the server let the bomb explode */
        net_game_send_bomb (bomb->id.p, bomb->id.b);

    snd_play (SND_explode);
};


/* moves the bomb with it's speed,
   dest.x|y = dx, dy of the current move */
void bomb_move (_bomb * bomb) {
    int keepdir = 0;
    _pointf fpos,
      rpos;
    float dist = 0.0f,
        step = 0.0f;

    map.bfield[(int) bomb->pos.x][(int) bomb->pos.y] = 0; /* delete bfield */
    stonelist_add (bomb->pos.x, bomb->pos.y);

    /* do this once, and again if the direction is still ok */
    do {
        /* get the current position of the bomb */
        fpos.x = (int) bomb->pos.x;
        fpos.y = (int) bomb->pos.y;
        rpos.x = CUTINT (bomb->pos.x);
        rpos.y = CUTINT (bomb->pos.y);

        /* calculate the next step speed or next full field.. 
           depend on what is the smaler one */
        if (bomb->dest.x < 0)
            step = rpos.x;
        else if (bomb->dest.x > 0) {
            step = 1.0f - rpos.x;
            fpos.x += 1.0f;
        }
        else if (bomb->dest.y < 0)
            step = rpos.y;
        else if (bomb->dest.y > 0) {
            step = 1.0f - rpos.y;
            fpos.y += 1.0f;
        }

        if (step > (timefactor * bomb->fdata) || step == 0.0f)
            step = (timefactor * bomb->fdata);

        /* move the bomb to the new position */
        if (bomb->dest.x < -0.5f)
            bomb->pos.x -= step;
        else if (bomb->dest.x > 0.5f)
            bomb->pos.x += step;
        else if (bomb->dest.y < -0.5f)
            bomb->pos.y -= step;
        else if (bomb->dest.y > 0.5f)
            bomb->pos.y += step;
		
        /* if we are on a complete field, check if we
           can move to the next one */
        if ((CUTINT (bomb->pos.x) == 0.0f) && (CUTINT (bomb->pos.y) == 0.0f)) {
            if (bomb->mode == BM_pushed)
                bomb->mode = BM_normal;
            else if (bomb->mode == BM_moving || bomb->mode == BM_liquid) {
                /* it is a moving liquid bomb so check for another field */
                _point b,
                  d;

                b.x = (int) bomb->pos.x;
                b.y = (int) bomb->pos.y;
                d.x = b.x + bomb->dest.x;
                d.y = b.y + bomb->dest.y;

                if (map.bfield[d.x][d.y] == 0
                    && (map.field[d.x][d.y].type == FT_nothing
                        || map.field[d.x][d.y].type == FT_tunnel))
                    /* this direction is still oky */
                    keepdir = 1;
                else if (bomb->mode == BM_liquid) {
                    /* liquid bomb so move to the other side */
                    keepdir = 0;
                    bomb->dest.x = -bomb->dest.x;
                    bomb->dest.y = -bomb->dest.y;
                }
                else {
                    /* stop moving this bomb */
                    keepdir = 0;
                    bomb->mode = BM_normal;
                }

                /* if a network game is running send bomb data with the
                   current information */
                if (GT_MP) {
                    int b = -1,
                        i = 0;

                    do {
                        if (&players[bman.p_nr].bombs[i] == bomb)
                            b = i;
                        i++;
                    } while (b == -1 && i < MAX_BOMBS);

					/* if the bomb from our player send the current data again.. */
                    if (b != -1)
                        net_game_send_bomb (bman.p_nr, b);
                }
            }
        }
        dist += step;
    } while (dist < (timefactor * bomb->fdata)
             && (bomb->mode == BM_liquid || bomb->mode == BM_moving) && keepdir);

    map.bfield[(int) bomb->pos.x][(int) bomb->pos.y] = 1; /* set new bfield */
    stonelist_add (bomb->pos.x, bomb->pos.y);
}


void bomb_loop () {
    int p,
      i;
    _player *player;
    _bomb *bomb;

    for (p = 0; p < MAX_PLAYERS; p++) {
        player = &players[p];
        if ((players[p].state & PSFM_used) != 0) {
            for (i = 0; i < MAX_BOMBS; i++) {
                bomb = &player->bombs[i];
                switch (bomb->state) {
                case BS_ticking:
                case BS_trigger:
                    if (GT_MP_PTPM || GT_SP) {
                        bomb->to -= timediff;
                        if (bomb->to <= 0.0f) // bomb will have to explode in the next loop
                            bomb_explode (bomb, 1);
                        else
                            draw_bomb (bomb);
                    }
                    else {
                        bomb->to -= timediff;
                        if (bomb->to <= 0.0f) { // bomb did not explode -> resend bombdata
                            if (bomb->state == BS_ticking)
                                bomb->to = bman.bomb_tickingtime;
                            else
                                bomb->to = SPECIAL_TRIGGER_TIMEOUT;
                            net_game_send_bomb (bman.p_nr, i);
                            bomb->to = bomb->to + 2 * RESENDCACHE_RETRY;
                        }
                        draw_bomb (bomb);
                    }
					/* 
					 * if the bomb is moving or something... 
					 */
                    if (bomb->mode != BM_normal)
                        bomb_action (bomb);
                    break;

                case BS_exploding:
                    if (bomb->to > 0.0f) {
                        explosion_do (bomb);
                    }
                    else if (bomb->to <= 0.0f) { // explosion done
                        explosion_restore (bomb);
                        bomb->to = 0.0f;
                        bomb->state = BS_off;
						bomb->oldpos.x = bomb->oldpos.y = -1.0f; 
                    }
                    bomb->to -= timediff;
                    break;
				}
            }
        }
	}
    return;
};


/*
 * check if on the givin place is a bomb 
 * bombs[].x = player, bombs[].y = bombnumber
 */
void get_bomb_on (float x, float y, _point bombs[]) {
    int p,
      b,
      i;
    _bomb *bomb;

    for (i = 0, p = 0; p < MAX_PLAYERS; p++)
        if ((players[p].state & PSFM_used) != 0) {
            for (b = 0; b < MAX_BOMBS; b++) {
                bomb = &players[p].bombs[b];
                if (bomb->state == BS_ticking || bomb->state == BS_trigger) {
                    if (bomb->pos.x - 1.0f < x && bomb->pos.x + 1.0f > x && bomb->pos.y - 1.0f < y
                        && bomb->pos.y + 1.0f > y) {
                        bombs[i].x = p;
                        bombs[i].y = b;
                        i++;
                    }
                }
            }
        }
    bombs[i].x = bombs[i].y = -1;
};


/*
 * restore the bombexplosion,
 * will be excecuted after a explosion has finished.
 * delete all old explosion data from the field.
 */
void explosion_restore (_bomb *bomb) {
    int i,
      d,
      _x,
      _y;
	
	d_printf ("explosion_restore: %f,%f\n", bomb->pos.x, bomb->pos.y);	
    for (d = 0; d < 4; d++) {
        _x = bomb->pos.x;
        _y = bomb->pos.y;

		if (map.field[_x][_y].ex[d].count > 0)
			map.field[_x][_y].ex[d].count--;
		map.field[_x][_y].ex[d].frame = 0.0f; // reset the framenumber
		/* refresh the screen here.. */
		if (d==3)
			stonelist_add (_x, _y);

		
		/* with every field where was an fire on it decrease the ex[].count value 
		 * and force an drawing of this field */
        for (i = 0; i < rintf(bomb->firer[d]); i++) {	// lower the number of explosions
            _x = _x + dir_change[d].x;
            _y = _y + dir_change[d].y;

            if (map.field[_x][_y].ex[d].count > 0)
				map.field[_x][_y].ex[d].count--;
			map.field[_x][_y].ex[d].frame = 0.0f; // reset the framenumber
            stonelist_add (_x, _y);
        }

        /* delete the stone completly if there was any in the way 
		 * push the values field->type = fiels->special */
        if (bomb->firer[d] <= bomb->r && map.field[_x][_y].type != FT_block
            && map.field[_x][_y].type != FT_tunnel && bomb->ex_nr != map.field[_x][_y].ex_nr) {

            map.field[_x][_y].ex_nr = bomb->ex_nr;
            map.field[_x][_y].frame = 0.0f;
            if (map.field[_x][_y].special != FT_nothing) {
                map.field[_x][_y].type = map.field[_x][_y].special;
                map.field[_x][_y].special = FT_nothing;
            }
            else
                map.field[_x][_y].type = FT_nothing;

            stonelist_add (_x, _y);

            if (GT_MP_PTPM)     /* send only if we are the master */
                net_game_send_field (_x, _y);
        }
    }
    _x = bomb->pos.x;
    _y = bomb->pos.y;

    /* delete field from the bfield map */
    if (bomb->mode == BM_moving || bomb->mode == BM_pushed || bomb->mode == BM_liquid)
        map.bfield[(int) bomb->pos.x + (int)bomb->dest.x][(int) bomb->pos.y + (int)bomb->dest.y] = 0;

    map.bfield[(int) bomb->pos.x][(int) bomb->pos.y] = 0;
};



/*
 * draw the explosion as far as she got 
 */
void explosion_draw (_bomb * bomb) {
    int d,
      r;
    _point p;

    for (d = 0; d < 4; d++) {
        p.x = bomb->pos.x;
        p.y = bomb->pos.y;

        for (r = 0; r < bomb->firer[d]; r++) {
			map.field[p.x][p.y].ex[d].bomb_p = bomb->id.p;
			map.field[p.x][p.y].ex[d].bomb_b = bomb->id.b;
            map.field[p.x][p.y].ex[d].frame += timefactor;
            if (map.field[p.x][p.y].ex[d].frame >= gfx.fire.frames)
                map.field[p.x][p.y].ex[d].frame = 0.0f;
            stonelist_add (p.x, p.y);
            p.x += dir_change[d].x;
            p.y += dir_change[d].y;
        }
    }
}


/*
 * calculate the explosion itself,
 * check the direction of the explosion and and and
 *
 * as long as the explosion grows, make sure the explosion will reach it's 'maximum' 
 * (MAX_RANGE) end at 1.5 seconds. Make sure all explosion will have the same speed.
 *    range += (MAX_RANGE/1.0) * timediff;
 * 
 */
void explosion_do (_bomb *bomb) {
	int d, dx, dy, ftype;
	float range_grow = (timediff * (float) MAX_RANGE)/EXPLOSION_GROW_SPEED;
	float new_range;
	float step;
	
    if (bomb->state == BS_exploding) {
		for (d = 0; d < 4; d++) {
			if (bomb->firemaxr[d] == 0 && bomb->firer[d] < bomb->r) {
				
				new_range = bomb->firer[d] + range_grow;
				if (new_range > bomb->r) new_range = bomb->r;

				/* check all fields between [dx,dy] and [odx,ody] */
				while (bomb->firemaxr[d] == 0 && (bomb->firer[d] < new_range)) {
					step = new_range - bomb->firer[d];

					if (step > 1.0f) {
						step = 1.0f;
					}
					bomb->firer[d] += step;
					
					dx = rintf(bomb->pos.x + dir_change[d].x * bomb->firer[d]);
					dy = rintf(bomb->pos.y + dir_change[d].y * bomb->firer[d]);
					
					ftype = explosion_check_field (dx, dy, bomb);
					if (ftype == FT_nothing || ftype == FT_tunnel) {
						if (map.field[dx][dy].ex[d].count == 0 || map.field[dx][dy].ex_nr != bomb->ex_nr)
							map.field[dx][dy].ex[d].count++;
   		        		map.field[dx][dy].ex[d].frame = 0.0f;
						map.field[dx][dy].ex_nr = bomb->ex_nr;
						map.field[dx][dy].ex[d].bomb_p = bomb->id.p;
						map.field[dx][dy].ex[d].bomb_b = bomb->id.b;
						stonelist_add (dx, dy);
					}
					else {
						bomb->firemaxr[d] = 1;
						bomb->firer[d] = rintf (bomb->firer[d]);
					}
				}
			}

		}
        explosion_draw (bomb);
	}
};



/* check the field if there is another bomb stone or wathever
 * if a bomb is found let this one explode, on a player well this player
 * will die and if a stone was found, start with the stone explosion 
 * RETURN: value of the stonetype (FT_*) */
int explosion_check_field (int x, int y, _bomb *bomb)
{
    int pl[MAX_PLAYERS];
    int i;
    _point bo[MAX_PLAYERS * MAX_BOMBS];
    _bomb *tmpbomb;
    _player *tmpplayer;

    if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y)
        return FT_block;

    get_player_on (x, y, pl);
    get_bomb_on (x, y, bo);

    /* check if any bomb have to explode.. */
    for (i = 0; bo[i].x != -1; i++) {
        tmpbomb = &players[bo[i].x].bombs[bo[i].y];
        if (tmpbomb != bomb && tmpbomb->state != BS_exploding 
			&& tmpbomb->mode != BM_kicked) {
              tmpbomb->ex_nr = bomb->ex_nr; // set the ex_nr to identify explosions
              // Add propagation of owner first explosion propagation
              tmpbomb->id.pIgnition = bomb->id.pIgnition;
              // d_printf ("explosion_check_field: launch bomb_explode == ex_nr:%d pl:%d pli:%d\n", tmpbomb->ex_nr, tmpbomb->id.p, tmpbomb->id.pIgnition);	
              bomb_explode (tmpbomb, 1);
        }
    }

    // check if any player is in the explosion
    for (i = 0; pl[i] != -1; i++) {
        tmpplayer = &players[pl[i]];
        if (((tmpplayer->state & PSF_alife) != 0)
            && (GT_SP
				|| (GT_MP && (&players[bman.p_nr] == tmpplayer || (IS_LPLAYER2 && &players[bman.p2_nr] == tmpplayer)))
                || (GT_MP_PTPM && PS_IS_aiplayer (tmpplayer->state)))){

                    // Check if the bomb owner is the player killed in this case the killer is owner of bomb ignition
                    if (tmpplayer == &players[bomb->id.p]) // suicide case
                        player_died (tmpplayer, bomb->id.pIgnition, 0);
                    else 
            player_died (tmpplayer, bomb->id.p, 0);
    }
    }

    // let the stones right beside explode
    if (map.field[x][y].type != FT_nothing && map.field[x][y].type != FT_tunnel
        && map.field[x][y].type != FT_block && bomb->ex_nr != map.field[x][y].ex_nr)
        if (map.field[x][y].frame <= 0.0f) {
            map.field[x][y].frame = 1.0f;

            stonelist_add (x, y);
        }

    return map.field[x][y].type;
};


/*
 * the bomb was kicked.. so move the bomb in the right way..
 */
void bomb_kicked (_bomb * bomb) {
	float dist, dX, dY, pX, pY;
	
	pX = dX = bomb->dest.x - bomb->source.x;
	pY = dY = bomb->dest.y - bomb->source.y;
	if (pX < 0.0f) pX = -dX;
	if (pY < 0.0f) pY = -dY;
	if (pX == 0.0f) dist = pY;
	else if (pY == 0.0f) dist = pX;
	else {
		dist = sqrtf (powf (pX,2) + powf (pY,2));
	}
	
	bomb->fdata += timediff; // * (SPECIAL_KICK_MAXDIST / dist);
	if (bomb->fdata >= 1.0f) {
		bomb->pos.x = bomb->dest.x;
		bomb->pos.y = bomb->dest.y;
		bomb->mode = BM_normal;
		bomb->fdata = 0.0f;
		map.bfield[(int)bomb->dest.x][(int)bomb->dest.y] = 1;
	}
	else {
		bomb->pos.x = bomb->source.x + dX * bomb->fdata;
		bomb->pos.y = (bomb->source.y + dY * bomb->fdata) - (4.0-(4.0 * powf ((-0.5+bomb->fdata)*2.0f, 2.0f)));
	}
};


inline void bomb_action (_bomb * bomb)
{
    switch (bomb->mode) {
    case (BM_moving):
    case (BM_liquid):
    case (BM_pushed):
        bomb_move (bomb);
        break;
    case (BM_kicked):
		bomb_kicked (bomb);
        break;
    default:
        bomb->mode = BM_normal;
        break;
    }
};
