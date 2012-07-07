/* $Id: player.c,v 1.108 2009-10-11 17:14:47 stpohle Exp $
 * player.c - everything what have to do with the player */

#include <SDL.h>
#include "bomberclone.h"
#include "network.h"
#include "packets.h"
#include "sound.h"
#include "flyingitems.h"
#include "player.h"
#include "bomb.h"
#include "menu.h"

void
draw_player (_player * player)
{
    SDL_Rect src,
      dest;
    int i;
	
    if ((int)player->pos.x < 0 || (int)player->pos.x >= map.size.x ||
        (int)player->pos.y < 0 || (int)player->pos.y >= map.size.y) {
        d_printf ("FATAL: Draw Player out of range : [%f,%f]\n", player->pos.x,
                  player->pos.y);
        return;
    }

    if (PS_IS_alife (player->state)) {
        /* player is alife */
		while (player->frame >= player->gfx->ani.frames)
			player->frame -= player->gfx->ani.frames;
	
        dest.w = src.w = player->gfx->ani.w;
        dest.h = src.h = player->gfx->ani.h;
        dest.x =
            gfx.offset.x + player->gfx->offset.x + player->pos.x * gfx.block.x;
        dest.y =
            gfx.offset.y + player->gfx->offset.y + player->pos.y * gfx.block.y;
        src.x = player->d * player->gfx->ani.w;
        src.y = (int)player->frame * player->gfx->ani.h;

        gfx_blit (player->gfx->ani.image, &src, gfx.screen, &dest, (player->pos.y * 256) + 128);

        /* if the player is ill, draw this image above him */
        for (i = PI_max - 1; (i >= 0) && (player->ill[i].to <= 0.0f); i--);
          if (i >= 0) {
			  player->illframe += timefactor;
	    	  while (player->illframe >= gfx.ill.frames)
			     player->illframe -= gfx.ill.frames;
			  
              dest.w = src.w = gfx.block.x * 2;
              dest.h = src.h = gfx.block.y * 2;
              src.x = 0;
              src.y = (int)player->illframe * (2 * gfx.block.y);
              dest.x =
                  gfx.offset.x + ((player->pos.x-0.5f) * gfx.block.x);
              dest.y =
                  gfx.offset.y + ((player->pos.y - 1.0f) * gfx.block.y);
              gfx_blit (gfx.ill.image, &src, gfx.screen, &dest, (player->pos.y*256) + 129);
	      }
    }

    else if (PS_IS_respawn (player->state)) {
        /* player is respawning 
		 * first: draw the star */

        dest.w = src.w = gfx.respawn.image->w;
        dest.h = src.h = player->gfx->ani.h;

        dest.x = gfx.offset.x + ((player->pos.x-0.5f) * gfx.block.x);
        dest.y = gfx.offset.y + ((player->pos.y - 1.0f) * gfx.block.y);

        src.x = 0;
		if ((int)player->frame < gfx.respawn.frames)
			src.y = (2 * gfx.block.y) * (int)player->frame;
		else
			src.y = (2 * gfx.block.y) * (int)(gfx.respawn.frames - (((int)player->frame) - gfx.respawn.frames - 1));

        gfx_blit (gfx.respawn.image, &src, gfx.screen, &dest, 0xFFFF);

		if (player->frame > gfx.respawn.frames) {
			/* second: draw the player behind the star */
			dest.x =
            	gfx.offset.x + player->gfx->offset.x + player->pos.x * gfx.block.x;
        	dest.y =
            	gfx.offset.y + player->gfx->offset.y + player->pos.y * gfx.block.y;
			
	        dest.w = src.w = player->gfx->ani.w;
    	    dest.h = src.h = player->gfx->ani.h;
			
	        src.x = 0;
    	    src.y = 0;

        	gfx_blit (player->gfx->ani.image, &src, gfx.screen, &dest, 0xE000);
		}
	}
	
    else {
        /* player is dead */
        dest.w = src.w = gfx.dead.image->w;
        dest.h = src.h = player->gfx->ani.h;

        dest.x =
            gfx.offset.x + player->gfx->offset.x + player->pos.x * gfx.block.x;
        dest.y =
            gfx.offset.y + player->gfx->offset.y + player->pos.y * gfx.block.y;

        src.x = 0;
        src.y = (2 * gfx.block.y) * (int)player->frame;

        gfx_blit (gfx.dead.image, &src, gfx.screen, &dest, 0xFFFF);
    }
	
    player->old = player->pos;  // save this position
};


