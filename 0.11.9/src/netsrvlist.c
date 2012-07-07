/* $Id: netsrvlist.c,v 1.17 2005-08-07 17:46:21 stpohle Exp $
 * netsrvlist.c - shows a list of possible servers.*/
 
#include "basic.h"
#include "bomberclone.h"
#include "basic.h"
#include "bomberclone.h"
#include "network.h"
#include "menu.h"
#include "ogcache-client.h"
#include "broadcast.h"

#define SRVLST_TIMEREBUILD 2000

_charlist srvlst_text[MAX_SRVLIST];
struct __srvlst_entry {
	char host[LEN_SERVERNAME];
	char port[LEN_PORT];
	char gamename[LEN_GAMENAME];
	char version[LEN_VERSION];
	int maxplayers;
	int curplayers;
	int ai_family;
	int ping;
	char comment[32];
} srvlst_dat[MAX_SRVLIST];
int srvlst_cnt = 0;

/* will build up our srvlst list with 
 * all servers we have in there */
void srvlist_rebuildlist () {
	int ogclst, i, j;
	char txt1[255];
	char txt2[255];
	char txt3[255];
	
	/* delete the whole list */
	for (i = 0; i < MAX_SRVLIST; i++) {
		srvlst_text[i].text[0] = 0;
		srvlst_text[i].next = NULL;
		srvlst_dat[i].host[0] = 0;
		srvlst_dat[i].port[0] = 0;
		srvlst_dat[i].version[0] = 0;
		srvlst_dat[i].gamename[0] = 0;
		srvlst_dat[i].comment[0] = 0;
		srvlst_dat[i].ping = -1;
	}

	srvlst_cnt = 0;
	
	/* add broadcasted list (j == LAN GAME) */
	for (j = 1; j >= 0; j--) for (i = 0; i < BC_MAXENTRYS; i++) {
		if (broadcast_list[i].host[0] != 0 && broadcast_list[i].lan == j) {
			strncpy (srvlst_dat[srvlst_cnt].host, broadcast_list[i].host, LEN_SERVERNAME);
			strncpy (srvlst_dat[srvlst_cnt].port, broadcast_list[i].port, LEN_PORT);
			strncpy (srvlst_dat[srvlst_cnt].version, broadcast_list[i].version, LEN_VERSION);
			strncpy (srvlst_dat[srvlst_cnt].gamename, broadcast_list[i].gamename, LEN_GAMENAME);
			srvlst_dat[srvlst_cnt].ping = broadcast_list[i].ping;
			srvlst_dat[srvlst_cnt].curplayers = broadcast_list[i].curplayers;
			srvlst_dat[srvlst_cnt].maxplayers = broadcast_list[i].maxplayers;
			srvlst_dat[srvlst_cnt].ai_family = bman.net_ai_family;
			if (broadcast_list[i].lan)
				sprintf (srvlst_dat[srvlst_cnt].comment, "LAN");
			else
				sprintf (srvlst_dat[srvlst_cnt].comment, "Inet");
			srvlst_cnt++;
		}
	}

	/* add the OpenGameCache Entrys which are not in the broadcasted list */
	if (bman.notifygamemaster) {
		for (ogclst = 0; (ogclst < MAX_OGC_ENTRYS && srvlst_cnt < MAX_SRVLIST); ogclst++)
			if ((ogc_array[ogclst].serial != -1) && (broadcast_find (ogc_array[ogclst].host, ogc_array[ogclst].port) == -1)) {
				srvlst_dat[srvlst_cnt].host[0] = 0;
				srvlst_dat[srvlst_cnt].port[0] = 0;
				srvlst_dat[srvlst_cnt].version[0] = 0;
				srvlst_dat[srvlst_cnt].gamename[0] = 0;
				srvlst_dat[srvlst_cnt].ping = -2;
				
				srvlst_dat[srvlst_cnt].ai_family = ogc_array[ogclst].ai_family;
				srvlst_dat[srvlst_cnt].curplayers = ogc_array[ogclst].curplayers;
				srvlst_dat[srvlst_cnt].maxplayers = ogc_array[ogclst].maxplayers;
				strncpy (srvlst_dat[srvlst_cnt].host,ogc_array[ogclst].host, LEN_SERVERNAME);
				strncpy (srvlst_dat[srvlst_cnt].port,ogc_array[ogclst].port, LEN_PORT);
				strncpy (srvlst_dat[srvlst_cnt].gamename,ogc_array[ogclst].gamename, LEN_GAMENAME);
				strncpy (srvlst_dat[srvlst_cnt].version,ogc_array[ogclst].version, LEN_VERSION);
				sprintf (srvlst_dat[srvlst_cnt].comment, "INet");
			
				srvlst_cnt++;
			}
	}
	if (srvlst_cnt >= MAX_SRVLIST)
		d_fatal ("srvlist_rebuildlist : srvlst_cnt >= MAX_SRVLIST\n");
	
	/* make the list viewable */
	for (i = 0; i < srvlst_cnt; i++) {
		sprintf (txt3, "%d/%d",srvlst_dat[i].curplayers, srvlst_dat[i].maxplayers);
		
		if (srvlst_dat[i].ping == -2)
			txt2[0] = 0;
		else if (srvlst_dat[i].ping == -1)
			sprintf (txt2, "NoCon");
		else
			sprintf (txt2, "%dms", srvlst_dat[i].ping);
		
		if (srvlst_dat[i].gamename[0] != 0) /* gamename is present */
			sprintf (txt1, "%-15s %-5s %5s  %-11s %-4s", srvlst_dat[i].gamename, txt3, txt2, srvlst_dat[i].version, srvlst_dat[i].comment);
		else {
			sprintf (txt1, "%s:%s", srvlst_dat[i].host, srvlst_dat[i].port);
			sprintf (txt1, "%-20s %5s  %-11s %-4s", txt1,txt2, srvlst_dat[i].version, srvlst_dat[i].comment);
		}
		strncpy (srvlst_text[i].text, txt1, LEN_CHARENTRY);
	}
	if (srvlst_cnt == 0)
		strcpy (srvlst_text[0].text, "No Servers Found");
	charlist_fillarraypointer (srvlst_text, srvlst_cnt);
};



