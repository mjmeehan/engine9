/* $Id: flyingitems.c,v 1.4 2004-04-03 14:48:43 stpohle Exp $ */

#include "bomberclone.h"
#include "flyingitems.h"
#include "player.h"

_flyingitem flitems [FLITEMS_MAXITEMS];
_flyingitem *flitems_first = NULL;



/* find first free entry */
_flyingitem *flitems_findfree () {
	int i;
	
	for (i = 0; i < FLITEMS_MAXITEMS; i++)
		if (flitems[i].type == FT_nothing) break;
	
	if (i < FLITEMS_MAXITEMS)
		return &flitems[i];
	else
		return NULL;
};


/* reset all items */
void flitems_reset () {
	int i;
	
	for (i = 0; i < FLITEMS_MAXITEMS; i++) {
		flitems[i].type = FT_nothing;
		flitems[i].next = NULL;
	}
	
	flitems_first = NULL;
};	



void flitems_loop () {
	_flyingitem *flitem = flitems_first;
	_flyingitem **old = &flitems_first;   // pointer of the preview next pointer
	
	for (; flitem != NULL; flitem = flitem->next) {
		/* restore the old position */
		stonelist_add (floorf (flitem->pos.x), floorf (flitem->pos.y));
		stonelist_add (floorf (flitem->pos.x), floorf (flitem->pos.y)+1);
		stonelist_add (floorf (flitem->pos.x)+1, floorf (flitem->pos.y));
		stonelist_add (floorf (flitem->pos.x)+1, floorf (flitem->pos.y)+1);
		
		flitem->step += (2*timediff);
		if (flitem->type == FT_nothing || flitem->step >= 1.0f) { 
			/* finished delete element and put it on the right place */
			if (map.field[(int)flitem->to.x][(int)flitem->to.y].type == FT_nothing)
				map.field[(int)flitem->to.x][(int)flitem->to.y].type = flitem->type;
			else
				map.field[(int)flitem->to.x][(int)flitem->to.y].special = flitem->type;
	
			stonelist_add ((int)flitem->to.x, (int)flitem->to.y);
			*old = flitem->next; /* set the preview next pointer to the next element */
			flitem->type = FT_nothing;
			flitem->next = NULL;
		}
		else { /* still moving draw item */
			flitem->pos.x = (1.0f - flitem->step) * (flitem->from.x - flitem->to.x) + flitem->to.x;
			flitem->pos.y = (1.0f - flitem->step) * (flitem->from.y - flitem->to.y) + flitem->to.y;
			flitems_draw (flitem);
			old = &flitem->next;
		}
	}
};


/* add this item into the drop list */
_flyingitem *flitems_additem (_pointf from, _point to, signed char type) {
	_flyingitem *flitem = flitems_findfree ();
	
	if (flitem == NULL || type == FT_nothing) {
		d_fatal ("flitems_additem: couldn't get any free flyitem \n");
		return NULL;
	}
	
	/* set the pointers */
	flitem->next = flitems_first;
	flitems_first = flitem;
	
	flitem->from = from;
	flitem->to.x = (float) to.x;
	flitem->to.y = (float) to.y;
	flitem->step = 0.0f;
	flitem->type = type;
	flitem->pos = from;
	
	return flitem;
};


/* give us a good position on the field */
_point flitems_randompos (int p_nr) {
	int radius = 2, try = 0, maxtry = 0, check;
	_point to = { -1, -1 };
	
	do {
		/* check that we won't end up in a infinite loop */
		try++;
		maxtry++;
		if (try > 10) {
			radius++;
			try = 0;
		}

		/* get a random position */		
		if (p_nr == -1) { 	/* get a position on the field */
			to.x = s_random (map.size.x -2) + 1;
			to.y = s_random (map.size.y -2) + 1;
		}
		else { 				/* get a good position for the 
							 * destination of the element */
			to.x = s_random (radius*2 + 1) - radius + (int) players[p_nr].pos.x;
			to.y = s_random (radius*2 + 1) - radius + (int) players[p_nr].pos.y;
		}
		
		/* check if the field is good */
		check = (to.x > 0 && to.y > 0 && to.x < map.size.x-1 && to.y < map.size.y-1 
				&& flitems_checkfreepos (to)
				&& (map.field[to.x][to.y].type == FT_nothing
					|| (map.field[to.x][to.y].type == FT_stone && map.field[to.x][to.y].special == FT_nothing))
				&& map.bfield[to.x][to.y] == 0);
	} while ( !check && maxtry < 200);

	if (!check) {
		to.x = -1;
		to.y = -1;
	}
	
	return to;
};

/* a player dropped these items */
void flitems_dropitems (int p_nr, _pointf from, int cnt_speed, int cnt_bombs, int cnt_range) {
	int i, lpos = 0;
	_point to;
	_flyingitem *fiptr[MAX_BOMBS+MAX_RANGE+50];
	
	
	
	for (i = 0; i < cnt_speed; i++) {
		to = flitems_randompos (p_nr);
		if (to.x != -1)
			fiptr[lpos++] = flitems_additem (from, to, FT_shoe);
		else
			d_printf ("flitems_dropitems: (FT_shoe) item could not been set\n");
	}
	
	for (i = 0; i < cnt_bombs; i++) {
		to = flitems_randompos (p_nr);
		if (to.x != -1)
			fiptr[lpos++] = flitems_additem (from, to, FT_bomb);
		else
			d_printf ("flitems_dropitems: (FT_bomb) item could not been set\n");
	}
	
	for (i = 0; i < cnt_range; i++) {
		to = flitems_randompos (p_nr);
		if (to.x != -1)
			fiptr[lpos++] = flitems_additem (from , to, FT_fire);
		else
			d_printf ("flitems_dropitems: (FT_fire) item could not been set\n");
	}
	
	fiptr[lpos] = NULL;
	
	if (GT_MP)
		net_game_send_dropitems (p_nr, fiptr, lpos);
};


void flitems_draw (_flyingitem *flitem) {	
	SDL_Rect src, dest;
	SDL_Surface *srci;
	

	if (flitem == NULL || (flitem->type != FT_shoe && flitem->type != FT_fire && flitem->type != FT_bomb))
		return;

    src.w = dest.w = gfx.block.x;
    src.h = dest.h = gfx.block.y;

	src.x = 0;
	src.y = 0;
	
	dest.x = gfx.offset.x + (gfx.block.x * flitem->pos.x);
	dest.y = gfx.offset.y + (gfx.block.y * flitem->pos.y);
	
	srci = gfx.field[flitem->type].image;
	gfx_blit (gfx.powerup[0].image, &src, gfx.screen, &dest, (((int)flitems->pos.y + 1.0)*256) + 255);
	gfx_blit (srci, &src, gfx.screen, &dest, (((int)flitems->pos.y + 1.0)*256) + 256);
};


/* check if not a flying item is going to this position */
int flitems_checkfreepos (_point to) {
	_flyingitem *item = flitems_first;
	
	while (item != NULL && ((int)item->to.x != to.x || (int)item->to.y != to.y))
		item = item->next;

	return (item == NULL);
};
