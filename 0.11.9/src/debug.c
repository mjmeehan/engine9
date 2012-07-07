
#include "bomberclone.h"
#include "network.h"
#include "packets.h"

int debug;

void d_gamedetail (char *head) {
	d_playerdetail (head);
	
    d_printf ("p_nr = %d          ", bman.p_nr);
    d_printf ("p_servnr = %d     \n", bman.p_servnr);
    d_printf ("players_nr = %d    ", bman.players_nr);
    d_printf ("players_nr_s = %d \n", bman.players_nr_s);
    d_printf ("gametype = %d      ", bman.gametype);
    d_printf ("state = %d\n", bman.state);
};


void d_printsdlrect (char *text, SDL_Rect *rect) {
	d_printf ("%s  [x=%4d, y=%4d, w=%4d, h=%4d]\n", text, rect->x, rect->y, rect->w, rect->h);	
};

void d_printf (char *fmt,...) {
	va_list args;
	
	if (debug == 0)
		return;
	
	va_start (args, fmt);
	fprintf (stdout, "[%8d] :", timestamp);
	vfprintf (stdout, fmt, args);
	va_end (args);
};


void d_playerstat (char *head) {
	int i, j;
	char text[255];

	d_printf ("---------------> %s nb play: %d\n", head, bman.playnum);

	sprintf (text, "id Name             killed Unknown");
	for (i = 0; i < MAX_PLAYERS; i++)
		if (players[i].gamestats.isaplayer == 1)
			sprintf(text, "%s %02d", text, i);
	sprintf(text, "%s\n", text);
	d_printf (text);

	for (i = 0; i < MAX_PLAYERS; i++){
		if (players[i].gamestats.isaplayer == 1 ) {
			sprintf(text, "%02d %-16s   %2d       %d  ", i, players[i].name, players[i].gamestats.killed, players[i].gamestats.unknown);
			for (j = 0; j < MAX_PLAYERS; j++)
				if (players[j].gamestats.isaplayer == 1 )
					sprintf(text, "%s %2d", text, players[i].gamestats.killedBy[j]);
			sprintf(text, "%s\n", text);
			d_printf (text);
		}
	}
};


// int killed[MAX_PLAYERS];



void d_playerdetail (char *head) {
	int i;

	d_printf ("---------------> %s\n", head);
	d_printf ("Nr Name              GFX Sta Pkt Win kil Team net_flag [Addr]\n");
	for (i = 0; i < MAX_PLAYERS; i++)
	//	if (players[i].gfx_nr != -1 )
			d_printf ("%2d %16s %3d %3d %3d %3d %3d %4d   %3d     %p[%s:%s]\n",i, players[i].name, players[i].gfx_nr, players[i].state, players[i].points, players[i].wins, players[i].nbrKilled, players[i].team_nr, players[i].net.flags, players[i].net.addr.host, &players[i].net.addr, players[i].net.addr.port);
};


void d_teamdetail (char *head) {
	int p;
	char name[MAX_TEAMS][LEN_PLAYERNAME];
	
	d_printf ("---------------> %s\n", head);
	d_printf ("Teams: | %-10s | %-10s | %-10s | %-10s | Players\n", teams[0].name,teams[1].name,teams[2].name,teams[3].name);
	for (p = 0; p < MAX_PLAYERS; p++) {
		if (teams[0].players[p] == NULL) name[0][0] = 0;
		else strncpy (name[0], teams[0].players[p]->name, LEN_PLAYERNAME);
		if (teams[1].players[p] == NULL) name[1][0] = 0;
		else strncpy (name[1], teams[1].players[p]->name, LEN_PLAYERNAME);
		if (teams[2].players[p] == NULL) name[2][0] = 0;
		else strncpy (name[2], teams[2].players[p]->name, LEN_PLAYERNAME);
		if (teams[3].players[p] == NULL) name[3][0] = 0;
		else strncpy (name[3], teams[3].players[p]->name, LEN_PLAYERNAME);
	
		d_printf ("       | %-10s | %-10s | %-10s | %-10s | %-10s Team:%d\n", name[0], name[1], name[2], name[3], players[p].name, players[p].team_nr);
	}
};


void d_bitprint (int bits, int nr) {
	int i;

	for (i = nr-1; i >= 0; i--)
		if ((bits & (1 << i)) == 0)
			printf ("-");
		else 
			printf ("X");
	printf (" ");
};


void d_fatal (char *fmt,...) {
	va_list args;
	
	va_start (args, fmt);
	fprintf (stdout, "FATAL:");
	vfprintf (stdout, fmt, args);
	va_end (args);
}


void debug_ingameinfo() {
	int i;
	char text[2048];
	
	for (i = 0; i < map.size.x; i++)
		stonelist_add (i, map.size.y-1);

	redraw_logo (0, gfx.res.y-font[0].size.y * 2, gfx.res.y, gfx.res.x);
	sprintf (text, "TDiff: %2.3f TFactor: %2.3f", timediff, timefactor);	
	font_gfxdraw (0, gfx.res.y-font[0].size.y, text, 0, 0, (map.size.y*256)+10);
	
	
/*	sprintf (text, "P:");
	for (i = 0; i < MAX_PLAYERS; i++)
		if (PS_IS_playing (players[i].state)) {
			sprintf (text, "%s Pos:%f,%f ", text, players[i].pos.x, players[i].pos.y);
		}
	font_gfxdraw (0, gfx.res.y-font[0].size.y*2, text, 0, 0, (map.size.y*256)+10); */

	sprintf (text, "Pl_nr: %d  TO: %3.2f", bman.players_nr, bman.timeout);	
	font_gfxdraw (100, gfx.res.y-font[0].size.y, text, 0, 0, (map.size.y*256)+10);
};

