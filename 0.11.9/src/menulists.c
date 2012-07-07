/* $Id: menulists.c,v 1.6 2005-03-27 01:31:50 stpohle Exp $
 * Menuhandling: lists */


#include "basic.h"
#include "bomberclone.h"
#include "menu.h"
#include "menugui.h"



/* create a entryimages only in the menudatas.. darf all this only when menu_loop
 * is called */
_menuitem *menu_create_list (_menu *menu, char *name, int x, int y, int w, int h, _charlist *data, _charlist **selected, int id) {
	_menuitem *menuitems = menuitem_findfree (menu);

	if (menuitems == NULL) return NULL;
	
	menuitems->type = MENU_list;
	menuitems->pos.w = (1 + (int)
		((w - menulistimages[0][0]->w - menulistimages[0][2]->w) / menulistimages[0][1]->w))
			* menulistimages[0][1]->w + menulistimages[0][0]->w + menulistimages[0][2]->w;
	menuitems->pos.h = (1 + (int)
		((h - menulistimages[0][0]->h - menulistimages[0][6]->h) / menulistimages[0][3]->h))
			* menulistimages[0][3]->h + menulistimages[0][0]->h + menulistimages[0][6]->h;
	if (x != -1)
		menuitems->pos.x = x;
	else 
		menuitems->pos.x = (menu->oldscreenpos.w - 2 * menuimages[0]->w - menuitems->pos.w) / 2;
	menuitems->pos.y = y;
	menuitems->ptrdata = (char *) selected;
	menuitems->list = data;
	menuitems->id = id;
    menuitems->changed=1;
	strncpy (menuitems->label, name, MENU_TITLELEN);

	return menuitems;
};



/* draw only a part of the background */
void menu_draw_listbackground (_menuitem *mi, SDL_Rect *updaterect) {
	int x,y, dx, dy;
	SDL_Rect dest, cdest, src, csrc, window;
	
	y = 0; // start at the updaterect. start pos
	for (; y <= (mi->pos.h - 2*menulistimages[0][0]->h - 1)/menulistimages[0][4]->h; y++) {
		x = 0; // start at the updaterect. start pos
		for (; x <= (mi->pos.w - 2*menulistimages[0][0]->w - 1)/menulistimages[0][4]->w; x++) {
			dest.x = x * menulistimages[0][4]->w; // start pos
			dest.y = y * menulistimages[0][4]->h;
			
			dx = (1+x) * menulistimages[0][4]->w; // end pos
			if (dx >= (mi->pos.w - 2*menulistimages[0][0]->w))
				dest.w = menulistimages[0][4]->w - (dx - (mi->pos.w - 2*menulistimages[0][0]->w));
			else
				dest.w = menulistimages[0][4]->w;

			dy = (1+y) * menulistimages[0][4]->h;
			if (dy >= (mi->pos.h - 2*menulistimages[0][0]->h))
				dest.h = menulistimages[0][4]->h - (dy - (mi->pos.h - 2*menulistimages[0][0]->h));
			else
				dest.h = menulistimages[0][4]->h;
			
			if (dest.w > 0 || dest.h > 0) {
				dest.x += MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x + menulistimages[0][0]->w;
				dest.y += MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y + menulistimages[0][0]->h;
				src.x = 0; src.y = 0; src.h = dest.h; src.w = dest.w;
				
				if (updaterect == NULL)
					gfx_blit (menulistimages[0][4], &src, gfx.screen, &dest, 10000);	
				else { 
					window = *updaterect;
					window.x += MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x + menulistimages[0][0]->w;
					window.y += MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y + menulistimages[0][0]->h;
					rect_clipping (&src, &dest, &window, &csrc, &cdest);
					if (csrc.w < UINT16_HALF && csrc.h < UINT16_HALF && cdest.w < UINT16_HALF && cdest.h < UINT16_HALF)
						gfx_blit (menulistimages[1][4], &csrc, gfx.screen, &cdest, 10000);
				}
			}
		}
	}
};


/* gets the number of the selected item 
 * returns the elemennumber or -1 if none where selected */
