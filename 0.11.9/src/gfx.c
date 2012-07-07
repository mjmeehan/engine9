/* $Id: gfx.c,v 1.42 2009-10-10 09:43:55 stpohle Exp $ */
/* gfx.c */

#include "bomberclone.h"
#include "menu.h"

_gfx gfx;

int gfx_get_nr_of_playergfx ();
static void gfx_load_menusmall_players ();
static void gfx_load_fieldtype_menu ();

/*
 * count all playergfx 
 */
int gfx_get_nr_of_playergfx () {
	int i = 0;
	FILE *f = NULL;
	char filename[255];
	
	do {
		if (f) {
			fclose (f);
			i++;
		}
		sprintf (filename, "%s/player/player%d.png", bman.datapath, i);
		f = fopen (filename, "r");
	} while (f);

	return i;
}


/*
 * Load all players graphics we have
 */
void
gfx_load_players (int sx, int sy)
{
    float sfkt,
      ssfkt;
    char filename[255];
    int i,
      r,
      g,
      b;
    SDL_Surface *tmpimage,
     *tmpimage1;
    sfkt = ((float) sx) / ((float) GFX_IMGSIZE);
    ssfkt = ((float) GFX_SMALLPLAYERIMGSIZE_X) / ((float) GFX_IMGSIZE);

	d_printf ("gfx_load_players (%d, %d)\n", sx, sy);
	
    /* loading the player images */
    for (i = 0; i < gfx.player_gfx_count; i++) {
        sprintf (filename, "%s/player/player%d.png", bman.datapath, i);
        tmpimage = IMG_Load (filename);
        if (tmpimage == NULL) {
			printf ("Can't load image: %s\n", SDL_GetError ());
			exit (1);
		}
	
        else {
            /* load the game player image */
            gfx.players[i].ani.h = sy * 2;
            gfx.players[i].ani.w = (tmpimage->w / 4) * sfkt;
            gfx.players[i].ani.frames = tmpimage->h / GFX_PLAYERIMGSIZE_Y;
			
            tmpimage1 = scale_image (tmpimage, gfx.players[i].ani.w * 4, gfx.players[i].ani.frames * gfx.players[i].ani.h);
            getRGBpixel (tmpimage1, 0, 0, &r, &g, &b);
            SDL_SetColorKey (tmpimage1, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimage1->format, r, g, b));
		    gfx.players[i].ani.image = SDL_DisplayFormat (tmpimage1);
            SDL_FreeSurface (tmpimage1);
	    
	    /* calculate the numbers of images for the animation */
	    gfx.players[i].offset.x = (sx - gfx.players[i].ani.w) / 2;
	    gfx.players[i].offset.y = -sy;
	    SDL_FreeSurface (tmpimage);
        }
    }

    /* load the death image */
    sprintf (filename, "%s/player/dead0.png", bman.datapath);
	tmpimage = IMG_Load (filename);
    if (tmpimage == NULL) {
        /* no image found - set field clear */
        printf ("Player Animation Could not be loaded (%s)\n", filename);
        exit (1);
    }

    gfx.dead.frames = tmpimage->h / (2* GFX_IMGSIZE);
    tmpimage1 = scale_image (tmpimage, ((2 * sx * tmpimage->w) / (2 * GFX_IMGSIZE)), gfx.dead.frames * (2 * sy));
    getRGBpixel (tmpimage1, 0, 0, &r, &g, &b);
    SDL_SetColorKey (tmpimage1, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimage1->format, r, g, b));
    gfx.dead.image = SDL_DisplayFormat (tmpimage1);
    SDL_FreeSurface (tmpimage1);
    SDL_FreeSurface (tmpimage);
    
    /* load the illnessthing */
    sprintf (filename, "%s/player/playersick.png", bman.datapath);
    tmpimage = IMG_Load (filename);
    if (tmpimage == NULL) {
		printf ("Can't load image: %s\n", SDL_GetError ());
		exit (1);
    }
    gfx.ill.frames = tmpimage->h / (2 * GFX_IMGSIZE);
    tmpimage1 = scale_image (tmpimage, (2 * sx * tmpimage->w) / (2 * GFX_IMGSIZE), gfx.ill.frames * (2 * sy));
    getRGBpixel (tmpimage1, 0, 0, &r, &g, &b);
    SDL_SetColorKey (tmpimage1, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimage1->format, r, g, b));
    gfx.ill.image = SDL_DisplayFormat (tmpimage1);
    SDL_FreeSurface (tmpimage);
    SDL_FreeSurface (tmpimage1);
   
    /* load the respawn gfx */
    sprintf (filename, "%s/player/respawn.png", bman.datapath);
    tmpimage = IMG_Load (filename);
    if (tmpimage == NULL) {
		printf ("Can't load image: %s\n", SDL_GetError ());
		exit (1);
    }
    gfx.respawn.frames = tmpimage->h / (2 * GFX_IMGSIZE);
    gfx.respawn.image  = scale_image (tmpimage, (2 * sx * tmpimage->w) / (2 * GFX_IMGSIZE), gfx.respawn.frames * (2 * sy)); 
    SDL_FreeSurface (tmpimage);
};


