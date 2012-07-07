/* $Id: menu.c,v 1.53 2006-02-06 21:18:01 stpohle Exp $
 * Menuhandling */

#include "basic.h"
#include "bomberclone.h"
#include "menu.h"
#include "menugui.h"
#include "network.h"
#include "ogcache-client.h"

#define MENU_MESSAGES_MAXLINES 5
#define MENU_MESSAGES_MAXLINELEN 50


SDL_Surface *menuimages[9];     // holds the gfx
SDL_Surface *menulistimages[2][9]; // holds the gfx for the lists
SDL_Surface *menubuttonimages[3][3]; // holds the images for the buttons 
SDL_Surface *menuentryimages[2][3]; // [PRESSED][Left|Center|Right]

/* delete all informations and create a totally new menuscreen */
_menu *
menu_new (char *title, int x, int y)
{
    _menu *menu = malloc (sizeof (_menu));
    int i;

    strncpy (menu->title, title, MENU_TITLELEN);
    menu->items = NULL;
    menu->focus = NULL;
    for (i = 0; i < MENU_MAXENTRYS; i++) {
        menu->menuitems[i].id = -1;
        menu->menuitems[i].type = -1;
        menu->menuitems[i].next = NULL;
        menu->menuitems[i].menu = menu;
    }

    /* save the old background screen */
    x = (1 + (int) (x / menuimages[1]->w)) * menuimages[1]->w;
    y = (1 + (int) (y / menuimages[3]->h)) * menuimages[3]->h;
    menu->oldscreenpos.x = ((gfx.res.x - (x + 2 * menuimages[0]->w)) / 2);
    menu->oldscreenpos.y = ((gfx.res.y - (y + 2 * menuimages[0]->h)) / 2);
    menu->oldscreenpos.w = x + 2 * menuimages[0]->w;
    menu->oldscreenpos.h = y + 2 * menuimages[0]->h;
    menu->oldscreen = gfx_copyscreen (&menu->oldscreenpos);
    menu->focus = NULL;
    menu->looprunning = 0;
	menu->focusto = MENU_FOCUSVIS_BLINKTO;

	menu->oldkey = 1;
	
    return menu;
};


/* restore the screen and reset all needed informations, free the old screen */
void
menu_delete (_menu * menu)
{

	gfx_blitdraw ();   // to make sure nothing is left in the blitbuffer
    gfx_restorescreen (menu->oldscreen, &menu->oldscreenpos);
    gfx_blitdraw ();
    SDL_FreeSurface (menu->oldscreen);

    free (menu);

    if (GS_RUNNING)
        draw_field ();
};


/* draw only a part of the Screen */
void
menu_draw_background (_menu * menu, SDL_Rect * updaterect)
{
    int x,
      y,
      dx,
      dy;
    SDL_Rect dest,
      cdest,
      src,
      csrc,
      window;

    y = 0;                      // start at the updaterect. start pos
    for (; y <= (menu->oldscreenpos.h - 2 * menuimages[0]->h - 1) / menuimages[4]->h; y++) {
        x = 0;                  // start at the updaterect. start pos
        for (; x <= (menu->oldscreenpos.w - 2 * menuimages[0]->w - 1) / menuimages[4]->w; x++) {
            dest.x = x * menuimages[4]->w; // start pos
            dest.y = y * menuimages[4]->h;

            dx = (1 + x) * menuimages[4]->w; // end pos
            if (dx >= (menu->oldscreenpos.w - 2 * menuimages[0]->w))
                dest.w = menuimages[4]->w - (dx - (menu->oldscreenpos.w - 2 * menuimages[0]->w));
            else
                dest.w = menuimages[4]->w;

            dy = (1 + y) * menuimages[4]->h;
            if (dy >= (menu->oldscreenpos.h - 2 * menuimages[0]->h))
                dest.h = menuimages[4]->h - (dy - (menu->oldscreenpos.h - 2 * menuimages[0]->h));
            else
                dest.h = menuimages[4]->h;

            if (dest.w > 0 || dest.h > 0) {
                dest.x += MENUOFFSET_X (menu);
                dest.y += MENUOFFSET_Y (menu);
                src.x = 0;
                src.y = 0;
                src.h = dest.h;
                src.w = dest.w;

                if (updaterect == NULL)
                    gfx_blit (menuimages[4], &src, gfx.screen, &dest, 10000);
                else {
                    window = *updaterect;
                    window.x += MENUOFFSET_X (menu);
                    window.y += MENUOFFSET_Y (menu);
                    rect_clipping (&src, &dest, &window, &csrc, &cdest);
                    if (csrc.w < UINT16_HALF && csrc.h < UINT16_HALF && cdest.w < UINT16_HALF
                        && cdest.h < UINT16_HALF)
                        gfx_blit (menuimages[4], &csrc, gfx.screen, &cdest, 10000);
                }
            }
        }
    }
};