void
restore_players_screen ()
{
    int i,
      x,
      xs,
      xe,
      y,
      ys,
      ye;

    for (i = 0; i < MAX_PLAYERS; i++)
        if ((PS_IS_used (players[i].state)) && players[i].old.x >= 0.0f ) {

            if (players[i].old.x < 0.0f || players[i].old.x >= map.size.x
                || players[i].old.y < 0.0f || players[i].old.y >= map.size.y)
                d_printf ("FATAL: Restore Player out of range : playernr %d [%f,%f]\n", i,
                          players[i].old.x, players[i].old.y);
            else {
				// start and end position for the stones to redraw X Position
                if (CUTINT(players[i].old.x) > 0.5f) {
                    x = players[i].old.x;
                    xe = players[i].old.x + 2;
                }
                else {
                    x = players[i].old.x - 1;
                    xe = players[i].old.x + 1;
                }
                if (x < 0)
                    x = 0;
                if (xe >= map.size.x)
                    xe = map.size.x - 1;
				// start and end position for the stones to redraw X Position
                ys = players[i].old.y - 1;
                ye = players[i].old.y + 1;
                if (ys < 0)
                    ys = 0;
                if (ye >= map.size.y)
                    ye = map.size.y - 1;
				// redrawing of the stone
                xs = x;
                for (; x <= xe; x++)
                    for (y = ys; y <= ye; y++)
                        stonelist_add (x, y);
            }
        }
};


void
player_check_powerup (int p_nr)
{
    _player *p = &players[p_nr];
    int fx = p->pos.x;
    int fy = p->pos.y;
    int ft,
      i;

    if (PS_IS_netplayer (p->state) && (bman.p2_nr != p_nr || bman.p2_nr == -1))
        return;

    /* Get the right field position */
    if (CUTINT(p->pos.x) > 0.5)
        fx = fx + 1;
    if (CUTINT(p->pos.y) > 0.5)
        fy = fy + 1;
    ft = map.field[fx][fy].type;

    /* we found a mixed powerup */
    if (ft == FT_mixed) {
        i = s_random (6);
        switch (i) {
        case 0:
            ft = FT_bomb;
            break;
        case 1:
            ft = FT_fire;
            break;
        case 2:
            ft = FT_shoe;
            break;
        case 3:
        case 4:
        case 5:
            ft = FT_death;
            break;
        }
    }

    switch (ft) {
        /* we found a bomb powerup */
    case FT_bomb:
        if (p->bombs_n < MAX_BOMBS && p->ill[PI_nobomb].to <= 0.0) {
            p->bombs_n++;
            bman.updatestatusbar = 1;
        }
        field_clear (fx, fy);
        break;
        /* we found a fire powerup */
    case FT_fire:
        if (p->range < MAX_RANGE && p->ill[PI_range].to <= 0.0) {
            p->range++;
            bman.updatestatusbar = 1;
        }
        field_clear (fx, fy);
        break;
        /* we found a shoe powerup */
    case FT_shoe:
        if (p->speed < MAX_SPEED && p->ill[PI_slow].to <= 0.0) {
            bman.updatestatusbar = 1;
            p->speed *= SPEEDMUL;
			p->collect_shoes++;
        }
        field_clear (fx, fy);
        break;
        /* we found a death ?powerup? */
    case FT_death:
        player_set_ilness (p, -1, ILL_TIMEOUT);
        bman.updatestatusbar = 1;
        if (GT_MP)			/* maybe we have to check if it is our player */
            net_game_send_ill (p_nr);
        field_clear (fx, fy);
        break;
        /* we found a special */
    case FT_sp_trigger:
    case FT_sp_row:
    case FT_sp_push:
    case FT_sp_moved:
    case FT_sp_liquid:
    case FT_sp_kick:
        special_pickup (p_nr, ft - FT_sp_trigger + 1);
        bman.updatestatusbar = 1;
        field_clear (fx, fy);
        break;
    }
};


/*
  check the givin field. if we are able to move on it
  fx,fy = position on the field
*/
int
check_field (short int x, short int y)
{
    int res = 0;

    if (map.field[x][y].type != FT_stone && map.field[x][y].type != FT_block)
        res = 1;

    return res;
}


/* check if there is any explosion on this field */	
int check_exfield (short int x, short int y) {
    int res = 1, i;

    if (map.field[x][y].type == FT_stone || map.field[x][y].type == FT_block)
        res = 0;
	
	for (i = 0; (i < 4 && res == 1); i++)
		if (map.field[x][y].ex[i].count > 0)
			res = 0;

	return res;
}


/* make only a smal step until i can go around the corner
   return the rest speed for this move */
