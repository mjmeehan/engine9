/* $Id: tileset.c,v 1.16 2004-09-26 02:28:07 stpohle Exp $ */
/* load and select tilesets */

#include "bomberclone.h"

extern int UpdateRects_nr;

const char *ft_filenames[] = { 
	"background",
	"stone",
	"block",
	"tunnel" ,
	"pwdeath",
	"pwfire",
	"pwbomb",
	"pwshoe",
	NULL,
	"sptrigger",
	"sprow",
	"sppush",
	"spmoved",
	"spliquid",
	"spkick"
};


/* load a random tileset */
void
tileset_random ()
{
    _direntry *destart,
     *de,
     *desel;
    char path[LEN_PATHFILENAME];
    int max,
      sel;

    sprintf (path, "%s/tileset", bman.datapath);
    desel = destart = s_getdir (path);

    for (max = 0, de = destart; de != NULL; de = de->next)
        if (de->name[0] != '.' && (de->flags & DF_dir) == DF_dir)
            max++;

    sel = s_random (max);
    d_printf ("Random Tileset %d of %d selected\n", sel, max);

    for (max = 0, de = destart; max <= sel && de != NULL; de = de->next)
        if (de->name[0] != '.' && (de->flags & DF_dir) == DF_dir) {
            desel = de;
            max++;
        }
    d_printf ("               %s\n", desel->name);

    if (desel != NULL)
        strncpy (map.tileset, desel->name, LEN_TILESETNAME);
    map.tileset[LEN_TILESETNAME - 1] = 0;
}


/* load the tileset or if not present the files from the default folder
 * if dx or dy is set to -1 test for best tileset resolution */