/* draws the menuborders, this function does not save the background */
void
menu_draw_border (_menu * menu)
{
    SDL_Rect dest;
    int i,
      dx;

    // draw top left
    dest.x = menu->oldscreenpos.x;
    dest.y = menu->oldscreenpos.y;
    dest.w = menuimages[0]->w;
    dest.h = menuimages[0]->h;
    gfx_blit (menuimages[0], NULL, gfx.screen, &dest, 10000);

    // draw top and below
    for (i = 0; i < ((menu->oldscreenpos.w - (2 * menuimages[0]->w)) / menuimages[1]->w); i++) {
        dest.x = menu->oldscreenpos.x + menuimages[0]->w + (i * menuimages[1]->w);
        dest.y = menu->oldscreenpos.y;
        dest.w = menuimages[1]->w;
        dest.h = menuimages[1]->h;
        gfx_blit (menuimages[1], NULL, gfx.screen, &dest, 10000);
        dest.y = menu->oldscreenpos.y + menu->oldscreenpos.h - menuimages[7]->h;
        gfx_blit (menuimages[7], NULL, gfx.screen, &dest, 10000);
    }

    // draw top right
    dest.x = menu->oldscreenpos.x + menu->oldscreenpos.w - menuimages[2]->w;
    dest.y = menu->oldscreenpos.y;
    dest.w = menuimages[2]->w;
    dest.h = menuimages[2]->h;
    gfx_blit (menuimages[2], NULL, gfx.screen, &dest, 10000);

    // draw left and right
    for (i = 0; i < ((menu->oldscreenpos.h - (2 * menuimages[0]->h)) / menuimages[3]->h); i++) {
        dest.x = menu->oldscreenpos.x;
        dest.y = menu->oldscreenpos.y + menuimages[0]->h + menuimages[3]->h * i;
        dest.w = menuimages[1]->w;
        dest.h = menuimages[1]->h;
        gfx_blit (menuimages[3], NULL, gfx.screen, &dest, 10000);
        dest.x = menu->oldscreenpos.x + menu->oldscreenpos.w - menuimages[5]->w;
        gfx_blit (menuimages[5], NULL, gfx.screen, &dest, 10000);
    }

    // draw below left
    dest.x = menu->oldscreenpos.x;
    dest.y = menu->oldscreenpos.y + menu->oldscreenpos.h - menuimages[7]->h;
    dest.w = menuimages[6]->w;
    dest.h = menuimages[6]->h;
    gfx_blit (menuimages[6], NULL, gfx.screen, &dest, 10000);

    // draw below right
    dest.x = menu->oldscreenpos.x + menu->oldscreenpos.w - menuimages[8]->w;
    dest.y = menu->oldscreenpos.y + menu->oldscreenpos.h - menuimages[8]->h;
    dest.w = menuimages[8]->w;
    dest.h = menuimages[8]->h;
    gfx_blit (menuimages[8], NULL, gfx.screen, &dest, 10000);

    menu_draw_background (menu, NULL);
    // draw title
    dx = menu->oldscreenpos.x + (menu->oldscreenpos.w - font[2].size.x * strlen (menu->title)) / 2;
    font_gfxdrawbold (dx, menu->oldscreenpos.y + menuimages[0]->h + 8, menu->title, 2, COLOR_brown,
                      2, 10000);
    font_gfxdraw (dx, menu->oldscreenpos.y + menuimages[0]->h + 8, menu->title, 2, COLOR_yellow,
                  10000);
};