int menu_list_getselected (_menuitem *mi) {
	int res = -1, i;
	_charlist *l;
	
	if (mi == NULL || mi->ptrdata == NULL) return -1;
	
	for (i = 0, l = mi->list; l != NULL && res == -1; l = l->next, i++)
		if (l == *((_charlist **)mi->ptrdata))
			res = i;
	
	return res;	
};


/* gets the number of elements */
int menu_list_getcount (_menuitem *mi) {
	int count = 0;
	_charlist *l;

	for (l = mi->list; l != NULL; l = l->next, count++);
	
	return count;
};


/* select the element, return 0 if it was not working else 1 */
int menu_list_select (_menuitem *mi, int element) {
	_charlist *l = mi->list;
	int i = element;

	/* set the new element if there is any */
	for (; i > 0 && l != NULL; l = l->next, i--);

	// check element
	if (l == NULL || i < 0)
		return 0;
	
	*(_charlist **)mi->ptrdata = l;
	
	mi->changed = 1;
	menu_draw_list (mi);
	
	return 1;
};


/* select the previous element in the list */
void menu_list_select_prev (_menuitem *mi) {
	int sel = menu_list_getselected(mi);
	
	if (!menu_list_select (mi, sel - 1))
		menu_list_select (mi, menu_list_getcount(mi) - 1);
	mi->changed = 1;
};


/* select the next element in the list */
void menu_list_select_next (_menuitem *mi) {
	int sel = menu_list_getselected (mi);
	
	if (!menu_list_select (mi, sel + 1))
		menu_list_select (mi, 0);
	mi->changed = 1;
};



/* this part will draw the textelement in a list */
void menu_draw_listtext (_menuitem *mi) {
	int count = menu_list_getcount (mi);
	int selected = menu_list_getselected (mi);
	int countvis = (mi->pos.h - menulistimages[0][0]->h - menulistimages[0][6]->h) / font[MENU_BUTTON_FONTSIZE].size.y;   // number of visible elements
	_charlist *list;
	int maxx, // max chars in X
		dy,   // current y position
		dx,   // x position, to start drawing
		start;// start with this element
	SDL_Rect wnd; // needed for the selected field to redraw the background
	char text[255];
	
	/* start element */
	if (selected == -1)
		start = 0;
	else {
		if ((start = selected - countvis/2) < 0) 
			start = 0;
		else if ((start + countvis > count) && (count - countvis >= 0))
			start = count - countvis;
	}
	list = &mi->list[start];
	
	/* calculate the max numbers of chars to draw */
	maxx = (mi->pos.w - menulistimages[0][0]->w - menulistimages[0][2]->w) / font[MENU_BUTTON_FONTSIZE].size.x;
	
	/* calculate start point (y) */
	dy = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y + ((mi->pos.h - countvis*font[MENU_BUTTON_FONTSIZE].size.y) / 2);
	
	/* draw the elements */
	for (;countvis > 0 && list != NULL; countvis--, list = list->next) {
		
		/* calculate dx and print only the text which fixs in the list */
		strncpy (text, list->text, maxx);
		text[maxx] = 0;
		dx = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x + ((mi->pos.w - strlen (text)*font[MENU_BUTTON_FONTSIZE].size.x) / 2);
		
		if (mi->ptrdata != NULL && list == *(_charlist **)mi->ptrdata) {
			// this is the selected element
			wnd.x = menulistimages[0][0]->w;
			wnd.y = dy - (MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y + menulistimages[0][0]->h);
			wnd.w = mi->pos.w - menulistimages[0][0]->w - menulistimages[0][2]->w;
			wnd.h = font[MENU_BUTTON_FONTSIZE].size.y;
			menu_draw_listbackground (mi, &wnd);
			font_gfxdraw (dx, dy, text, MENU_BUTTON_FONTSIZE, COLOR_black, 10000);
		}
		else
			font_gfxdraw (dx, dy, text, MENU_BUTTON_FONTSIZE, COLOR_yellow, 10000); 
		
		dy += font[MENU_BUTTON_FONTSIZE].size.y;
	}
};



/* draw the menuitem button or bool
 * menuitem->pos.[x|y|w] - Position and X-Size inside the menu
 *           label       - Text of the Button/Bool
 */
