/* $Id: network.h,v 1.24 2006-08-13 21:26:50 stpohle Exp $
 * network.h file... for everything what have to do with the network stuff 
 */

#include <SDL.h>
#include "flyingitems.h"

#ifndef _NETWORK_H_
#define _NETWORK_H_

#define MAX_UDPDATA 1024
#define PKG_RESENDCACHE_SIZE 64
#define PKG_IN_INDEX_NUM 512
#define RESENDCACHE_TIMEOUT 400
#define RESENDCACHE_TIMEOUT_MENU 3000
#define RESENDCACHE_RETRY 10
#define DOWNLOAD_TIMEOUT 2000
#define DYN_PKG_MAX_MISSING 4
#define DYN_PKG_MIN_MISSING 1
#define PKG_SENDSETOPT 2
#define MAX_SRVLIST 512
#define TIME_UPDATEINFO 1000

#define GT_MP_PTPM (bman.p_nr == bman.p_servnr && bman.sock >= 0)
#define GT_MP_PTPS (bman.p_nr != bman.p_servnr && bman.sock >= 0)
#define GT_MP (bman.sock > 0)
#define GT_SP (bman.sock <= 0)

#define GS_WAITRUNNING (bman.state == GS_wait || bman.state == GS_ready || bman.state == GS_running)
#define GS_RUNNING (bman.state == GS_ready || bman.state == GS_running)

/* Little / Big Endian Convert */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	#define NTOH16(__i) s_swap16(__i)
	#define HTON16(__i) s_swap16(__i)
	#define NTOH32(__i) s_swap32(__i)
	#define HTON32(__i) s_swap32(__i)
#else
    /* intel system */
	#define NTOH16(__i) (__i)
	#define HTON16(__i) (__i)
	#define NTOH32(__i) (__i)
	#define HTON32(__i) (__i)
#endif

/* converting of float to int and other direction */
#define FTOI16(__x) ((Sint16)((float)(__x * 256.0f)))
#define FTOI32(__x) ((Sint32)((float)(__x * 4096.0f)))
#define I16TOF(__x) (((float)__x) / 256.0f)
#define I32TOF(__x) (((float)__x) / 4096.0f)

#include "udp.h"

enum _networkflags {
	NETF_firewall = 1,	// User Behind Firewall
	NETF_local2			// Lokaler 2 Player
};


struct {  // this holds the network data
	char host[LEN_SERVERNAME];
	char port[LEN_PORT];
	struct _sockaddr sAddr;
	signed char pl_nr; // pl_nr so we know it in the pkgcache.
} typedef _net_addr;


struct { /* this will hold all needed data for the packet
			timeout function */
    signed char send_to;  // sending packet data (playermove) on 0 it will be send
    signed char send_set; // start value for the packet data option (dynamic set)
	int to_2sec;		// how many unreached packets was send
	Uint32 to_timestamp;
} typedef _net_pkgopt;


struct {
	_net_addr addr;			// holds the address
	int pingreq;			// just to send a ping and to save the number in here
	int pingack;			// just to wait for an ping reply.. it will show up here
	Uint32 timestamp;		// time of the last incoming package
	signed char net_istep;	// -1 gfx all is loaded
	signed char net_status;
	unsigned char flags;	// keep some flags.. like NETF_firewall
	int firstplayer;		// number of the first player (only needed if NETF_local2 is set
	_net_pkgopt pkgopt;		// packet and network controll data
} typedef _net_player;	


// network menu
extern void netmenu();
extern void network_options ();
extern void join_multiplayer_game ();
extern void host_multiplayer_game ();
extern void multiplayer_firstrun ();
extern void multiplayer_game ();
extern void network_helpscreen ();

// network.c
extern int network_server_port (char *server, char *host, int hostlen, char *port, int portlen);
extern void network_shutdown ();
extern int network_init ();
extern int network_loop ();
extern void net_send_playerid (int pl_nr);
extern void net_new_game ();
extern void net_transmit_gamedata ();
extern void net_game_send_player (int p_nr);
extern void net_game_send_playermove (int p_nr, int mustsend);
extern void net_game_send_bomb (int p, int b);
extern void net_game_send_field (int x, int y);
extern void net_game_send_special (int pl_nr, int ex_nr, int type);
extern void net_game_send_dropitems (int pl_nr, _flyingitem **fiptr, int cnt);
extern void net_game_send_respawn (int pl_nr);
extern void net_game_fillsockaddr ();
extern void net_game_send_ill (int p_nr);
extern void net_game_send_delplayer (int pl_nr);
extern void draw_netupdatestate (char st);
extern void net_send_servermode ();
extern void net_send_players ();
extern void net_send_teamdata (int team_nr);
extern int net_check_timeout (int pl_nr);
extern void net_dyn_pkgoption ();
extern void net_send_chat (char *text, signed char notigamesrv);
extern void net_send_mapinfo ();
extern void net_send_updateinfo ();
extern void send_ogc_update ();

// multiwait.c
extern void wait_for_players ();

// netsrvlist.c
extern void net_getserver ();
extern void srvlist_rebuildlist ();

/* check if we can send to the player */
#define NET_CANSEND(__pl) (!PS_IS_aiplayer (players[__pl].state) \
							&& PS_IS_netplayer (players[__pl].state) \
							&& (players[__pl].net.addr.host[0] != 0 && players[__pl].net.addr.port[0] != 0) \
							&& __pl != bman.p_nr && __pl != bman.p2_nr \
							&& (players[__pl].net.flags & NETF_local2) == 0 \
							&& (( GT_MP_PTPM && (__pl != bman.p_servnr )) || ( __pl == bman.p_servnr && GT_MP_PTPS ) \
								|| (((players[__pl].net.flags & NETF_firewall) == 0) \
									&& !bman.firewall)))

#endif