/* draw the menu, even it is only put into the gfx_blitlist. gfx_blitdraw needs
 * to be called before the menu is drawed on the screen */
void
menu_draw (_menu * menu)
{
    _menuitem *m;
    if (!menu->looprunning)
        return;

    menu_draw_border (menu);

    for (m = menu->items; m != NULL; m = m->next) {
        menu_draw_menuitem (m);
	}
};



/* draw an item on the screen */
inline void
menu_draw_menuitem (_menuitem * m)
{
    _menu *menu;

    if (m == NULL)
        return;

	menu = (_menu *) m->menu;
	
    if (!menu->looprunning)
        return;
    switch (m->type) {
    case (MENU_label):
        menu_draw_label (m);
        break;
    case (MENU_button):
        menu_draw_button (m);
        break;
    case (MENU_bool):
        menu_draw_bool (m);
        break;
    case (MENU_entrytext):
    case (MENU_entryint16):
    case (MENU_entryint32):
    case (MENU_entryfloat):
        menu_draw_entry (m);
        break;
    case (MENU_list):
        menu_draw_list (m);
        break;
    case (MENU_image):
        menu_draw_image (m);
        break;
    }
};


/* delete the menuitem from the list, mark it as not used */
void menu_del_menuitem (_menuitem *m) {
	m->type = MENU_none;
	m->id = -1;
	m->changed = 0;
	m->ptrdata = NULL;
	m->list = NULL;
};


/* reload all variables into all menuelements */
void
menu_reload (_menu * menu)
{
    _menuitem *m;

    for (m = menu->items; m != NULL; m = m->next)
        menu_reload_menuitem (m);
};



/* reload variable into menuelement */
inline void
menu_reload_menuitem (_menuitem * m)
{
    switch (m->type) {
    case (MENU_entrytext):
    case (MENU_entryint16):
    case (MENU_entryint32):
    case (MENU_entryfloat):
        menu_entry_restore (m);
        break;
    default:
        break;
    }
};



/* find the last menuitem in the list. */
int
menu_getlastitem (_menuitem * first)
{
    int i = 0;
    _menuitem *result = first;

    if (first == NULL)          // no first item there
        return -1;

    for (; result->next != NULL; result = result->next)
        i++;

    return i;
}


/* get the last and the first id number */
_menuitem *
menu_get_firstid (_menu * menu)
{
    _menuitem *result = NULL,
        *mi = menu->items;
    for (mi = menu->items; mi != NULL; mi = mi->next)
        if ((result == NULL || mi->id < result->id) && mi->id != -1)
            result = mi;
    return result;
};

_menuitem *
menu_get_lastid (_menu * menu)
{
    _menuitem *result = NULL,
        *mi = menu->items;
    for (mi = menu->items; mi != NULL; mi = mi->next)
        if ((result == NULL || mi->id > result->id) && mi->id != -1)
            result = mi;
    return result;
};



/* change the focus to the givin element */
void
menu_change_focus (_menuitem * newfocus)
{
    _menuitem *oldmi;
    _menu *menu = (_menu *) newfocus->menu;

    if (newfocus == menu->focus) // no focus change
        return;

    /* lose focus */
    if (menu->focus != NULL) {
        switch (menu->focus->type) {
        case (MENU_entryfloat):
        case (MENU_entryint16):
        case (MENU_entryint32):
        case (MENU_entrytext):
            menu_entry_lose_focus (menu->focus);
            break;
        }
    }

    /* draw the old and the new element */
    oldmi = menu->focus;
    menu->focus = newfocus;
    if (oldmi != NULL)
        menu_draw_menuitem (oldmi);
    menu_draw_menuitem (menu->focus);

    /* get focus ... no function yet */

    d_printf ("menu_change_focus: ID:%d Name:%s\n", menu->focus->id, menu->focus->label);
};


/* focus next element, order by ID */
void
menu_focus_next (_menu * menu)
{
    _menuitem *newmi = menu->focus,
        *mi,
        *oldmi = menu->focus;

    for (mi = menu->items; mi != NULL; mi = mi->next)
        if (mi->id != oldmi->id && mi->id > menu->focus->id
            && (mi->id < newmi->id || newmi == oldmi))
            newmi = mi;
    if (newmi == oldmi)
        menu_change_focus (menu_get_firstid (menu));
    else
        menu_change_focus (newmi);
};


