/* $Id: field.c,v 1.60 2006-12-15 18:41:45 stpohle Exp $ */
/* field.c - procedures which are needed to control the field */

#include "bomberclone.h"
#include "map.h"
#include "bomb.h"
#include "player.h"

static _point fieldani[MAX_FIELDANIMATION];
static _point stonelist[MAX_STONESTODRAW]; // keep all stones to draw

static float fieldcheck_to;     // timeout for the next fieldcheck FIELDCHECK_TIMEOUT
static float fieldhurry_to;     // in warning mode set the blinking time
static _point fieldhurrypos;    // x,y for the hurry
static int fieldhurryd;   		// direction for the hurry

/* delete the stone entry list */
void
stonelist_del ()
{
    register int i;

    for (i = 0; i < MAX_STONESTODRAW; i++)
        stonelist[i].x = stonelist[i].y = -1;
}


/* stonelist will be draw and cleaned */
void
stonelist_draw ()
{
    int i;

    for (i = 0; i < MAX_STONESTODRAW && stonelist[i].x != -1 && stonelist[i].y != -1; i++) {
        draw_stone (stonelist[i].x, stonelist[i].y);
        stonelist[i].x = stonelist[i].y = -1;
    }
};


/* add stone to draw */
static void
_stonelist_add (int x, int y, int recursive)
{
    int i;
    _point *slentry = NULL;

    for (i = 0, slentry = NULL; i < MAX_STONESTODRAW && slentry == NULL; i++)
        if (stonelist[i].x == -1 || stonelist[i].y == -1
            || (stonelist[i].x == x && stonelist[i].y == y))
            slentry = &stonelist[i];

    if (slentry == NULL)        // no space left
        d_fatal ("field.c adddraw_stone(): out of space in stonelist[]\n");
    else {
        slentry->x = x;
        slentry->y = y;

        if (recursive && gfx.field[map.field[x][y].type].h != gfx.field[map.field[x][y].type].w)
            /* field is higher as usual */
            _stonelist_add (x, y - 1, 0);
    }
};


inline void
stonelist_add (int x, int y)
{
    _stonelist_add (x, y, 1);
};