/* frees the player images */
void
gfx_free_players ()
{
    int i;
    
    d_printf ("gfx_free_players\n");
	
    for (i = 0; i < gfx.player_gfx_count; i++) {
        if (gfx.players[i].ani.image != NULL)
            SDL_FreeSurface (gfx.players[i].ani.image);
        gfx.players[i].ani.image = NULL;
    }

    if (gfx.dead.image != NULL)
        SDL_FreeSurface (gfx.dead.image);
    if (gfx.ill.image != NULL)
        SDL_FreeSurface (gfx.ill.image);
    if (gfx.respawn.image != NULL)
        SDL_FreeSurface (gfx.respawn.image);
}


/* init the whole GFX Part */
void
gfx_init ()
{
	int i;

    if (gfx.fullscreen)
        gfx.screen = SDL_SetVideoMode (gfx.res.x, gfx.res.y, gfx.bpp, SDL_SWSURFACE | SDL_DOUBLEBUF | SDL_HWACCEL | SDL_FULLSCREEN);

    else
        gfx.screen = SDL_SetVideoMode (gfx.res.x, gfx.res.y, gfx.bpp, SDL_SWSURFACE | SDL_DOUBLEBUF | SDL_HWACCEL);
    if (gfx.screen == NULL) {
        d_printf ("Unable to set video mode: %s\n", SDL_GetError ());
        return;
    }
    SDL_ShowCursor (SDL_DISABLE);
	
	/* delete small gfx und the menu player gfx */
	gfx.player_gfx_count = gfx_get_nr_of_playergfx();
	gfx.players = malloc (gfx.player_gfx_count * sizeof (_gfxplayer));
	for (i = 0; i < gfx.player_gfx_count; i++) {
		gfx.players[i].ani.image = NULL;
		gfx.players[i].small_image = NULL;
		gfx.players[i].menu_image = NULL;
	}
	
	for (i = 0; i < FT_max; i++) gfx.menu_field[i] = NULL;

	gfx_blitupdaterectclear ();
    gfx_loaddata ();
};