/* focus previous element, order by ID */
void
menu_focus_prev (_menu * menu)
{
    _menuitem *newmi = menu->focus,
        *mi,
        *oldmi = menu->focus;

    for (mi = menu->items; mi != NULL; mi = mi->next)
        if (mi->id != -1 && mi->id != oldmi->id && mi->id < oldmi->id
            && (mi->id > newmi->id || newmi == oldmi))
            newmi = mi;
    if (newmi == oldmi)
        menu_change_focus (menu_get_lastid (menu));
    else
        menu_change_focus (newmi);
};


/* focus element with id ID */
void
menu_focus_id (_menu * menu, int id)
{
    _menuitem *mi,
     *oldmi = menu->focus;

    for (mi = menu->items; mi != NULL; mi = mi->next)
        if (mi->id == id)
            menu_change_focus (mi);

    menu_draw_menuitem (oldmi);
    if (menu->focus != oldmi)
        menu_draw_menuitem (menu->focus);
};


/*
 * do the evenhandling and forward all events to the menuitems
 * menusingle loop, this function can be called from outside or from there
 * menu_loop () functions.
 *
 * the return value will hole the done value if the menu closed
 * RETURN: -1  - ESCAPE pressed
 *          0  - menu is still running
 *          1  - got SDL_Quit message
 */
int
menu_event_loop (_menu * menu, SDL_Event * event, int eventstate)
{
    Uint8 *keys;
    int done = 0;
	
    if (eventstate >= 1) {
		
		/* make sure no old key is disturbing us */
		if (event->type != SDL_KEYDOWN)
			menu->oldkey = 0;

        switch (event->type) {
        case (SDL_QUIT):
            bman.state = GS_quit;
            done = 1;
            return -1;
            break;
        case (SDL_KEYDOWN):    /* focus next element */
            if (menu->oldkey == 0 && event->key.keysym.sym == SDLK_TAB) {
                keys = SDL_GetKeyState (NULL);
                if (keys[SDLK_LSHIFT] || keys[SDLK_RSHIFT])
                    menu_focus_prev (menu);
                else
                    menu_focus_next (menu);
                break;
            }
            else if (menu->oldkey == 0 && event->key.keysym.sym == SDLK_ESCAPE) {
                return -1;
                break;
            }
        default:               /* push events to the menu items */
            switch (menu->focus->type) {
            case (MENU_button):
                done = menu_event_button (menu->focus, event);
                break;
            case (MENU_bool):
                done = menu_event_bool (menu->focus, event);
                break;
            case (MENU_entrytext):
            case (MENU_entryfloat):
            case (MENU_entryint16):
            case (MENU_entryint32):
                done = menu_event_entry (menu->focus, event);
                break;
            case (MENU_label):
                break;
            case (MENU_list):
                done = menu_event_list (menu->focus, event);
                break;
            }
        }
    }
	else 
		menu->oldkey = 0;

    menu->focusto -= timediff;
    if (menu->focusto <= 0.0f) {
        menu->focusto = MENU_FOCUSVIS_BLINKTO;
        menu->focusvis = !menu->focusvis;
        menu_draw_menuitem (menu->focus);
    }
    return done;
};


/* menu loop, programm will stay in here as long as no ESCAPE is pressed
 * and as long as no Button is clicked.  Return of -2 means something needs to reordered */
int
menu_loop (_menu * menu)
{
    SDL_Event event;
    int done = 0, eventstate = 0, reorder = 0;

    menu->looprunning = 1;

    /* check if the focus is set to something, if not
     * set the focus to the first item */
    if (menu->focus == NULL) {
        menu->focus = menu->items;
        menu_focus_id (menu, 0);
    }
    if (menu->focus == NULL) {
        d_fatal ("menu_loop: focus == NULL, something went wrong\n");
        menu->looprunning = 0;
        return -1;
    }

    timestamp = SDL_GetTicks (); // needed for time sync.

    menu_draw (menu);

    while (!reorder && !done && bman.state != GS_quit) {
        gfx_blitdraw ();

        /* do the network loop if we have to */
        if (bman.sock > 0) {
            network_loop ();
            if (bman.notifygamemaster)
                reorder = ogc_loop ();
            else
                reorder = 0;
        }

        eventstate = s_fetchevent (&event);

	done = menu_event_loop (menu, &event, eventstate);

        s_calctimesync ();
    }

    menu->looprunning = 0;

    if (reorder) {
        menu->focus->changed = 1;
        return -2;
    }
    return (done == -1) ? -1 : menu->focus->id;
};


