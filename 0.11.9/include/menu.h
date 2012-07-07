/* $Id: menu.h,v 1.13 2004-09-25 10:57:50 stpohle Exp $
 * GUI for menuhandling
 */

#ifndef _MENU_H_
#define _MENU_H_

#define MENU_TITLELEN 64
#define MENU_ENTRYNAME 64
#define MENU_BG_SHADE_DARK -64
#define MENU_BG_SHADE_BRIGHT 64
#define MENU_DATASIZE 256
#define MENU_MAXENTRYS 64
#define MENU_BUTTON_FONTSIZE 0
#define MENU_FOCUSVIS_BLINKTO 0.25f
#define MENU_DATAENTRYLEN 128
#define MENUOFFSET_X(_m_) _m_->oldscreenpos.x + menuimages[0]->w
#define MENUOFFSET_Y(_m_) _m_->oldscreenpos.y + menuimages[0]->h

enum _menu_type {
	MENU_label = 0,
	MENU_none,			/* deleted item */
	MENU_button,
	MENU_entrytext,
	MENU_entryint32,
	MENU_entryint16,
	MENU_entryfloat,
	MENU_bool,
	MENU_list,
	MENU_image
};


struct __menuitem {
	SDL_Rect pos;
	int type;
	int len;
	int id;
	char changed;
	char label[MENU_TITLELEN];
	_keybinput keybi;
	int state;
	char *ptrdata;			// pointer to some data
	SDL_Rect rect;			// only used for images
	_charlist *list;
	void *menu;				// parent menu
	struct __menuitem *next;
} typedef _menuitem;


struct {
	char title[MENU_TITLELEN];
	_menuitem *items;
	_menuitem *focus;
	SDL_Surface *oldscreen; // hold old screendata
	SDL_Rect oldscreenpos;
	int oldkey;
	float focusto;
	int focusvis;
	_menuitem menuitems[MENU_MAXENTRYS];
	int looprunning;
} typedef _menu;


extern SDL_Surface *menuimages[9]; // holds the gfx
extern SDL_Surface *menulistimages[2][9]; // holds the gfx for the lists
extern SDL_Surface *menubuttonimages[3][3]; // holds the images for the buttons 
extern SDL_Surface *menuentryimages[2][3];  // [PRESSED][Left|Center|Right]

extern SDL_Surface *menu_players[MAX_PLAYERS];	// holds playergfx of a single frame
extern SDL_Surface *menu_stones[FT_max];		// hold a frame of every stone type

extern _menu *menu_new (char *title, int x, int y);
extern void menu_delete (_menu *menu);
extern int menu_getlastitem (_menuitem *first);
extern _menuitem *menuitem_findfree (_menu *menu);
extern _menuitem *menu_create_list (_menu *menu, char *name, int x, int y, int w, int h, _charlist *data, _charlist **selected, int id);
extern _menuitem *menu_create_entry (_menu *menu, char *name, int x, int y, int w, void *data, int len, int typ, int id);
extern _menuitem *menu_create_label (_menu *menu, char *name, int x, int y, int fontsize, int fontcolor);
extern void menu_create_text (_menu *menu, char *name, int x, int y, int maxlen, int maxlines, int fontcolor, char *fmt,...);
extern _menuitem *menu_create_button (_menu *menu, char *name, int x, int y, int w, int id);
extern _menuitem *menu_create_bool (_menu *menu, char *name, int x, int y, int w, int *data, int id);
extern _menuitem *menu_create_image (_menu *menu, char *name, int x, int y, int layer, SDL_Surface *img, SDL_Rect *rect);
extern int menu_loop (_menu *menu);
extern int menu_event_loop (_menu *menu, SDL_Event *event, int eventstate);
extern void menu_draw (_menu *menu);
extern void menu_draw_border (_menu *menu);
extern void menu_draw_background (_menu *menu, SDL_Rect *dest);
extern inline void menu_draw_menuitem (_menuitem *m);
extern void menu_del_menuitem (_menuitem *m);
extern void menu_reload (_menu *menu);
extern inline void menu_reload_menuitem (_menuitem *m);
extern void menu_focus_next (_menu *menu);
extern void menu_focus_prev (_menu *menu);
extern void menu_focus_id (_menu *menu, int id);
extern void menu_change_focus (_menuitem *newfocus);
extern _menuitem *menu_get_lastid (_menu *menu);
extern _menuitem *menu_get_firstid (_menu *menu);
extern int menu_create_dirlist (char *path, signed char dirflags, _charlist *cl, int maxentry);
extern char *menu_dir_select (char *title, char *path, signed char dirflags);

extern void menu_displaymessage (char *title, char *fmt,...);
extern void menu_displaytext (char *title, char *fmt,...);
extern void menu_formattext (char *input, char *out, char **start, int *lines, int *maxlinelen, int max_chars, int max_lines);
extern void menu_drawborder (int x, int y, int w, int h);

#endif
