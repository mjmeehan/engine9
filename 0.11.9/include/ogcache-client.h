/* $Id: ogcache-client.h,v 1.2 2005-03-27 01:31:50 stpohle Exp $
 * include file for the opengamesrv.c file
 */

#ifndef _OGCACHE_CLIENT_H
#define _OGCACHE_CLIENT_H

#define MAX_OGC_ENTRYS 255
#define UDP_DEFAULTPORT "11111"
#define LEN_OGCHOST 64
#define LEN_OGCPORT 10
#define LEN_GAME 32

#ifndef LEN_VERSION
	#define LEN_VERSION 20
#endif

#define LEN_STATUS 6
#define LEN_GAMENAME 32
#define BUF_SIZE 1024

struct game_entry {
	int serial;
    char host[LEN_OGCHOST];
    char port[LEN_OGCPORT];
	char game[LEN_GAME];
	char version [LEN_VERSION];
    char gamename[LEN_GAMENAME];
    int curplayers;
    int maxplayers;
    signed char ai_family;
    char status[LEN_STATUS];
};

extern struct game_entry ogc_array[MAX_OGC_ENTRYS];
extern int ogc_browsing;

int ogc_init (char *localport, char *server, char *port, char *game, int ai_family);
void ogc_shutdown ();
int ogc_loop ();
int ogc_sendgamestatus (int sock, char *game, char *version, char *gamename,
						int curplayers, int maxplayers, char *status);
int ogc_sendgamequit (int sock); /* send that the game quitted */
void ogc_browsestart ();
void ogc_browsestop ();

#endif