/* create a list with all directory entrys, 
 * except we can't put everything in the list because the list is too smal.
 * Return: number of entrys, Pointers will be set*/
int
menu_create_dirlist (char *path, signed char dirflags, _charlist * cl, int maxentry)
{
    int cnt;
    _direntry *destart,
     *de;

    destart = s_getdir (path);
    destart = s_dirfilter (destart, dirflags);
    for (cnt = 0, de = destart; (de != NULL && cnt < maxentry); de = de->next) {
        strncpy (cl[cnt].text, de->name, 255);
        if (de->next != NULL)
            cl[cnt].next = &cl[cnt + 1];
        else
            cl[cnt].next = NULL;
        cnt++;
    }

    return cnt;
};


/* displays a file selectionmenu and 
 * returns the name of the file */
static char menu_dir_name[LEN_PATHFILENAME];
char *
menu_dir_select (char *title, char *path, signed char dirflags)
{
    _charlist flist[MAX_DIRENTRYS];
    int flcnt,
      menuselect;
    _charlist *selfile = flist;
    _menu *menu;
	_menuitem *dirmi;

    flcnt = menu_create_dirlist (path, dirflags, flist, MAX_DIRENTRYS);
    menu = menu_new (title, 300, 300);
    dirmi = menu_create_list (menu, "Dir", -1, 50, 200, 200, flist, &selfile, 1);
    menu_create_button (menu, "OK", -1, 270, 150, 0);
	menu_focus_id (menu, 1);
	
    menuselect = menu_loop (menu);

    menu_delete (menu);

    if (menuselect < 0 || selfile - &flist[0] < 0 || selfile - &flist[0] >= flcnt)
        return NULL;

    strncpy (menu_dir_name, selfile->text, LEN_PATHFILENAME);
    return menu_dir_name;
};


/* display a message on the screen and wait untill ESC is pressed */
void
menu_displaymessage (char *title, char *fmt, ...)
{
    va_list args;

    int maxlinelen = 0,         // max y size for the window
        i,
        linenr;
    char out[MENU_MESSAGES_MAXLINES * (MENU_MESSAGES_MAXLINELEN + 1)];
    char *lines[MENU_MESSAGES_MAXLINES + 1]; // textlines for the screen
    char text[512];
    _menu *menu;

    /* read the whole text and convert it to a normal char text */
    memset (text, 0, sizeof (text));
    memset (out, 0, sizeof (out));
    va_start (args, fmt);
    vsnprintf (text, 512, fmt, args);
    va_end (args);

    menu_formattext (text, out, lines, &linenr, &maxlinelen, MENU_MESSAGES_MAXLINELEN,
                     MENU_MESSAGES_MAXLINES);

    i = maxlinelen * font[0].size.x + 32;
    if (i < (strlen (title) * font[2].size.x + 16))
        i = strlen (title) * font[2].size.x + 16;
    menu = menu_new (title, i, linenr * font[0].size.y + 75);

    for (i = 0; (i <= linenr && i < MENU_MESSAGES_MAXLINES); i++)
        menu_create_label (menu, lines[i], -1, 55 + i * font[0].size.y, 0, COLOR_brown);
    menu_loop (menu);
    menu_delete (menu);
};

/* format messages to seperated lines
 * input - input text 
 * out   - outtext all lines will be hold in here seperated my \0
 * start - array of pointer to each line saved in out
 * lines - returns number of lines saved (start must hold one pointer more as in lines defined)
 * maxlinelen - returen the max lenght of chars used in one line 
 * max_chars  - max number of chars in one line
 * max_lines  - max number of lines to use (start must olh one pointer more) */