float
stepmove_player (int pl_nr)
{
    _point bomb1[MAX_PLAYERS * MAX_BOMBS],
      bomb2[MAX_PLAYERS * MAX_BOMBS];
    _player *p = &players[pl_nr];
    
    int i,
      j,
      f;
    _pointf _pos,	// position inside the field
	  d;			// distance to move
	float speed = 0.0f;

	d.x = d.y = 0.0f;

    if (p->m == 1) {
        _pos.x = CUTINT(p->pos.x);
        _pos.y = CUTINT(p->pos.y);

        // do direction correction for going up/down
        if (_pos.x > 0.0f && _pos.x <= 0.5f && (p->d == up || p->d == down))
            p->d = left;
        if (_pos.x > 0.5f && _pos.x < 1.0f && (p->d == up || p->d == down))
            p->d = right;
        // do direction correction for left/right
        if (_pos.y > 0.0f && _pos.y <= 0.5f && (p->d == left || p->d == right))
            p->d = up;
        if (_pos.y > 0.5f && _pos.y < 1.0f && (p->d == left || p->d == right))
            p->d = down;

        /* get the distance/speed until we reach the next position */
        if (p->d == left)
            speed = _pos.x;
        else if (p->d == right)
            speed = 1.0f - _pos.x;
        else if (p->d == up)
            speed = _pos.y;
        else
            speed = 1.0f - _pos.y;

        if (speed > (p->stepsleft) || speed == 0.0f)
            speed = p->stepsleft;
		if (speed > 1.0f)
			speed = 1.0f;

        // check the new field position
        d.x = d.y = 0.0f;
        if (p->d == left && _pos.y == 0.0f
            && ((_pos.x == 0.0f && check_field (p->pos.x - 1.0f, p->pos.y)) || (_pos.x > 0.0f)))
            d.x = -speed;
        if (p->d == right && _pos.y == 0.0f
            && ((_pos.x == 0.0f && check_field (p->pos.x + 1.0f, p->pos.y)) || (_pos.x > 0.0f)))
            d.x = speed;
        if (p->d == up && _pos.x == 0.0f
            && ((_pos.y == 0.0f && check_field (p->pos.x, p->pos.y - 1.0f)) || (_pos.y > 0.0f)))
            d.y = -speed;
        if (p->d == down && _pos.x == 0.0f
            && ((_pos.y == 0.0f && check_field (p->pos.x, p->pos.y + 1.0f)) || (_pos.y > 0.0f)))
            d.y = speed;

        // check if we can move and if there is any bomb
        if (d.y != 0 || d.x != 0) {
            get_bomb_on (p->pos.x, p->pos.y, bomb1);
            get_bomb_on (p->pos.x + d.x, p->pos.y + d.y, bomb2);

            if (bomb1[0].x == -1 && bomb2[0].x != -1)
                /* old pos no bomb, new pos no bomb */
                d.x = d.y = 0.0f;
            else if (bomb2[0].x != -1) {
                /* new pos bomb, old pos bomb... check if it's the same
                   use f to save if we found the bomb or not 
                   f == 0 no bomb found, f == 1 bomb found  */
                for (i = 0, f = 1; (bomb2[i].x != -1 && f == 1); i++)
                    for (f = 0, j = 0; (bomb1[j].x != -1 && f == 0); j++)
                        if (bomb1[j].x == bomb2[i].x && bomb1[j].y == bomb2[i].y)
                            /* identical bomb found ... f = 1 */
                            f = 1;
                if (f == 0)
                    d.x = d.y = 0.0f;
            }
        }

        p->pos.x += d.x;
        p->pos.y += d.y;

        player_check_powerup (pl_nr);

        _pos.x = CUTINT(p->pos.x);
        _pos.y = CUTINT(p->pos.y);

		/* check if we can go though a tunnel */
        if (_pos.x == 0.0f && _pos.y == 0.0f && map.field[(int)p->pos.x][(int)p->pos.y].type == FT_tunnel
            && p->tunnelto <= 0.0f) {
				
			int tunnelnr = map.field[(int)p->pos.x][(int)p->pos.y].special;
            d_printf ("Tunnel [%d] Player %s is going to (%d,%d)\n", tunnelnr, p->name,
                      map.tunnel[tunnelnr].x, map.tunnel[tunnelnr].y);
            d.x = d.y = 0.0f;
            if (map.bfield[map.tunnel[tunnelnr].x][map.tunnel[tunnelnr].y])
                d_printf ("      *** End of tunnel is with an bomb.\n");
            else {
                p->pos.x = map.tunnel[tunnelnr].x;
                p->pos.y = map.tunnel[tunnelnr].y;
                p->tunnelto = GAME_TUNNEL_TO;
				speed = p->stepsleft;
            }
        }
    }

    if (d.x == 0.0f && d.y == 0.0f)
        return 0;

    return (p->stepsleft - speed);
};


/* check if the givin position is oky 
   1 = ok, 0 = bad */