void menu_draw_list (_menuitem *mi) {
	int i, focus;
	SDL_Rect dest;

	if (mi->type != MENU_list)
		return;
	
	dest.x = mi->pos.x;
	dest.y = mi->pos.y;
	dest.w = mi->pos.w;
	dest.h = menulistimages[0][0]->h;
	
	if (mi->changed) 
		menu_draw_background ((((_menu *)mi->menu)),&dest);

	/* check the focus of the button */
	if (((_menu *)mi->menu)->focusvis && mi == ((_menu *)mi->menu)->focus)
		focus = 1;
	else 
		focus = 0;

	// draw the top left and right of the list
	dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x;
	dest.y = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y;
	dest.w = menulistimages[focus][0]->w;
	dest.h = menulistimages[focus][0]->h;
	gfx_blit (menulistimages[focus][0], NULL, gfx.screen, &dest, 10000);
	dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x + mi->pos.w - menulistimages[focus][2]->w;
	gfx_blit (menulistimages[focus][2], NULL, gfx.screen, &dest, 10000);
	// draw the bottom left and right of the list
	dest.y = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y + mi->pos.h - menulistimages[focus][8]->h;
	gfx_blit (menulistimages[focus][8], NULL, gfx.screen, &dest, 10000);
	dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x;
	gfx_blit (menulistimages[focus][6], NULL, gfx.screen, &dest, 10000);
	
	// draw the top and blow center of the list
	for (i = 0; i < ((mi->pos.w - 
					(menulistimages[focus][0]->w + menulistimages[focus][2]->w)) / menulistimages[focus][1]->w); i++) {
		dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x + menulistimages[focus][0]->w + (i * menulistimages[focus][1]->w);
		dest.y = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y;
		dest.w = menulistimages[focus][1]->w;
		dest.h = menulistimages[focus][1]->h;
		gfx_blit (menulistimages[focus][1], NULL, gfx.screen, &dest, 10000);
		dest.y = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y + mi->pos.h - menulistimages[focus][7]->h;
		gfx_blit (menulistimages[focus][7], NULL, gfx.screen, &dest, 10000);
	}

	// draw the left and the right side of the list
	for (i = 0; i < ((mi->pos.h - 
					(menulistimages[focus][0]->h + menulistimages[focus][6]->h)) / menulistimages[focus][3]->h); i++) {
		dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x;
		dest.y = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y + menulistimages[focus][0]->h + (i * menulistimages[focus][3]->h);
		dest.w = menulistimages[focus][3]->w;
		dest.h = menulistimages[focus][3]->h;
		gfx_blit (menulistimages[focus][3], NULL, gfx.screen, &dest, 10000);
		dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x + mi->pos.w - menulistimages[focus][5]->w;
		gfx_blit (menulistimages[focus][5], NULL, gfx.screen, &dest, 10000);
	}

	if (mi->changed) {
		menu_draw_listbackground (mi, NULL);
		menu_draw_listtext (mi);
		mi->changed=0;
	}
};


/* handle the event on the button
 * on ESC - Reload Old Data
 * on lose focus - Save Data
 */
int menu_event_list (_menuitem *mi, SDL_Event *event) {
	switch (event->type) {
		case (SDL_KEYDOWN): /* key was pressed */
			mi->changed=1;
			if (event->key.keysym.sym == SDLK_LEFT) 
				menu_focus_prev ((_menu *)mi->menu);
			else if (event->key.keysym.sym == SDLK_RIGHT) 
				menu_focus_next ((_menu *)mi->menu);
			else if (event->key.keysym.sym == SDLK_UP) 
				menu_list_select_prev (mi);
			else if (event->key.keysym.sym == SDLK_DOWN) 
				menu_list_select_next (mi);
			else if (event->key.keysym.sym == SDLK_RETURN || event->key.keysym.sym == SDLK_RCTRL || event->key.keysym.sym == SDLK_RCTRL)
				return 1;
			break;
		case (SDL_KEYUP):
			keybinput_loop (&mi->keybi, event);
//			menu_draw_entry (mi);
			break;
	}

	return 0;
};
