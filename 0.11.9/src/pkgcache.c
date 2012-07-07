/* $Id: pkgcache.c,v 1.14 2009-05-11 20:51:25 stpohle Exp $
 * Resendcache work, We need this to resend lost packets over the network.
 * we will keep every packet with the PKGF_ackreq flag as long as we haven't
 * got any answer from the destination host. And resend the packet after a givin
 * time. The maximum time for a resend is 10 times if we haven't got any reply
 * by then we delete the packet from the cache. By doing this the game will not 
 * anymore sync. with all clients.
 */
#include "bomberclone.h"
#include "network.h"
#include "packets.h"

struct _resend_cache rscache;

/* reset the counter for the resendcache */
void rscache_init () {
	rscache.count = 0; 
};

/* add a new packet to the list */
void rscache_add (_net_addr *addr, struct pkg *packet) {
	int len;
	
	// d_printf ("rscache_add: pl:%p, typ:%u id:%u\n", addr->pl_nr, packet->h.typ, packet->h.id);
	/* check if there is still some free space left. */
	if (rscache.count >= PKG_RESENDCACHE_SIZE) {
		d_printf ("rscache_add no free rscache entry left.\n");
		return;
	}
	
	/* copy all the data we need. */
	rscache.entry[rscache.count].timestamp = timestamp;
	rscache.entry[rscache.count].retry = 0;
	memcpy (&rscache.entry[rscache.count].addr, addr, sizeof (_net_addr));
	len = NTOH16 (packet->h.len);
	if (MAX_UDPDATA < len) len = MAX_UDPDATA;
	memcpy (&rscache.entry[rscache.count].packet, packet, MAX_UDPDATA);
	rscache.count ++;
};

/* delete the entry */
void rscache_delnr (int nr) {
	int a;

	if (nr >= 0 && nr < PKG_RESENDCACHE_SIZE) {
		for (a = nr; a < rscache.count - 1; a++)
			rscache.entry[a] = rscache.entry[a+1];
		rscache.count--;
		d_printf ("rscache_delnr: element %d deleted.\n", nr);
	}
	else 
		d_printf ("rscache_delnr: number is out of range (%d)\n", nr);
}


/* find and delete the givin packet.
 * Return Value: 0 = nothing deleted, 1 one entry deleted */
int rscache_del (_net_addr *addr, unsigned char typ, short unsigned int id) {
	int i;

	 // d_printf ("rscache_del: addr %p (pl_nr:%d, typ:%d, id:%u\n", addr, addr->pl_nr, typ, id);

	for (i = 0; (i < rscache.count) && (i < PKG_RESENDCACHE_SIZE); i++) {
		if (rscache.entry[i].addr.pl_nr == addr->pl_nr &&
			NTOH16(rscache.entry[i].packet.h.id) == id &&
			rscache.entry[i].packet.h.typ == typ) { // found element
				rscache_delnr (i);
				return 1;
		}
	}
	return 0;
};

/* test for old packets where we haven't got a ackreq packet for. 
 * If the timeout is up resend the packet again until RESENDCACHE_RETRY
 * has reached then delete the packet. */
void rscache_loop () {
	int i;
	int timeout;
	
	if (bman.state==GS_running) timeout = RESENDCACHE_TIMEOUT; else timeout=RESENDCACHE_TIMEOUT_MENU;

	for (i = 0; (i < rscache.count) && (i < PKG_RESENDCACHE_SIZE); i++) {
        if (timestamp - rscache.entry[i].timestamp >= timeout
            && rscache.entry[i].retry < RESENDCACHE_RETRY) {
            /* send it again */
            d_printf
                ("Data Send Timeout Resend pl:%p, typ:%u id:%u Fill:%d Pos:%d\n", 
                rscache.entry[i].addr.pl_nr, rscache.entry[i].packet.h.typ, rscache.entry[i].packet.h.id, rscache.count, i);

            udp_send (bman.sock, (char *) &rscache.entry[i].packet,
            			NTOH16 (rscache.entry[i].packet.h.len), 
						&rscache.entry[i].addr.sAddr,
            			bman.net_ai_family);
            rscache.entry[i].timestamp = timestamp;
            rscache.entry[i].retry++;
            if (rscache.entry[i].addr.pl_nr >= 0 && rscache.entry[i].addr.pl_nr < MAX_PLAYERS)
				players[rscache.entry[i].addr.pl_nr].net.pkgopt.to_2sec++;
        }

        if (timestamp - rscache.entry[i].timestamp >= timeout
            && rscache.entry[i].retry >= RESENDCACHE_RETRY) {
            d_printf
                ("Data Send Timeout Delete pl:%p, typ:%u id:%u Fill:%d Pos:%d\n", 
                rscache.entry[i].addr.pl_nr, rscache.entry[i].packet.h.typ, rscache.entry[i].packet.h.id, rscache.count, i);
            if (rscache.entry[i].addr.pl_nr >= 0 && rscache.entry[i].addr.pl_nr < MAX_PLAYERS)
				players[rscache.entry[i].addr.pl_nr].net.pkgopt.to_2sec++;
			
            rscache_delnr (i);
			if (i > 0) i--;
        }
	}
};