int
player_checkpos (int x, int y)
{
    int i,
      d;

    for (i = 0, d = 0; d < 4; d++)
        if (map.field[x][y].ex[d].count > 0)
            i++;

    if (map.field[x][y].type == FT_block || i > 0)
        return 0;
    else
        return 1;
};



/* Search if an killer exist for explosion at position */	
/* Must be used after check_exfield function */
int 
get_killer_for_explosion (short int x, short int y) 
{
    int killer = -1, i;
	
    //for (i = 0; (i < 4 && killer == -1); i++)
    for (i = 0; i < 4; i++)
        if (map.field[x][y].ex[i].count > 0) {
            if (players[map.field[x][y].ex[i].bomb_p].bombs[map.field[x][y].ex[i].bomb_b].ex_nr == map.field[x][y].ex_nr )
                killer = map.field[x][y].ex[i].bomb_p;
            d_printf("get_killer_for_explosion: found killer pl_nr:%d killer:%d for index %d\n", map.field[x][y].ex[i].bomb_p, killer, i);
        }
    return killer;
}

/* move the player if he have to move AND check if we are on a block
   or over fire */
void
player_move (int pl_nr)
{
    int oldd, coll_speed;
    _player *p = &players[pl_nr];

    if (p->tunnelto > 0.0f) {
        p->tunnelto -= timediff;
		p->m = 0;
		if (p->tunnelto <= 0.0f && GT_MP)
			net_game_send_playermove (pl_nr, 1);
	}
    else {
        if (p->m) {
			/* prepare playervariables for the moving */
            player_animation (p);
    		oldd = p->d;
            p->stepsleft = p->speed * timefactor;
			coll_speed = p->collect_shoes;
            do {
                p->d = oldd;
			} while ((p->stepsleft = stepmove_player (pl_nr)) > 0);

            /* network packet send control - send data if it's time to send or if we need to */
            if (GT_MP)
                net_game_send_playermove (pl_nr, (p->old_m == 0));
        }

        /* the player just stopt moving so send data */
        if (GT_MP && p->m == 0 && p->old_m != 0)
            net_game_send_playermove (pl_nr, 1);
        p->old_m = p->m;        // save the old state 
        p->m = 0;

        /* check the players position */
        if (PS_IS_alife (p->state) && (CUTINT(p->pos.x) > EXPLOSION_SAVE_DISTANCE && (p->d == left || p->d == right))
            && (!check_exfield (p->pos.x + 1.0f, p->pos.y)))
                player_died (p, get_killer_for_explosion(p->pos.x + 1.0f, p->pos.y), 0);
        else if (PS_IS_alife (p->state) && (CUTINT(p->pos.y) > EXPLOSION_SAVE_DISTANCE && (p->d == up || p->d == down))
            && (!check_exfield (p->pos.x, p->pos.y + 1.0f)))
                player_died (p, get_killer_for_explosion(p->pos.x, p->pos.y + 1.0f), 0);
        else if (PS_IS_alife (p->state) && ((CUTINT(p->pos.x) < (1.0f - EXPLOSION_SAVE_DISTANCE) && (p->d == left || p->d == right))
            || (CUTINT(p->pos.y) < (1.0f - EXPLOSION_SAVE_DISTANCE)
                && (p->d == up || p->d == down)))
			&& (!check_exfield (p->pos.x, p->pos.y)))
                player_died (p, get_killer_for_explosion(p->pos.x, p->pos.y), 0);
    }
};


void
player_drop_bomb (int pl_nr)
{
    _player *player = &players[pl_nr];
    _bomb *bomb = NULL;
    int i;
    _point bombs[MAX_PLAYERS * MAX_BOMBS];

    i = player_findfreebomb (player);

    if (i >= 0 && i < MAX_BOMBS && PS_IS_alife (player->state)) { // free bomb found
        // get the best position for the bomb.
        bomb = &player->bombs[i];
        bomb->pos.x = rintf (player->pos.x);
        bomb->pos.y = rintf (player->pos.y);

        get_bomb_on (bomb->pos.x, bomb->pos.y, bombs);
        if (bombs[0].x != -1)   // is there already a bomb
            return;

        d_printf ("Player %d Dropped Bomb %d\n", pl_nr, i);
        bomb->r = player->range;
        if (player->special.type == SP_trigger) {
            bomb->state = BS_trigger;
            bomb->to = SPECIAL_TRIGGER_TIMEOUT;
        }
        else {
            bomb->state = BS_ticking;
            bomb->to = bman.bomb_tickingtime;
        }
		bomb->mode = BM_normal;
        bomb->ex_nr = -1;
        map.bfield[(int)bomb->pos.x][(int)bomb->pos.y] = 1;
        if (GT_MP) {
            net_game_send_bomb (pl_nr, i);
            if (GT_MP_PTPS)
                bomb->to = bomb->to + 2 * RESENDCACHE_RETRY;
        }

        snd_play (SND_bombdrop);
    }
};


