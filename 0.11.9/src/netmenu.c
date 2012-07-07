/* network menu */

#include "bomberclone.h"
#include "network.h"
#include "packets.h"
#include "gfx.h"
#include "menu.h"

extern int UpdateRects_nr;


void
netmenu ()
{
    int menuselect = 0;
	_menu *menu;
	
    if (bman.gamename[0] == 0)
        sprintf (bman.gamename, "%s's Game", bman.playername);

    while (menuselect != -1 && bman.state != GS_quit) {
		menu = menu_new ("Multiplayer", 250, 200);
		menu_create_button (menu, "Host a Game", -1, 70, 200, 0);
		menu_create_button (menu, "Join a Game", -1, 110, 200, 1);
		menu_create_button (menu, "Network Options", -1, 150, 200, 2);
        menuselect = menu_loop (menu);
		menu_delete (menu);
		
		game_resetdata ();

        switch (menuselect) {
        case (0):              // Create a Game
            bman.sock = -1;
            host_multiplayer_game ();
            break;
        case (1):              // Join a Game
			bman.servername[0] = 0; // make sure no old server is in here
            join_multiplayer_game ();
            break;
        case (2):              // Options
            network_options ();
            break;
        }
    }
}


void
network_options ()
{
    int menuselect = 0, done = 0, net_ai_typ, i;
    _charlist nrplayerlist[MAX_PLAYERS];
	_charlist *selnrplayer = &nrplayerlist[bman.maxplayer-1];
	_menu *menu;
	
	/* fill in the nrplayerlist */
	for (i = 0; i < MAX_PLAYERS; i++) {
		sprintf (nrplayerlist[i].text, "%d", (i+1));
		if (i < MAX_PLAYERS-1)
			nrplayerlist[i].next = &nrplayerlist[i+1];
		else 
			nrplayerlist[i].next = NULL;
	}
	
    while (menuselect != -1 && !done && bman.state != GS_quit) {
		/* fill in net_ai_typ */
		net_ai_typ = (PF_INET != bman.net_ai_family);
		menu = menu_new ("Network Options", 400, 380);
		menu_create_label (menu, "Max Players", 30, 75, 0, COLOR_brown);
		menu_create_list (menu, "Players", 150, 55, 55, 70, nrplayerlist, &selnrplayer, 1);
		menu_create_entry (menu, "Gamename", -1, 150, 300, &bman.gamename, LEN_GAMENAME, MENU_entrytext, 3);
		menu_create_entry (menu, "UDP Port:", -1, 180, 200, &bman.port, LEN_PORT, MENU_entrytext,4); 
#ifndef _WIN32
		menu_create_bool (menu, "IPV6", 260, 60, 100, &net_ai_typ, 2);
#endif
		menu_create_bool (menu, "Private Game", 20, 220, 150, &bman.passwordenabled, 6);
		menu_create_entry (menu, "Password:", 190, 220, 210, &bman.password, LEN_PASSWORD, MENU_entrytext, 7);
		
		menu_create_bool (menu, "Use OGC", 70, 250, 120, &bman.notifygamemaster, 8);
		menu_create_bool (menu, "Broadcast", 250, 250, 120, &bman.broadcast, 9);
		menu_create_entry (menu, "OpenGameCache:", -1, 280, 350, &bman.ogcserver, LEN_SERVERNAME, MENU_entrytext, 10);
		menu_create_entry (menu, "Local OGC Port", -1, 310, 350, &bman.ogc_port, LEN_PORT, MENU_entrytext, 11);
		menu_create_button (menu, "Ok", -1, 350, 150, 0);
        menuselect = menu_loop (menu);
		bman.maxplayer = (selnrplayer - &nrplayerlist[0]) + 1;		
		menu_delete (menu);
		
		/* fix net_ai_typ */
#ifndef _WIN32   // fix for windows i don't know how to get there ipv6 mode.
		if (net_ai_typ)
			bman.net_ai_family = PF_INET6;
		else 
#endif
			bman.net_ai_family = PF_INET;

        switch (menuselect) {
        case (0):   // all options made Save Config
			done = 1;
			config_write ();
			break;
		}
	}
};


void
multiplayer_firstrun ()
{
    int i, j;
    /* 
       reset some gamedata
     */
    bman.p_nr = -1;
    bman.state = GS_wait;

    for (i = 0; i < MAX_PLAYERS; i++) {
        players[i].net.addr.host[0] = 0;
        players[i].net.addr.port[0] = 0;
        players[i].name[0] = 0;
        players[i].gfx_nr = -1;
        players[i].gfx = NULL;
        players[i].net.timestamp = timestamp;
        players[i].points = 0;
        players[i].nbrKilled = 0;
        players[i].wins = 0;
        players[i].net.pingreq = 20;
        players[i].net.pingack = 22;
        players[i].state = 0;
		players[i].team_nr = -1;
        players[i].gamestats.killed = 0;
        players[i].gamestats.unknown = 0;
        players[i].gamestats.isaplayer = 0;
        for (j = 0; j < MAX_PLAYERS; j++)
            players[i].gamestats.killedBy[j] = 0;
    }

	for (i = 0; i < MAX_TEAMS; i++) {
		teams[i].wins = 0;
		teams[i].points = 0;
		
		for (j= 0; j < MAX_PLAYERS; j++)
			teams[i].players[j] = NULL;
	}
	
    bman.lastwinner = -1;
    bman.players_nr_s = 1;
};