void
menu_formattext (char *input, char *out, char **start, int *lines, int *maxlinelen, int max_chars,
                 int max_lines)
{
    int i,
      pos,
      outpos;
    char *tmpchar1,
     *tmpchar2;
    *maxlinelen = i = pos = outpos = *lines = 0;
    start[0] = out;

    /* create all lines and do some word wrapping */
    do {
        if (input[i] == ' ') {  // new word check if there is still space left for another word
            /* check what will be found first #13 or space */
            tmpchar1 = strchr (&input[i + 1], ' ');
            tmpchar2 = strchr (&input[i + 1], '\n');
            if (tmpchar2 != NULL && tmpchar2 < tmpchar1)
                tmpchar1 = tmpchar2;
            if (tmpchar1 == NULL)
                tmpchar1 = input + strlen (input);

            if (tmpchar1 - (&input[i] - pos) >= max_chars) { /* new line */
                out[outpos++] = 0;
                start[++(*lines)] = &out[outpos];
                if (pos > *maxlinelen)
                    *maxlinelen = pos;
                pos = 0;
            }
            else                /* add this to the line */
                out[outpos++] = input[i];
            pos++;
        }
        else if (input[i] == '\n') {
            out[outpos++] = 0;
            start[++(*lines)] = &out[outpos];
            if (pos > *maxlinelen)
                *maxlinelen = pos;
            pos = 0;
        }
        else {                  /* copy the text */
            out[outpos++] = input[i];
            pos++;
        }
        i++;
    } while (i < strlen (input) && i < max_lines * (max_chars + 1) && *lines < max_lines);
};


/* display a text on the screen and return */
void
menu_displaytext (char *title, char *fmt, ...)
{
    va_list args;

    int maxlinelen = 0,         // max y size for the window
        i,
        linenr;
    char out[MENU_MESSAGES_MAXLINES * (MENU_MESSAGES_MAXLINELEN + 1)];
    char *lines[MENU_MESSAGES_MAXLINES + 1]; // textlines for the screen
    char text[512];
    _menu *menu;

    /* read the whole text and convert it to a normal char text */
    memset (text, 0, sizeof (text));
    memset (out, 0, sizeof (out));
    va_start (args, fmt);
    vsnprintf (text, 512, fmt, args);
    va_end (args);

    menu_formattext (text, out, lines, &linenr, &maxlinelen, MENU_MESSAGES_MAXLINELEN,
                     MENU_MESSAGES_MAXLINES);

    i = maxlinelen * font[0].size.x + 32;
    if (i < (strlen (title) * font[2].size.x + 16))
        i = strlen (title) * font[2].size.x + 16;
    menu = menu_new (title, i, linenr * font[0].size.y + 75);
    for (i = 0; (i <= linenr && i < MENU_MESSAGES_MAXLINES); i++)
        menu_create_label (menu, lines[i], -1, 55 + i * font[0].size.y, 0, COLOR_brown);
    menu->looprunning = 1;
    menu_draw (menu);
    gfx_blitdraw ();
    SDL_FreeSurface (menu->oldscreen);
    menu->oldscreen = NULL;
    menu->menuitems[0].next = NULL;
    menu->items = NULL;
    menu->looprunning = 0;
    gfx_blitdraw ();
};


/*
 * find the first free item
 */
_menuitem *
menuitem_findfree (_menu * menu)
{
    _menuitem *menuitems = menu->items;

    if (menuitems == NULL) {    /* this is the first element */
        menu->items = &menu->menuitems[0];
        menuitems = menu->items;
        menuitems->type = MENU_none;
        menuitems->next = NULL;
    }
    else {
        /* find first empty element or the last one */
        for (;
             (menuitems->type != MENU_none) && (menuitems->next != NULL)
             && ((menuitems - menu->items) < MENU_MAXENTRYS); menuitems++);
        if (menuitems - menu->items >= MENU_MAXENTRYS) {
            d_fatal ("menu_create_label: MENU_MAXENTRYS reached. Item Ignored\n");
            return NULL;
        }
        else if (menuitems->type != MENU_none) {
            /* letzte Element */
            menuitems->next = menuitems + 1;
            menuitems++;
            menuitems->next = NULL;
            menuitems->type = MENU_none;
        }
    }
    return menuitems;
};
