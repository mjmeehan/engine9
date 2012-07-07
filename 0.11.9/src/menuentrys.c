/* $Id: menuentrys.c,v 1.4 2004-05-20 16:55:30 stpohle Exp $
 * Menuhandling: entry */
 

#include "basic.h"
#include "bomberclone.h"
#include "menu.h"
#include "menugui.h"


/* save the data into the pointer */
void menu_entry_save (_menuitem *mi) {
	int tmp;
	switch (mi->type) {
		case (MENU_entryint16):
			sscanf (mi->keybi.text, "%d", &tmp);
			*(Sint16*)mi->ptrdata = (Sint16) tmp;
			break;
		case (MENU_entryint32):
			sscanf (mi->keybi.text, "%d", (Sint32*)mi->ptrdata);
			break;
		case (MENU_entrytext):
			strncpy (mi->ptrdata, mi->keybi.text, mi->len);
			break;
		case (MENU_entryfloat):
			sscanf (mi->keybi.text, "%f", (float*)mi->ptrdata);
			break;
	}
};


/* restore the data into the pointer */
void menu_entry_restore (_menuitem *mi) {
	switch (mi->type) {
		case (MENU_entryint16):
			sprintf (mi->keybi.text, "%d", *(Sint16*)mi->ptrdata);
			break;
		case (MENU_entryint32):
			sprintf (mi->keybi.text, "%d", *(Sint32*)mi->ptrdata);
			break;
		case (MENU_entrytext):
			sprintf (mi->keybi.text, "%s", (char*)mi->ptrdata);
			break;
		case (MENU_entryfloat):
			sprintf (mi->keybi.text, "%f", *(float*)mi->ptrdata);
			break;
	}
	mi->keybi.curpos = strlen (mi->keybi.text);
};


/* create a entryimages only in the menudatas.. darf all this only when menu_loop
 * is called */
_menuitem *menu_create_entry (_menu *menu, char *name, int x, int y, int w, void *data, int len, int typ, int id) {
	_menuitem *m = menuitem_findfree (menu);
	if (m == NULL) return NULL;
	
	m->type = typ;
	m->pos.w = (1 + (int)((w - (strlen (name) * font[MENU_BUTTON_FONTSIZE].size.x) - 8 - menuentryimages[0][0]->w - menuentryimages[0][2]->w) / menuentryimages[0][1]->w)) * menuentryimages[0][1]->w + menuentryimages[0][0]->w + menuentryimages[0][2]->w + (strlen (name) * font[MENU_BUTTON_FONTSIZE].size.x) + 8;
	
	if (x != -1)
		m->pos.x = x;
	else 
		m->pos.x = (menu->oldscreenpos.w - 2 * menuimages[0]->w - m->pos.w) / 2;
	m->pos.y = y;
	m->len = len;
	m->ptrdata = (char *) data;
	m->id = id;
	strncpy (m->label, name, MENU_TITLELEN);
	
	if (typ == MENU_entrytext)
		keybinput_new (&m->keybi, KEYBI_text, len);
	else if (typ == MENU_entryint16)
		keybinput_new (&m->keybi, KEYBI_int, len);
	else if (typ == MENU_entryint32)
		keybinput_new (&m->keybi, KEYBI_int, len);
	else
		keybinput_new (&m->keybi, KEYBI_float, 10);

	menu_entry_restore (m);

	return m;
};


/* draw the menuitem button or bool
 * menuitem->pos.[x|y|w] - Position and X-Size inside the menu
 *           label       - Text of the Button/Bool
 */
