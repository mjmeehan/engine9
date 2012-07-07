/* $Id: menuimages.c,v 1.3 2004-09-26 02:28:06 stpohle Exp $
 * Menuhandling: labels */

#include "basic.h"
#include "bomberclone.h"
#include "menu.h"
#include "menugui.h"


/* draw the menuitem label 
 * menuitem->pos.[x|y] - Position inside the menu
 *           pos.w     - Fontsize
 *           label     - Text of the label
 */
void menu_draw_image (_menuitem *mi) {
	SDL_Rect src, dest;

	_menu *menu = (_menu *) mi->menu;
	if (mi->type != MENU_image)
		return;

	if (mi->pos.x == -1)
		dest.x = (menu->oldscreenpos.w - 2*menuimages[0]->w - mi->rect.w) / 2;
	else
		dest.x = mi->pos.x;
	if (mi->pos.y == -1)
		dest.y =  (menu->oldscreenpos.h - 2*menuimages[0]->h - mi->rect.h) / 2;
	else
		dest.y = mi->pos.y;
	
	src.w = dest.w = mi->rect.w;
	src.h = dest.h = mi->rect.h;
	src.x = mi->rect.x;
	src.y = mi->rect.y;
	dest.x += menu->oldscreenpos.x + menuimages[0]->w;
	dest.y += menu->oldscreenpos.y + menuimages[0]->h;
	
	gfx_blit ((SDL_Surface *) mi->ptrdata, &src, gfx.screen, &dest, 10000+mi->pos.w);
};


_menuitem *menu_create_image (_menu *menu, char *name, int x, int y, int layer, SDL_Surface *img, SDL_Rect *rect) {
	_menuitem *menuitems = menuitem_findfree (menu);
	if (menuitems == NULL) return NULL;

	menuitems->pos.x = x;
	menuitems->pos.y = y;
	menuitems->pos.w = layer;
	menuitems->type = MENU_image;
	menuitems->ptrdata = (char *) img;
	
	if (rect == NULL) {
		menuitems->rect.x = 0;
		menuitems->rect.y = 0;
		menuitems->rect.w = img->w;
		menuitems->rect.h = img->h;
	} else
		menuitems->rect = *rect;

	strncpy (menuitems->label, name, MENU_TITLELEN);

	return menuitems;
};