void
gfx_loaddata ()
{
    int i, j;
    char filename[255];
    SDL_Surface *tmpimage,
     *tmpimage1;

    /* load the logo */
    sprintf (filename, "%s/gfx/logo.png", bman.datapath);
    tmpimage = IMG_Load (filename);
    if (tmpimage == NULL) {
		printf ("Can't load image: %s\n", SDL_GetError ());
    	exit (1);
    }
    tmpimage1 = scale_image (tmpimage, gfx.res.x, gfx.res.y);
    SDL_FreeSurface (tmpimage);
    SDL_SetColorKey (tmpimage1, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimage1->format, 255, 255, 0));
    gfx.logo = SDL_DisplayFormat (tmpimage1);
    SDL_FreeSurface (tmpimage1);

	font_load ();
	
	/* load the menugraphics */
	for (i = 0; i < 9; i++) {
		sprintf (filename, "%s/gfx/menu%d.png", bman.datapath, i);
		menuimages[i] = IMG_Load (filename);
	    if (menuimages[i] == NULL) {
    	    printf ("Can't load image: %s\n", SDL_GetError ());
        	exit (1);
	    }
	}
	
	/* load menu buttongraphic */
	for (j = 0; j < 3; j++) 
		for (i = 0; i < 3; i++) {
			sprintf (filename, "%s/gfx/menubutton%d_%d.png", bman.datapath, j, i);
			tmpimage = IMG_Load (filename);
	    	if (tmpimage == NULL) {
	    	    printf ("Can't load image: %s\n", SDL_GetError ());
    	    	exit (1);
	    	}
		    SDL_SetColorKey (tmpimage, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimage->format, 255, 255, 255));
    		menubuttonimages[j][i] = SDL_DisplayFormat (tmpimage);
	    	SDL_FreeSurface (tmpimage);
	}

	/* load menu buttongraphic */
	for (j = 0; j < 2; j++) 
		for (i = 0; i < 3; i++) {
			sprintf (filename, "%s/gfx/menuentry%d_%d.png", bman.datapath, j, i);
			tmpimage = IMG_Load (filename);
	    	if (tmpimage == NULL) {
	    	    printf ("Can't load image: %s\n", SDL_GetError ());
    	    	exit (1);
	    	}
		    SDL_SetColorKey (tmpimage, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimage->format, 255, 255, 255));
    		menuentryimages[j][i] = SDL_DisplayFormat (tmpimage);
	    	SDL_FreeSurface (tmpimage);
	}
	
	/* load menu listgraphic */
	for (j = 0; j < 2; j++) 
		for (i = 0; i < 9; i++) {
			sprintf (filename, "%s/gfx/menulist%d_%d.png", bman.datapath, j, i);
			tmpimage = IMG_Load (filename);
	    	if (tmpimage == NULL) {
	    	    printf ("Can't load image: %s\n", SDL_GetError ());
    	    	exit (1);
	    	}
		    SDL_SetColorKey (tmpimage, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimage->format, 255, 255, 255));
    		menulistimages[j][i] = SDL_DisplayFormat (tmpimage);
	    	SDL_FreeSurface (tmpimage);
	}
	
	/* load menuselect animation */
    sprintf (filename, "%s/gfx/menuselect.png", bman.datapath);
    gfx.menuselect.image = IMG_Load (filename);
    if (gfx.menuselect.image == NULL) {
        printf ("Can't load image: %s\n", SDL_GetError ());
        exit (1);
    }
    gfx.menuselect.frames = tmpimage->h / (2 * GFX_IMGSIZE);
	
	gfx_load_fieldtype_menu ();
	gfx_load_menusmall_players ();
};


/***
 *	load a single frame of the player 
 */