/*
check the field - 4 pixels from every side.. so it's not anymore that tricky to get
away from bombs.. */
void
get_player_on (float x, float y, int pl_nr[])
{
    int i,
      p;

    for (i = 0, p = 0; p < MAX_PLAYERS; p++)
        if (PS_IS_alife (players[p].state) && players[p].tunnelto <= 0) {
            if ((players[p].pos.x - EXPLOSION_SAVE_DISTANCE) > x - 1.0f
                && (players[p].pos.x + EXPLOSION_SAVE_DISTANCE) < x + 1.0f
                && (players[p].pos.y - EXPLOSION_SAVE_DISTANCE) > y - 1.0f
                && (players[p].pos.y + EXPLOSION_SAVE_DISTANCE) < y + 1.0f) {
                pl_nr[i] = p;
                i++;
            }
        }
    pl_nr[i] = -1;
};


/*
 * check if we control this player, and let it die.
 */
void
player_died (_player * player, signed char dead_by, int network)
{
	if ((player - players) != bman.p_nr && (player - players) != bman.p2_nr	&& PS_IS_netplayer (player->state) && network == 0) 
		return;
	
    // player die !
    d_printf ("player_died net:%d pl_nr:%d dead_by_nr:%d - %-10s\n", network, player - players, dead_by, players[dead_by].name);

    bman.updatestatusbar = 1;   // force an update
    if (PS_IS_alife (player->state) && dead_by >= 0 && dead_by < MAX_PLAYERS){
		// Update player's statistics
		players[player - players].gamestats.killedBy[dead_by]++;
		// Update player's data
        if (player - players != dead_by) {
            players[dead_by].points++;
            players[dead_by].nbrKilled++;
			if (bman.gametype == GT_team && players[dead_by].team_nr >= 0 && players[dead_by].team_nr < MAX_TEAMS)
				teams[players[dead_by].team_nr].points++;
            // refresh data for killer when the dead is local
            if (GT_MP && !network)
                net_game_send_player (dead_by);
        }
		}
    else if ( dead_by == -1 ){
        players[player - players].gamestats.unknown++;
        d_printf("ERRROR get killer it is impossible check get_the_killer_for_explosion and player_move funtion traces\n");
    }

    player->frame = 0;
    player->state &= (0xFF - PSF_alife);
    player->dead_by = dead_by;
	special_clear (player - players);

    // Send player died when is local
    if (GT_MP && !network)
        net_game_send_player (player - players);
	
    snd_play (SND_dead);
	
	if ((GT_SP || player == &players[bman.p_nr] || (IS_LPLAYER2 && player == &players[bman.p2_nr])
		|| (PS_IS_aiplayer (player->state) && GT_MP_PTPM)) && bman.dropitemsondeath) {
		flitems_dropitems ((player - players), player->pos, player->collect_shoes, 
							player->bombs_n - bman.start_bombs, player->range - bman.start_range);
		}
};


void
draw_players ()
{
    int p;
	
    for (p = 0; p < MAX_PLAYERS; p++) {
        if (PS_IS_playing (players[p].state) && players[p].tunnelto <= 0)
            draw_player (&players[p]);
    }
};


void
player_animation (_player * player)
{
    if (player->gfx == NULL)
        return;

	/*
	 * animation for the movement
	 */
    if (PS_IS_alife (player->state)) {
		player->frame += (timefactor * ANI_PLAYERTIMEOUT * 15 * player->speed);
        while ((int)player->frame >= player->gfx->ani.frames)
			player->frame -= player->gfx->ani.frames;
        while ((int)player->frame < 0)
			player->frame += player->gfx->ani.frames;
    }

	/*
	 * dead player animation
	 */
    if (PS_IS_dead (player->state)) {
        if ((int)player->frame < gfx.dead.frames)
            player->frame += (timefactor * ANI_PLAYERTIMEOUT);
    }
	
	/*
	 * respawning animation
	 */
	if (PS_IS_respawn (player->state)) {
        if ((int)player->frame < 2*gfx.respawn.frames)
            player->frame += (timefactor * ANI_PLAYERTIMEOUT);
	}
};


void
dead_playerani ()
{
    int i;
	
    for (i = 0; i < MAX_PLAYERS; i++)
        if (PS_IS_dead (players[i].state) || PS_IS_respawn (players[i].state)) {
            player_animation (&players[i]);
        }
};



