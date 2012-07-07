/* $Id: packets.h,v 1.39 2009-05-11 20:51:25 stpohle Exp $
 * network packets.. */

#ifndef _PACKETS_H_
#define _PACKETS_H_

#include "flyingitems.h"
#include "player.h"
#include "bomb.h"


/* All known Packettypes for the game. All types before PKG_field are
 * only packets between client and server, all packets behinf PKG_field
 * are between all clients so they will be forwarded.					*/
enum _network_data {
	PKG_error = 0,		// 0
	PKG_gameinfo,		// 1
	PKG_joingame,		// 2 every packet below here will checked 
						// if it comes from a orginal player
	PKG_contest,		// 3
	PKG_playerid,		// 4
	PKG_servermode,		// 5
	PKG_pingreq,		// 6
	PKG_pingack,		// 7
	PKG_getfield,		// 8
	PKG_getplayerdata,	// 9 
	PKG_teamdata,		// 10
	PKG_fieldline,		// 11
	PKG_pkgack,			// 12
	PKG_mapinfo,		// 13
	PKG_tunneldata,		// 14
	PKG_updateinfo,		// 15
	PKG_field,			// 16 forward - always be the first field
	PKG_playerdata,		// 17 forward
	PKG_bombdata,		// 18 forward
	PKG_playerstatus,	// 19 forward
	PKG_playermove,		// 20 forward
	PKG_chat,			// 21 forward
	PKG_ill,			// 22 forward
	PKG_special,		// 23 forward
	PKG_dropitem,		// 24 forward
	PKG_respawn,		// 25 forward
	PKG_quit			// 26 forward - always the last known type forwarded type
};


enum _pkgflags {
	PKGF_ackreq = 1,
	PKGF_ipv6 = 2
};


struct pkgheader {
	unsigned char typ;
	unsigned char flags;
	Sint16 id;
	Sint16 len;
} __attribute__((packed));

struct pkg {
	struct pkgheader h;
	char data[0];
} __attribute__((packed));

struct pkg_bcmservchat {
	char typ;
    char data[128];
} __attribute__((packed));

struct pkg_tunneldata {
	struct pkgheader h;
	signed char tunnel_nr;
	_point target;
} __attribute__((packed));

struct pkg_pkgack {
	struct pkgheader h;
    char typ;
	Sint16 id;
} __attribute__((packed));

struct pkg_ping {
	struct pkgheader h;
    Sint32 data;
} __attribute__((packed));

struct pkg_contest {
	struct pkgheader h;
    signed char from;
	signed char to;
} __attribute__((packed));

struct pkg_teamdata {
	struct pkgheader h;
    signed char team_nr;
	signed char col;
	signed char wins;
	char name[LEN_PLAYERNAME];
} __attribute__((packed));

struct pkgdropitemelemt {
	signed char x;
	signed char y;
	unsigned char typ;
} __attribute__((packed));

struct pkg_dropitem {
	struct pkgheader h;
	_point from;				// from where the items are coming
	signed char pl_nr;			// playernumber -1 if the server jsut dropped something
	unsigned char count;		// number of elements
	struct pkgdropitemelemt items[0];	// elements		
} __attribute__((packed));

struct pkg_field {
	struct pkgheader h;
    unsigned char x;
    unsigned char y;
    unsigned char type;
	signed char mixframe;		// data for the mixed frame
    Sint16 frame;               // frame (frame > 0 && FS_stone)
    unsigned char special;      // to save special stones, or the tunnel number
    struct {
		unsigned char count;
		unsigned char frame;
		unsigned char bomb_b;
		unsigned char bomb_p;
	} ex[4]; // count up every explosion there is on this field for ever direction
    Sint32 ex_nr;               // number to identify the explosion.
} __attribute__((packed));

struct pkg_error {
	struct pkgheader h;
    unsigned char nr;
    char text[128];
} __attribute__((packed));

struct pkg_servermode {
	struct pkgheader h;
    unsigned char type;
    unsigned char state;
    unsigned char gametype;
	unsigned char dropitemsondeath;
    unsigned char players;
	unsigned char mapstate;
    unsigned char maxplayer;
	signed char last_winner;
	signed char fieldsize_x;
	signed char fieldsize_y;
	char tileset[LEN_TILESETNAME];
	signed char p_servnr;
	signed char lplayer2;	/* indicates that we mean local player 2 */
    signed char p_nr;       /* if the server sends this to a client... 
                               it will be the clients in_nr number 
                               (-1) for not set */
} __attribute__((packed));

struct pkg_joingame {
	struct pkgheader h;
	signed char ver_major;		// Version
	signed char ver_minor;		// Version
	signed char ver_sub;		// Version
	signed char secondplayer;	// this is the second player
	char name[LEN_PLAYERNAME];
	char password[LEN_PASSWORD];
} __attribute__((packed));