static void gfx_load_menusmall_players () {
	SDL_Surface *orgimg, *tmpimg;
	int i, r, g, b;
	float sfkt;
	char filename[255];

	for (i = 0; i < gfx.player_gfx_count; i++) {
		if (gfx.players[i].small_image == NULL || gfx.players[i].menu_image == NULL) {
			SDL_Surface *img;
			SDL_Rect rect;

			sprintf (filename, "%s/player/player%d.png", bman.datapath, i);
			orgimg = IMG_Load (filename);
			if (orgimg == NULL) {
	   	     	printf ("Can't load image: %s\n", SDL_GetError ());
	   	    	exit (1);
	   		}
			rect.x = 3 * (orgimg->w/4);
			rect.y = 0;
			rect.w = orgimg->w/4;
			rect.h = GFX_PLAYERIMGSIZE_Y;
			img = gfx_copyfrom (orgimg, &rect);
			
			SDL_FreeSurface (orgimg);
			
			/* small image */
			sfkt = (float)(((float)(GFX_SMALLPLAYERIMGSIZE_X * 2)) / ((float)img->h));
			if (gfx.players[i].small_image == NULL) {
				tmpimg = scale_image (img, (int)(((float)img->w)*sfkt), GFX_SMALLPLAYERIMGSIZE_X * 2);
            	getRGBpixel (tmpimg, 0, 0, &r, &g, &b);
		        SDL_SetColorKey (tmpimg, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimg->format, r, g, b));
				gfx.players[i].small_image = SDL_DisplayFormat (tmpimg);
				SDL_FreeSurface (tmpimg);
			}
			/* menu image */
			sfkt = (float)(((float)(GFX_MENUPLAYERIMGSIZE_X * 2)) / ((float)img->h));
			if (gfx.players[i].menu_image == NULL) {
				tmpimg = scale_image (img, (int)(((float)img->w)*sfkt), GFX_MENUPLAYERIMGSIZE_X * 2);
	            getRGBpixel (tmpimg, 0, 0, &r, &g, &b);
		        SDL_SetColorKey (tmpimg, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimg->format, r, g, b));
				gfx.players[i].menu_image = SDL_DisplayFormat (tmpimg);
				SDL_FreeSurface (tmpimg);
			}
			SDL_FreeSurface (img);
		}
	}
	
	/* load the ghost player */
	sprintf (filename, "%s/player/ghost.png", bman.datapath);
	orgimg = IMG_Load (filename);
	sfkt = (float)(((float)(GFX_MENUPLAYERIMGSIZE_X * 2)) / ((float)orgimg->h));
	gfx.ghost = scale_image (orgimg, (int)(((float)orgimg->w)*sfkt), GFX_MENUPLAYERIMGSIZE_X * 2);
	if (gfx.ghost == NULL) {
     	printf ("Can't load image: %s\n", SDL_GetError ());
    	exit (1);
	}
	sfkt = (float)(((float)(GFX_SMALLPLAYERIMGSIZE_X * 2)) / ((float)orgimg->h));
	gfx.ghost_small = scale_image (orgimg, (int)(((float)orgimg->w)*sfkt), GFX_SMALLPLAYERIMGSIZE_X * 2);

	SDL_FreeSurface (orgimg);
}



/***
 * load a single frame from the powerups
 */