/*
	calc the position on the screen for moving network players 
*/
void
player_calcpos ()
{
    _player *pl;
    int oldm,
      oldd,
      p;
	float oldspeed;

    for (p = 0; p < MAX_PLAYERS; p++) {
        pl = &players[p];
        if (PS_IS_netplayer (pl->state) && PS_IS_alife (pl->state) && pl->m != 0) {
		    player_animation (pl);
			oldspeed = pl->speed;
            oldm = pl->m;
            oldd = pl->d;
            if (pl->speed > 0.0) {
				pl->speed *= timefactor;
                stepmove_player (p);
			}
			pl->speed = oldspeed;
        }
    }
};


void
player_ilness_loop (int plnr)
{
    _player *p;
    int type,
      pnr,
      i,
      send;
	float tmpf;
    int pl[MAX_PLAYERS + 1];

    /* do the illness for the network and ai players */
    for (pnr = 0; pnr < MAX_PLAYERS; pnr++)
          if (pnr != plnr && PS_IS_alife (players[pnr].state) && PS_IS_netplayer (players[pnr].state)) {
              p = &players[pnr];
              for (type = 0; type < PI_max; type++)
                  if (p->ill[type].to > 0.0f) {
                      p->ill[type].to -= timediff;
                      p->illframe += timefactor;
                          if (p->illframe < 0.0f || p->illframe >= gfx.ill.frames)
                              p->illframe = 0.0f;
                  }
          }

    /* check if we have contact with an other ill player */
    p = &players[plnr];
    get_player_on (p->pos.x, p->pos.y, pl);
    for (i = 0; (pl[i] != -1 && i < MAX_PLAYERS); i++)
        if (pl[i] != plnr) {
            send = 0;
            for (type = 0; type < PI_max; type++) {
                if (players[pl[i]].ill[type].to > p->ill[type].to) {
                    tmpf = p->ill[type].to;
                    player_set_ilness (p, type, players[pl[i]].ill[type].to);
                    p->ill[type].to = players[pl[i]].ill[type].to;
                    if (tmpf <= 0.0f)
                        send = 1;
                }
            }
            if (send != 0 && GT_MP)
                net_game_send_ill (plnr);
        }

    /* do the illness for the givin player */
    for (type = 0; type < PI_max; type++)
        if (p->ill[type].to > 0.0f) {
            p->ill[type].to -= timediff;
            if (p->ill[type].to <= 0.0f)
                player_clear_ilness (p, type);
            else {
                p->illframe += timefactor;
                if (p->illframe < 0 || p->illframe >= gfx.ill.frames)
                    p->illframe = 0.0f;
 
                if (type == PI_keys) {
                    /* switch direction for player key illness */
                    if (p->m > 0)
                        switch (p->d) {
                        case (left):
                            p->d = right;
                            break;
                        case (right):
                            p->d = left;
                            break;
                        case (up):
                            p->d = down;
                            break;
                        case (down):
                            p->d = up;
                            break;
                        }
                }
                else if (type == PI_bomb)
                    /* player is dropping bombs */
                    player_drop_bomb (plnr);
            }
        }
}


/* Set or add the timeout for one illness, depends on if one was givin.
 * if the type is -1 so the timeout will be added, else the timeout is
 * set to the new value */
void
player_set_ilness (_player * p, int t, float new_to)
{
    int type;
    if (t == -1)
        type = s_random (PI_max);
    else
        type = t;

    d_printf ("Ilness Player %s : Type:%d\n", p->name, type);
    switch (type) {
    case PI_slow:
        if (p->ill[type].to <= 0.0f) {
            if (p->ill[PI_fast].to > 0.0f) {
                p->ill[type].dataf = p->ill[PI_fast].dataf;
                p->ill[PI_fast].to = 0.0f;
            }
            else
                p->ill[type].dataf = p->speed;
        }
        p->speed = ILL_SLOWSPEED;
        break;
    case PI_fast:
        if (p->ill[type].to <= 0.0f) {
            if (p->ill[PI_slow].to > 0.0f) {
                p->ill[type].dataf = p->ill[PI_slow].dataf;
                p->ill[PI_slow].to = 0.0f;
            }
            else
                p->ill[type].dataf = p->speed;
        }
        p->speed = ILL_FASTSPEED;
        break;
    case PI_range:
        if (p->ill[type].to <= 0.0f)
            p->ill[type].datai = p->range;
		d_printf ("    PI_range %d\n", p->ill[type].datai);
        p->range = 1;
        break;
    case PI_nobomb:
        if (p->ill[type].to <= 0.0f)
            p->ill[type].datai = p->bombs_n;
		d_printf ("    PI_nobomb %d\n", p->ill[type].datai);
        p->bombs_n = s_random (2);
        break;
    }
    bman.updatestatusbar = 1;
	
	/* add or set the new timeout */
	if (t == -1)
		p->ill[type].to += new_to;
	else
		p->ill[type].to = new_to;
};


