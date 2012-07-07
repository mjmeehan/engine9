/* $Id: bomberclone.h,v 1.37 2009-05-11 20:51:25 stpohle Exp $ */
/* bomberclone.h */

#ifndef _BOMBERCLONE_H_
#define _BOMBERCLONE_H_

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <math.h>
#ifdef _WIN32
	#include <windows.h>
	#include <winsock.h>
	#include <sys/stat.h>
	#ifndef S_ISDIR
		#define S_ISDIR(a) ((a & _S_IFDIR) == _S_IFDIR)
	#endif
	#ifndef S_ISREG
		#define S_ISREG(a) ((a & _S_IFREG) == _S_IFREG)
	#endif
#else
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <dirent.h>
#endif
#include <SDL.h>
#include <SDL_image.h>

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "basic.h"
#include "map.h"
#include "gfx.h"
#include "font.h"
#include "sound.h"
#include "network.h"
#include "sysfunc.h"
#include "keybinput.h"

struct {
	char name[255];
} typedef _serverlist;


struct {
    char datapath[512];
	
    int p_nr;                   // playernumber of the first local player
	int p2_nr;					// playernumber of the second local player
	int p_servnr;				// Playernumber of the Server
    int last_ex_nr;             // number of the last explosion
	int updatestatusbar;		// 1 if statusbar has to be updated
    unsigned char state;
	int init_timeout;			// gametimeout init value
	float timeout;				// game timeout
    char playername[LEN_PLAYERNAME];
    char player2name[LEN_PLAYERNAME];
    int players_nr_s;           // number of players at the beginning
    int players_nr;             // number of player who are alife
	signed char lastwinner;		// number of the last winnet

    int maxplayer;              // number of max players for the server
	int playnum;				// Number of play
    int sock;                   // the server socket
    int net_ai_family;
    char port[LEN_PORT];        // what port we're using
    char servername[LEN_SERVERNAME + LEN_PORT + 2]; // holds the name of the current server
    char gamename[LEN_GAMENAME]; // this will hold the game name
	char ogcserver[LEN_SERVERNAME+LEN_PORT+2];
	char password[LEN_PASSWORD]; // password for the game
	int passwordenabled;		// if the server use the password
	char ogc_port[LEN_PORT];
	int firewall;
	int notifygamemaster;
	int broadcast;
	int autostart;			// time for an autostart of the game
	int minplayers;			// minimal number of players
	
	int askplayername;		// ask player for name at startup
	
	int start_bombs;		// start values
	int start_range;
	float start_speed;
	float bomb_tickingtime;	// time before the bomb explodes
    unsigned char gametype; // GT_* values type of the Game if Deathmatch or other
	int dropitemsondeath;	// if a player should drop items when he die

	unsigned char ai_players;	// number of ai players
} typedef _bomberclone;

#define IS_LPLAYER2 (bman.p2_nr != bman.p_nr && bman.p2_nr >= 0 && bman.p2_nr < MAX_PLAYERS)


struct {
	signed char dir;
	signed char bestdir;
} typedef _airunaway;


extern _bomberclone bman;
extern Uint32 timestamp;
extern float timefactor;
extern float timediff;
extern int debug;

// Game routines.. 
extern void game_draw_info ();
extern void game_keys_loop ();
extern void game_loop ();
extern void game_end ();
extern void game_start();
extern void game_showresult ();
extern int game_check_endgame ();
extern void game_showresultnormal ();
extern void game_showresultteam ();
extern void game_menu_create ();
extern void game_menu_loop (SDL_Event *event, int eventstate);
extern void game_resetdata ();

// everything is declared in field.c
extern void draw_field ();
extern void field_clear(int x, int y);
extern void field_animation_add (int x, int y);
extern void field_animation ();
extern void field_loop ();
extern void field_hurrysize ();
extern void field_hurrydropitems ();
extern int field_check_alldirs (int x, int y, int type);
extern void draw_stone (int x, int y);
extern void stone_drawfire (int x, int y, int frame);
extern void stonelist_add (int x, int y);
extern void stonelist_del ();
extern void stonelist_draw ();

// configuration
extern void config_init (int argc, char **argv);
extern void config_menu ();
extern int config_read();
extern int config_write();
extern void ReadPrgArgs (int argc, char **argv);
extern void ReadPrgArgs_Jump (int argc, char **argv);
extern int check_version (int ma, int mi, int su, char *ver);
void joypad_config ();

// debug.c
extern void d_in_pl_detail (char *head);
extern void d_playerdetail (char *head);
extern void d_gamedetail (char *head);
extern void d_printf (char *fmt,...);
extern void d_playerstat(char *head);
extern void d_bitprint (int bits, int nr);
extern void d_fatal (char *fmt,...);
extern void debug_ingameinfo();
extern void d_printsdlrect (char *text, SDL_Rect *rect);
extern void d_teamdetail (char *head);

// special.c
extern void special_use (int p_nr);
extern void special_pickup (int p_nr, int s_type);
extern void special_loop ();
extern void special_clear (int p_nr);
extern void special_push (int p_nr);
extern void special_row (int p_nr);
extern void special_trigger (int p_nr);
extern void special_liquidmoved (int p_nr);

extern void tileset_load (char *tileset, int dx, int dy);
extern void tileset_random ();
extern void tileset_free ();


// help
extern void help (int showpage);

#endif
