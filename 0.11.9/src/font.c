/* $Id: font.c,v 1.16 2005-07-06 13:11:55 stpohle Exp $ */
// Using Fonts in SDL

#include <string.h>
#include <SDL.h>

#include "bomberclone.h"
#include "font.h"

_point font_lastsize;  // so we can get the last size of the drawn font
_font font[3];

char *color_map[COLOR_max] = {	"white", "red", "green", "blue", 
								"yellow", "brown", "gray", "black"};

void
font_draw (int x, int y, char *text, int size, int color)
{
    int i,
      c;
    SDL_Rect src,
      dest;

    src.y = 0;
    dest.w = src.w = font[size].size.x;
    dest.h = src.h = font[size].size.y;
    dest.x = x;
    dest.y = y;

    for (i = 0; text[i] != 0; i++) {
        c = text[i];
		src.x = font[size].size.x * (c & 15);
        src.y = font[size].size.y * ((c & 240) >> 4);
		SDL_BlitSurface (font[size].image[color], &src, gfx.screen, &dest);
        dest.x += font[size].size.x;
    }
	
	font_lastsize = font[size].size;
};


void
font_gfxdraw (int x, int y, char *text, int size, int color, int ypos)
{
    int i,
      c;
    SDL_Rect src,
      dest;

    src.y = 0;
    dest.w = src.w = font[size].size.x;
    dest.h = src.h = font[size].size.y;
    dest.x = x;
    dest.y = y;

    for (i = 0; text[i] != 0; i++) {
        c = text[i];
		src.x = font[size].size.x * (c & 15);
        src.y = font[size].size.y * ((c & 240) >> 4);
		gfx_blit (font[size].image[color], &src, gfx.screen, &dest, ypos);
        dest.x += font[size].size.x;
    }
	
	font_lastsize = font[size].size;
};



void font_load () {
	int c,i,r,g,b;
	char filename[LEN_PATHFILENAME];
	SDL_Surface *raw, *tmp;
	
    /* load the font */
	for (i = 0; i < 3; i++) {
		sprintf (filename, "%s/gfx/font%d.png", bman.datapath, i);
		tmp = IMG_Load (filename);
		if (tmp  == NULL) {
			printf ("Could not load font.\n");
			exit (1);
		}
		
		SDL_SetColorKey (tmp , SDL_SRCCOLORKEY, SDL_MapRGB (tmp->format, 255, 255, 255));
		raw = SDL_DisplayFormat (tmp);
		
		for (c = 0; c < COLOR_max; c++) {
			switch (c) {
				case COLOR_white: // color white
					r = 255;
					g = 255;
					b = 255;
					break;
				case COLOR_red: // color Red
					r = 255;
					g = 0;
					b = 0;
					break;
				case COLOR_green: // color Green
					r = 0;
					g = 255;
					b = 0;
					break;
				case COLOR_blue: // color Blue
					r = 0;
					g = 0;
					b = 255;
					break;
				case COLOR_yellow: // color Yellow
					r = 255;
					g = 255;
					b = 0;
					break;
				case COLOR_brown: // color Brown
					r = 255;
					g = 128;
					b = 0;
					break;
				case COLOR_black: // color Black
					r = 8;
					g = 8;
					b = 8;
					break;
				default: // color Gray
					r = 128;
					g = 128;
					b = 128;
					break;
			}
			
			font[i].image[c] = SDL_DisplayFormat (tmp);
			SDL_SetColorKey (font[i].image[c] , SDL_SRCCOLORKEY, SDL_MapRGB (font[i].image[c]->format, 0,0,0));
			SDL_FillRect (font[i].image[c], NULL, SDL_MapRGB (font[i].image[c]->format, r,g,b));
			SDL_BlitSurface (raw, NULL, font[i].image[c], NULL);
		}
		
		font[i].size.x = tmp->w / 16;
		font[i].size.y = tmp->h / 16;
		
		SDL_FreeSurface (raw);
		SDL_FreeSurface (tmp);
	}
};


void font_free() {
	int i, c;
	
	for (i = 0; i < 3; i++) for (c = 0; c < 7; c++) {
		SDL_FreeSurface (font[i].image[c]);
		font[i].image[c] = NULL;
	}
};


void font_drawbold (int x, int y, char *text, int size, int color, int bold) {
    font_draw (x - bold, y, text, size, color);
    font_draw (x + bold, y, text, size, color);
    font_draw (x, y - bold, text, size, color);
    font_draw (x, y + bold, text, size, color);
};


void font_gfxdrawbold (int x, int y, char *text, int size, int color, int bold, int ypos) {
    font_gfxdraw (x - bold, y, text, size, color, ypos);
    font_gfxdraw (x + bold, y, text, size, color, ypos);
    font_gfxdraw (x, y - bold, text, size, color, ypos);
    font_gfxdraw (x, y + bold, text, size, color, ypos);
};
