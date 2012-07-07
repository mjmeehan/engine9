/* $Id: menulabels.c,v 1.8 2006-02-06 21:18:01 stpohle Exp $
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
void menu_draw_label (_menuitem *mi) {
	int dx, dy;
	_menu *menu = (_menu *)mi->menu;
	SDL_Rect rect;
		
	if (mi->type != MENU_label)
		return;
	
	/* redraw background */
	rect.x = mi->pos.x;
	rect.y = mi->pos.y;
	rect.w = font[mi->pos.w].size.x * strlen (mi->label);
	rect.h = font[mi->pos.w].size.y;
	menu_draw_background (menu, &rect);
	

	if (mi->pos.x == -1)
		dx = (menu->oldscreenpos.w - 2*menuimages[0]->w - (strlen (mi->label) * font[mi->pos.w].size.x)) / 2;
	else
		dx = mi->pos.x;
	if (mi->pos.y == -1)
		dy =  (menu->oldscreenpos.h - 2*menuimages[0]->h - font[mi->pos.w].size.y) / 2;
	else
		dy = mi->pos.y;
	
	font_gfxdraw (menu->oldscreenpos.x + menuimages[0]->w + dx, menu->oldscreenpos.y + menuimages[0]->h + dy, mi->label, mi->pos.w, mi->pos.h, 10000);
};


_menuitem *menu_create_label (_menu *menu, char *name, int x, int y, int fontsize, int fontcolor) {
	_menuitem *menuitems = menuitem_findfree (menu);
	if (menuitems == NULL) return NULL;

	menuitems->pos.x = x;
	menuitems->pos.y = y;
	menuitems->pos.w = fontsize;
	menuitems->pos.h = fontcolor;
	menuitems->type = MENU_label;
	strncpy (menuitems->label, name, MENU_TITLELEN);
	
	return menuitems;
};


/*
 * this will wrap a text into more labels,
 * in this function we are not able to return
 * any pointer
 */
void menu_create_text (_menu *menu, char *name, int x, int y, int maxlen, int maxlines, int fontcolor, char *fmt,...) {
	char text[1024];
	char out[1024];
	char **lineptr = malloc (sizeof (char*)* maxlines + 1);
	int linecnt, maxchar, i;
	va_list args;

	/* read the whole text and convert it to a normal char text */
	memset (text, 0, sizeof (text));
	memset (out, 0, sizeof (out));
	va_start (args, fmt);
	vsnprintf (text, 1024, fmt, args);
	va_end (args);

	menu_formattext (text, out, lineptr, &linecnt, &maxchar, maxlen, maxlines);
	for (i = 0; (i <= linecnt && i < maxlines); i++) 
		menu_create_label (menu, lineptr[i], x, y + i * font[0].size.y, 0, fontcolor);
};
