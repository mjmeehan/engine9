/* 
 * chat.c - this file will do everything what have to do with the chat.. 
 */

#include "bomberclone.h"
#include "network.h"
#include "packets.h"
#include "gfx.h"
#include "keybinput.h"
#include "chat.h"

_chat chat;

static void chat_drawlines ();
static void chat_drawinput ();

/* find a free line or delete the oldest one */
int
chat_findfreeline ()
{
    int i;
    i = chat.curline;

    if (i >= CHAT_MAX_LINES-1) {
        memcpy (&chat.lines[0], &chat.lines[1], sizeof (chat.lines[1]) * (CHAT_MAX_LINES - 1));
        i = CHAT_MAX_LINES - 2;
    }
    else
        chat.curline++;

    chat.changed = 1;

    return i;
}


/*
 * add a new line to the chat, if a line is bigger as the X Space, 
 * split the line in more
 */
void
chat_addline (char *text, int color)
{
	char *pos = text;
    int l, i;
	int maxlen = chat.window.w/font[0].size.x;
	if (maxlen > KEYBI_LINE_LEN-1 || maxlen <= 0) maxlen = KEYBI_LINE_LEN;
	
	while (pos != NULL) {
		l = chat_findfreeline ();
		if (color == -1)
			chat.lines[l].color = CHAT_TEXTCOLOR;
		else
			chat.lines[l].color = color;
		chat.lines[l].end = 1;
		strncpy (chat.lines[l].text, pos, maxlen);
		if ((i = strlen (pos)) > maxlen) {
			pos = pos + maxlen;
			chat.lines[l].end = 0;
			chat.lines[l].text[maxlen] = 0;
		}
		else pos = NULL;
	}
		
    chat.changed = 1;
}


/*
 * draw the empty chat box
 */
void
chat_drawbox ()
{
    SDL_Rect src;
    int i;

    if (gfx_locksurface (gfx.screen))
        return;

	src = chat.window;
	SDL_BlitSurface (chat.oldscreen, NULL, gfx.screen, &src);
	
    for (i = 0; i < 2; i++) {
        src.x = chat.window.x + i;
        src.w = src.x + chat.window.w - 2;
        src.y = chat.window.y + i;
        src.h = src.y + chat.window.h - 2;
		if (chat.active) draw_shadefield (gfx.screen, &src, CHAT_BG_SHADE_BRIGHT);
			else draw_shadefield (gfx.screen, &src, CHAT_BG_SHADE_DARK >> 2);
    }
	
	src = chat.window;
	src.x += 2;
	src.y += 2;
	src.h -= 4;
	src.w -= 4;
	SDL_BlitSurface (chat.oldscreen, NULL, gfx.screen, &src);

    src.x = chat.window.x + 2;
    src.y = chat.window.y + 2;
    src.w = src.x + chat.window.w - 4;
    src.h = src.y + chat.window.h - 4;
    draw_shadefield (gfx.screen, &src, CHAT_BG_SHADE_DARK);
	
    gfx_unlocksurface (gfx.screen);
};



/*
 * Draw the chatscreen
 */
void
chat_draw ()
{
    if (chat.oldscreen != NULL) {
        chat_drawbox ();
        chat_drawlines ();
        chat_drawinput ();
    }

    chat.changed = 0;
	chat.input.changed = 0;
};


/*
 * Draw all the textlines
 */
void
chat_drawlines ()
{
	int vislines = (chat.window.h - 20) / font[0].size.y; // number of  visible lines
	int i = chat.curline - vislines;
	int nr;

	if (i < 0) i = 0;
	
	for (nr = 0; i <= chat.curline; i++, nr++)
		font_gfxdraw (chat.window.x + 4, chat.window.y + 4 + font[0].size.y * nr, chat.lines[i].text, 0, chat.lines[i].color, 0x1000);
};


/*
 * draw the input field
 */
void
chat_drawinput ()
{
    SDL_Rect src, dest;
	int maxlen = chat.window.x / font[0].size.x;
	int start;
	if (maxlen > KEYBI_LINE_LEN-1 || maxlen <= 0) maxlen = KEYBI_LINE_LEN;
	
    dest.x = chat.window.x + 2;
    dest.y = chat.window.y + chat.window.h - (font[0].size.y + 2);
    dest.w = chat.window.w - 4;
    dest.h = font[0].size.y;
	
	src.x = 2;
	src.y = chat.window.h - (font[0].size.y + 2);
	src.w = chat.window.w - 4;
	src.h = font[0].size.y;
	SDL_BlitSurface (chat.oldscreen, &src, gfx.screen, &dest);

    src.x = chat.window.x + 2;
    src.y = chat.window.y + chat.window.h - (font[0].size.y + 2);
    src.w = chat.window.x + chat.window.w - 4;
    src.h = chat.window.y + chat.window.h - 4;
	
	if (chat.active)
		draw_shadefield (gfx.screen, &src, CHAT_BG_SHADE_BRIGHT);
	else
		draw_shadefield (gfx.screen, &src, CHAT_BG_SHADE_DARK);
    gfx_blitupdaterectadd (&chat.window);

	start = strlen (chat.input.text) - maxlen;
	if (start < 0) start = 0;
	
	font_gfxdraw (chat.window.x + 2, (chat.window.y + chat.window.h) - (2 + font[0].size.y), chat.input.text, 0, COLOR_black, 0x1000);
		
    chat.input.changed = 0;
}

/*
 * set a new position for the chat window,
 * or delete the chat windows if it's out of range
 */
void
chat_show (int x, int y, int w, int h)
{
    /* restore the old screen */
    if (chat.oldscreen != NULL) {
		gfx_blitupdaterectadd (&chat.window);
        SDL_BlitSurface (chat.oldscreen, NULL, gfx.screen, &chat.window);
        SDL_FreeSurface (chat.oldscreen);
		chat.oldscreen = NULL;
    }
	
    /* 1) save the old screen 
     * 2) draw chatbox
     */
    if (x >= 0 && x < gfx.res.x && y >= 0 && y < gfx.res.y && w > 0 && h > 0) {
		chat.window.x = x;
		chat.window.y = y;
		chat.window.w = w;
		chat.window.h = h;
		
        chat.oldscreen = gfx_copyscreen (&chat.window);
		
        chat_draw ();
		
		gfx_blitupdaterectadd (&chat.window);
    }
};



/*
 * loop through the chat and draw it
 */
void
chat_loop (SDL_Event * event)
{
    int i;
    char text[255];

    if (chat.active) {          /* the chat mode is active */
        if (event != NULL)
            i = keybinput_loop (&chat.input, event);
        else {
            i = 0;
            chat.input.changed = 1;
        }

        if (i == 1 && chat.input.text[0] != 0) {
            sprintf (text, "%s: %s", bman.playername, chat.input.text);
            net_send_chat (text, 1);
            chat_addline (text, CHAT_TEXTCOLOR);
            keybinput_new (&chat.input, KEYBI_text, 255);
            i = 0;
            if (!chat.keepactive)
                chat.active = 0;
            chat.changed = 1;
        }
    }
    else
        i = 0;

    /*
     * check if we have to redraw the input line
     */
    if (chat.changed)
        chat_draw ();

    if (chat.input.changed)
        chat_drawinput ();
};


/*
 * activeate the chat and set up that the chat keeps 
 * active after we have pressed the Return Key
 */
void
chat_setactive (int active, int keepactive)
{
    chat.active = active;
	chat.changed = 1;
    chat_draw ();
    chat.keepactive = keepactive;
};