static void gfx_load_fieldtype_menu () {	
	int i, ft, r, g, b;
	SDL_Surface *background = NULL, *orgimg = NULL, *tmpimg = NULL;
	char filename[255];
	SDL_Rect rect;
	
	for (i = 0; i < FT_max; i++) {
		if (gfx.menu_field[i]!=NULL) SDL_FreeSurface (gfx.menu_field[i]);
		gfx.menu_field[i] = NULL;
	}

	rect.x = 0;
	rect.y = 0;
	rect.w = GFX_IMGSIZE;
	rect.h = GFX_IMGSIZE;
	
	for (ft = 0; ft < FT_max; ft++) if (ft != FT_mixed) {
		/*
		 * load background image
		 */
		if (ft == 0) { 
			if (background != NULL) SDL_FreeSurface (background);
			sprintf (filename, "%s/tileset/default/background.png", bman.datapath);
			orgimg = IMG_Load (filename);
			if (!orgimg) {
				printf ("Can't load image. :%s\n", SDL_GetError ());
				exit (1);
			}

			tmpimg = gfx_copyfrom (orgimg, &rect);
			SDL_FreeSurface (orgimg);
			orgimg = scale_image (tmpimg, GFX_MENUFIELDIMGSIZE, GFX_MENUFIELDIMGSIZE);
			SDL_FreeSurface (tmpimg);
			background = SDL_DisplayFormat (orgimg);
			SDL_FreeSurface (orgimg);
		}
		
		if (ft == FT_death) {
			if (background != NULL) SDL_FreeSurface (background);
			sprintf (filename, "%s/tileset/default/powerbad.png", bman.datapath);
			orgimg = IMG_Load (filename);
			if (!orgimg) {
				printf ("Can't load image. :%s\n", SDL_GetError ());
				exit (1);
			}

			tmpimg = gfx_copyfrom (orgimg, &rect);
			SDL_FreeSurface (orgimg);
			orgimg = scale_image (tmpimg, GFX_MENUFIELDIMGSIZE, GFX_MENUFIELDIMGSIZE);
			SDL_FreeSurface (tmpimg);
			background = SDL_DisplayFormat (orgimg);
			SDL_FreeSurface (orgimg);
		}
		
		if (ft == FT_fire) {
			if (background != NULL) SDL_FreeSurface (background);
			sprintf (filename, "%s/tileset/default/powerup.png", bman.datapath);
			orgimg = IMG_Load (filename);
			if (!orgimg) {
				printf ("Can't load image. :%s\n", SDL_GetError ());
				exit (1);
			}

			tmpimg = gfx_copyfrom (orgimg, &rect);
			SDL_FreeSurface (orgimg);
			orgimg = scale_image (tmpimg, GFX_MENUFIELDIMGSIZE, GFX_MENUFIELDIMGSIZE);
			SDL_FreeSurface (tmpimg);
			background = SDL_DisplayFormat (orgimg);
			SDL_FreeSurface (orgimg);
		}

		if (ft == FT_sp_trigger) {
			if (background != NULL) SDL_FreeSurface (background);
			sprintf (filename, "%s/tileset/default/powersp.png", bman.datapath);
			orgimg = IMG_Load (filename);
			if (!orgimg) {
				printf ("Can't load image. :%s\n", SDL_GetError ());
				exit (1);
			}

			tmpimg = gfx_copyfrom (orgimg, &rect);
			SDL_FreeSurface (orgimg);
			orgimg = scale_image (tmpimg, GFX_MENUFIELDIMGSIZE, GFX_MENUFIELDIMGSIZE);
			SDL_FreeSurface (tmpimg);
			background = SDL_DisplayFormat (orgimg);
			SDL_FreeSurface (orgimg);
		}
		
		/*
		 * load fieldgfx for the menu
		 */
		gfx.menu_field[ft] = gfx_copyfrom (background, NULL);
		sprintf (filename, "%s/tileset/default/%s.png", bman.datapath, ft_filenames[ft]);
		
		orgimg = IMG_Load (filename);
		if (!orgimg) {
			printf ("Can't load image. :%s\n", SDL_GetError ());
			exit (1);
		}
		
		tmpimg = gfx_copyfrom (orgimg, &rect);
		SDL_FreeSurface (orgimg);
		orgimg = scale_image (tmpimg, GFX_MENUFIELDIMGSIZE, GFX_MENUFIELDIMGSIZE);
		SDL_FreeSurface (tmpimg);
       	getRGBpixel (orgimg, 0, 0, &r, &g, &b);
        SDL_SetColorKey (orgimg, SDL_SRCCOLORKEY, SDL_MapRGB (orgimg->format, r, g, b));
		tmpimg = SDL_DisplayFormat (orgimg);
		SDL_FreeSurface (orgimg);
		SDL_BlitSurface (tmpimg, NULL, gfx.menu_field[ft], NULL);
		SDL_FreeSurface (tmpimg);
	}
	
	if (background)
		SDL_FreeSurface (background);
}