void menu_draw_entry (_menuitem *mi) {
	int px, py, i;
	SDL_Rect dest;

	if (mi->type != MENU_entrytext && mi->type != MENU_entryint16 && mi->type != MENU_entryint32 && mi->type != MENU_entryfloat)
		return;

	dest.x = mi->pos.x;
	dest.y = mi->pos.y;
	dest.w = mi->pos.w;
	dest.h = menuentryimages[0][0]->h;
	menu_draw_background (((_menu *) mi->menu), &dest);

	/* check the focus of the button */
	if (((_menu *) mi->menu)->focusvis && mi == ((_menu *) mi->menu)->focus)
		mi->state = 1;
	else 
		mi->state = 0;

	// draw the left side of the button
	dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x + (strlen (mi->label) * font[MENU_BUTTON_FONTSIZE].size.x) + 8;
	dest.y = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y;
	dest.w = menuentryimages[mi->state][0]->w;
	dest.h = menuentryimages[mi->state][0]->h;
	gfx_blit (menuentryimages[mi->state][0], NULL, gfx.screen, &dest, 10000);
	/* draw the center of the button
	 * checkt first if there is something wrong */
	if (mi->pos.w < ((strlen (mi->label) * font[MENU_BUTTON_FONTSIZE].size.x) + 8) - (menuentryimages[mi->state][0]->w + menuentryimages[mi->state][2]->w))
		d_fatal ("menuentry Error with Element: %s\n", mi->label);
	else 
	for (i = 0; i < ((mi->pos.w - ((strlen (mi->label) * font[MENU_BUTTON_FONTSIZE].size.x) + 8) - (menuentryimages[mi->state][0]->w + menuentryimages[mi->state][2]->w)) / menuentryimages[mi->state][1]->w); i++) {
		dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x + (strlen (mi->label) * font[MENU_BUTTON_FONTSIZE].size.x) + 8 + menuentryimages[mi->state][0]->w + (i * menuentryimages[mi->state][1]->w);
		dest.y = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y;
		dest.w = menuentryimages[mi->state][1]->w;
		dest.h = menuentryimages[mi->state][1]->h;
		gfx_blit (menuentryimages[mi->state][1], NULL, gfx.screen, &dest, 10000);
	}
	// draw the right side of the button
	dest.x = MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x + mi->pos.w - menuentryimages[mi->state][2]->w;
	dest.y = MENUOFFSET_Y(((_menu *)mi->menu)) + mi->pos.y;
	dest.w = menuentryimages[mi->state][2]->w;
	dest.h = menuentryimages[mi->state][2]->h;
	gfx_blit (menuentryimages[mi->state][2], NULL, gfx.screen, &dest, 10000);

	// calculate the center of the button
	px = (strlen (mi->label) * font[MENU_BUTTON_FONTSIZE].size.x) + 8 + (mi->pos.w - (strlen (mi->label) * font[MENU_BUTTON_FONTSIZE].size.x) - 8 - (strlen (mi->keybi.text) * font[MENU_BUTTON_FONTSIZE].size.x)) / 2 + mi->pos.x;
	py = (menuentryimages[mi->state][0]->h - font[MENU_BUTTON_FONTSIZE].size.y) / 2 + mi->pos.y;
	font_gfxdraw (MENUOFFSET_X(((_menu *)mi->menu)) + px, MENUOFFSET_Y(((_menu *)mi->menu)) + py, mi->keybi.text, MENU_BUTTON_FONTSIZE, COLOR_yellow, 10000);
	font_gfxdraw (MENUOFFSET_X(((_menu *)mi->menu)) + mi->pos.x, MENUOFFSET_Y(((_menu *)mi->menu)) + py, mi->label, MENU_BUTTON_FONTSIZE, COLOR_yellow, 10000);
};


/* handle the event on the button
 * on ESC - Reload Old Data
 * on lose focus - Save Data
 */
int menu_event_entry (_menuitem *mi, SDL_Event *event) {
	switch (event->type) {
		case (SDL_KEYDOWN): /* key was pressed */
			if (event->key.keysym.sym == SDLK_UP) 
				menu_focus_prev ((_menu *)mi->menu);
			else if (event->key.keysym.sym == SDLK_DOWN) 
				menu_focus_next ((_menu *)mi->menu);
			else if (event->key.keysym.sym == SDLK_RETURN || event->key.keysym.sym == SDLK_RCTRL || event->key.keysym.sym == SDLK_RCTRL)
				menu_entry_save (mi);
			else if (event->key.keysym.sym == SDLK_ESCAPE)
				menu_entry_restore (mi);
			else {
				keybinput_loop (&mi->keybi, event);
				menu_draw_entry (mi);
			}
			break;
		case (SDL_KEYUP):
			keybinput_loop (&mi->keybi, event);
			menu_draw_entry (mi);
			break;
	}

	return 0;
};