void
tileset_load (char *tilesetname, int dx, int dy)
{
    int i,
      r,
      g,
      b;
    char fullname[LEN_PATHFILENAME];
    char filename[LEN_FILENAME];
    char tileset[LEN_TILESETNAME];
    SDL_Surface *tmpimage,
     *tmpimage1;
    float sfkt;

    d_printf ("Loading Tileset: %s\n", tilesetname);
    strncpy (tileset, tilesetname, LEN_TILESETNAME);

    /* set the block size to dx and dy, if one of both is -1
     * Calculate the Best Size of the Images */
    if (dx <= 0 || dy <= 0) {
        gfx.block.x = gfx.res.x / (map.size.x + 1);
        if (GT_MP && gfx.res.y == 480)
            gfx.block.y = (gfx.res.y - 120) / (map.size.y + 1);
        else if (GT_MP && gfx.res.y == 600)
            gfx.block.y = (gfx.res.y - 140) / (map.size.y + 1);
        else if (GT_MP && gfx.res.y > 600)
            gfx.block.y = (gfx.res.y - 160) / (map.size.y + 1);
        else
            gfx.block.y = (gfx.res.y - 48) / (map.size.y + 1);
        if (gfx.block.x < gfx.block.y)
            gfx.block.y = gfx.block.x;
        else
            gfx.block.x = gfx.block.y;
    }
    else {
        gfx.block.x = dx;
        gfx.block.y = dy;
    }

    /* create Table of points */
    scale (gfx.postab, gfx.block.x, 256);
    sfkt = ((float) gfx.block.x) / ((float) GFX_IMGSIZE);

    /* calculating the best offset for the field on the screen */
    gfx.offset.x = (gfx.res.x - (gfx.block.x * map.size.x)) / 2;
    gfx.offset.y = gfx.res.y - (gfx.block.y * map.size.y);

    /* load the fire */
    sprintf (fullname, "%s/tileset/%s/fire.png", bman.datapath, tileset);
    tmpimage = IMG_Load (fullname);
    if (tmpimage == NULL) {
        /* file could not be load, so load teh default tileset */
        sprintf (fullname, "%s/tileset/default/fire.png", bman.datapath);
        tmpimage = IMG_Load (fullname);
        if (tmpimage == NULL) {
            printf ("default tileset could not be loaded. [%s]\n", fullname);
            exit (1);
        }
    }
    gfx.fire.frames = tmpimage->h / GFX_IMGSIZE;
    tmpimage1 =
        scale_image (tmpimage, (tmpimage->w / GFX_IMGSIZE) * gfx.block.x,
                     gfx.fire.frames * gfx.block.y);
    getRGBpixel (tmpimage1, 0, 0, &r, &g, &b);
    SDL_SetColorKey (tmpimage1, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimage1->format, r, g, b));
    gfx.fire.image = SDL_DisplayFormat (tmpimage1);
    SDL_FreeSurface (tmpimage);
    SDL_FreeSurface (tmpimage1);

    /* load the bomb */
    sprintf (fullname, "%s/tileset/%s/bomb.png", bman.datapath, tileset);
    tmpimage = IMG_Load (fullname);
    if (tmpimage == NULL) {
        /* file could not be load, so load teh default tileset */
        sprintf (fullname, "%s/tileset/default/bomb.png", bman.datapath);
        tmpimage = IMG_Load (fullname);
        if (tmpimage == NULL) {
            printf ("default tileset could not be loaded. [%s]\n", fullname);
            exit (1);
        }
    }
    gfx.bomb.frames = tmpimage->h / GFX_IMGSIZE;
    tmpimage1 =
        scale_image (tmpimage, (tmpimage->w / GFX_IMGSIZE) * gfx.block.x,
                     gfx.bomb.frames * gfx.block.y);
    getRGBpixel (tmpimage1, 0, 0, &r, &g, &b);
    SDL_SetColorKey (tmpimage1, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimage1->format, r, g, b));
    gfx.bomb.image = SDL_DisplayFormat (tmpimage1);
    SDL_FreeSurface (tmpimage);
    SDL_FreeSurface (tmpimage1);

    /* load the powerup's image */
    for (i = 0; i < PWUP_max; i++) {
        switch (i) {
        case (PWUP_good):
            sprintf (filename, "powerup.png");
            break;
        case (PWUP_bad):
            sprintf (filename, "powerbad.png");
            break;
        default:
            sprintf (filename, "powersp.png");
            break;
        }

        sprintf (fullname, "%s/tileset/%s/%s", bman.datapath, tileset, filename);
        tmpimage = IMG_Load (fullname);
        if (tmpimage == NULL) {
            /* file could not be load, so load teh default tileset */
            sprintf (fullname, "%s/tileset/default/%s", bman.datapath, filename);
            tmpimage = IMG_Load (fullname);
            if (tmpimage == NULL) {
                printf ("default tileset could not be loaded. [%s]\n", fullname);
                exit (1);
            }
        }
        gfx.powerup[i].frames = tmpimage->h / GFX_IMGSIZE;
        tmpimage1 =
            scale_image (tmpimage, (tmpimage->w / GFX_IMGSIZE) * gfx.block.x,
                         gfx.powerup[i].frames * gfx.block.y);
        SDL_SetColorKey (tmpimage1, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimage1->format, 255, 0, 255));
        gfx.powerup[i].image = SDL_DisplayFormat (tmpimage1);
        SDL_FreeSurface (tmpimage);
        SDL_FreeSurface (tmpimage1);
    }
    /* loading the field images */
    for (i = 0; i < FT_max; i++) {
        if (i != FT_mixed) {
            sprintf (fullname, "%s/tileset/%s/%s.png", bman.datapath, tileset, ft_filenames[i]);
            gfx.field[i].w = GFX_IMGSIZE;
            gfx.field[i].h = GFX_IMGSIZE;
            tmpimage = IMG_Load (fullname);
            if (tmpimage == NULL) {
                sprintf (fullname, "%s/tileset/%s/%s96.png", bman.datapath, tileset, ft_filenames[i]);
                gfx.field[i].h = GFX_IMGBIGSIZE;
                tmpimage = IMG_Load (fullname);
                if (tmpimage == NULL) {
                    sprintf (fullname, "%s/tileset/default/%s.png", bman.datapath, ft_filenames[i]);
                    gfx.field[i].h = GFX_IMGSIZE;
                    tmpimage = IMG_Load (fullname);
                    if (tmpimage == NULL) {
                        printf ("Can't load image: %s\n", SDL_GetError ());
                        exit (1);
                    }
                }
            }
            gfx.field[i].frames = tmpimage->h / gfx.field[i].h;
            gfx.field[i].h =
                (float) ((float) gfx.field[i].h / (float) GFX_IMGSIZE) * (float) gfx.block.y;
            gfx.field[i].w =
                (float) ((float) gfx.field[i].w / (float) GFX_IMGSIZE) * (float) gfx.block.x;
            tmpimage1 =
                scale_image (tmpimage, gfx.field[i].w * (tmpimage->w / GFX_IMGSIZE),
                             gfx.field[i].frames * gfx.field[i].h);
            if (i == FT_nothing || i == FT_block || i == FT_stone)
                r = g = b = 255;
            else
                getRGBpixel (tmpimage1, 0, 0, &r, &g, &b);
            SDL_SetColorKey (tmpimage1, SDL_SRCCOLORKEY, SDL_MapRGB (tmpimage1->format, r, g, b));
            gfx.field[i].image = SDL_DisplayFormat (tmpimage1);
            SDL_FreeSurface (tmpimage1);
            SDL_FreeSurface (tmpimage);
        }
    }
};


void
tileset_free ()
{
    int i;

    for (i = 0; i < FT_max; i++) {
        if (gfx.field[i].image != NULL)
            SDL_FreeSurface (gfx.field[i].image);
        gfx.field[i].image = NULL;
    }
    if (gfx.bomb.image != NULL)
        SDL_FreeSurface (gfx.bomb.image);
    if (gfx.fire.image != NULL)
        SDL_FreeSurface (gfx.fire.image);
    for (i = 0; i < PWUP_max; i++)
        if (gfx.powerup[i].image != NULL)
            SDL_FreeSurface (gfx.powerup[i].image);

    gfx.bomb.image = NULL;
    gfx.fire.image = NULL;
};
