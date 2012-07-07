/* $Id: menugui.h,v 1.2 2004-02-07 13:35:28 stpohle Exp $
 * Menuhandling: gui elements */

#ifndef _MENUGUI_H_
#define _MENUGUI_H_

extern _menuitem menuitems[MENU_MAXENTRYS];
extern _menu menu;

/* buttons */
extern void menu_draw_button (_menuitem *mi);
extern int menu_event_button (_menuitem *mi, SDL_Event *event);

/* labels */
extern void menu_draw_label (_menuitem *mi);

/* images */
extern void menu_draw_image (_menuitem *mi);

/* bools */
#define menu_draw_bool menu_draw_button
extern int menu_event_bool (_menuitem *mi, SDL_Event *event);

/* entrytext and entryint */
extern void menu_draw_entry (_menuitem *mi);
extern int menu_event_entry (_menuitem *mi, SDL_Event *event);
extern void menu_entry_save (_menuitem *mi);
extern void menu_entry_restore (_menuitem *mi);
#define menu_entry_lose_focus menu_entry_save

extern void menu_draw_list (_menuitem *mi);
extern int menu_event_list (_menuitem *mi, SDL_Event *event);
extern int menu_list_getcount (_menuitem *mi);
extern int menu_list_getselected (_menuitem *mi);
extern void menu_draw_listtext (_menuitem *mi);
extern int menu_list_select (_menuitem *mi, int element);
extern void menu_list_select_prev (_menuitem *mi);
extern void menu_list_select_next (_menuitem *mi);
#endif