void
draw_stone (int x, int y)
{
    _field *stone = &map.field[x][y];
    SDL_Rect dest,
      src;
    SDL_Surface *srcimg = NULL;
    int i,
      d;

    if (x < 0 || y < 0 || x >= map.size.x || y >= map.size.y) {
        d_fatal ("Draw Stone out of range [%d,%d]\n", x, y);
        return;
    }

    src.w = dest.w = gfx.block.x;
    src.h = dest.h = gfx.block.y;

    dest.x = x * gfx.block.x + gfx.offset.x;
    dest.y = y * gfx.block.y + gfx.offset.y;

    src.x = 0;

    /* draw background if we have a stone, block or nothing */
    if (stone->type <= FT_tunnel) {
        SDL_Rect srcbg;

        srcbg.w = dest.w;
        srcbg.h = dest.h;
        srcbg.x = (x % gfx.field[FT_nothing].frames) * gfx.block.x;
        srcbg.y = (y % gfx.field[FT_nothing].frames) * gfx.block.y;

        gfx_blit (gfx.field[FT_nothing].image, &srcbg, gfx.screen, &dest, 0);
    }

    if (stone->type == FT_mixed) {
        i = stone->mixframe;
        if (i < FT_death || i >= FT_mixed)
            i = FT_death;
    }
    else
        i = stone->type;

    /* animate the stone if needed only for exploding stone */
    if (stone->type == FT_stone && stone->frame > 0) {
        field_animation_add (x, y);

        if (stone->frame < gfx.field[FT_stone].frames) {
            src.y = (int)stone->frame * gfx.field[FT_stone].h;
            dest.h = src.h = gfx.field[FT_stone].h;
            dest.y -= (gfx.field[stone->type].h - gfx.field[stone->type].w);
            srcimg = gfx.field[FT_stone].image;
        }
        else {
            src.y = 0;
            srcimg = gfx.field[FT_nothing].image;
        }
    }

    else if (stone->type > FT_nothing && stone->type < FT_death) {
        src.y = (int)stone->frame * gfx.field[stone->type].h;
        dest.h = src.h = gfx.field[stone->type].h;
        dest.y -= (gfx.field[stone->type].h - gfx.field[stone->type].w);
        srcimg = gfx.field[stone->type].image;
    }

    /* some powerup so we need to animate this too */
    if (i == FT_death)
        d = PWUP_bad;
    else if (i >= FT_sp_trigger)
        d = PWUP_special;
    else
        d = PWUP_good;

    if (i >= FT_death) {
        field_animation_add (x, y);
        srcimg = gfx.powerup[d].image;
        if (stone->frame >= gfx.powerup[d].frames)
            stone->frame = 0;
        src.y = (int)stone->frame * gfx.block.y;
    }

    if (srcimg != NULL && stone->type != FT_tunnel)
        gfx_blit (srcimg, &src, gfx.screen, &dest, (y*256) + 1);
	else if (srcimg != NULL && stone->type == FT_tunnel)
        gfx_blit (srcimg, &src, gfx.screen, &dest, (y*256) - 1);
		
    if (i >= FT_death) {        /* draw now the powerup itself */
        srcimg = gfx.field[i].image;
        src.y = 0;
        gfx_blit (srcimg, &src, gfx.screen, &dest, (y*256) + 2);
    }

    /* if the current field is half hidden by the lower 
       field (y+1) draw this little part too */
    if (y < map.size.y - 1
        && gfx.field[map.field[x][y + 1].type].h > gfx.field[map.field[x][y + 1].type].w) {
        src.x = 0;
        src.y = (int)map.field[x][y + 1].frame * gfx.field[map.field[x][y + 1].type].h;
        dest.h = src.h =
            gfx.field[map.field[x][y + 1].type].h - gfx.field[map.field[x][y + 1].type].w;
        dest.w = src.w = gfx.field[map.field[x][y + 1].type].w;
        dest.y =
            gfx.offset.y + ((gfx.block.y * (y + 1)) -
                            (gfx.field[map.field[x][y + 1].type].h -
                             gfx.field[map.field[x][y + 1].type].w));
        gfx_blit (gfx.field[map.field[x][y + 1].type].image, &src, gfx.screen, &dest, (y*256) + 5);
    }

    // draw explosions if there is any
    for (d = 0; d < 4; d++)
        if (stone->ex[d].count > 0) {
            stone_drawfire (x, y, -1);
			break;
        }

	if (debug) {
		char txt[64];
		/* ex numbers..
		sprintf (txt, "%d,%d%d", map.field[x][y].ex[0].count, map.field[x][y].ex[0].bomb_p, map.field[x][y].ex[0].bomb_b); font_gfxdraw (dest.x, dest.y, txt, 0, COLOR_white, (y*256) + 10);
		sprintf (txt, "%d,%d%d", map.field[x][y].ex[1].count, map.field[x][y].ex[1].bomb_p, map.field[x][y].ex[1].bomb_b); font_gfxdraw (dest.x, dest.y+10, txt, 0, COLOR_white, (y*256) + 10);
		sprintf (txt, "%d,%d%d", map.field[x][y].ex[2].count, map.field[x][y].ex[2].bomb_p, map.field[x][y].ex[2].bomb_b); font_gfxdraw (dest.x, dest.y+20, txt, 0, COLOR_white, (y*256) + 10);
		sprintf (txt, "%d,%d%d", map.field[x][y].ex[3].count, map.field[x][y].ex[3].bomb_p, map.field[x][y].ex[3].bomb_b); font_gfxdraw (dest.x, dest.y+30, txt, 0, COLOR_white, (y*256) + 10);
		*/
	
		/* number of bombs.. */		
		sprintf (txt, "%d", map.bfield[x][y]); 
		font_gfxdraw (dest.x, dest.y+30, txt, 0, COLOR_white, (y*256) + 10);
	}

    return;
};