/* show a list of servers you can select */
void net_getserver () {
    int menuselect = 0, entry = 0, newentry = 0, eventstate, done, srvlst_lastrebuild = 0;
	_charlist *sel_entry = &srvlst_text[0];
	_menu *menu;
	_menuitem *srvlst_listmenu;
	_menuitem *srvlst_entry;
    SDL_Event event;
	
	d_printf ("net_getserver\n");

	broadcast_init ();
	if (bman.servername[0] != 0)
		return; // a server has already been selected
	if (bman.notifygamemaster)
		ogc_browsestart ();

	menu = menu_new ("Join a Game", 500, 400);
	srvlst_listmenu = menu_create_list (menu, "Host a Game", -1, 50, 475, 250, srvlst_text, &sel_entry, 1);
	srvlst_entry = menu_create_entry (menu, "IP :", -1, 320, 475, bman.servername, LEN_SERVERNAME+LEN_PORT + 2, MENU_entrytext, 2);	
	menu_create_button (menu, "OK", -1, 350, 150, 0);
	menu_focus_id (menu, 1);

	menu->looprunning = 1;
	menu_draw (menu);
	
    while (menuselect != -1 && bman.state != GS_quit) {
	
		broadcast_loop ();
		if (bman.notifygamemaster)
			ogc_loop ();
	
		if ((timestamp - srvlst_lastrebuild) > SRVLST_TIMEREBUILD) {
			srvlist_rebuildlist ();
			srvlst_lastrebuild = timestamp;
			srvlst_listmenu->changed = 1;
			menu_draw (menu);
		}
		
		eventstate = s_fetchevent (&event);
		gfx_blitdraw ();
	
        done = menu_event_loop (menu, &event, eventstate);
		menuselect = menu->focus->id;
		newentry = sel_entry - &srvlst_text[0];
		
		if ((newentry != entry || (bman.servername[0] == 0 && srvlst_dat[entry].host[0] != 0)) && menu->focus->id != 2) {
			entry = newentry;
        	d_printf ("Selected Entry (%d) %s:%s Game:%s\n", entry, srvlst_dat[entry].host, srvlst_dat[entry].port, srvlst_dat[entry].gamename);
			if (srvlst_dat[entry].host[0] != 0 
				&& srvlst_dat[entry].port[0] != 0 
			    && srvlst_dat[entry].gamename[0] != 0) {  /* test if there was a selection */
				  bman.net_ai_family = srvlst_dat[entry].ai_family;
				  sprintf (bman.servername, "%s:%s", srvlst_dat[entry].host, srvlst_dat[entry].port);
			}
			srvlst_listmenu->changed = 1;
			menu_reload (menu);
			menu_draw (menu);
		}
		
		if (done) {
			switch (menuselect) {
			case (0):				// Ok Join Selected Game
				menuselect = -1;
				break;
	        case (1):              // Join a Game
				entry = newentry;
    	    	d_printf ("Selected Entry (%d) %s:%s Game:%s\n", entry, srvlst_dat[entry].host, srvlst_dat[entry].port, srvlst_dat[entry].gamename);
				if (srvlst_dat[entry].host[0] != 0 
					&& srvlst_dat[entry].port[0] != 0 
			    	&& srvlst_dat[entry].gamename[0] != 0) {  /* test if there was a selection */
					  bman.net_ai_family = srvlst_dat[entry].ai_family;
					  sprintf (bman.servername, "%s:%s", srvlst_dat[entry].host, srvlst_dat[entry].port);
				}
				srvlst_listmenu->changed = 1;
				menu_reload (menu);
				menu_draw (menu);
				menu_focus_id (menu, 0);
    	        break;
	        }
	    }
		if (done == -1) {
			menuselect = -1;
			bman.servername[0] = 0;
		}
		
		s_calctimesync ();
	}

	if (bman.notifygamemaster)
		ogc_browsestop ();
	menu_delete (menu);
};