/*
 * We will host a network game
 */
void host_multiplayer_game () {
	bman.firewall = 0;
	multiplayer_firstrun ();

    if (network_init () < 0) {
        d_printf ("network_init () FAILED\n");
        return;
    }

	/* prepare the server */
    strncpy (players[0].name, bman.playername, LEN_PLAYERNAME);
    bman.p_nr = bman.p_servnr = 0;
	team_choose (&players[0]);
    players[0].state = PSF_used;
    if (bman.notifygamemaster)
        send_ogc_update ();   /* send the information that we started an server */
	
	multiplayer_game ();

    if (bman.notifygamemaster)
        send_ogc_update ();   /* send the information that we started an server */

    network_shutdown ();
};


/* the loop for the multiplayer games */
void multiplayer_game () {
	int olddebug = debug;
	
    while (bman.state != GS_startup && bman.state != GS_quit && bman.sock != -1) {
        /* check players and in_pl lists */
        wait_for_players ();

        if (bman.p_nr != -1 && (GS_WAITRUNNING || bman.state == GS_update)) {
            net_new_game ();
            game_start ();
			if (GT_MP_PTPM) {
                bman.state = GS_update;
				net_send_mapinfo ();    // maybe we have to make a break
				net_send_servermode (); // between mapinfo and servermode
			}
            net_transmit_gamedata ();

            if (bman.state == GS_ready || bman.state == GS_running) {
				if (GT_MP_PTPM)
					net_send_servermode ();

                game_loop ();
                if (bman.state == GS_running || bman.state == GS_ready)
                    bman.state = GS_wait;

                bman.lastwinner = -1;
                game_end ();

				if (GT_MP_PTPM) {
					net_send_servermode ();
                	net_send_players ();
				}
            }
			else 
				game_end ();
        }

    }
    // Only for update statistics
    debug = 1;
    d_playerstat("Stats at the end of game");
    debug = olddebug;

};




/* We will join a network game */
void
join_multiplayer_game ()
{
	int old_ai_family;
	
	old_ai_family = bman.net_ai_family;
	
    multiplayer_firstrun ();
    if (network_init () < 0) {
        d_printf ("network_init () FAILED\n");
        return;
    }
	
	net_getserver ();   /* show Serverlist */
	
	/* if the AI Family Changed, restart the whole network */
	if (old_ai_family != bman.net_ai_family) {	
		network_shutdown ();
	    if (network_init () < 0) {
    	    d_printf ("network_init () FAILED\n");
        	return;
    	}
	}		
	
	if (bman.servername[0] != 0) {
		/* join the server, convert to a usable sockaddr first */
    	network_server_port (bman.servername, players[0].net.addr.host,
        	                 LEN_SERVERNAME, players[0].net.addr.port, LEN_PORT);
	
    	d_printf ("Connect To: %s[:%s]\n", players[0].net.addr.host,
        	       players[0].net.addr.port);
		
    	dns_filladdr (players[0].net.addr.host, LEN_SERVERNAME, players[0].net.addr.port,
        	          LEN_PORT, bman.net_ai_family, &players[0].net.addr.sAddr);
    	players[0].net.addr.port[0] = players[0].net.addr.host[0] = 0;
    	dns_filladdr (players[0].net.addr.host, LEN_SERVERNAME, players[0].net.addr.port,
        	          LEN_PORT, bman.net_ai_family, &players[0].net.addr.sAddr);
	
		/* and join the server */
		bman.p_nr = -1;
    	send_joingame (&players[0].net.addr, bman.playername, 0);

		/* go into the normal multiplayer loop */
		multiplayer_game ();
	}

    network_shutdown ();
};


/* Network Help Screen */
void network_helpscreen () {
	_menu *menu; 
	
	menu = menu_new ("Network Help Screen", 400, 380);
	menu_create_label (menu, "F1", 60, 50, 1, COLOR_brown);
	menu_create_label (menu, "Display this Helpscreen", 180, 54, 0, COLOR_brown);

	menu_create_label (menu, "F2", 60, 80, 1, COLOR_brown);
	menu_create_label (menu, "Change/Show Mapsettings", 180, 84, 0, COLOR_brown);

	menu_create_label (menu, "F3", 60, 110, 1, COLOR_brown);
	menu_create_label (menu, "Add one AI Player", 180, 114, 0, COLOR_brown);

	menu_create_label (menu, "Shift-F3", 10, 140, 1, COLOR_brown);
	menu_create_label (menu, "Delete one AI P.", 180, 144, 0, COLOR_brown);

	menu_create_label (menu, "F4", 60, 170, 1, COLOR_brown);
	menu_create_label (menu, "Start the Game", 180, 174, 0, COLOR_brown);
	
	menu_loop (menu);
	menu_delete (menu);
};
