/* $Id: gfx.h,v 1.10 2005-04-09 18:22:40 stpohle Exp $ */
#ifndef _GFX_H_

#define _GFX_H_

#define SCALE_MAXRES 10000
#define MAX_BLITRECTS 32000
#define GFX_IMGSIZE 64
#define GFX_IMGBIGSIZE 96
#define GFX_PLAYERIMGSIZE_Y 128
#define GFX_SMALLPLAYERIMGSIZE_X 12
#define GFX_MENUPLAYERIMGSIZE_X 32
#define GFX_MENUFIELDIMGSIZE 24

#include "basic.h"

struct __gfxblit {
	SDL_Rect srcr;
	SDL_Surface *srci;
	SDL_Rect destr;
	SDL_Surface *desti;
    int y;
} typedef _gfxblit;


struct __gfxani {
	SDL_Surface *image;
    int frames;                 // how many single frames (image -> heigh / (1.5 * gamestyle.height))
	int w;						// size of a single frame
	int h;
} typedef _gfxani;


struct __gfxplayer {
	_gfxani ani;
    _point offset;
	SDL_Surface *small_image;			// small size of the player (single frame)
	SDL_Surface *menu_image;			// menu image of the player (single frame)
} typedef _gfxplayer;


struct __gfx {
	SDL_Surface *screen;
	_point res;		// resolution
	_point block;	// block size
	short int bpp;  // bits per pixel
	
    int fullscreen;

	_point offset;    // where the game field starts
	
	_gfxplayer *players;
	int player_gfx_count;
	short int postab[256];  // table of points where we need to go to.
	
	_gfxani field[FT_max];  // the field animations
	SDL_Surface *menu_field[FT_max];
	_gfxani powerup[3];	// powerup field animation
	_gfxani fire;       // fire (explostion)
	_gfxani bomb;		// bomb animation
	_gfxani ill;		// sick animation above the player
	_gfxani dead;		// load the dead player animation
	SDL_Surface *ghost;	// gfx of the ghost player.
	SDL_Surface *ghost_small;	// small ghost player
	_gfxani respawn;	// respawn image
	
	_gfxani menuselect; // The Menu Select GFX (the bomb ?)
	
    SDL_Surface *logo;
} typedef _gfx;

extern _gfx gfx;


// gfx.c
extern void gfx_loaddata ();
extern void redraw_logo (int x, int y, int w, int h);
extern void draw_logo ();
extern void gfx_init ();		// Load Base Image Data
extern void gfx_shutdown ();
extern void draw_shadefield (SDL_Surface *s, SDL_Rect *rec, int c);
extern int gfx_locksurface (SDL_Surface *surface);
extern void gfx_unlocksurface (SDL_Surface *surface);
extern void redraw_logo_shaded (int x, int y, int w, int h, int c);
extern void gfx_load_players (int sx, int sy);
extern void gfx_free_players ();
extern int gfx_get_nr_of_playergfx ();

// gfxpixelimage.c
extern void getRGBpixel (SDL_Surface *surface, int x, int y, int *R, int *G, int *B);
extern Uint32 getpixel(SDL_Surface *surface, int x, int y);
extern void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
extern void scale (short int *dpattern, short int x, short int y);
extern SDL_Surface *scale_image (SDL_Surface * orginal, int newx, int newy);
extern void shade_pixel(SDL_Surface *s, int x, int y, int c);
extern SDL_Surface *makegray_image (SDL_Surface *org);
extern SDL_Surface *gfx_copyfrom(SDL_Surface *img, SDL_Rect *wnd);
#define gfx_copyscreen(__wnd) gfx_copyfrom(gfx.screen, __wnd)
extern void gfx_restorescreen (SDL_Surface *img, SDL_Rect *wnd);


// gfxengine.c
extern void gfxengine_init ();
extern void gfx_blitdraw ();
extern void gfx_blit (SDL_Surface *srci, SDL_Rect *srcr, SDL_Surface *desti, SDL_Rect *destr, int y);
extern void gfx_blitsort ();
extern inline void gfx_blitsortclear ();
extern inline void gfx_blitupdaterectclear ();
extern void gfx_blitupdaterectdraw ();
extern void gfx_blitupdaterectadd (SDL_Rect *rect);

#endif
