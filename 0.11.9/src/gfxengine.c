/* $Id: gfxengine.c,v 1.7 2005-07-06 13:11:55 stpohle Exp $ */
/* GFX Game Engine */

#include "bomberclone.h"

int blitdb_nr = 0, blitrects_nr = 0;
static _gfxblit *blitdb;      /* unsorted list of blitdb's */
static _gfxblit **sortblitdb; /* sorted list of blitdb's */
static SDL_Rect *blitrects;   /* SDLUpdate Rects */

/* alloc all needed space */
void gfxengine_init () {
	blitdb = malloc (sizeof (_gfxblit)* MAX_BLITRECTS);
	sortblitdb = malloc (sizeof (_gfxblit *)* MAX_BLITRECTS);
	blitrects = malloc (sizeof (SDL_Rect) * MAX_BLITRECTS);
};


/* sort the list of blitting objects highest will be drawn at last */
void gfx_blitsort () {
	register int i,y;
	
	gfx_blitsortclear ();
	for (i = 0; i < MAX_BLITRECTS && i < blitdb_nr; i++) {
		for (y = i; (y > 0 && blitdb[i].y < sortblitdb[y-1]->y); y--)
			sortblitdb[y] = sortblitdb[y-1];
		sortblitdb[y] = &blitdb[i];
	}
}


/* delete sorted order of gfx updates */
inline void gfx_blitsortclear () {
	register int i;
	for (i = 0; i < MAX_BLITRECTS; i++)
		sortblitdb[i] = NULL;
};


/* delete all updaterect entrys */
inline void gfx_blitupdaterectclear () {
	register int i;
	for (i = 0; i < MAX_BLITRECTS; i++)
		blitrects[i].x = blitrects[i].y = blitrects[i].h = blitrects[i].w = -1;
	blitrects_nr = 0;
};


/* SDL Update of the rects */
void gfx_blitupdaterectdraw () {
	if (blitrects_nr > 0)
		SDL_UpdateRects (gfx.screen, blitrects_nr, blitrects);
	
	blitrects_nr = 0;
	gfx_blitupdaterectclear ();
};


/* add updaterect entrys and skipp unneeded or double rects */
void gfx_blitupdaterectadd (SDL_Rect *rect) {
	_point p1a, p1e, p2a, p2e;
	int i, done = 0;
	
	/* search for a match or an close update rect */
	p2a.x = rect->x;
	p2a.y = rect->y;
	p2e.x = rect->x + rect->w;
	p2e.y = rect->y + rect->h;
	
	for (i = 0; i < blitrects_nr && !done; i++) {
		p1a.x = blitrects[i].x;
		p1a.y = blitrects[i].y;
		p1e.x = blitrects[i].x + blitrects[i].w;
		p1e.y = blitrects[i].y + blitrects[i].h;
			
		if (p2a.x >= p1a.x && p2e.x <= p1e.x &&
			p2a.y >= p1a.y && p2e.y <= p1e.y) { 
			/* p2 is in p1 >> drop rects */
			done = 1;
		}

			/* p1 is in p2 >> change rect */
		else if (p1a.x >= p2a.x && p1e.x <= p2e.x &&
			p1a.y >= p2a.y && p1e.y <= p2e.y) { 
			blitrects[i].x = rect->x;
			blitrects[i].y = rect->y;
			blitrects[i].w = rect->w;
			blitrects[i].h = rect->h;
			done = 1;
		}
	}
	
	/* no match found, add new */
	if (i == blitrects_nr && !done) {
		blitrects_nr++;
		blitrects[i] = *rect;
	}
};


void
gfx_blitdraw ()
{
	int i;
	
	if (blitdb_nr < 0) {
		blitdb_nr = 0;
		return;
	}

	gfx_blitsort ();

	for (i = 0; i < MAX_BLITRECTS && sortblitdb[i] != NULL; i++) {
		SDL_BlitSurface (sortblitdb[i]->srci, &sortblitdb[i]->srcr, sortblitdb[i]->desti, &sortblitdb[i]->destr);
		gfx_blitupdaterectadd (&sortblitdb[i]->destr);
	}
	
	gfx_blitupdaterectdraw ();
	blitdb_nr = 0;
};


/* Add a new image to draw/blit on the screen and in thw right order.
   srcr srci:   source image and rect
   destr desti: destination image and rect
   y:           deep of the image */
void
gfx_blit (SDL_Surface *srci, SDL_Rect *srcr, SDL_Surface *desti, SDL_Rect *destr, int y)
{
	int i;
	SDL_Rect r_src, r_dest;
	
	if (srcr == NULL) {
		srcr = &r_src;
		r_src.x = 0;
		r_src.y = 0;
		r_src.h = srci->h;
		r_src.w = srci->w;
	};
	if (destr == NULL) {
		destr = &r_dest;
		r_dest.x = 0;
		r_dest.y = 0;
		r_dest.h = desti->h;
		r_dest.w = desti->w;
	};
	
	
	/* check if the rects are out of the images and drop this blitting */
	if (srcr->x > srci->w || srcr->y > srci->h || 
		destr->x > desti->w || destr->y > desti->h)
		return;

	/* clipping src */
	if (srcr->x < 0) { // x < 0
		srcr->w += srcr->x;
		destr->w += srcr->x;
		destr->x -= srcr->x;
		srcr->x = 0;
	}
	if (srcr->x+srcr->w > srci->w) { // x+w > img.w
		i = srcr->x+srcr->w - srci->w;
		srcr->w -= i;
		destr->w -= i;
	}
	if (srcr->y < 0) { // y < 0
		srcr->h += srcr->y;
		destr->h += srcr->h;
		destr->y -= srcr->y;
		srcr->y = 0;
	}
	if (srcr->y+srcr->h > srci->h) { // y+h > img.h
		i = srcr->y+srcr->h - srci->h;
		srcr->h -= i;
		destr->h -= i;
	}

	/* clipping dest */
	if (destr->x < 0) { // x < 0
		destr->w += destr->x;
		srcr->w += destr->x;
		srcr->x -= destr->x;
		destr->x = 0;
	}
	if (destr->x+destr->w > desti->w) { // x+w > img.w
		i = destr->x+destr->w - desti->w;
		srcr->w -= i;
		destr->w -= i;
	}
	if (destr->y < 0) { // y < 0
		destr->h += destr->y;
		srcr->h += destr->h;
		srcr->y -= destr->y;
		destr->y = 0;
	}
	if (destr->y+destr->h > desti->h) { // y+h > img.h
		i = destr->y+destr->h - desti->h;
		srcr->h -= i;
		destr->h -= i;
	}
	
	/* add to list */
	if (blitdb_nr < 0)
		blitdb_nr = 0;
	else if (blitdb_nr >= MAX_BLITRECTS) {
		d_fatal ("blitdb_nr > MAX_BLITRECTS\n");
		return;
	}
	
	blitdb[blitdb_nr].srcr = *srcr;
	blitdb[blitdb_nr].srci = srci;
	blitdb[blitdb_nr].destr = *destr;
	blitdb[blitdb_nr].desti = desti;
	blitdb[blitdb_nr].y = y;
	
	blitdb_nr++;
};