void
draw_field ()
{
    int x,
      y;

    for (x = 0; x < map.size.x; x++)
        for (y = 0; y < map.size.y; y++)
            draw_stone (x, y);
};


/* will check all direction without the field on pos x,y
   for the fieldtype */
int field_check_alldirs (int x, int y, int type) {
	if (x <= 0 || y <= 0 || x >= map.size.x-1 || y >= map.size.y-1)
		return 0;
	if (map.field[x-1][y].type == type || map.field[x+1][y].type == type || map.field[x][y-1].type == type || map.field[x][y+1].type == type)
		return 1;
	else 
		return 0;
}

// clear field and send this to all netplayers
void
field_clear (int x, int y)
{
    map.field[x][y].type = FT_nothing;
    if (GT_MP)
        net_game_send_field (x, y);
}


/* run this to every game cycle for the animations on the field */
void
field_animation ()
{
    int i,
      j,
	  oldframe;
    _field *stone;

    for (i = 0; i < MAX_FIELDANIMATION; i++)
        if (fieldani[i].x >= 0 && fieldani[i].x < map.size.x && fieldani[i].y >= 0
            && fieldani[i].y < map.size.y) {
            /* check if there is a need to animate this */
            stone = &map.field[fieldani[i].x][fieldani[i].y];

            if ((stone->type == FT_stone && stone->frame > 0.0f) || (stone->type >= FT_death)) {
                /* animate this stone */
                if (stone->type == FT_stone) {
                    if (stone->frame < gfx.field[FT_stone].frames) 
	                    stone->frame += ((timefactor/4.0f) * ANI_STONETIMEOUT);
                }
                else {          /* animation is a powerup */
                    /* select right powerup animation */
                    if (stone->type == FT_death)
                        j = PWUP_bad;
                    else if (stone->type > FT_sp_trigger)
                        j = PWUP_special;
                    else
                        j = PWUP_good;

                    /* do the animation of the FT_mixed */
					oldframe = (int)stone->frame;
                    stone->frame += ((timefactor/4.0f) * ANI_POWERUPTIMEOUT);
                    if ((int)stone->frame != oldframe && stone->type == FT_mixed) {
                        stone->mixframe++;
                        if (stone->mixframe < FT_death || stone->mixframe >= FT_mixed)
                            stone->mixframe = FT_death;
                    }

                    if (stone->frame >= gfx.powerup[j].frames)
                        stone->frame = 0;
                }
                stonelist_add (fieldani[i].x, fieldani[i].y);
            }
            else {
				/* check for a fire, and if so add to the drawing if not delete */
				unsigned int d;
				
				for (d = 0; d < 4; d++) 
					if (map.field[fieldani[i].x][fieldani[i].y].ex[d].count > 0)
						break;
				
				if (d >= 4)
					fieldani[i].y = fieldani[i].x = -1;
				else 
					stonelist_add (fieldani[i].x, fieldani[i].y);
			}
        }
        else                    /* delete this entry */
            fieldani[i].y = fieldani[i].x = -1;
};


/* add a new field to the animation data */
void
field_animation_add (int x, int y)
{
    int i,
      j = -1,
      d = -1;

    for (i = 0; i < MAX_FIELDANIMATION; i++) {
        if (fieldani[i].x == x && fieldani[i].y == y)
            d = i;
        if (fieldani[i].x == -1 || fieldani[i].y == -1)
            j = i;
    }

    if (j == -1) {              /* nothing anymore free */
        d_printf ("field_animation_add: animation data line too full\n");
        return;
    }

    if (d != -1)                /* this stone is already in the list */
        return;

    fieldani[j].x = x;
    fieldani[j].y = y;
};


/* check the field, if everything is empty */
int
field_checkisempty ()
{
    register int x,
      y;
    int empty = 1;

    for (x = 1; (x < (map.size.x - 1) && empty); x++)
        for (y = 1; (y < (map.size.y - 1) && empty); y++)
            if (map.field[x][y].type == FT_stone)
                empty = 0;

    return empty;
}


