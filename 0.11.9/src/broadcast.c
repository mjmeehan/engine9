/* $Id: broadcast.c,v 1.7 2007-12-12 19:27:35 stpohle Exp $
 * find broadcasted games and also hold in this list 
 * all games which the ogc reports us */
 
#include "bomberclone.h"
#include "network.h"
#include "packets.h"
#include "ogcache-client.h"
#include "broadcast.h"

static int bc_lastrequest;
struct broadcast_entry broadcast_list [BC_MAXENTRYS];


/* find a game on the same server and port */
int broadcast_find (char *host, char *port) {
	int i;
	
	printf ("breadcast_find  host:%s port:%s\n", host, port);
	
	for (i = 0; (((strncmp (host, broadcast_list[i].host, LEN_SERVERNAME) != 0) 
				|| (strncmp (port, broadcast_list[i].port, LEN_PORT) != 0))
				&& (i < BC_MAXENTRYS)); i++);
	
	if (i >= BC_MAXENTRYS)
		i = -1;
	return i;
};


/* find next free game */
int broadcast_findfree () {
	int i;
	
	for (i = 0; (broadcast_list[i].host[0] != 0) && (i < BC_MAXENTRYS); i++);
		
	return i;
}


/* delete entry from the broadcasted list */
void broadcast_del (int nr) {

	if (nr < (BC_MAXENTRYS -1))
		memcpy (&broadcast_list[nr], &broadcast_list[nr+1], sizeof (struct broadcast_entry) * (BC_MAXENTRYS - (nr+1)));

	broadcast_list[BC_MAXENTRYS-1].host[0] = 0;
}


/* check for old not seen games */
void broadcast_check () {
	int i, j;
	
	/* check for a new update request on a single game and if a game had timed 
	 * out */
	for (i = 0; i < BC_MAXENTRYS; i++) {
		while (broadcast_list[i].host[0] != 0
				&& ((broadcast_list[i].try > BC_MAXREQUEST && broadcast_list[i].lan == 0)
					|| (timestamp - broadcast_list[i].timestamp > BC_REQUESTTIMEOUT && broadcast_list[i].lan == 1)))
			broadcast_del (i);
		
		if (timestamp - broadcast_list[i].timestamp > BC_REQUESTTIME
			&& broadcast_list[i].lan == 0 && broadcast_list[i].host[0] != 0) {
				broadcast_send (broadcast_list[i].host, broadcast_list[i].port);
				broadcast_list[i].timestamp = timestamp;
				broadcast_list[i].try++;
		}
		
		if (((broadcast_list[i].timestamp < 0) || (timestamp - broadcast_list[i].timestamp < 0))
			&& broadcast_list[i].host[0] != 0 && broadcast_list[i].lan == 0)	{
				broadcast_send (broadcast_list[i].host, broadcast_list[i].port);
				broadcast_list[i].timestamp = timestamp;
		}
	}
	
	/* check for games in the OGC list which are not in the broadcast_list */
	for (i = 0; i < MAX_OGC_ENTRYS; i++) 
		if ((ogc_array[i].serial != -1) && (broadcast_find (ogc_array[i].host, ogc_array[i].port) == -1)) {
			j = broadcast_findfree ();
			if ((check_version (0,11,6, ogc_array[i].version) >= 0) && j >= 0) {
				strncpy (broadcast_list[j].host, ogc_array[i].host, LEN_SERVERNAME);
				strncpy (broadcast_list[j].port, ogc_array[i].port, LEN_PORT);
				strncpy (broadcast_list[j].version, ogc_array[i].version, LEN_VERSION);
				strncpy (broadcast_list[j].gamename, ogc_array[i].gamename, LEN_GAMENAME);				
				broadcast_list[j].maxplayers = ogc_array[i].maxplayers;
				broadcast_list[j].curplayers = ogc_array[i].curplayers;
				
				d_printf ("broadcast_check: Add: nr:%d game:%s %s:%s\n", j, 
					broadcast_list[j].gamename, broadcast_list[j].host,	broadcast_list[j].port);
				broadcast_list[j].timestamp = timestamp;
				broadcast_list[j].ping = -1;
				broadcast_list[j].lan = 0;
				broadcast_list[j].try = 0;
			}
		}
}