void
gfx_shutdown ()
{
	int i, j;
	
	for (i = 0; i < 9; i++) {
		SDL_FreeSurface (menuimages[i]);
		if (i < 3)
			for (j = 0; j < 3; j++)	{
				SDL_FreeSurface (menubuttonimages[j][i]);
				if (j < 2) SDL_FreeSurface (menuentryimages[j][i]);
			}
		for (j = 0; j < 2; j++)
			SDL_FreeSurface (menulistimages[j][i]);
	}

	for (i = 0; i < gfx.player_gfx_count; i++) {
		if (gfx.players[i].small_image != NULL) {
			SDL_FreeSurface (gfx.players[i].small_image);
			gfx.players[i].small_image = NULL;
		}
		if (gfx.players[i].menu_image != NULL) {
			SDL_FreeSurface (gfx.players[i].menu_image);
			gfx.players[i].menu_image = NULL;
		}
	}
	
	if (gfx.ghost != NULL)
		SDL_FreeSurface (gfx.ghost);
	if (gfx.ghost_small != NULL)
		SDL_FreeSurface (gfx.ghost_small);
	
	for (i = 0; i < FT_max; i++) if (gfx.menu_field[i] != NULL) {
		SDL_FreeSurface (gfx.menu_field[i]);
		gfx.menu_field[i] = NULL;
	}
	
    SDL_FreeSurface (gfx.logo);
    SDL_FreeSurface (gfx.menuselect.image);
    gfx.screen = SDL_SetVideoMode (gfx.res.x, gfx.res.y, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
    SDL_FreeSurface (gfx.screen);

	font_free();
};


void
draw_logo ()
{
	SDL_Rect dest;
	
	
	dest.x = dest.y = 0;
	dest.w = gfx.res.x;
	dest.h = gfx.res.y;
	
	SDL_BlitSurface (gfx.logo, NULL, gfx.screen, NULL);
	gfx_blitupdaterectadd (&dest);
};


void
redraw_logo_shaded (int x, int y, int w, int h, int c)
{
    SDL_Rect dest;
    dest.w = w;
    dest.h = h;
    dest.x = x;
    dest.y = y;
    redraw_logo (x, y, w + 1, h + 1);
    if (gfx_locksurface (gfx.screen))
        return;
    dest.h += dest.y;
    dest.w += dest.x;
    draw_shadefield (gfx.screen, &dest, c);
    gfx_unlocksurface (gfx.screen);
};


void
redraw_logo (int x, int y, int w, int h)
{
    SDL_Rect src,
      dest;
    dest.w = src.w = w;
    dest.h = src.h = h;
    dest.x = x;
    dest.y = y;
    src.x = x;
    src.y = y;
    SDL_BlitSurface (gfx.logo, &src, gfx.screen, &dest);
	gfx_blitupdaterectadd (&dest);
};



void
shade_pixel (SDL_Surface * s, int x, int y, int c)
{
    Uint32 p;
    Uint8 r,
      g,
      b;
    p = getpixel (s, x, y);
    SDL_GetRGB (p, s->format, &r, &g, &b);
    if (c > 0) {
        if (((Sint16) r) + c < 256) {
            r += c;
        }
        else
            r = 255;
        if (((Sint16) g) + c < 256) {
            g += c;
        }
        else
            g = 255;
        if (((Sint16) b) + c < 256) {
            b += c;
        }
        else
            b = 255;
    }

    else {
        if (((Sint16) r) + c > 0) {
            r += c;
        }
        else
            r = 0;
        if (((Sint16) g) + c > 0) {
            g += c;
        }
        else
            g = 0;
        if (((Sint16) b) + c > 0) {
            b += c;
        }
        else
            b = 0;
    }
    p = SDL_MapRGB (s->format, r, g, b);
    putpixel (s, x, y, p);
};


int
gfx_locksurface (SDL_Surface * surface)
{
    if (SDL_MUSTLOCK (surface))
        if (SDL_LockSurface (surface) < 0) {
            fprintf (stderr, "Can't lock screen: %s\n", SDL_GetError ());
            return -1;
        }
    return 0;
};



void
gfx_unlocksurface (SDL_Surface * surface)
{
    if (SDL_MUSTLOCK (surface)) {
        SDL_UnlockSurface (surface);
    }
};


void
draw_shadefield (SDL_Surface * s, SDL_Rect * rec, int c)
{
    int x1,
      x,
      x2,
      y1,
      y;
    if (rec->x > rec->w) {
        x1 = rec->w;
        x2 = rec->x;
    }

    else {
        x2 = rec->w;
        x1 = rec->x;
    }
    if (rec->y > rec->h) {
        y = rec->h;
        y1 = rec->y;
    }

    else {
        y1 = rec->h;
        y = rec->y;
    }
    for (; y <= y1; y++)
        for (x = x1; x <= x2; x++)
            shade_pixel (s, x, y, c);
};
