/* $Id: menubuttons.c,v 1.2 2004-05-20 16:55:30 stpohle Exp $
 * Menuhandling: buttons */

#include "basic.h"
#include "bomberclone.h"
#include "menu.h"
#include "menugui.h"


/* create a button only in the menudatas.. darf all this only when menu_loop
 * is called */
_menuitem *menu_create_button (_menu *menu, char *name, int x, int y, int w, int id) {
	_menuitem *m = menuitem_findfree (menu);
	if (m == NULL) return NULL;

	m->type = MENU_button;
	m->pos.w = (1 + (int)((w - menubuttonimages[0][0]->w - menubuttonimages[0][2]->w) / menubuttonimages[0][1]->w)) * menubuttonimages[0][1]->w + menubuttonimages[0][0]->w + menubuttonimages[0][2]->w;	
	if (x != -1)
		m->pos.x = x;
	else 
		m->pos.x = (menu->oldscreenpos.w - 2 * menuimages[0]->w - m->pos.w) / 2;
	m->pos.y = y;
	m->state = 0;
	m->id = id;
	strncpy (m->label, name, MENU_TITLELEN);

	return m;
};


/* draw the menuitem button or bool
 * menuitem->pos.[x|y|w] - Position and X-Size inside the menu
 *           label       - Text of the Button/Bool
 */
void menu_draw_button (_menuitem *mi) {
	int px, py, i;
	SDL_Rect dest;

	if (mi->type != MENU_button && mi->type != MENU_bool)
		return;

	dest.x = mi->pos.x;
	dest.y = mi->pos.y;
	dest.w = mi->pos.w;
	dest.h = menubuttonimages[0][0]->h;
	menu_draw_background ((_menu *)mi->menu, &dest);

	/* check the focus of the button */
	if (((_menu *)mi->menu)->focusvis && mi == ((_menu *)mi->menu)->focus)
		mi->state = 1;
	else if (mi->type == MENU_bool && (*((int*) mi->ptrdata)) > 0)
		mi->state = 2; // bool
	else 
		mi->state = 0; // button or bool == FALSE

	// draw the left side of the button
	dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x;
	dest.y = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y;
	dest.w = menubuttonimages[mi->state][0]->w;
	dest.h = menubuttonimages[mi->state][0]->h;
	gfx_blit (menubuttonimages[mi->state][0], NULL, gfx.screen, &dest, 10000);
	// draw the center of the button
	for (i = 0; i < ((mi->pos.w - (menubuttonimages[mi->state][0]->w + menubuttonimages[mi->state][2]->w)) / menubuttonimages[mi->state][1]->w); i++) {
		dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x + menubuttonimages[mi->state][0]->w + (i * menubuttonimages[mi->state][1]->w);
		dest.y = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y;
		dest.w = menubuttonimages[mi->state][1]->w;
		dest.h = menubuttonimages[mi->state][1]->h;
		gfx_blit (menubuttonimages[mi->state][1], NULL, gfx.screen, &dest, 10000);
	}
	// draw the right side of the button
	dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x + mi->pos.w - menubuttonimages[mi->state][2]->w;
	dest.y = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y;
	dest.w = menubuttonimages[mi->state][2]->w;
	dest.h = menubuttonimages[mi->state][2]->h;
	gfx_blit (menubuttonimages[mi->state][2], NULL, gfx.screen, &dest, 10000);

	// calculate the center of the button
	px = (mi->pos.w - (strlen (mi->label) * font[MENU_BUTTON_FONTSIZE].size.x)) / 2 + mi->pos.x;
	py = (menubuttonimages[mi->state][0]->h - font[MENU_BUTTON_FONTSIZE].size.y) / 2 + mi->pos.y;

	if (mi->type == MENU_bool && mi->state == 2) // disabled bool == FALSE
		font_gfxdraw (MENUOFFSET_X(((_menu *)mi->menu)) + px, MENUOFFSET_Y(((_menu *)mi->menu)) + py, mi->label, MENU_BUTTON_FONTSIZE, COLOR_black, 10000);
	else
		font_gfxdraw (MENUOFFSET_X(((_menu *)mi->menu)) + px, MENUOFFSET_Y(((_menu *)mi->menu)) + py, mi->label, MENU_BUTTON_FONTSIZE, COLOR_yellow, 10000);

};


/* handle the event on the button
 * Return: 1 - if the button was pressed (With Enter) */
int menu_event_button (_menuitem *mi, SDL_Event *event) {
	switch (event->type) {
		case (SDL_KEYDOWN): /* key was pressed */
			if (event->key.keysym.sym == SDLK_LEFT || event->key.keysym.sym == SDLK_UP) 
				menu_focus_prev ((_menu *)mi->menu);
			else if (event->key.keysym.sym == SDLK_RIGHT || event->key.keysym.sym == SDLK_DOWN) 
				menu_focus_next ((_menu *)mi->menu);
			else if (event->key.keysym.sym == SDLK_RETURN || event->key.keysym.sym == SDLK_LCTRL || event->key.keysym.sym == SDLK_RCTRL)
				return 1;
			break;
	}
	
	return 0;
};
