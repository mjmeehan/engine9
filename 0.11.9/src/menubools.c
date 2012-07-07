/* $Id: menubools.c,v 1.2 2004-05-20 16:55:30 stpohle Exp $
 * Menuhandling: bools */

#include "basic.h"
#include "bomberclone.h"
#include "menu.h"
#include "menugui.h"


/* create a bool only in the menudatas.. darf all this only when menu_loop
 * is called */
_menuitem *menu_create_bool (_menu *menu, char *name, int x, int y, int w, int *data, int id) {
	_menuitem *m = menuitem_findfree (menu);
	if (m == NULL) return NULL;

	
	m->type = MENU_bool;
	m->pos.w = (1 + (int)((w - menubuttonimages[0][0]->w - menubuttonimages[0][2]->w) / menubuttonimages[0][1]->w)) * menubuttonimages[0][1]->w + menubuttonimages[0][0]->w + menubuttonimages[0][2]->w;	
	if (x != -1)
		m->pos.x = x;
	else 
		m->pos.x = (menu->oldscreenpos.w - 2 * menuimages[0]->w - m->pos.w) / 2;
	m->pos.y = y;
	m->state = 0;
	m->id = id;
	m->ptrdata = (char *)data;
	strncpy (m->label, name, MENU_TITLELEN);
	
	return m;
};



/* handle the event on the button
 * Return: 1 - if the button was pressed (With Enter) */
int menu_event_bool (_menuitem *mi, SDL_Event *event) {
	switch (event->type) {
		case (SDL_KEYDOWN): /* key was pressed */
			if (event->key.keysym.sym == SDLK_LEFT || event->key.keysym.sym == SDLK_UP) 
				menu_focus_prev ((_menu *) mi->menu);
			else if (event->key.keysym.sym == SDLK_RIGHT || event->key.keysym.sym == SDLK_DOWN) 
				menu_focus_next ((_menu *) mi->menu);
			else if (event->key.keysym.sym == SDLK_RETURN || event->key.keysym.sym == SDLK_LCTRL || event->key.keysym.sym == SDLK_RCTRL) {
				*(int *)mi->ptrdata = !(*(int *)mi->ptrdata);
				menu_draw_bool (mi);
			}
			break;
	}
	
	return 0;
};
