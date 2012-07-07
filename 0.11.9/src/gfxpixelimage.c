/* $Id: gfxpixelimage.c,v 1.13 2005-04-09 18:22:41 stpohle Exp $ */
/* gfx pixel manipulation and image manipulation */

#include "bomberclone.h"

void
getRGBpixel (SDL_Surface * surface, int x, int y, int *R, int *G, int *B)
{
    Uint32 pixel = 0;
    Uint8 r,
      g,
      b;

    /* Lock the screen for direct access to the pixels */
    if (SDL_MUSTLOCK (surface))
        if (SDL_LockSurface (surface) < 0) {
            fprintf (stderr, "Can't lock screen: %s\n", SDL_GetError ());
            return;
        }
    pixel = getpixel (surface, x, y);
    if (SDL_MUSTLOCK (surface)) {
        SDL_UnlockSurface (surface);
    }
    SDL_GetRGB (pixel, surface->format, &r, &g, &b);
    *R = r;
    *G = g;
    *B = b;
};


/* getpixel for every BPP version */
Uint32
getpixel (SDL_Surface * surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;

    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * bpp;
    switch (bpp) {
    case 1:
        return *p;
    case 2:
        return *(Uint16 *) p;
    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
    case 4:
        return *(Uint32 *) p;
    default:
        return 0;               /* shouldn't happen, but avoids warnings */
    }
};


static inline Uint32
getpixel32 (SDL_Surface * surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel;
    return *(Uint32 *) p;
};


static inline Uint32
getpixel24 (SDL_Surface * surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel;;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        return p[0] << 16 | p[1] << 8 | p[2];
#else
        return p[0] | p[1] << 8 | p[2] << 16;
#endif
};


static inline Uint32
getpixel16 (SDL_Surface * surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel;
    return *(Uint16 *) p;
};


/* putpixel seperated for every BPP version */
inline void
putpixel (SDL_Surface * surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    int bpp = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
    case 1:
        *p = pixel;
        break;
    case 2:
        *(Uint16 *) p = pixel;
        break;
    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        }
        else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;
    case 4:
        *(Uint32 *) p = pixel;
        break;
    }
};


static inline void
putpixel32 (SDL_Surface * surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel;
    *(Uint32 *) p = pixel;
};


static inline void
putpixel24 (SDL_Surface * surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    p[0] = (pixel >> 16) & 0xff;
    p[1] = (pixel >> 8) & 0xff;
    p[2] = pixel & 0xff;
#else 
    p[0] = pixel & 0xff;
    p[1] = (pixel >> 8) & 0xff;
    p[2] = (pixel >> 16) & 0xff;
#endif
};


static inline void
putpixel16 (SDL_Surface * surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel;
    *(Uint16 *) p = pixel;
};


void
scale (short int *dpattern, short int x, short int y)
{
    int a,
      dx,
      dy;
    if (x >= SCALE_MAXRES || y >= SCALE_MAXRES) {
        for (x = 0; x < SCALE_MAXRES; x++)
            dpattern[x] = 0;
        return;
    }
    if (x > y) {
        dy = 2 * y;
        dx = a = 2 * x - dy;

        do {
            if (a <= 0) {
                dpattern[(y--) - 1] = x;
                a = a + dx;
            }

            else
                a = a - dy;
        } while (x--);
    }

    else {
        dy = 2 * x;
        dx = a = 2 * y - dy;

        do {
            dpattern[y] = x;
            if (a <= 0) {
                x--;
                a = a + dx;
            }

            else
                a = a - dy;
        } while (y--);
    }
};