struct pkg_playerid {
	struct pkgheader h;
    char name[LEN_PLAYERNAME];
    char host[LEN_SERVERNAME];
    char port[LEN_PORT];
	signed char pl_nr;			// Player Nummer
	signed char gfx_nr;			// number of the graphic
	signed char state;
	signed char netflags;		// network flags
	Sint16 points;
	Sint16 wins;
	Sint16 nbrKilled;
	signed char team_nr;		// team number
	Sint16 team_points;			// team points
	Sint16 team_wins;			// team wins
} __attribute__((packed));	

struct pkg_playerdata {
	struct pkgheader h;
    signed char p_nr;			// Playernumber
	Sint16 points;				// points
	Sint16 nbrKilled;		        // number of player kill during a round
	Sint16 wins;				// how many times we win
	signed char gfx_nr;			// the gfx number we want to use
	signed char team_nr;		// teamnumber of the player
    _point pos;
    unsigned char bombs_n;
    unsigned char range;
    unsigned char state;
    unsigned char d;
    unsigned char frame;
	signed char dead_by;
	signed char ready;			// if the player is ready for the game
} __attribute__((packed));

struct pkg_playermove {
	struct pkgheader h;
    signed char p_nr;
	signed char m;
	signed char d;
	Sint16 speed;
	Sint16 tunnelto;
    _point pos;
} __attribute__((packed));

struct pkg_bombdata {
	struct pkgheader h;
    unsigned char p_nr;
    unsigned char pi_nr;
    unsigned char b_nr;
    Sint16 x;
    Sint16 y;
    unsigned char state;
    unsigned char r;
    Sint32 ex_nr;
    Sint32 to;
    Sint16 destx;
    Sint16 desty;
	Sint16 fdata;
} __attribute__((packed));

struct pkg_quit {
	struct pkgheader h;
	signed char pl_nr;
	signed char new_server;
} __attribute__((packed));

struct pkg_getfield {
	struct pkgheader h;
    signed char line;
} __attribute__((packed));

struct pkg_playerstatus {
	struct pkgheader h;
    signed char pl_nr;
    signed char net_istep;
    signed char status;
} __attribute__((packed));

struct pkg_fieldline {
	struct pkgheader h;
    signed char line;
    unsigned char type[MAX_FIELDSIZE_X];
    unsigned char special[MAX_FIELDSIZE_X];
} __attribute__((packed));

struct pkg_ill {
	struct pkgheader h;
	signed char pl_nr;
	Sint32 to[PI_max];
} __attribute__((packed));

struct pkg_updateinfo {
	struct pkgheader h;
	signed char pl_nr;
	unsigned char step[MAX_PLAYERS];
	unsigned char status[MAX_PLAYERS];	
} __attribute__((packed));

struct pkg_respawn {
	struct pkgheader h;
	signed char pl_nr;
	signed char x;
	signed char y;
	unsigned char state;
} __attribute__((packed));

struct pkg_getplayerdata {
	struct pkgheader h;
    signed char pl_nr;
} __attribute__((packed));

struct pkg_chat {
	struct pkgheader h;
	char text[128];
} __attribute__((packed));

struct pkg_special {
	struct pkgheader h;
	signed char pl_nr;
	Sint32 ex_nr;
    unsigned char typ;
} __attribute__((packed));

struct pkg_mapinfo {
	struct pkgheader h;
	char tileset[LEN_TILESETNAME];
	char mapname[LEN_FILENAME];
	unsigned char map_selection;
	unsigned char bombs;
	unsigned char shoes;
	unsigned char fire;
	unsigned char mixed;
	unsigned char death;
	unsigned char sp_trigger;
	unsigned char sp_push;
	unsigned char sp_row;
	unsigned char size_x;
	unsigned char size_y;
	unsigned char start_bombs;
	unsigned char start_range;
	char start_speed[8];		// to make sure there will be no difference
	char bomb_tickingtime[8];	// to make sure there will be no difference
} __attribute__((packed));

/* this packet type is also used in netsrvlist.c */
struct pkg_gameinfo {
	struct pkgheader h;
	Uint32 timestamp;
	unsigned char curplayers;
	unsigned char maxplayers;
	char gamename[LEN_GAMENAME];
	char version[LEN_VERSION];
	char broadcast;
	signed char password;		/* if password == -1, we do a game request */
} __attribute__((packed));

/* structures needed by the resend cache */
struct _rscache_entry {
	Uint32 timestamp;			// pointer to the timestamp
	signed char retry;			// retry's how many times we tryed this
	_net_addr addr;				// the address
	struct pkg packet;   // the packet
	char data[MAX_UDPDATA]; // packet data without header
} __attribute__((packed));	

struct _resend_cache {
	struct _rscache_entry entry[PKG_RESENDCACHE_SIZE]; // pointer to our data
	int count;
};

struct _inpkg_index {
	signed char pl_nr;
	unsigned char typ;
	Sint16 id;
};