/* players ilness is over now */
void
player_clear_ilness (_player * p, int type)
{
    if (type < 0 || type >= PI_max)
        return;
	
	d_printf ("player_clear_ilness Player: %s Type: %d\n", p->name, type);
	
    switch (type) {
    case PI_slow:
    case PI_fast:
        p->speed = p->ill[type].dataf;
        break;
    case PI_range:
		d_printf ("player_clear_ilness PI_range to %d\n", p->ill[type].datai);
        p->range = p->ill[type].datai;
        break;
    case PI_nobomb:
		d_printf ("player_clear_ilness PI_nobomb to %d\n", p->ill[type].datai);
        p->bombs_n = p->ill[type].datai;
        break;
    }
    p->ill[type].to = 0.0f;
    if (GT_MP)
        net_game_send_ill (bman.p_nr);
    bman.updatestatusbar = 1;
};


void
player_set_gfx (_player * p, signed char gfx_nr)
{
	d_printf ("player_set_gfx: name:%15s from gfx %d to gfx %d.\n", p->name, p->gfx_nr, gfx_nr);
	
    p->gfx_nr = gfx_nr;
    if (p->gfx_nr < 0 || p->gfx_nr >= gfx.player_gfx_count)
        p->gfx_nr = -1;
    if (p->gfx_nr == -1) {
        p->gfx = NULL;
        p->state &= (0xFF - (PSF_alife + PSF_playing));
    }
    else {
        p->gfx = &gfx.players[gfx_nr];
        p->state |= PSF_playing;
    }
	if (GT_MP) net_send_playerid (p-players);
};


/* find a free bomb */
int
player_findfreebomb (_player * player)
{
    int i,
	  bombused = 0,
      res = -1,
      nr;

    /* check every free bomb from next entry of the last 
       exploded bomb to the last exploded bomb */
    if (player->bomb_lastex < 0 || player->bomb_lastex >= MAX_BOMBS)
        player->bomb_lastex = 0;

    for (i = 0, nr = player->bomb_lastex, bombused = 0; i < MAX_BOMBS; i++) {
		nr++;
        if (nr >= MAX_BOMBS) // start at 0
            nr = 0;
		
        if (player->bombs[nr].state == BS_off) { /* check if this bomb is free */
            if (res == -1)
                res = nr;
        }
        else
            bombused++;         // count number of used bombs
    }

    if (bombused >= player->bombs_n)
        res = -1;               /* all max number of bombs lay */

    return res;
};


/* check if a player died and check if we have to respawn */
void player_checkdeath (int pnr) {
	_player	*player = &players[pnr];
	int i;
	
	/* respawn only as long as not the game end have reached 
	 * and when the gametype is deathmatch mode */
    if (map.state == MS_normal && bman.gametype == GT_deathmatch 
		&& PS_IS_dead (player->state) && player->frame >= gfx.dead.frames) {
		/* check new position */
		d_printf ("player_checkdeath: Respawn for player %s\n", player->name);

		player->pos.x = -1;
		player->pos.y = -1;
		
		map_respawn_player(pnr);

		if (player->pos.x != -1 && player->pos.y != -1) {
			player->frame = 0;
			player->state |= PSF_respawn;
			
			if (GT_MP)
				net_game_send_respawn (pnr);
		}
    }
	
	/* if the player is respawning check for finish of the animation 
	 * reset some data of the player and force update of the status bar */
    if (bman.gametype == GT_deathmatch && PS_IS_respawn (player->state) && player->frame >= 2*gfx.respawn.frames) {
		d_printf ("Respawn completed for player %s\n", player->name);
		player->frame = 0;
		player->state &= (0xFF - PSF_respawn);
		player->state |= PSF_alife;
		special_clear (pnr);
		
		if (bman.dropitemsondeath) {
			player->speed = bman.start_speed;
			player->bombs_n = bman.start_bombs;
			player->range = bman.start_range;
			player->collect_shoes = 0;
			for (i = 0; i < PI_max; i++)
				player->ill[i].to = 0.0f;
		}

		for (i = 0; i < PI_max; i++)
			if (player->ill[i].to > 0.0f)
				player_clear_ilness (player, i);
			
		if (GT_MP) {
			net_game_send_respawn (pnr);
	        net_game_send_ill (pnr);
		}
		bman.updatestatusbar = 1;
	}
};


/*
 * check all player related values and work with the key flags
 * call the illness check
 */
void player_check (int pl_nr) {

	if (bman.state != GS_running)
		return;

	/* check if the player is still alife */
	if ((players[pl_nr].state & PSFM_alife) == PSFM_alife) {
		playerinput_loop (pl_nr);
		player_ilness_loop (pl_nr);
		if (players[pl_nr].keyf_bomb)
			player_drop_bomb (pl_nr);
		players[pl_nr].keyf_bomb = 0;
		if (players[pl_nr].keyf_special)
			special_use (pl_nr);
		players[pl_nr].keyf_special = 0;
	}
	else {
		players[pl_nr].m = 0;
	}
	
	player_checkdeath (pl_nr);
};