SDL_Surface *
scale_image (SDL_Surface *orginal, int newx, int newy)
{
    Uint32 rmask,
      gmask,
      bmask,
      amask;
    SDL_Surface *surface;
    int y,
      x;
	int bpp = orginal->format->BytesPerPixel;
    short int xpattern[SCALE_MAXRES];
    short int ypattern[SCALE_MAXRES];

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else /*  */
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif /*  */

    surface = SDL_CreateRGBSurface (SDL_HWSURFACE, newx, newy, 32, rmask, gmask, bmask, amask);
    if (surface == NULL) {
        fprintf (stderr, "CreateRGBSurface failed: %s\n", SDL_GetError ());
        return NULL;
    }

    /* Lock the screen for direct access to the pixels */
    if (SDL_MUSTLOCK (surface))
        if (SDL_LockSurface (surface) < 0) {
            fprintf (stderr, "Can't lock screen: %s\n", SDL_GetError ());
            return NULL;
        }
    if (SDL_MUSTLOCK (orginal))
        if (SDL_LockSurface (orginal) < 0) {
            fprintf (stderr, "Can't lock screen: %s\n", SDL_GetError ());
            if (SDL_MUSTLOCK (surface)) {
                SDL_UnlockSurface (surface);
            }
            return NULL;
        }

    /* do the scaling work */
	scale (xpattern, orginal->w - 1, newx);
	scale (ypattern, orginal->h - 1, newy);
	
	switch (bpp) {
		case (2):
			for (x = newx - 1; x >= 0; x--)
				for (y = newy - 1; y >= 0; y--)
					putpixel16 (surface, x, y, getpixel16 (orginal, xpattern[x], ypattern[y]));
			break;
		case (3):
			for (x = newx - 1; x >= 0; x--)
				for (y = newy - 1; y >= 0; y--)
					putpixel24 (surface, x, y, getpixel24 (orginal, xpattern[x], ypattern[y]));
			break;
		case (4):
			for (x = newx - 1; x >= 0; x--)
				for (y = newy - 1; y >= 0; y--)
					putpixel32 (surface, x, y, getpixel32 (orginal, xpattern[x], ypattern[y]));
			break;
		default:
			printf ("scale_image, wrong bpp value (%d).\n", bpp);
			exit (1);
			break;
	}
			
    if (SDL_MUSTLOCK (orginal)) {
        SDL_UnlockSurface (orginal);
    }
    if (SDL_MUSTLOCK (surface)) {
        SDL_UnlockSurface (surface);
    }
    return surface;
};



SDL_Surface *
makegray_image (SDL_Surface * org)
{
    Uint32 rmask,
      gmask,
      bmask,
      amask;
    Uint32 pixel,
      transpixel = 0;
    SDL_Surface *dest;
	int bpp = org->format->BytesPerPixel,
	  y,
      x;
    Uint8 r,
      g,
      b,
      gray;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else /*  */
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif /*  */

	dest = SDL_CreateRGBSurface (SDL_SWSURFACE, org->w, org->h, org->format->BitsPerPixel,
                                 org->format->Rmask, org->format->Gmask,
                                 org->format->Bmask, org->format->Amask);
    if (dest == NULL) {
        fprintf (stderr, "CreateRGBSurface failed: %s\n", SDL_GetError ());
        return NULL;
    }

    /* Lock the screen for direct access to the pixels */
    if (SDL_MUSTLOCK (dest))
        if (SDL_LockSurface (dest) < 0) {
            fprintf (stderr, "Can't lock screen: %s\n", SDL_GetError ());
            return NULL;
        }
    if (SDL_MUSTLOCK (org))
        if (SDL_LockSurface (org) < 0) {
            fprintf (stderr, "Can't lock screen: %s\n", SDL_GetError ());
            if (SDL_MUSTLOCK (dest)) {
                SDL_UnlockSurface (dest);
            }
            return NULL;
        }

	switch (bpp) {
		case (2):
			for (x = 0; x < org->w; x++)
				for (y = 0; y < org->h; y++) {
					pixel = getpixel16 (org, x, y);
					if (x == 0 && y == 0)
						transpixel = pixel;
					if (pixel != transpixel) {
						SDL_GetRGB (pixel, org->format, &r, &g, &b);
						gray = (r / 3 + g / 3 + b / 3);
						pixel = SDL_MapRGB (dest->format, gray, gray, gray);
					}
					putpixel16 (dest, x, y, pixel);
				}
			break;
		case (3):
			for (x = 0; x < org->w; x++)
				for (y = 0; y < org->h; y++) {
					pixel = getpixel24 (org, x, y);
					if (x == 0 && y == 0)
						transpixel = pixel;
					if (pixel != transpixel) {
						SDL_GetRGB (pixel, org->format, &r, &g, &b);
						gray = (r / 3 + g / 3 + b / 3);
						pixel = SDL_MapRGB (dest->format, gray, gray, gray);
					}
					putpixel24 (dest, x, y, pixel);
				}
			break;
		case (4):
			for (x = 0; x < org->w; x++)
				for (y = 0; y < org->h; y++) {
					pixel = getpixel32 (org, x, y);
					if (x == 0 && y == 0)
						transpixel = pixel;
					if (pixel != transpixel) {
						SDL_GetRGB (pixel, org->format, &r, &g, &b);
						gray = (r / 3 + g / 3 + b / 3);
						pixel = SDL_MapRGB (dest->format, gray, gray, gray);
					}
					putpixel32 (dest, x, y, pixel);
				}
			break;
		default:
			printf ("gray_image, wrong bpp value (%d).\n", bpp);
			exit (1);
			break;
	}

    if (SDL_MUSTLOCK (org)) {
        SDL_UnlockSurface (org);
    }
    if (SDL_MUSTLOCK (dest)) {
        SDL_UnlockSurface (dest);
    }
    SDL_SetColorKey (dest, SDL_SRCCOLORKEY, transpixel);
    return dest;
};