/* will check for some gamerelated map informations
   check if the map is empty, and so something like a timeout so we won't
   end up in a endless game */
void
field_loop ()
{
    /* single game or multiplayer master, so check field state */
    if ((GT_MP_PTPM || GT_SP) && bman.state == GS_running) {

        /* timeout for rechecking every 5 secs the field */
		fieldcheck_to -= timediff;
        if (map.state == MS_normal && map.map_selection == MAPS_randgen
            && (fieldcheck_to > FIELDCHECK_TIMEOUT || fieldcheck_to <= 0)
		    && bman.timeout > FIELDHURRYTIMEOUT) {
            if (field_checkisempty ())
                bman.timeout = FIELDHURRYTIMEOUT; // let the hurry time begin
			fieldcheck_to = FIELDCHECK_TIMEOUT;
        }

		/* set the map_state to the right setting and send the information to all clients */
		if (map.state == MS_normal 
			&& bman.timeout <= FIELDHURRYTIMEOUT 
			&& bman.timeout > (FIELDHURRYTIMEOUT-FIELDHURRYWARN)) {
			map.state = MS_hurrywarn;
			bman.updatestatusbar = 1;
			if (GT_MP_PTPM)
				net_send_servermode ();
		}
		
        /* after the end of the warning set the new flag */
        if (map.state == MS_hurrywarn 
			&& bman.timeout <= (FIELDHURRYTIMEOUT-FIELDHURRYWARN)) {
            int rndmax;

            if (map.map_selection == MAPS_randgen)
                rndmax = MS_max - 2; // generaged map
            else
                rndmax = MS_max - 3; // user defined map

            map.state = 2 + s_random (rndmax);
            d_printf ("Game Timeout 1 over: Random map.state = %d\n", map.state);

            fieldhurrypos.x = fieldhurrypos.y = 0;

			if (GT_MP_PTPM)
				net_send_servermode ();
        }

        /* check if we need to small down the map */
        if (map.state == MS_hurry)
            field_hurrysize ();
        if (map.state == MS_dropitems)
            field_hurrydropitems ();
    }

	field_animation ();
};


/* hurrymode drop a item randomly */
void
field_hurrydropitems ()
{
    int x = 0,
      y = 0,
      try = 100;

	fieldhurry_to -= timediff;

    if (fieldhurry_to <= 0 || fieldhurry_to > FIELDHURRYDROPTO) {
        fieldhurry_to = FIELDHURRYDROPTO;

        while (map.field[x][y].type != FT_nothing && (try--)) {
            x = s_random (map.size.x - 2) + 1;
            y = s_random (map.size.y - 2) + 1;
        }

        if (try) {
            map.field[x][y].type = s_random (FT_mixed - FT_tunnel) + FT_tunnel+1;
            stonelist_add (x, y);

            if (GT_MP_PTPM)
                net_game_send_field (x, y);
        }
    }
};