/* open and send the first broadcast request */
void broadcast_init () {
	int i;
	
	for (i = 0; i < BC_MAXENTRYS; i++)
		broadcast_list[i].host[0] = 0;
	
	if (bman.broadcast)
		broadcast_send (NULL, NULL);
	
	bc_lastrequest = timestamp;
};


/* send the broadcast information */
void broadcast_send (char *host, char *port) {
	_net_addr addr;
	
	if (host != NULL && port != NULL) {
		strncpy (addr.host, host, LEN_SERVERNAME);
		strncpy (addr.port, port, LEN_PORT);
		d_printf ("broadcast_send (%s:%s)\n", addr.host, addr.port);
		dns_filladdr (addr.host, LEN_SERVERNAME, addr.port, LEN_PORT, bman.net_ai_family,  &addr.sAddr);
		send_gameinfo (&addr, bman.sock, 0);
	}
	else {
		sprintf (addr.port, "%d", DEFAULT_UDPPORT);
		strncpy (addr.host, "0.0.0.0", LEN_SERVERNAME);
		d_printf ("broadcast_send (%s:%s)\n", addr.host, addr.port);
		dns_filladdr (addr.host, LEN_SERVERNAME, addr.port, LEN_PORT, bman.net_ai_family,  &addr.sAddr);
		send_gameinfo (&addr, bman.sock, 1);
	}
};


void broadcast_loop () {
	int inlen;
	char indata [MAX_UDPDATA];
	struct pkg_gameinfo *pgi;
    _net_addr addr;

	if (bman.broadcast && (timestamp - bc_lastrequest) > BC_REQUESTTIME) {
		broadcast_send (NULL, NULL);
		bc_lastrequest = timestamp;
	}

	pgi = (struct pkg_gameinfo*) &indata;
	pgi->gamename[0] = 0;
    addr.port[0] = addr.host[0] = 0;
    inlen = udp_get (bman.sock, indata, MAX_UDPDATA, &addr.sAddr, bman.net_ai_family);
    if (inlen > 0)
        dns_filladdr (addr.host, LEN_SERVERNAME, addr.port, LEN_PORT, bman.net_ai_family,
                      &addr.sAddr);

    while (inlen > 0) {
		
		if (pgi->h.typ == PKG_gameinfo && pgi->gamename[0] != 0) {
			int nr;
			
			nr = broadcast_find (addr.host, addr.port);
			if (nr == -1)
				nr = broadcast_findfree ();
			if (nr < BC_MAXENTRYS) {
				strncpy (broadcast_list[nr].host, addr.host, LEN_SERVERNAME);
				strncpy (broadcast_list[nr].port, addr.port, LEN_PORT);
				strncpy (broadcast_list[nr].version, pgi->version, LEN_VERSION);
				strncpy (broadcast_list[nr].gamename, pgi->gamename, LEN_GAMENAME);
				broadcast_list[nr].curplayers = pgi->curplayers;
				broadcast_list[nr].maxplayers = pgi->maxplayers;
				
				d_printf ("broadcast_loop: Add: nr:%d game:%s %s:%s (lan:%d)\n", nr, 
					broadcast_list[nr].gamename, broadcast_list[nr].host, 
					broadcast_list[nr].port, pgi->broadcast);
				broadcast_list[nr].timestamp = timestamp;
				
				broadcast_list[nr].ping = timestamp - pgi->timestamp;
				broadcast_list[nr].lan = pgi->broadcast;
			}
		}
		
		pgi->gamename[0] = 0;
    	addr.port[0] = addr.host[0] = 0;
        inlen = udp_get (bman.sock, indata, MAX_UDPDATA, &addr.sAddr, bman.net_ai_family);
        if (inlen > 0)
            dns_filladdr (addr.host, LEN_SERVERNAME, addr.port, LEN_PORT, bman.net_ai_family,
                          &addr.sAddr);
	}
	
	broadcast_check ();
};