extern void do_error (struct pkg_error *data, _net_addr *addr);
extern void do_playerid (struct pkg_playerid *p_id, _net_addr *addr);
extern void do_servermode (struct pkg_servermode *s_mod, _net_addr *addr);
extern void do_joingame (struct pkg_joingame *p_jg, _net_addr * addr);
extern void do_field (struct pkg_field *f_dat, _net_addr *addr);
extern void do_ping (struct pkg_ping *p_dat, _net_addr *addr);
extern void do_playerdata (struct pkg_playerdata *p_dat, _net_addr *addr);
extern void do_updateinfo(struct pkg_updateinfo *stat, _net_addr *addr);
extern void do_playermove (struct pkg_playermove *p_dat, _net_addr *addr);
extern void do_bombdata (struct pkg_bombdata *b_dat, _net_addr *addr);
extern void do_quit (struct pkg_quit *q_dat, _net_addr *addr);
extern void do_getfield (struct pkg_getfield *gf_dat, _net_addr *addr);
extern void do_fieldline (struct pkg_fieldline *f_dat, _net_addr *addr);
extern void do_getplayerdata (struct pkg_getplayerdata *gp_dat, _net_addr *addr);
extern void do_playerstatus (struct pkg_playerstatus *gp_dat, _net_addr *addr);
extern void do_pkgack (struct pkg_pkgack *p_ack, _net_addr *addr);
extern void do_chat (struct pkg_chat *chat_pkg, _net_addr *addr);
extern void do_pkg (struct pkg *packet, _net_addr *addr, int len);
extern void do_bcmservchat (struct pkg_bcmservchat *packet, _net_addr *addr);
extern void do_ill (struct pkg_ill *ill_pkg, _net_addr *addr);
extern void do_special (struct pkg_special *sp_pkg, _net_addr *addr);
extern void do_mapinfo (struct pkg_mapinfo *map_pkg, _net_addr *addr);
extern void do_tunneldata (struct pkg_tunneldata *tun_pkg, _net_addr *addr);
extern void do_dropitems (struct pkg_dropitem *di_pkg, _net_addr *addr);
extern void do_respawn (struct pkg_respawn *r_pkg, _net_addr *addr);
extern void do_contest (struct pkg_contest *ct_pkg, _net_addr *addr);
extern void do_teamdata (struct pkg_teamdata *td, _net_addr * addr);
extern void do_gameinfo (struct pkg_gameinfo *pgi, _net_addr *addr);

extern void send_pkg (struct pkg *packet, _net_addr *addr);
extern void send_playerid (_net_addr *addr, char *name, char *pladdr, char *plport, int p_nr, int gfx_nr, int team_nr, signed char netflags);
extern void send_servermode (_net_addr *addr, int pl_nr);
extern void send_joingame (_net_addr * addr, char *name, int secondplayer);
extern void send_error (_net_addr *addr, char *text);
extern void send_field (_net_addr *addr, int x, int y, _field * field);
extern void send_ping (_net_addr *addr, int data, unsigned char typ);
extern void send_playerdata (_net_addr *addr, int p_nr, _player * pl);
extern void send_playermove (_net_addr *addr, int p_nr, _player * pl);
extern void send_bombdata (_net_addr *addr, int p, int b, _bomb * bomb);
extern void send_quit (_net_addr *addr, int pl_nr, int new_server);
extern void send_getfield (_net_addr *addr, int line);
extern void send_fieldline (_net_addr *addr, int line);
extern void send_getplayerdata (_net_addr *addr, int pl);
extern void send_playerstatus (_net_addr *addr, int pl_nr, int net_istep, int status);
extern void send_pkgack (_net_addr *addr, unsigned char typ, short int id);
extern void send_chat (_net_addr *addr, char *text);
extern void send_ill (_net_addr *addr, int p_nr, _player *pl);
extern void send_special (_net_addr *addr, int p_nr, int typ, int ex_nr);
extern void send_mapinfo (_net_addr *addr);
extern void send_updateinfo(_net_addr *addr);
extern void send_tunneldata (_net_addr *addr, int tunnelnr, int x, int y);
extern void send_dropitems (_net_addr *addr, int pl_nr, _flyingitem **fitems, int cnt);
extern void send_respawn (_net_addr * addr, int plnr);
extern void send_contest (_net_addr * addr, int from, int to, int ackreq);
extern void send_teamdata (_net_addr * addr, int team_nr);
extern void send_gameinfo (_net_addr * addr, int sock, int broadcast);

extern void fwd_pkg (struct pkg *packet, _net_addr *addr);

extern int get_player_nr (char *host, char *port);
extern int inpkg_check (unsigned char typ, short int id, _net_addr *addr);
extern void inpkg_delplayer (int pl_nr);

/* this functions will be defined in pkgcache.c */
extern void rscache_init ();
extern void rscache_add (_net_addr *addr, struct pkg *packet);
extern void rscache_delnr (int nr);
extern int rscache_del (_net_addr *addr, unsigned char typ, short unsigned int id);
extern void rscache_loop ();

extern struct _resend_cache rscache;

#endif