SDL_Surface *
gfx_quater_image (SDL_Surface * org1, SDL_Surface * org2, SDL_Surface * org3, SDL_Surface * org4)
{
    Uint32 pixel;
    SDL_Surface *dest;
    int y,
      x;

    dest = SDL_CreateRGBSurface (SDL_HWSURFACE, org1->w, org1->h, org1->format->BitsPerPixel,
                                 org1->format->Rmask, org1->format->Gmask,
                                 org1->format->Bmask, org1->format->Amask);
    if (dest == NULL) {
        fprintf (stderr, "CreateRGBSurface failed: %s\n", SDL_GetError ());
        return NULL;
    }

    /* Lock the screen for direct access to the pixels */
    if (SDL_MUSTLOCK (dest))
        if (SDL_LockSurface (dest) < 0) {
            fprintf (stderr, "Can't lock screen: %s\n", SDL_GetError ());
            return NULL;
        }
    if (SDL_MUSTLOCK (org1))
        if (SDL_LockSurface (org1) < 0) {
            fprintf (stderr, "Can't lock screen: %s\n", SDL_GetError ());
            if (SDL_MUSTLOCK (dest)) {
                SDL_UnlockSurface (dest);
            }
            return NULL;
        }
    if (SDL_MUSTLOCK (org2))
        if (SDL_LockSurface (org2) < 0) {
            fprintf (stderr, "Can't lock screen: %s\n", SDL_GetError ());
            if (SDL_MUSTLOCK (dest)) {
                SDL_UnlockSurface (dest);
            }
            return NULL;
        }
    if (SDL_MUSTLOCK (org3))
        if (SDL_LockSurface (org3) < 0) {
            fprintf (stderr, "Can't lock screen: %s\n", SDL_GetError ());
            if (SDL_MUSTLOCK (dest)) {
                SDL_UnlockSurface (dest);
            }
            return NULL;
        }
    if (SDL_MUSTLOCK (org4))
        if (SDL_LockSurface (org4) < 0) {
            fprintf (stderr, "Can't lock screen: %s\n", SDL_GetError ());
            if (SDL_MUSTLOCK (dest)) {
                SDL_UnlockSurface (dest);
            }
            return NULL;
        }
    for (x = 0; x < org1->w / 2; x++)
        for (y = 0; y < org1->h / 2; y++) {
            pixel = getpixel (org1, x, y);
            putpixel (dest, x, y, pixel);
        }
    for (x = org1->w / 2; x < org1->w; x++)
        for (y = 0; y < org1->h / 2; y++) {
            pixel = getpixel (org2, x, y);
            putpixel (dest, x, y, pixel);
        }
    for (x = 0; x < org1->w / 2; x++)
        for (y = org1->h / 2; y < org1->h; y++) {
            pixel = getpixel (org3, x, y);
            putpixel (dest, x, y, pixel);
        }
    for (x = org1->w / 2; x < org1->w; x++)
        for (y = org1->h / 2; y < org1->h; y++) {
            pixel = getpixel (org4, x, y);
            putpixel (dest, x, y, pixel);
        }

    if (SDL_MUSTLOCK (org1))
        SDL_UnlockSurface (org1);
    if (SDL_MUSTLOCK (org2))
        SDL_UnlockSurface (org2);
    if (SDL_MUSTLOCK (org3))
        SDL_UnlockSurface (org3);
    if (SDL_MUSTLOCK (org4))
        SDL_UnlockSurface (org4);
    if (SDL_MUSTLOCK (dest)) {
        SDL_UnlockSurface (dest);
    }
    return dest;
};


/*
 * part of a surface from one to another with the same format 
 * if rect = NULL, copy the whole image
 */
SDL_Surface *gfx_copyfrom (SDL_Surface *img, SDL_Rect *rect) {
    SDL_Surface *res;
	SDL_Rect src, dest;

	if (img == NULL) return NULL;
	
	if (rect == NULL) {
		src.x = 0;
		src.y = 0;
		src.h = img->h;
		src.w = img->w;
	}
	else
		src = *rect;
	
    res =
        SDL_CreateRGBSurface (SDL_HWSURFACE, src.w, src.h, img->format->BitsPerPixel,
                              img->format->Rmask, img->format->Gmask,
                              img->format->Bmask, img->format->Amask);
	
    dest.x = 0;
    dest.y = 0;
    dest.w = src.w;
    dest.h = src.h;
    SDL_BlitSurface (img, &src, res, &dest);
    return res;
};


/* restore screen from copy */
void gfx_restorescreen (SDL_Surface *img, SDL_Rect *wnd)
{
	SDL_Rect dest;
	
    dest.x = 0;
    dest.y = 0;
    dest.w = wnd->w;
    dest.h = wnd->h;
    gfx_blit (img, &dest, gfx.screen, wnd, 0);
    return;
};