/* hurrymode small down the map */
void
field_hurrysize ()
{
	int i;
	_point old;

	old.x = old.y = 0;

	fieldhurry_to -= timediff;
	if (fieldhurry_to <= 0 || fieldhurry_to > FIELDHURRYSIZE) {
        	fieldhurry_to = FIELDHURRYSIZE;
	
		if (fieldhurrypos.x == 0) {
			fieldhurrypos.x = fieldhurrypos.y = 1;
			fieldhurryd = right;
		} 
		else if (fieldhurrypos.x > 0) {
			old = fieldhurrypos; /* save old value in case that there 
									is an explosion or a bomb */
			switch (fieldhurryd) {
				case (right):
					if (fieldhurrypos.x + 1 >= map.size.x - fieldhurrypos.y) {
						fieldhurryd = down;
						fieldhurrypos.y++;
					}
					else 
						fieldhurrypos.x++;
					break;
				case (down):
					if (fieldhurrypos.y >= map.size.y - (map.size.x - fieldhurrypos.x)) {
						fieldhurryd = left;
						fieldhurrypos.x--;
					}
					else
						fieldhurrypos.y++;
					break;
				case (left):
					if (fieldhurrypos.x <= (map.size.y - fieldhurrypos.y)-1) {
						fieldhurryd = up;
						fieldhurrypos.y--;
					}
					else
						fieldhurrypos.x--;
					break;
				default:
					if (fieldhurrypos.y-1 <= fieldhurrypos.x) {
						/* check if this is the end */
						i = map.size.x - (2 * fieldhurrypos.x);
						if (i > FIELDHURRYSIZEMIN)
							i = map.size.y - (2 * fieldhurrypos.y);
						if (i <= FIELDHURRYSIZEMIN)
							fieldhurrypos.x = fieldhurrypos.y = -1;
						else {
							fieldhurryd = right;
							fieldhurrypos.x++;
						}
					}
					else
						fieldhurrypos.y--;
					break;
				}
		}
		
		/* check if we have finished sizing down everything */
		if (fieldhurrypos.x > 0) {
			_point bombs[MAX_PLAYERS*MAX_BOMBS];
			int i, d;

			/* check if a bomb is at this position, if so let the bomb explode
			   and wait untill the explosion is over */
		    for (i = 0, d = 0; d < 4; d++)
        		if (map.field[fieldhurrypos.x][fieldhurrypos.y].ex[d].count > 0)
            		i++;	

			get_bomb_on (fieldhurrypos.x, fieldhurrypos.y, bombs);
			if (i)
				fieldhurrypos = old;
			else if (bombs[0].y != -1 && bombs[0].x != -1) {
				fieldhurrypos = old;
				bomb_explode (&players[bombs[0].x].bombs[bombs[0].y], 1);
			}
			else {
				/* set the block on the position */
				map.field[fieldhurrypos.x][fieldhurrypos.y].type = FT_block;
				map.field[fieldhurrypos.x][fieldhurrypos.y].special = FT_nothing;
				map.field[fieldhurrypos.x][fieldhurrypos.y].frame = 0;
        		stonelist_add (fieldhurrypos.x, fieldhurrypos.y);
        		if (GT_MP_PTPM)
            		net_game_send_field (fieldhurrypos.x, fieldhurrypos.y);
			}
		}
	}
};


/* draw the fire on one field 
 * if frame == -1 we will draw the framenumber in the field.ex data 
 * Add stone to the animation list, draw the fire and then check if
 * the bomb in the ex field is still showing on a explosion
 */
void stone_drawfire (int x, int y, int frame)
{
    SDL_Rect src,
      dest;
	int d;
	_field *stone = &map.field[x][y];
	
	/* add to the animation list */
	field_animation_add (x ,y);
	
	/* draw the stone */
   	dest.w = src.w = gfx.block.x;
	dest.h = src.h = gfx.block.y;

	dest.x = gfx.offset.x + x * gfx.block.x;
	dest.y = gfx.offset.y + y * gfx.block.y;

	for (d = 0; d < 4; d++) 
		if (stone->ex[d].count > 0)	{
			if (frame == -1)            // no giving frame
    	    	frame = map.field[x][y].ex[d].frame;

	    	src.y = frame * src.w;
   		 	src.x = d * src.w;

    		gfx_blit (gfx.fire.image, &src, gfx.screen, &dest, (y * 100));
		}
	
	/* check if the last explosion is still right */
	for (d = 0; d < 4; d++)
		if (stone->ex[d].count > 0) {
			/* check if the bombid is right */
			if (stone->ex[d].bomb_p >= 0 && stone->ex[d].bomb_p < MAX_PLAYERS
				&& stone->ex[d].bomb_b >= 0 && stone->ex[d].bomb_b < MAX_BOMBS) {
				/* check if the bomb explosion finished already */
				if (players[stone->ex[d].bomb_p].bombs[stone->ex[d].bomb_b].state != BS_exploding) {
					stone->ex[d].count--;
					stone->ex[d].bomb_b = -1;
					stone->ex[d].bomb_p = -1;
				}
			}
			else {
				/* bombid is not right, set: count-1  */
				stone->ex[d].count--;
			}
		}
};