/*
 * delete a player from the game
 */
void
player_delete (int pl_nr) {
    d_printf ("player_delete (%d)\n", pl_nr);
	bman.updatestatusbar = 1;   // force an update

    if (pl_nr == bman.p_nr) {
        /* we're not wanted */
        network_shutdown ();
		menu_displaymessage ("Kicked", "You have been kicked from the server.");
        bman.state = GS_startup;
    }
    else {
    	player_set_gfx (&players[pl_nr], -1);
        players[pl_nr].state &= (0xFF - (PSF_used + PSF_net + PSF_alife + PSF_ai)); /* delete 
												player flags */
        players[pl_nr].net.net_istep = 0; // needed for disconnect during the update
        bman.players_nr_s--;
        players[pl_nr].gfx_nr = -1;

		/* send network update */
		net_game_send_delplayer (pl_nr);
	}

    if (GT_MP_PTPM && bman.notifygamemaster)
        send_ogc_update ();
	
	if (bman.p_nr == pl_nr)
		bman.p_nr = -1;
	if (bman.p2_nr == pl_nr)
		bman.p2_nr = -1;
	
	d_playerdetail (" Player Left ... Playerlist\n");
};


/*** player2_join ()
 * the second local player wants to join the game
 * - check if we are the host of the game , if we are the client
 *   or if we are in single mode.
 * host:   add second local player and send update to the clients
 * client: send the information that a second local player wants
 *         to play too, do_playerid will add the player then and
 *         add the player finaly to the game over the function
 *         player2_add (pl_nr).
 * single: add the second player.
 */
void player2_join () {
	int i;
	
	if (IS_LPLAYER2) {
		menu_displaymessage ("Sorry", "Sorry there is already a second player from this computer.");
		return;
	}

	if (GT_MP_PTPM) {		/* multiplayer master/host */
		for (i = 0; PS_IS_used (players[i].state) && i < MAX_PLAYERS; i++);
		if (i < MAX_PLAYERS) { /* free player found */
			player2_add (i);
			net_send_players ();
		} else {
			menu_displaymessage ("Sorry", "There is no free playerslot left\n");
		}
	}
	else if (GT_MP_PTPS) {	/* multiplayer client, send only the request */
		send_joingame (&players[bman.p_servnr].net.addr, bman.player2name, 1);
	}
	else { 		/* single mode */
		for (i = 0; PS_IS_used (players[i].state) && i < MAX_PLAYERS; i++);
		if (i < MAX_PLAYERS) { /* free player found */
			player2_add (i);
		}
	}
};


/*
 * add the second player to the game and set all variables
 */
void player2_add (int pl_nr) {
	bman.p2_nr = pl_nr;
	strncpy (players[pl_nr].name, bman.player2name, LEN_PLAYERNAME);
	players[pl_nr].state += PSF_used;
	players[pl_nr].gfx_nr = -1;
	players[pl_nr].gfx = NULL;
	players[pl_nr].points = 0;
	players[pl_nr].wins = 0;
	players[pl_nr].team_nr = -1;
	d_printf ("player2_add: Local Player Added with pl_nr: %d\n", pl_nr);
};


/*
 * set the teams[] with the current player data
 */
void team_update () {
	int cnt[MAX_TEAMS] = { 0,0,0,0 };
	int pl;
	
	for (pl = 0; pl < MAX_PLAYERS; pl++) {
		if (PS_IS_used (players[pl].state) && (players[pl].team_nr < 0 || players[pl].team_nr >= MAX_TEAMS))
			players[pl].team_nr = 0;
		
		if (PS_IS_used (players[pl].state)) {
			teams[players[pl].team_nr].players[cnt[players[pl].team_nr]] = &players[pl];
			cnt[players[pl].team_nr]++;
		}
	}
	for (pl = 0; pl < MAX_TEAMS; pl++) for (;cnt[pl] < MAX_PLAYERS; cnt[pl]++)
		teams[pl].players[cnt[pl]] = NULL;
	
	// d_teamdetail ("Teaminformation");
}


/*
 * if the team number is -1 then join this player into the losing team
 */
void team_choose (_player *pl) {
	
	if (pl->team_nr >= 0 && pl->team_nr < MAX_TEAMS)
		return;
	
	d_printf ("team_choose need to rewrite this part\n");
	if (PS_IS_aiplayer (pl->state))
		pl->team_nr = 1;
	else pl->team_nr = 0;
};
