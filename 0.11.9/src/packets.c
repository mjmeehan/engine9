/* $id:$ */
/* network commands and data */

#include "basic.h"
#include "bomberclone.h"
#include "network.h"
#include "packets.h"
#include "chat.h"
#include "sound.h"
#include "menu.h"

extern _point debug_field;
extern int debug_lastping;

static short int pkg_lastid;    /* the packet id, this will only counted 
                                 * up nothing more.. if we are at 0x10000 
                                 * we will start at 0 */

struct _resend_cache resend_cache;
struct _inpkg_index inpkg_index[PKG_IN_INDEX_NUM];
int inpkg_index_pos = 0;


/*
 * help function to get the playernumber from the address.
 * this function  does not indicate which player it is, this function only checks
 * if the packet comed from a known player
 */
int get_player_nr (char *host, char *port) {
    int i,
      res;

    for (i = 0, res = -1; (i < MAX_PLAYERS && res == -1); i++) {
        if (players[i].net.addr.host[0] != 0)
            if ((strcmp (players[i].net.addr.host, host) == 0)
                && (strcmp (players[i].net.addr.port, port) == 0))
                res = i;
    }
    return res;
}


/*
 * Packettype: error
 */
void send_error (_net_addr * addr, char *text) {
    struct pkg_error p_err;

    d_printf ("Send Network Error : %s\n", text);
    strncpy (p_err.text, text, 127);
    p_err.h.typ = PKG_error;
    p_err.h.flags = 0;
    p_err.h.len = HTON16 (sizeof (struct pkg_error));
    p_err.nr = 0;

    send_pkg ((struct pkg *) &p_err, addr);
};


void do_error (struct pkg_error *data, _net_addr * addr) {
    d_printf ("Network Error from %s:%s : '%s'\n", addr->host, addr->port, data->text);

	/* check if the packet comes from the server, the server
	 * never gets errormessages from a client, if a client has 
	 * some trouble the client will just disconnect */
	if (GT_MP_PTPS && addr->pl_nr == bman.p_servnr) {
		menu_displaymessage ("Network Error", "Got Error from: %s:%s\nMessage:%s", addr->host,
                addr->port, data->text);
		network_shutdown ();
	}
};



/*
 * Packettype: joingame
 * client sends this to the server if he want's to join
 *
 * 1) check if this package comes from a already joined and known player
 *    if so: just send the current playerlist back.
 * else:
 * 2) find free playerslot and add the player there
 * 3) send to all players the new playerid
 */
void do_joingame (struct pkg_joingame *p_jg, _net_addr * addr) {
    _player *pl;
    int i,
      vma,
      vmi,
      vsu;
    char text[255];

    d_printf ("do_joingame (From:%s:%s Player(name:%s) addr:%p\n", addr->host, addr->port, p_jg->name, addr);

    sscanf (VERSION, "%d.%d.%d", &vma, &vmi, &vsu);

    /* Do some basic checks befor we accept this data packet
     * 1) check if we are a server */
    if (GT_MP_PTPS) {
        sprintf (text, "Sorry this is a client and not a server.");
        send_error (addr, text);
        return;
    }
    /* 2) check the version */
    if (p_jg->ver_sub != vsu || p_jg->ver_major != vma || p_jg->ver_minor != vmi) {
        sprintf (text, "Version Error - Host/Server Version: %s", VERSION);
        send_error (addr, text);
        return;
    }

    /* 3) check if we have a password */
    d_printf ("Password Check Server:\"%s\" Client:\"%s\"   %d, %d\n", bman.password,
              p_jg->password, strlen (bman.password), strlen (p_jg->password));
    if (bman.passwordenabled && (strncmp (bman.password, p_jg->password, LEN_PASSWORD)
                                 || strlen (bman.password) != strlen (p_jg->password))) {
        send_error (addr, "This game is Password protected. Your Password is wrong.");
        return;
    }

    /* find a free place for the player and add the player to the game */
    if (GS_WAITRUNNING && GT_MP_PTPM) {
        int j,
          freeslot;
        /* find a free playerslot and check if this player isn't already in the list */
        for (i = -1, freeslot = -1, j = 0; j < MAX_PLAYERS; j++) {
            if (!PS_IS_used (players[j].state) && freeslot == -1)
                freeslot = j;
            if (strncmp (players[j].net.addr.host, addr->host, LEN_SERVERNAME) == 0
                && strncmp (players[j].net.addr.port, addr->port, LEN_PORT) == 0
                && ((p_jg->secondplayer && (players[j].net.flags & NETF_local2) != 0)
                    || (!p_jg->secondplayer && (players[j].net.flags & NETF_local2) == 0)))
                i = j;
        }

        if (i == -1)
            i = freeslot;

        if ((i >= 0) && (i < MAX_PLAYERS) && (i < bman.maxplayer)) {
            /* free player slot found ... fill in data */
            pl = &players[i];

            pl->state = PSF_used + PSF_net;
            if (strncmp (pl->name, p_jg->name, LEN_PLAYERNAME) == 0)
                d_printf ("     Player ReJoined : Nr:[%d]   Name:%10s\n", i, p_jg->name);
            else {
                d_printf ("     Player Added : Nr:[%d]   Name:%10s\n", i, p_jg->name);
                pl->points = 0;
                pl->wins = 0;
                pl->nbrKilled = 0;
                pl->team_nr = -1;

                team_choose (pl);
            }

            strncpy (pl->name, p_jg->name, LEN_PLAYERNAME);

            pl->gfx_nr = -1;
            pl->gfx = NULL;

            pl->state &= (0xFF - (PSF_alife + PSF_playing));

            /* Reset the network data */
            pl->net.timestamp = timestamp;
            pl->net.pkgopt.to_timestamp = timestamp;
            pl->net.pkgopt.to_2sec = 0;
            pl->net.pkgopt.send_to = 0;
            pl->net.pkgopt.send_set = PKG_SENDSETOPT;

            strncpy (pl->net.addr.host, addr->host, LEN_SERVERNAME);
            strncpy (pl->net.addr.port, addr->port, LEN_PORT);
            dns_filladdr (pl->net.addr.host, LEN_SERVERNAME, pl->net.addr.port,
                          LEN_PORT, bman.net_ai_family, &pl->net.addr.sAddr);
            if (p_jg->secondplayer)
                pl->net.flags = NETF_local2 + NETF_firewall;
            else
                pl->net.flags = NETF_firewall;
            pl->net.addr.pl_nr = i;
            bman.players_nr_s++;
            addr->pl_nr = i;

            /* send to the new client the servermode and the complete playerlist */
            if ((!p_jg->secondplayer) && !(players[j].net.flags & NETF_local2))
                send_mapinfo (addr);
            send_servermode (addr, i); // with this packet the client know it'S pl_nr

        }

        else if (GS_WAITRUNNING) {
            send_error (addr, "No Free Playerslot found\n");
            return;
        }

        else if (!GS_WAITRUNNING) {
            send_error (addr, "Update mode, please try again.\n");
            return;
        }
    }

    d_playerdetail ("*** PLAYER List ***");
    bman.updatestatusbar = 1;
};


void send_joingame (_net_addr * addr, char *name, int secondplayer) {
    struct pkg_joingame p_jg;
    int vmi,
      vma,
      vsu;

    d_printf ("send_joingame SendTo: %s:%s (Name:%16s)\n", addr->host, addr->port, name);

    sscanf (VERSION, "%d.%d.%d", &vma, &vmi, &vsu);

    p_jg.h.typ = PKG_joingame;
    p_jg.h.flags = PKGF_ackreq;
    p_jg.h.len = HTON16 (sizeof (struct pkg_joingame));
    p_jg.ver_sub = vsu;
    p_jg.ver_major = vma;
    p_jg.ver_minor = vmi;
    p_jg.secondplayer = secondplayer;
    strncpy (p_jg.name, name, LEN_PLAYERNAME);
    strncpy (p_jg.password, bman.password, LEN_PASSWORD);

    bman.firewall = 1;
    send_pkg ((struct pkg *) &p_jg, addr);
};



/***
 *** Packettype: contest
 *** Test the connection from a new player for the firewall flag.
 *** This packet will be send from one player to the other the
 *** current server will only get this packet if a connection test was positive
 ***/
void
do_contest (struct pkg_contest *ct_pkg, _net_addr * addr)
{
    d_printf ("do_contest (pl_nr = %d) from=%d to=%d\n", addr->pl_nr, ct_pkg->from, ct_pkg->to);

    if (addr->pl_nr >= MAX_PLAYERS
        || (addr->pl_nr == -1 && PS_IS_netplayer (players[ct_pkg->from].state))) {
        d_printf ("     addr->pl_nr out of range (0-MAX_PLAYERS)\n");
        return;
    }

    /* master will have to change the firewall flag on a player */
    if (GT_MP_PTPM) {
		if ((ct_pkg->from < 0 || ct_pkg->from >= MAX_PLAYERS
			 || ct_pkg->to < -1 || ct_pkg->to >= MAX_PLAYERS
			 || addr->pl_nr != ct_pkg->from)) {
				 d_printf ("     from or to value out of range (0-MAX_PLAYERS)\n");
				 return;
		}
		/* ignore packet it's just a workaround for 
		 * some hardware router */
		if (ct_pkg->to == -1) return; 

        players[ct_pkg->to].net.flags &= (0xFF - NETF_firewall);
        net_game_send_player (ct_pkg->to);
    }

    /* if a client get this packet we send a packet
     * to the server that we have got this packet. */
    else {
        send_contest (&players[bman.p_servnr].net.addr, ct_pkg->from, bman.p_nr, 1);
        bman.firewall = 0;
    }
}


void
send_contest (_net_addr * addr, int from, int to, int ackreq)
{
    struct pkg_contest ct_pkg;

    d_printf ("send_contest addr->id:%d, from:%d, to:%d\n", addr->pl_nr, from, to);

    ct_pkg.h.typ = PKG_contest;
    if (ackreq)
        ct_pkg.h.flags = PKGF_ackreq;
    else
        ct_pkg.h.flags = 0;
    ct_pkg.h.len = HTON16 (sizeof (struct pkg_contest));

    ct_pkg.from = from;
    ct_pkg.to = to;

    send_pkg ((struct pkg *) &ct_pkg, addr);
}


/***
 *** Packettype: playerid
 *** Update Playerinformation, Teampoints of a Player, Points
 ***
 ***
 *** if  : the pl_nr == -1 send the whole list to the sender
 *** else: set the new values for this player
 ***/
void
do_playerid (struct pkg_playerid *p_id, _net_addr * addr)
{
    _player *pl;
    int i;

    d_printf
        ("do_playerid (From:%s:%s pl_nr=%d) Player(name:%s [%s:%s], pl_nr:%d state:%d,%d)\n",
         addr->host, addr->port, addr->pl_nr, p_id->name, p_id->host, p_id->port, p_id->pl_nr, p_id->state, PS_IS_alife (p_id->state));

    /*
     * check if we have to send the whole playerlist to a client 
     */
    if (GT_MP_PTPM && p_id->pl_nr == -1 && addr->pl_nr >= 0 && addr->pl_nr < MAX_PLAYERS) {
        /* Send all connected players the new PlayerID, except to the new player */
        pl = &players[addr->pl_nr];
        for (i = 0; i < MAX_PLAYERS; i++)
            if (NET_CANSEND (i) && addr->pl_nr != i)
                send_playerid (&players[i].net.addr, pl->name, pl->net.addr.host,
                               pl->net.addr.port, pl->net.addr.pl_nr, pl->gfx_nr,
                               pl->team_nr, pl->net.flags);

        for (i = 0; i < MAX_PLAYERS; i++)
            send_playerid (addr, players[i].name, players[i].net.addr.host,
                           players[i].net.addr.port, i, players[i].gfx_nr, players[i].team_nr,
                           players[i].net.flags);
    }

    /*
     * check if we have to update someones data
     */
    else if ((GT_MP_PTPM && p_id->pl_nr >= 0 && p_id->pl_nr < MAX_PLAYERS &&
              p_id->pl_nr != bman.p_nr && p_id->pl_nr != bman.p2_nr && bman.state == GS_wait)
             || (GT_MP_PTPS && p_id->pl_nr >= 0 && p_id->pl_nr < MAX_PLAYERS)) {

        pl = &players[p_id->pl_nr];

        pl->net.timestamp = timestamp;
        pl->net.pingreq = pl->net.pingack + 5;
        if (GT_MP_PTPS)
            pl->net.flags = p_id->netflags;
        if (p_id->host[0] != 0 && p_id->pl_nr != bman.p_servnr) { // copy addr only if p_id != server
            pl->net.addr.pl_nr = addr->pl_nr;
            strncpy (pl->net.addr.host, p_id->host, LEN_SERVERNAME);
            strncpy (pl->net.addr.port, p_id->port, LEN_PORT);
            dns_filladdr (pl->net.addr.host, LEN_SERVERNAME,
                          pl->net.addr.port, LEN_PORT, bman.net_ai_family, &pl->net.addr.sAddr);
        }
        /* player is used, we need to check if it's the second player from the server
         * to set it's host and port name */
        else if (PS_IS_used (p_id->state) && p_id->pl_nr != bman.p_servnr) { // copy addr only if p_id != server
            strncpy (pl->net.addr.host, players[bman.p_servnr].net.addr.host, LEN_SERVERNAME);
            strncpy (pl->net.addr.port, players[bman.p_servnr].net.addr.port, LEN_PORT);
            memcpy (&pl->net.addr.sAddr, &players[bman.p_servnr].net.addr.sAddr,
                    sizeof (struct _sockaddr));
        }

        /* Check if we have to make a network test.. only client to client 
         * we won't check 2 players too because it won't be possible to send
         * something to them. 
         *
         * Check only as long as pl->state is still not a network player */
        if (GT_MP_PTPS && !(PS_IS_netplayer (pl->state)) && (PS_IS_netplayer (p_id->state))
            && p_id->pl_nr != bman.p_servnr && p_id->pl_nr != bman.p_nr
            && !(pl->net.flags & NETF_local2) && p_id->pl_nr != bman.p2_nr) {

            send_contest (&pl->net.addr, bman.p_nr, -1, 0); // send contest without ackreq.
            /* make sure we still get messages from the server, this is a 
             * work around for some hardware routers */
            send_contest (&players[bman.p_servnr].net.addr, bman.p_nr, -1, 1);
        }
		
		/* to make sure that in games the client won't reborn without a reason */
		if (bman.state != GS_running || PS_IS_alife (pl->state) == 1)
			pl->state = p_id->state;
		else
			pl->state = p_id->state & (0xFF - PSF_alife);

		/* set the NETWORK flag for the network player.
		 * this had to be done after the contest part. */
        if (p_id->pl_nr != bman.p_nr && PS_IS_used (pl->state))
            pl->state |= PSF_net;
        else
            pl->state &= (0xff - PSF_net);

		/* make sure we won't end up in an infinite loop, because of a slow network.
		 * only accept the gfx data from other players. Our own we will not overwrite.*/
		if ((bman.state != GS_wait
			|| (p_id->pl_nr != bman.p_nr && p_id->pl_nr != bman.p2_nr))
			&& pl->gfx_nr != p_id->gfx_nr)
	            player_set_gfx (pl, p_id->gfx_nr);
        strncpy (pl->name, p_id->name, LEN_PLAYERNAME);

        if (GT_MP_PTPS) {
            pl->points = NTOH16 (p_id->points);
            pl->nbrKilled = NTOH16 (p_id->nbrKilled);
            pl->wins = NTOH16 (p_id->wins);
            pl->team_nr = p_id->team_nr;
            if (pl->team_nr >= 0 && pl->team_nr < MAX_TEAMS) {
                teams[pl->team_nr].points = NTOH16 (p_id->team_points);
                teams[pl->team_nr].wins = NTOH16 (p_id->team_wins);
            }
        }

        /* Send all connected players the new PlayerID */
        if (GT_MP_PTPM && GS_WAITRUNNING && addr->pl_nr >= 0 && addr->pl_nr < MAX_PLAYERS)
            net_send_playerid (addr->pl_nr);
    }

    /*
     * if we are already in a game don't let the player change the gfx */
    else if (GT_MP_PTPM && p_id->pl_nr >= 0 && p_id->pl_nr < MAX_PLAYERS &&
             p_id->pl_nr != bman.p_nr && p_id->pl_nr != bman.p2_nr && bman.state != GS_wait) {
        /* Send all connected players the new PlayerID */
        if (GT_MP_PTPM && GS_WAITRUNNING && addr->pl_nr >= 0 && addr->pl_nr < MAX_PLAYERS)
            net_send_playerid (addr->pl_nr);
    }

    //  d_playerdetail ("*** PLAYER List ***");
    team_update ();
    bman.updatestatusbar = 1;
};


void
send_playerid (_net_addr * addr, char *name, char *pladdr, char *plport,
               int pl_nr, int gfx_nr, int team_nr, signed char netflags)
{
    struct pkg_playerid p_id;

    d_printf ("send_playerid SendTo: %s:%s (Name:%16s p_nr:%d)\n", addr->host,
              addr->port, name, pl_nr);

    p_id.h.typ = PKG_playerid;
    p_id.h.flags = PKGF_ackreq;
    p_id.h.len = HTON16 (sizeof (struct pkg_playerid));

    if (name != NULL)
        strncpy (p_id.name, name, LEN_PLAYERNAME);
    else
        p_id.name[0] = 0;

    if (pladdr == NULL)
        p_id.host[0] = 0;
    else
        strncpy (p_id.host, pladdr, LEN_SERVERNAME);

    if (plport == NULL)
        p_id.port[0] = 0;
    else
        strncpy (p_id.port, plport, LEN_PORT);

    p_id.pl_nr = pl_nr;
    p_id.netflags = netflags;
    p_id.gfx_nr = gfx_nr;
    if (pl_nr != -1) {
        p_id.points = HTON16 (players[pl_nr].points);
        p_id.wins = HTON16 (players[pl_nr].wins);
        p_id.nbrKilled = HTON16 (players[pl_nr].nbrKilled);
        p_id.state = players[pl_nr].state;
        p_id.team_nr = team_nr;
        if (team_nr >= 0 && team_nr < MAX_TEAMS) {
            p_id.team_points = teams[team_nr].points;
            p_id.team_wins = teams[team_nr].wins;
        }
    }
    else {
        p_id.points = 0;
        p_id.wins = 0;
        p_id.nbrKilled = 0;
        p_id.state = 0;
        p_id.team_nr = -1;
        p_id.team_points = 0;
        p_id.team_wins = 0;
    }

    send_pkg ((struct pkg *) &p_id, addr);
};



/***
 *** Packettype: teamdata
 *** Server Side:
 ***    Send all teamdata to the client.
 *** Client Side:
 ***    Get all Teamdata
 ***/
void
do_teamdata (struct pkg_teamdata *td, _net_addr * addr)
{
    int i;

    if (addr->pl_nr == -1)
        return;

    d_printf ("do_teamdata (addr->pl_nr: %d): team:%d col:%d wins:%d\n", addr->pl_nr, td->team_nr, td->col, td->wins);

    if (addr->pl_nr == bman.p_servnr) { /* packet comes from the server */
        if (td->team_nr >= 0 && td->team_nr < MAX_TEAMS) {
            strncpy (teams[td->team_nr].name, td->name, LEN_PLAYERNAME);
            teams[td->team_nr].col = td->col;
            teams[td->team_nr].wins = td->wins;
            bman.updatestatusbar = 1;
        }
    }
    else {
        if (td->team_nr < 0 || td->team_nr >= MAX_TEAMS) {
            for (i = 0; i < MAX_TEAMS; i++)
                send_teamdata (addr, i);
        }
        else {
            send_teamdata (addr, td->team_nr);
        }
    }
}


void
send_teamdata (_net_addr * addr, int team_nr)
{
    struct pkg_teamdata td;

    d_printf ("send_teamdata (%s:%s) team:%d\n", addr->host, addr->port, team_nr);

    td.h.typ = PKG_teamdata;
    td.h.flags = PKGF_ackreq;
    td.h.len = HTON16 (sizeof (struct pkg_teamdata));

    td.team_nr = team_nr;
    if (team_nr >= 0 && team_nr < MAX_PLAYERS) {
        strncpy (td.name, teams[team_nr].name, LEN_PLAYERNAME);
        td.wins = teams[team_nr].wins;
        td.col = teams[team_nr].col;
    }
    else {
        td.name[0] = 0;
        td.wins = 0;
        td.col = 0;
    }
    send_pkg ((struct pkg *) &td, addr);
}


/***
 *** Packettype: servermode
 ***/
void
do_servermode (struct pkg_servermode *s_mod, _net_addr * addr)
{
    if (addr->pl_nr == -1)
        return;

    d_printf ("do_servermode (%s:%s) state = %d\n", addr->host, addr->port, s_mod->state);

    /* if we just have connected the bman.p_nr is still -1, so we handle the
       servermode packet still alittle diffrent */
    if ((!s_mod->lplayer2 && bman.p_nr == -1) || (s_mod->lplayer2 && bman.p2_nr == -1)) {
        d_printf ("     Server gave us: p_nr/p2_nr: %d(old:%d), p_servnr: %d(old:%d), lplayer2:%d\n", s_mod->p_nr,
                  bman.p_nr, s_mod->p_servnr, bman.p_servnr, s_mod->lplayer2);

        /* set the p_servnr to the playerslot which is the server */
        if (bman.p_servnr != s_mod->p_servnr) {
            memcpy (&players[s_mod->p_servnr].net, &players[bman.p_servnr].net,
                    sizeof (_net_player));
            memset (&players[bman.p_servnr], '\0', sizeof (_net_player));
            bman.p_servnr = s_mod->p_servnr;
        }

        /* set now the new p_nr number */
        if (s_mod->p_nr >= 0 && s_mod->p_nr < MAX_PLAYERS && bman.p_nr == -1
            && s_mod->lplayer2 == 0) {
            bman.p_nr = s_mod->p_nr;
            bman.firewall = 1;
            players[bman.p_nr].net.flags = NETF_firewall;
            players[bman.p_nr].state &= (0xFF - PSF_net);
            strncpy (players[s_mod->p_nr].name, bman.playername, LEN_PLAYERNAME);

            /* send playerid with p_nr -1 so we get the whole playerlist
             * do the same with the teamdata */
            send_playerid (addr, NULL, NULL, NULL, -1, -1, -1, 0);
            send_teamdata (addr, -1);
        }
        else if (s_mod->p_nr >= 0 && s_mod->p_nr < MAX_PLAYERS && bman.p2_nr == -1
                 && s_mod->lplayer2 == 1) {
            bman.p2_nr = s_mod->p_nr;
            players[bman.p2_nr].net.flags = NETF_firewall + NETF_local2;
            players[bman.p2_nr].state &= (0xFF - PSF_net);
            strncpy (players[s_mod->p_nr].name, bman.player2name, LEN_PLAYERNAME);
            send_playerid (addr, NULL, NULL, NULL, -1, -1, -1, 0);
            send_teamdata (addr, -1);
        }
    }

    /* do the normal update */
    if (GT_MP_PTPS && addr->pl_nr == bman.p_servnr) {
        bman.state = s_mod->state;
        bman.gametype = s_mod->gametype;
        bman.dropitemsondeath = s_mod->dropitemsondeath;
        map.state = s_mod->mapstate;

        bman.players_nr_s = s_mod->players;
        bman.lastwinner = s_mod->last_winner;

        map.size.x = s_mod->fieldsize_x;
        map.size.y = s_mod->fieldsize_y;

        strncpy (map.tileset, s_mod->tileset, LEN_TILESETNAME);
    }

    bman.updatestatusbar = 1;
};


void
send_servermode (_net_addr * addr, int pl_nr)
{
    struct pkg_servermode s_mod;
	
    switch (bman.state) {
    case (GS_startup):
        d_printf ("Send ServerMode : startup");
        break;
    case (GS_ready):
        d_printf ("Send ServerMode : ready");
        break;
    case (GS_running):
        d_printf ("Send ServerMode : running");
        break;
    case (GS_quit):
        d_printf ("Send ServerMode : quit");
        break;
    case (GS_wait):
        d_printf ("Send ServerMode : wait");
        break;
    case (GS_update):
        d_printf ("Send ServerMode : update");
        break;
    default:
        d_printf ("Send ServerMode : mode %d", bman.state);
    }

	d_printf ("Addr: %p\n", addr);
	
    s_mod.h.typ = PKG_servermode;
    s_mod.h.len = HTON16 (sizeof (struct pkg_servermode));
    s_mod.h.flags = PKGF_ackreq;
    s_mod.type = bman.gametype;
    s_mod.dropitemsondeath = bman.dropitemsondeath;
    s_mod.mapstate = map.state;
    if (bman.state == GS_quit)  /* do not send GS_quit */
        s_mod.state = GS_startup;
    else
        s_mod.state = bman.state;
    s_mod.gametype = bman.gametype;
    s_mod.players = bman.players_nr_s;
    s_mod.maxplayer = bman.maxplayer;
    s_mod.p_nr = pl_nr;
    if (pl_nr >= 0 && pl_nr < MAX_PLAYERS
        && (players[pl_nr].net.flags & NETF_local2) == NETF_local2)
        s_mod.lplayer2 = 1;
    else
        s_mod.lplayer2 = 0;
    s_mod.p_servnr = bman.p_servnr;
    s_mod.last_winner = bman.lastwinner;
    s_mod.fieldsize_x = map.size.x;
    s_mod.fieldsize_y = map.size.y;
    strncpy (s_mod.tileset, map.tileset, LEN_TILESETNAME);
    send_pkg ((struct pkg *) &s_mod, addr);
};



/***
 *** Packettype: field
 ***/
void
send_field (_net_addr * addr, int x, int y, _field * field)
{
    struct pkg_field f_dat;
    int i;

    d_printf ("send_field [%d,%d]\n", x, y);

    f_dat.h.typ = PKG_field;
    f_dat.h.len = HTON16 (sizeof (struct pkg_field));
    f_dat.h.flags = PKGF_ackreq;

    f_dat.x = x;
    f_dat.y = y;

    if (x < 0 || x >= map.size.x || y < 0 || y >= map.size.y)
        return;

    for (i = 0; i < 4; i++) {
        f_dat.ex[i].count = map.field[x][y].ex[i].count;
        f_dat.ex[i].frame = (int) map.field[x][y].ex[i].frame;
        f_dat.ex[i].bomb_p = map.field[x][y].ex[i].bomb_p;
        f_dat.ex[i].bomb_b = map.field[x][y].ex[i].bomb_b;
    }
    f_dat.type = map.field[x][y].type;
    f_dat.mixframe = map.field[x][y].mixframe;
    f_dat.special = map.field[x][y].special;
    f_dat.frame = HTON16 (FTOI16 (map.field[x][y].frame));
    f_dat.ex_nr = HTON32 (map.field[x][y].ex_nr);

    send_pkg ((struct pkg *) &f_dat, addr);
};


void
do_field (struct pkg_field *f_dat, _net_addr * addr)
{
    int i;

    if (addr->pl_nr == -1)
        return;

    if (f_dat->x < map.size.x && f_dat->y < map.size.y) {
        /* convert the fielddata */
        map.field[f_dat->x][f_dat->y].type = f_dat->type; // CHECK_BFIELD::: FOR BOMBS:::::
        map.field[f_dat->x][f_dat->y].mixframe = f_dat->mixframe;
        map.field[f_dat->x][f_dat->y].special = f_dat->special;
        for (i = 0; i < 4; i++) { /* set the explosion field data */
            map.field[f_dat->x][f_dat->y].ex[i].count = f_dat->ex[i].count;
            map.field[f_dat->x][f_dat->y].ex[i].frame = f_dat->ex[i].frame;
            map.field[f_dat->x][f_dat->y].ex[i].bomb_p = f_dat->ex[i].bomb_p;
            map.field[f_dat->x][f_dat->y].ex[i].bomb_b = f_dat->ex[i].bomb_b;
        }
        map.field[f_dat->x][f_dat->y].frame = I16TOF (NTOH16 (f_dat->frame));
        map.field[f_dat->x][f_dat->y].ex_nr = NTOH32 (f_dat->ex_nr);
    }
    if (bman.state == GS_running)
        stonelist_add (f_dat->x, f_dat->y);

    if (NTOH32 (f_dat->ex_nr) > bman.last_ex_nr)
        bman.last_ex_nr = NTOH32 (f_dat->ex_nr);

    d_printf ("do_field (%d,%d) ex_nr = %d, special = %d, type = %d\n",
              f_dat->x, f_dat->y, NTOH32 (f_dat->ex_nr), f_dat->special, f_dat->type);
};


/***
 *** Packettype: pingack/pingreq
 ***/
void
do_ping (struct pkg_ping *p_dat, _net_addr * addr)
{
    if (addr->pl_nr == -1)
        return;

    if (p_dat->h.typ == PKG_pingack)
        /* ping was an answer */
        players[addr->pl_nr].net.pingack = NTOH32 (p_dat->data);

    else
        /* send the answer */
        send_ping (addr, NTOH32 (p_dat->data), PKG_pingack);

    d_printf ("do_ping pl_nr[%d] (%s:%s)   req=%d, ack=%d\n", addr->pl_nr,
              addr->host, addr->port, players[addr->pl_nr].net.pingreq,
              players[addr->pl_nr].net.pingack);
};

void
send_ping (_net_addr * addr, int data, unsigned char typ)
{
    struct pkg_ping p_dat;

    if (addr->pl_nr == -1)
        return;

    p_dat.h.len = HTON16 (sizeof (struct pkg_ping));
    p_dat.h.flags = 0;

    if (typ == PKG_pingack) {
        p_dat.h.typ = typ;
        p_dat.data = HTON32 (data);
        send_pkg ((struct pkg *) &p_dat, addr);
    }

    else if (typ == PKG_pingreq) {
        p_dat.h.typ = typ;
        p_dat.data = HTON32 (data);
        send_pkg ((struct pkg *) &p_dat, addr);
    }

    d_printf ("send_ping Player[%d] (%s:%s)   req=%d, ack=%d\n", addr->pl_nr,
              addr->host, addr->port, players[addr->pl_nr].net.pingreq,
              players[addr->pl_nr].net.pingack);

    players[addr->pl_nr].net.timestamp = timestamp; /* we need to set it here, so we can check
                                                       for the timeout again */
};


/***
 *** Packettype: playerdata
 ***/
void
send_playerdata (_net_addr * addr, int p_nr, _player * pl)
{
    struct pkg_playerdata p_dat;

    p_dat.h.typ = PKG_playerdata;
    p_dat.h.len = HTON16 (sizeof (struct pkg_playerdata));

    if (bman.state == GS_update)
        p_dat.h.flags = 0;
    else
        p_dat.h.flags = PKGF_ackreq;

    p_dat.pos.x = HTON16 (FTOI16 (pl->pos.x));
    p_dat.pos.y = HTON16 (FTOI16 (pl->pos.y));
    p_dat.bombs_n = pl->bombs_n;
    p_dat.d = pl->d;
    p_dat.range = pl->range;
    p_dat.gfx_nr = pl->gfx_nr;
    p_dat.state = pl->state;
    p_dat.wins = HTON16 (pl->wins);
    p_dat.points = HTON16 (pl->points);
    p_dat.nbrKilled = HTON16 (pl->nbrKilled);
    p_dat.dead_by = pl->dead_by;
    p_dat.frame = HTON16 (FTOI16 (pl->frame));
    p_dat.p_nr = p_nr;
    p_dat.ready = pl->ready;
    p_dat.team_nr = pl->team_nr;

    send_pkg ((struct pkg *) &p_dat, addr);
};


void
do_playerdata (struct pkg_playerdata *p_dat, _net_addr * addr)
{
    _player *pl;

    if (addr->pl_nr == -1)
        return;

    if (p_dat->p_nr < 0 || p_dat->p_nr >= MAX_PLAYERS)
        return;

    d_printf ("do_playerdata (From: %d) p_nr: %d --> state alife? %d new %d\n", addr->pl_nr, p_dat->p_nr, PS_IS_alife(players[p_dat->p_nr].state) , PS_IS_alife(p_dat->state));
    bman.updatestatusbar = 1;   // force an update

    pl = &players[p_dat->p_nr];

    if (bman.state == GS_running && bman.p_nr != p_dat->p_nr) {
        pl->points = NTOH16 (p_dat->points);
        pl->nbrKilled = NTOH16 (p_dat->nbrKilled);
        pl->dead_by = NTOH16 (p_dat->dead_by);
        pl->team_nr = p_dat->team_nr;
    }
    else if (bman.state != GS_running || bman.p_nr != p_dat->p_nr) {
        pl->pos.x = I16TOF (NTOH16 (p_dat->pos.x));
        pl->pos.y = I16TOF (NTOH16 (p_dat->pos.y));
        pl->dead_by = p_dat->dead_by;
        pl->nbrKilled = NTOH16 (p_dat->nbrKilled);
        pl->points = NTOH16 (p_dat->points);
        pl->d = p_dat->d;
        pl->bombs_n = p_dat->bombs_n;
        pl->range = p_dat->range;
        pl->frame = I16TOF (NTOH16 (p_dat->frame));
        pl->state = p_dat->state;
        pl->team_nr = p_dat->team_nr;
        pl->ready = p_dat->ready;
        team_update ();
    }

    if (p_dat->dead_by < 0 || p_dat->dead_by >= MAX_PLAYERS) /* set player state no matter what */
        pl->state = p_dat->state;
    else {                      /* player died in the game */
        /* check if the player just died */
        if (PS_IS_alife (pl->state) && PS_IS_dead (p_dat->state)) {
            /* player just died */
                player_died (pl, p_dat->dead_by, 1);
            }
        pl->state = p_dat->state;
    }

    /* set the state of the player */
    if (p_dat->p_nr != bman.p_nr && PS_IS_used (pl->state))
        pl->state |= PSF_net;
    else
        pl->state &= (0xff - PSF_net);

    if (p_dat->gfx_nr == -1)
        pl->state &= (0xff - PSF_alife);

    if (bman.state == GS_update && PS_IS_used (p_dat->state))
        bman.players_nr_s++;

    if (players[bman.p_nr].net.net_istep == 1)
        players[bman.p_nr].net.net_status = p_dat->p_nr;
	
    player_set_gfx (pl, p_dat->gfx_nr);
}


/***
 *** Packettype: ill
 ***/
void
do_ill (struct pkg_ill *ill, _net_addr * addr)
{
    int i,
      old_to;

    if (addr->pl_nr == -1)
        return;

    d_printf ("do_ill (From: %d) For Player %d\n", addr->pl_nr, ill->pl_nr);
    if (ill->pl_nr < 0 || ill->pl_nr >= MAX_PLAYERS || ill->pl_nr == bman.p_nr)
        return;

    for (i = 0; i < PI_max; i++) {
        old_to = players[ill->pl_nr].ill[i].to;
        players[ill->pl_nr].ill[i].to = I32TOF (NTOH32 (ill->to[i]));
        if (players[ill->pl_nr].ill[i].to <= 0.0f && old_to > 0.0f)
            player_clear_ilness (&players[ill->pl_nr], i);
        else if (players[ill->pl_nr].ill[i].to > 0.0f && old_to <= 0.0f)
            player_set_ilness (&players[ill->pl_nr], i, players[ill->pl_nr].ill[i].to);
        d_printf ("    to[%d]:%f dataf:%f datai:%d\n", i, players[ill->pl_nr].ill[i].to,
                  players[ill->pl_nr].ill[i].dataf, players[ill->pl_nr].ill[i].datai);
    }
};

void
send_ill (_net_addr * addr, int p_nr, _player * pl)
{
    struct pkg_ill ill;
    int i;

    ill.h.typ = PKG_ill;
    ill.h.len = HTON16 (sizeof (struct pkg_ill));
    ill.h.flags = PKGF_ackreq;

    ill.pl_nr = p_nr;
    for (i = 0; i < PI_max; i++)
        ill.to[i] = HTON32 (FTOI32 (pl->ill[i].to));

    send_pkg ((struct pkg *) &ill, addr);
};


/***
 *** Packettype: playermove
 ***/
void
send_playermove (_net_addr * addr, int p_nr, _player * pl)
{
    struct pkg_playermove p_dat;

    p_dat.h.typ = PKG_playermove;
    p_dat.h.len = HTON16 (sizeof (struct pkg_playermove));
    p_dat.h.flags = 0;

    p_dat.pos.x = HTON16 (FTOI16 (pl->pos.x));
    p_dat.pos.y = HTON16 (FTOI16 (pl->pos.y));
    p_dat.m = pl->m;
    p_dat.d = pl->d;
    p_dat.p_nr = p_nr;
    p_dat.speed = HTON16 (FTOI16 (pl->speed));
    p_dat.tunnelto = HTON16 (FTOI16 (pl->tunnelto));

    send_pkg ((struct pkg *) &p_dat, addr);
};

void
do_playermove (struct pkg_playermove *p_dat, _net_addr * addr)
{
    _player *pl;

    if (addr->pl_nr == -1)
        return;

    if (p_dat->p_nr == -1)
        return;

    if (addr->pl_nr == -1)      // unknown player
        return;

    /* check if the right player is sending the information */
    pl = &players[p_dat->p_nr];

    pl->m = p_dat->m;
    pl->d = p_dat->d;
    pl->speed = I16TOF (NTOH16 (p_dat->speed));
    pl->pos.x = I16TOF (NTOH16 (p_dat->pos.x));
    pl->pos.y = I16TOF (NTOH16 (p_dat->pos.y));
    pl->tunnelto = I16TOF (NTOH16 (p_dat->tunnelto));
}


/***
 *** Packettype: bombdata
 ***/
void do_bombdata (struct pkg_bombdata *b_dat, _net_addr * addr) {
    _bomb *bomb;

    if (addr->pl_nr == -1)
        return;

    if (b_dat->p_nr >= MAX_PLAYERS || b_dat->b_nr >= MAX_BOMBS) {
        d_printf ("Bomb Error\n");
        return;
    }

    if (b_dat->state == BS_off)
        return;                 // if there was a bomb let it explose don't delete the bomb

	d_printf ("do_bombdata [%f,%f] Player: %d PlayerIgnition: %d Bomb: %d, ex_nr:%d\n",
              I16TOF (NTOH16 (b_dat->x)), I16TOF (NTOH16 (b_dat->y)), b_dat->p_nr, b_dat->pi_nr, b_dat->b_nr, NTOH32 (b_dat->ex_nr));

    bomb = &players[b_dat->p_nr].bombs[b_dat->b_nr];
    if (bomb->state == BS_exploding) {
        d_printf ("do_bombdata ---> bomb is already exploding\n");
        return;
    }
    // Update player ignition
    bomb->id.pIgnition = b_dat->pi_nr;
    if ((bomb->pos.x != NTOH16 (b_dat->x) || bomb->pos.y != NTOH16 (b_dat->y))
        && bomb->state == BS_exploding && b_dat->state != BS_exploding)
        d_printf ("do_bombdata  WARNING : bomb explosion haven't finished\n");

    if (bomb->state != BS_off) { // handle push & kick special
        map.bfield[(int) bomb->pos.x][(int) bomb->pos.y] = 0; //remove bomb at old location
        stonelist_add (bomb->pos.x, bomb->pos.y);
        stonelist_add (bomb->pos.x + 1, bomb->pos.y);
        stonelist_add (bomb->pos.x, bomb->pos.y + 1);
        stonelist_add (bomb->pos.x + 1, bomb->pos.y + 1);
    }

    if (bomb->state == BS_off && (b_dat->state == BS_ticking || b_dat->state == BS_trigger))
        snd_play (SND_bombdrop);

    /* convert position back to float */
    bomb->pos.x = I16TOF (NTOH16 (b_dat->x));
    bomb->pos.y = I16TOF (NTOH16 (b_dat->y));

    if (bomb->state != b_dat->state && bomb->state != BS_ticking)
        bomb->to = I32TOF (NTOH32 (b_dat->to)); /* only set if the bomb isn't already ticking
                                                   to make sure the timeout won't be resetted
                                                   by an old network packet */

    map.bfield[(int) bomb->pos.x][(int) bomb->pos.y] = 1; // keep the bfield up to date

    bomb->r = b_dat->r;
    bomb->ex_nr = NTOH32 (b_dat->ex_nr);
    bomb->state = b_dat->state & 0x0F;
	
	if (bomb->mode != b_dat->state >> 4) {		// bombmode changed set source to current position
		bomb->source.x = bomb->pos.x;
		bomb->source.y = bomb->pos.y;
	}
    bomb->mode = b_dat->state >> 4;
    bomb->fdata = I16TOF (NTOH16 (b_dat->fdata));
    bomb->dest.x = I16TOF (NTOH16 (b_dat->destx));
    bomb->dest.y = I16TOF (NTOH16 (b_dat->desty));

    if (bomb->state == BS_exploding)
        bomb_explode (bomb, 0);

    if (bomb->ex_nr > bman.last_ex_nr)
        bman.last_ex_nr = bomb->ex_nr;
};

void
send_bombdata (_net_addr * addr, int p, int b, _bomb * bomb)
{
    struct pkg_bombdata b_dat;

    b_dat.h.typ = PKG_bombdata;
    b_dat.h.len = HTON16 (sizeof (struct pkg_bombdata));
    b_dat.x = HTON16 (FTOI16 (bomb->pos.x));
    b_dat.y = HTON16 (FTOI16 (bomb->pos.y));
    b_dat.to = HTON32 (FTOI32 (bomb->to));
    b_dat.r = bomb->r;
    b_dat.ex_nr = HTON32 (bomb->ex_nr);
    b_dat.state = (bomb->mode << 4) | (bomb->state);
    b_dat.b_nr = b;
    b_dat.p_nr = p;
    b_dat.pi_nr = bomb->id.pIgnition;
    b_dat.h.flags = PKGF_ackreq;
    b_dat.fdata = HTON16 (FTOI16 (bomb->fdata));
    b_dat.destx = HTON16 (FTOI16 (bomb->dest.x));
    b_dat.desty = HTON16 (FTOI16 (bomb->dest.y));

    send_pkg ((struct pkg *) &b_dat, addr);
};


/***
 *** Packettype: bombdata
 *** recive a request for some tunneldata or receive tunneldata
 ***/
void
do_tunneldata (struct pkg_tunneldata *tun_pkg, _net_addr * addr)
{
    d_printf ("do_tunneldata: From %d [%s:%s] (Tunnel %d Target [%d,%d])\n",
              addr->pl_nr, addr->host, addr->port, tun_pkg->tunnel_nr,
              NTOH16 (tun_pkg->target.x), NTOH16 (tun_pkg->target.y));

    if (addr->pl_nr != bman.p_servnr && GT_MP_PTPM && NTOH16 (tun_pkg->target.y) == -1
        && NTOH16 (tun_pkg->target.x) == -1) {
        send_tunneldata (addr, tun_pkg->tunnel_nr,
                         map.tunnel[tun_pkg->tunnel_nr].x, map.tunnel[tun_pkg->tunnel_nr].y);
        players[addr->pl_nr].net.net_status = tun_pkg->tunnel_nr;
        players[addr->pl_nr].net.net_istep = 3;
    }
    else if (addr->pl_nr == bman.p_servnr && tun_pkg->tunnel_nr < GAME_MAX_TUNNELS) {
        map.tunnel[tun_pkg->tunnel_nr].x = NTOH16 (tun_pkg->target.x);
        map.tunnel[tun_pkg->tunnel_nr].y = NTOH16 (tun_pkg->target.y);
        players[bman.p_nr].net.net_status = tun_pkg->tunnel_nr;
    }
};

/* send a tunneldata request (x && y == -1) or send tunneldata */
void
send_tunneldata (_net_addr * addr, int tunnelnr, int x, int y)
{
    struct pkg_tunneldata tun_pkg;

    d_printf ("send_tunneldata (Tunnel %d Target [%d,%d])\n", tunnelnr, x, y);

    tun_pkg.h.typ = PKG_tunneldata;
    tun_pkg.h.flags = PKGF_ackreq;
    tun_pkg.h.len = HTON16 (sizeof (struct pkg_tunneldata));

    if (GT_MP_PTPM || (GT_MP_PTPS && x == -1 && y == -1)) {
        tun_pkg.tunnel_nr = tunnelnr;
        tun_pkg.target.x = HTON16 (x);
        tun_pkg.target.y = HTON16 (y);

        send_pkg ((struct pkg *) &tun_pkg, addr);
    }
};


/***
 *** Packettype: quit
 ***/
void
send_quit (_net_addr * addr, int pl_nr, int new_server)
{
    struct pkg_quit q_dat;

    d_printf ("send_quit (%s:%s) pl_nr: %d\n", addr->host, addr->port, pl_nr);

    q_dat.h.typ = PKG_quit;
    q_dat.h.flags = 0;
    q_dat.h.len = HTON16 (sizeof (struct pkg_quit));
    if (pl_nr == -1)
        q_dat.pl_nr = bman.p_nr;
    else
        q_dat.pl_nr = pl_nr;
    q_dat.new_server = new_server;
    send_pkg ((struct pkg *) &q_dat, addr);
};


void
do_quit (struct pkg_quit *q_dat, _net_addr * addr)
{
    d_printf ("do_quit (%s:%s) pl_nr=%d new_server=%d\n", addr->host, addr->port, q_dat->pl_nr,
              q_dat->new_server);

    if (addr->pl_nr == -1)
        return;
	
	if (q_dat->pl_nr == -1)
		q_dat->pl_nr = addr->pl_nr;

    bman.updatestatusbar = 1;
    player_delete (q_dat->pl_nr);

    /* the player who send this quit */
    if (q_dat->pl_nr == bman.p_servnr && q_dat->new_server != bman.p_servnr) {
        d_printf ("do_quit: new server is set to: %d\n", q_dat->new_server);
        bman.p_servnr = q_dat->new_server;

        if (GT_MP_PTPM && bman.notifygamemaster) {
            /* if there are any AI players delete the network flag from them */
            int i;

            for (i = 0; i < MAX_PLAYERS; i++)
                if (PS_IS_aiplayer (players[i].state))
                    players[i].state &= (0xFF - PSF_net);

            send_ogc_update ();
        }
    }
    else if (q_dat->pl_nr == bman.p_servnr && q_dat->new_server == bman.p_servnr)
        menu_displaymessage ("Server Quit",
                             "The game closed because you are the only one who is left."
                             " Or the server could not find any other possible new server.");
};


/***
 *** Packettype: getfield
 ***/
void
send_getfield (_net_addr * addr, int line)
{
    struct pkg_getfield gf_dat;

    gf_dat.h.typ = PKG_getfield;
    gf_dat.h.len = HTON16 (sizeof (struct pkg_getfield));
    gf_dat.h.flags = 0;
    gf_dat.line = line;
    send_pkg ((struct pkg *) &gf_dat, addr);
};


void
do_getfield (struct pkg_getfield *gf_dat, _net_addr * addr)
{
    if (addr->pl_nr == -1)
        return;

    if (gf_dat->line < 0 || gf_dat->line >= MAX_FIELDSIZE_Y)
        return;

    if (addr->pl_nr != -1 && bman.state == GS_update && GT_MP_PTPM) {
        if (addr->pl_nr != bman.p_servnr && addr->pl_nr < MAX_PLAYERS) {
            players[addr->pl_nr].net.net_status = gf_dat->line;
            players[addr->pl_nr].net.net_istep = 2;
        }
    }
    send_fieldline (addr, gf_dat->line);
};


/***
 *** Packettype: fieldline
 ***/
void
send_fieldline (_net_addr * addr, int line)
{
    int i,
      j;
    struct pkg_fieldline f_dat;

    f_dat.h.typ = PKG_fieldline;
    f_dat.h.len = HTON16 (sizeof (struct pkg_fieldline));
    f_dat.h.flags = 0;
    f_dat.line = line;

    if (line < 0 || line >= MAX_FIELDSIZE_Y)
        return;

    for (i = 0; i < MAX_FIELDSIZE_X; i++) {
        f_dat.type[i] = map.field[i][line].type;
        f_dat.special[i] = map.field[i][line].special;
        map.field[i][line].frame = 0.0f;
        map.field[i][line].ex_nr = 0;
        for (j = 0; j < 4; j++) {
            map.field[i][line].ex[j].frame = 0.0f;
            map.field[i][line].ex[j].count = 0;
        }
    }
    send_pkg ((struct pkg *) &f_dat, addr);
};


void
do_fieldline (struct pkg_fieldline *f_dat, _net_addr * addr)
{
    int i,
      d;

    if (addr->pl_nr == -1)
        return;

    if (addr->pl_nr != bman.p_servnr) {
        /* the data we have got are not from the server */
        d_printf ("do_fieldline: the data we have got are not from the server\n");
        return;
    }
    if (f_dat->line < 0 || f_dat->line >= MAX_FIELDSIZE_Y) {
        /* the line number is wrong */
        d_printf ("do_fieldline: the line number is not correct\n");
        return;
    }

    if (players[bman.p_nr].net.net_istep == 2)
        players[bman.p_nr].net.net_status = f_dat->line;

    for (i = 0; i < MAX_FIELDSIZE_X; i++) {
        map.field[i][f_dat->line].type = f_dat->type[i];
        map.field[i][f_dat->line].special = f_dat->special[i];
        map.field[i][f_dat->line].frame = 0.0f;
        for (d = 0; d < 4; d++) {
            map.field[i][f_dat->line].ex[d].frame = 0.0f;
            map.field[i][f_dat->line].ex[d].count = 0;
        }
    }
};


/***
 *** Packettype: getplayerdata
 ***/
void
send_getplayerdata (_net_addr * addr, int pl)
{
    struct pkg_getplayerdata gp_dat;

    gp_dat.h.typ = PKG_getplayerdata;
    gp_dat.h.len = HTON16 (sizeof (struct pkg_getplayerdata));
    gp_dat.pl_nr = pl;
    gp_dat.h.flags = 0;
    send_pkg ((struct pkg *) &gp_dat, addr);
};


void
do_getplayerdata (struct pkg_getplayerdata *gp_dat, _net_addr * addr)
{
    if (addr->pl_nr == -1)
        return;

    if (gp_dat->pl_nr < 0 || gp_dat->pl_nr >= MAX_PLAYERS)
        return;

    if (addr->pl_nr != -1 && bman.state == GS_update && GT_MP_PTPM) {
        if (addr->pl_nr != bman.p_servnr && addr->pl_nr < MAX_PLAYERS) {
            players[addr->pl_nr].net.net_status = gp_dat->pl_nr;
            players[addr->pl_nr].net.net_istep = 1;
        }
    }
    send_playerdata (addr, gp_dat->pl_nr, &players[gp_dat->pl_nr]);
};


/***
 *** Packettype: playerstatus
 ***/
void
do_playerstatus (struct pkg_playerstatus *stat, _net_addr * addr)
{
    d_printf ("do_playerstatus (%s,%s)\n", addr->host, addr->port);

    if (addr->pl_nr != bman.p_servnr && !(GT_MP_PTPM)) {
        /* the data we have got are not from the server */
        d_printf ("do_playerstatus: the data we have got are not from the server\n");
        return;
    }
    if (stat->pl_nr < 0 || stat->pl_nr >= MAX_PLAYERS) {
        /* the player number is wrong */
        d_printf ("do_playerstatus: playernumber not correct\n");
        return;
    }

    players[addr->pl_nr].net.net_status = stat->status;
    players[addr->pl_nr].net.net_istep = stat->net_istep;
    /*
       if (GT_MP_PTPM)
       for (i = 0; i < MAX_PLAYERS; i++)
       if (players[i].net.addr.host[0] != 0)
       send_playerstatus (addr, stat->pl_nr,
       stat->net_istep, stat->status);
     */
};

void
send_playerstatus (_net_addr * addr, int pl_nr, int net_istep, int status)
{
    struct pkg_playerstatus stat;

    d_printf ("send_playerstatus (%s,%s) %d, %d, %d\n", addr->host,
              addr->port, pl_nr, net_istep, status);

    stat.h.typ = PKG_playerstatus;
    stat.h.flags = 0;
    stat.h.len = HTON16 (sizeof (struct pkg_playerstatus));
    stat.pl_nr = pl_nr;
    stat.net_istep = net_istep;
    stat.status = status;

    send_pkg ((struct pkg *) &stat, addr);
};

/***
 *** Packettype: updateinfo
 ***/
void
do_updateinfo (struct pkg_updateinfo *stat, _net_addr * addr)
{
    int i;

    d_printf ("do_updateinfo (%s,%s)\n", addr->host, addr->port);

    if (addr->pl_nr != bman.p_servnr && !(GT_MP_PTPM)) {
        /* the data we have got are not from the server */
        d_printf ("do_updateinfo: the data we have got are not from the server\n");
        return;
    }
    for (i = 0; i < MAX_PLAYERS; i++)
        if ((i != bman.p_servnr) && (i != bman.p_nr)) {
            players[i].net.net_status = stat->status[i];
            players[i].net.net_istep = stat->step[i];
        }
};

void
send_updateinfo (_net_addr * addr)
{
    struct pkg_updateinfo stat;
    int i;
    stat.h.typ = PKG_updateinfo;
    stat.h.flags = 0;
    stat.h.len = HTON16 (sizeof (struct pkg_updateinfo));
    for (i = 0; i < MAX_PLAYERS; i++) {
        stat.step[i] = players[i].net.net_istep;
        stat.status[i] = players[i].net.net_status;
    }
    send_pkg ((struct pkg *) &stat, addr);
};


/***
 *** Packettype: chat
 ***/
void
do_chat (struct pkg_chat *chat_pkg, _net_addr * addr)
{
    d_printf ("do_chat (%s:%s) %d Text:%s\n", addr->host, addr->port, addr->pl_nr, chat_pkg->text);

    chat_addline (chat_pkg->text, -1);
};


void
send_chat (_net_addr * addr, char *text)
{
    struct pkg_chat chat_pkg;
    int i;

    chat_pkg.h.typ = PKG_chat;
    chat_pkg.h.flags = 0;
    chat_pkg.h.len = HTON16 (sizeof (struct pkg_chat));

    for (i = 0; i < sizeof (chat_pkg.text); i++)
        chat_pkg.text[i] = 0;
    strncpy (chat_pkg.text, text, sizeof (struct pkg_chat) - sizeof (struct pkg));

    send_pkg ((struct pkg *) &chat_pkg, addr);
};


/***
 *** Packettype: pkgack
 ***/
void
send_pkgack (_net_addr * addr, unsigned char typ, short int id)
{
    struct pkg_pkgack p_ack;

    p_ack.h.typ = PKG_pkgack;
    p_ack.h.flags = 0;
    p_ack.h.len = HTON16 (sizeof (struct pkg_pkgack));
    p_ack.id = HTON16 (id);
    p_ack.typ = typ;

    send_pkg ((struct pkg *) &p_ack, addr);
};


void
do_pkgack (struct pkg_pkgack *p_ack, _net_addr * addr)
{
    d_printf ("do_pkgack pl_nr:%d type:%u id:%u\n", addr->pl_nr, p_ack->typ, p_ack->id);
    if ( ! rscache_del (addr, p_ack->typ, NTOH16 (p_ack->id)))
		d_printf ("do_pkgack ERROR rscache_del data not found : pl_nr:%d type:%u id:%u\n", addr->pl_nr, p_ack->typ, p_ack->id);
};


/***
 *** Packettype: dropitems
 *** send a generated list of drop items
 ***/
void
send_dropitems (_net_addr * addr, int pl_nr, _flyingitem ** fitems, int cnt)
{
    char outdata[BUF_SIZE];     // this should be enough memory for the outgoin data
    struct pkg_dropitem *dipkg = (struct pkg_dropitem *) outdata; // set the pointer to outdata
    int i;

    dipkg->h.typ = PKG_dropitem;
    dipkg->h.len = HTON16 (sizeof (struct pkg_dropitem) + cnt * sizeof (struct pkgdropitemelemt));
    dipkg->h.flags = PKGF_ackreq;
    dipkg->pl_nr = pl_nr;
    dipkg->from.x = HTON16 (FTOI16 (players[pl_nr].pos.x));
    dipkg->from.y = HTON16 (FTOI16 (players[pl_nr].pos.y));
    dipkg->count = cnt;

    for (i = 0; (i < cnt && fitems[i] != NULL); i++) {
        dipkg->items[i].x = (int) fitems[i]->to.x;
        dipkg->items[i].y = (int) fitems[i]->to.y;
        dipkg->items[i].typ = (int) fitems[i]->type;
    }

    send_pkg ((struct pkg *) dipkg, addr);
};


void
do_dropitems (struct pkg_dropitem *di_pkg, _net_addr * addr)
{
    int i;
    _pointf from;
    _point to;

    d_printf ("do_dropitems from:%d (pl_nr %d, cnt %d)\n", addr->pl_nr, di_pkg->pl_nr,
              di_pkg->count);
    if (addr->pl_nr == -1)
        return;
    from.x = I16TOF (NTOH16 (di_pkg->from.x));
    from.y = I16TOF (NTOH16 (di_pkg->from.y));

    for (i = 0; i < di_pkg->count; i++) {
        to.x = di_pkg->items[i].x;
        to.y = di_pkg->items[i].y;
        flitems_additem (from, to, di_pkg->items[i].typ);
    }
};



/***
 *** Packettype: special
 *** moves/bombs... whatever will be send as we use it 
 ***/
void do_special (struct pkg_special *sp_pkg, _net_addr * addr) {
    d_printf ("do_special (addr %d, pl_nr %d, typ %d)\n", addr->pl_nr, sp_pkg->pl_nr, sp_pkg->typ);
    if (addr->pl_nr == -1 || sp_pkg->pl_nr == -1 || sp_pkg->pl_nr == bman.p_nr
        || sp_pkg->pl_nr == bman.p2_nr)
        return;

    /* set or use special */
    if (sp_pkg->typ < SP_max) {
        players[sp_pkg->pl_nr].special.type = sp_pkg->typ;
        bman.last_ex_nr = NTOH32 (sp_pkg->ex_nr);
        special_use (sp_pkg->pl_nr);
    }

    /* clear special */
    else if (sp_pkg->typ == SP_clear)
        players[sp_pkg->pl_nr].special.clear = 1;
};


void
send_special (_net_addr * addr, int p_nr, int typ, int ex_nr)
{
    struct pkg_special sp_dat;

    sp_dat.h.typ = PKG_special;
    sp_dat.h.len = HTON16 (sizeof (struct pkg_special));
    sp_dat.h.flags = PKGF_ackreq;
    sp_dat.pl_nr = p_nr;
    sp_dat.typ = typ;
    sp_dat.ex_nr = HTON32 (ex_nr);
    send_pkg ((struct pkg *) &sp_dat, addr);
};


/***
 *** Packettype: mapinfo
 ***/
void
send_mapinfo (_net_addr * addr)
{
    struct pkg_mapinfo map_pkg;
#ifndef BUG_MAPINFO
	_net_addr *test;			// VERY DIRTY WORKAROUND WHY IS HERE A BUG
#endif
    map_pkg.h.typ = PKG_mapinfo;
    map_pkg.h.len = HTON16 (sizeof (struct pkg_mapinfo));
    map_pkg.h.flags = PKGF_ackreq;
    map_pkg.size_x = map.size.x;
    map_pkg.size_y = map.size.y;
    map_pkg.bombs = map.bombs;
    map_pkg.fire = map.fire;
    map_pkg.shoes = map.shoes;
    map_pkg.mixed = map.mixed;
    map_pkg.death = map.death;
    map_pkg.sp_trigger = map.sp_trigger;
    map_pkg.sp_row = map.sp_row;
    map_pkg.sp_push = map.sp_push;
    map_pkg.start_bombs = bman.start_bombs;
    map_pkg.start_range = bman.start_range;
    sprintf (map_pkg.start_speed, "%4f", bman.start_speed);
    sprintf (map_pkg.bomb_tickingtime, "%4f", bman.bomb_tickingtime);

    if (map.random_tileset)
        map_pkg.tileset[0] = 0;
    else
        strncpy (map_pkg.tileset, map.tileset, LEN_TILESETNAME);
    map_pkg.map_selection = map.map_selection;
    strncpy (map_pkg.mapname, map.map, LEN_FILENAME);
    d_printf ("send_mapinfo: Tileset: %s\n", map.tileset);
#ifndef BUG_MAPINFO
	test = addr;				// VERY DIRTY WORKAROUND WHY IS HERE A BUG
    send_pkg ((struct pkg *) &map_pkg, addr);
	addr = test;				// VERY DIRTY WORKAROUND WHY IS HERE A BUG
#else
	printf ("Addr before send: %p\n", addr);
    send_pkg ((struct pkg *) &map_pkg, addr);
	printf ("Addr after send: %p\n", addr);
#endif
};


void
do_mapinfo (struct pkg_mapinfo *map_pkg, _net_addr * addr)
{
    d_printf ("do_mapinfo (addr %d) size[%d,%d]\n", addr->pl_nr, map_pkg->size_x, map_pkg->size_y);

    /* check if the server send this information */
    if (addr->pl_nr != bman.p_servnr)
        return;

    if (map_pkg->tileset[0] == 0) {
        map.random_tileset = 1;
        map.tileset[0] = 0;
    }
    else {
        map.random_tileset = 0;
        strncpy (map.tileset, map_pkg->tileset, LEN_TILESETNAME);
    }
    strncpy (map.map, map_pkg->mapname, LEN_FILENAME);
    map.map_selection = map_pkg->map_selection;
    map.size.x = map_pkg->size_x;
    map.size.y = map_pkg->size_y;
    map.bombs = map_pkg->bombs;
    map.fire = map_pkg->fire;
    map.shoes = map_pkg->shoes;
    map.mixed = map_pkg->mixed;
    map.death = map_pkg->death;
    map.sp_trigger = map_pkg->sp_trigger;
    map.sp_push = map_pkg->sp_push;
    map.sp_row = map_pkg->sp_row;

    bman.start_bombs = map_pkg->start_bombs;
    bman.start_range = map_pkg->start_range;
    sscanf (map_pkg->start_speed, "%f", &bman.start_speed);
    sscanf (map_pkg->bomb_tickingtime, "%f", &bman.bomb_tickingtime);
};


/***
 *** Respawn Date Handling
 ***/
void
send_respawn (_net_addr * addr, int plnr)
{
    struct pkg_respawn r_dat;

    r_dat.h.typ = PKG_respawn;
    r_dat.h.len = HTON16 (sizeof (struct pkg_respawn));
    r_dat.h.flags = PKGF_ackreq;
    r_dat.pl_nr = plnr;
    r_dat.state = players[plnr].state;
    r_dat.x = players[plnr].pos.x;
    r_dat.y = players[plnr].pos.y;
    send_pkg ((struct pkg *) &r_dat, addr);
};


void
do_respawn (struct pkg_respawn *r_pkg, _net_addr * addr)
{
    d_printf ("do_respawn (addr %d, pl_nr %d, pos %d,%d)\n", addr->pl_nr, r_pkg->pl_nr, r_pkg->x,
              r_pkg->y);
    if (addr->pl_nr == -1 || r_pkg->pl_nr == -1)
        return;

    if ((r_pkg->state & PSF_respawn) == PSF_respawn) {
        players[r_pkg->pl_nr].pos.x = r_pkg->x;
        players[r_pkg->pl_nr].pos.y = r_pkg->y;
        players[r_pkg->pl_nr].state &= (0xFF - PSF_alife);
        players[r_pkg->pl_nr].state |= PSF_respawn;
        players[r_pkg->pl_nr].frame = 0.0f;

    }
    else if (r_pkg->state & (PSF_respawn + PSF_alife)) {
        players[r_pkg->pl_nr].pos.x = r_pkg->x;
        players[r_pkg->pl_nr].pos.y = r_pkg->y;
        players[r_pkg->pl_nr].state |= PSF_alife;
        players[r_pkg->pl_nr].state &= (0xFF - PSF_respawn);
    }
};



/***
 *** gameinfo packet is used to get some data from a running game
 *** just fill in the informations we need and send this packet back.
 ***/
void
do_gameinfo (struct pkg_gameinfo *pgi, _net_addr * addr)
{
    if (GT_MP_PTPM && pgi->password == -1) {
        d_printf ("do_gameinfo (from: %s:%s) Broadcast Req: %d\n", addr->host, addr->port,
                  pgi->broadcast);

        strncpy (pgi->version, VERSION, LEN_VERSION);
        pgi->maxplayers = bman.maxplayer;
        pgi->curplayers = bman.players_nr_s;
		pgi->h.len = HTON16 (sizeof (struct pkg_gameinfo));
        strncpy (pgi->gamename, bman.gamename, LEN_GAMENAME);
        send_pkg ((struct pkg *) pgi, addr);
    }
    else if (pgi->password != -1)
        d_printf ("do_gameinfo (from: %s:%s) ** NO REQUEST **\n", addr->host, addr->port);
    else
        d_printf ("do_gameinfo (from: %s:%s) ** WE ARE NOT THE MASTER OF THIS GAME **\n",
                  addr->host, addr->port);
}


void
send_gameinfo (_net_addr * addr, int sock, int broadcast)
{
    struct pkg_gameinfo pgi;

    pgi.h.typ = PKG_gameinfo;
    pgi.h.len = HTON16 (sizeof (struct pkg_gameinfo));
    pgi.h.flags = 0;
    pgi.timestamp = timestamp;
    pgi.gamename[0] = 0;
    pgi.version[0] = 0;
    pgi.password = -1;
    pgi.broadcast = broadcast;

    if (bman.net_ai_family != PF_INET)
        pgi.h.flags = pgi.h.flags | PKGF_ipv6;
    if (broadcast)
        udp_sendbroadcast (sock, (char *) &pgi, NTOH16 (pgi.h.len), &addr->sAddr,
                           bman.net_ai_family);
    else
        udp_send (sock, (char *) &pgi, NTOH16 (pgi.h.len), &addr->sAddr, bman.net_ai_family);
};


/***
 *** general packet handling, like check for double recived packets
 *** network type. Autoreply on PKGF_ackreq and such things.
 ***/

/* check incoming packet, if we have got the same already if so return != -1
   if we haven't got it yet, Add to the incache and return -1 */
int
inpkg_check (unsigned char typ, short int id, _net_addr * addr)
{
    int i,
      pos;

    /* check if the player is still connected */
    if (!PS_IS_used (players[addr->pl_nr].state))
        return -1;

    /* find packet */
    for (i = 0, pos = -1; (i < PKG_IN_INDEX_NUM && pos == -1); i++)
        if (inpkg_index[i].pl_nr == addr->pl_nr
            && inpkg_index[i].typ == typ && inpkg_index[i].id == id)
            pos = i;

    if (pos == -1) {
        /* add to index */
        if (++inpkg_index_pos >= PKG_IN_INDEX_NUM)
            inpkg_index_pos = 0;

        inpkg_index[inpkg_index_pos].pl_nr = addr->pl_nr;
        inpkg_index[inpkg_index_pos].typ = typ;
        inpkg_index[inpkg_index_pos].id = id;
    }
    return pos;
};


/* delete all old pkg indexes about a player */
void
inpkg_delplayer (int pl_nr)
{
    int i;

    for (i = 0; i < PKG_IN_INDEX_NUM; i++)
        if (inpkg_index[i].pl_nr == pl_nr)
            inpkg_index[i].pl_nr = -1;
}


/* sends the packet and if PKGF_ackreq is set add packet to the resendcache */
void
send_pkg (struct pkg *packet, _net_addr * addr)
{
	d_printf ("send_pkg: plnr:%d, typ:%u id:%u\n", addr->pl_nr, packet->h.typ, packet->h.id);

    /* check if the packet would be send to
     * an AI_Player, so ignore it. */
    if ((addr->pl_nr >= 0 && addr->pl_nr < MAX_PLAYERS)
        && PS_IS_aiplayer (players[addr->pl_nr].state))
        return;

    /* set the id for the packet and the network flags 
     * the id is needed for the inpkg index to check for 
     * double reached packets
     * The id is limited to 32700 if */
    packet->h.id = HTON16 (pkg_lastid++ % 32767);
    if (bman.net_ai_family != PF_INET)
        packet->h.flags = packet->h.flags | PKGF_ipv6;
    udp_send (bman.sock, (char *) packet, NTOH16 (packet->h.len), &addr->sAddr, bman.net_ai_family);

    /* if PKGF_ackreq is set add the packet to the resendcache
     * so we can resend it if no PKF_ackreq returned for the packet. */
    if (packet->h.flags & PKGF_ackreq) 
		rscache_add (addr, packet);
};


/* forward the packet to all who are behind a firewall */
void
fwd_pkg (struct pkg *packet, _net_addr * addr)
{
    int pl;

    if (GT_MP_PTPS)             /* clients don't forward anything */
        return;

    if (packet->h.typ >= PKG_field && packet->h.typ < PKG_quit) {
        for (pl = 0; pl < MAX_PLAYERS; pl++)
            if ((!PS_IS_aiplayer (players[pl].state)) && PS_IS_netplayer (players[pl].state)
                && ((players[addr->pl_nr].net.flags & NETF_firewall) == NETF_firewall
                    || (players[pl].net.flags & NETF_firewall) == NETF_firewall)
                && pl != addr->pl_nr && (players[pl].net.flags & NETF_local2) == 0)
                send_pkg (packet, &players[pl].net.addr);
    }
    else if (packet->h.typ > PKG_quit)
        d_fatal ("fwd_pkg: not forwarding unknown packet From Player:%d (%s) Typ:%d Len:%d\n",
                 addr->pl_nr, players[addr->pl_nr].name, packet->h.typ, NTOH16 (packet->h.len));
};


/* entry point for all incoming network data. determinate packet type and 
   forward it to all needed functions, like inpkg_check()--> send answer if needed,
   if we are the server then forward the packet if needed
   and go into the do_PACKETTYP function */
void
do_pkg (struct pkg *packet, _net_addr * addr, int len)
{
    d_printf ("do_pkg: plnr:%d, typ:%u id:%u\n", addr->pl_nr, packet->h.typ, packet->h.id);
    if (((packet->h.flags & PKGF_ipv6) == 0 && bman.net_ai_family != PF_INET)
        || ((packet->h.flags & PKGF_ipv6) != 0 && bman.net_ai_family == PF_INET)) {
        d_printf ("do_pkg: packet comes from the wrong network type\n");
        return;
    }

	/* Check the size of the incoming packet */
	if (len != NTOH16(packet->h.len)) {
        d_printf ("do_pkg: len(%d) of the incoming packet is not the same as in pkg->h.len(%d)\n", len, NTOH16(packet->h.len));
        return;
	}

    /* get the addr and set the ping timeout value
     * check if the packet is from a player in the game and not from someone else
     * this exception is only for PKG_joingame, PKG_error */
    addr->pl_nr = get_player_nr (addr->host, addr->port);
    if ((addr->pl_nr < 0 || addr->pl_nr >= MAX_PLAYERS) && packet->h.typ > PKG_joingame
        && PS_IS_netplayer (players[addr->pl_nr].state)) {
        d_printf ("do_pkg: error addr->pl_nr out of range\n");
        return;
    }

    if (addr->pl_nr >= 0 && addr->pl_nr < MAX_PLAYERS) {
        players[addr->pl_nr].net.timestamp = timestamp;
        players[addr->pl_nr].net.pingreq = players[addr->pl_nr].net.pingack + 5;

        /* test if we have any important packet */
        if (packet->h.flags & PKGF_ackreq) {

            /* we need to send an acknolege so the client 
             * knows we have got this packet and delete 
             * it from the resend cache. */
            send_pkgack (addr, packet->h.typ, NTOH16 (packet->h.id));

            /* check the packet with the index so we can 
             * ignore packets we already have got 
             * this is important to keep away from 
             * the bomb is dropped twice bug. */
            if (inpkg_check (packet->h.typ, NTOH16 (packet->h.id), addr) != -1) {
                /* we have got this packet already */
                d_printf ("do_pkg: double packet ignoring addr->pl_nr=%d type:%d\n", addr->pl_nr, packet->h.typ);
                if (addr->pl_nr >= 0 && addr->pl_nr < MAX_PLAYERS)
                    players[addr->pl_nr].net.pkgopt.to_2sec++;
                return;
            }
        }

        /* forward packet */
        if (GT_MP_PTPM)
            fwd_pkg (packet, addr);
    }

    switch (packet->h.typ) {
    case (PKG_error): 
        do_error ((struct pkg_error *) packet, addr);
        break;
    case (PKG_gameinfo):
        do_gameinfo ((struct pkg_gameinfo *) packet, addr);
        break;
    case (PKG_playerid):
        do_playerid ((struct pkg_playerid *) packet, addr);
        break;
    case (PKG_servermode):
        do_servermode ((struct pkg_servermode *) packet, addr);
        break;
    case (PKG_field):
        do_field ((struct pkg_field *) packet, addr);
        break;
    case (PKG_contest):
        do_contest ((struct pkg_contest *) packet, addr);
        break;
    case (PKG_pingreq):
        do_ping ((struct pkg_ping *) packet, addr);
        break;
    case (PKG_pingack):
        do_ping ((struct pkg_ping *) packet, addr);
        break;
    case (PKG_bombdata):
        do_bombdata ((struct pkg_bombdata *) packet, addr);
        break;
    case (PKG_playerdata):
        do_playerdata ((struct pkg_playerdata *) packet, addr);
        break;
    case (PKG_quit):
        do_quit ((struct pkg_quit *) packet, addr);
        break;
    case (PKG_getfield):
        do_getfield ((struct pkg_getfield *) packet, addr);
        break;
    case (PKG_getplayerdata):
        do_getplayerdata ((struct pkg_getplayerdata *) packet, addr);
        break;
    case (PKG_fieldline):
        do_fieldline ((struct pkg_fieldline *) packet, addr);
        break;
    case (PKG_playerstatus):
        do_playerstatus ((struct pkg_playerstatus *) packet, addr);
        break;
    case (PKG_pkgack):
        do_pkgack ((struct pkg_pkgack *) packet, addr);
        break;
    case (PKG_chat):
        do_chat ((struct pkg_chat *) packet, addr);
        break;
    case (PKG_playermove):
        do_playermove ((struct pkg_playermove *) packet, addr);
        break;
    case (PKG_ill):
        do_ill ((struct pkg_ill *) packet, addr);
        break;
    case (PKG_special):
        do_special ((struct pkg_special *) packet, addr);
        break;
    case (PKG_mapinfo):
        do_mapinfo ((struct pkg_mapinfo *) packet, addr);
        break;
    case (PKG_tunneldata):
        do_tunneldata ((struct pkg_tunneldata *) packet, addr);
        break;
    case (PKG_joingame):
        do_joingame ((struct pkg_joingame *) packet, addr);
        break;
    case (PKG_dropitem):
        do_dropitems ((struct pkg_dropitem *) packet, addr);
        break;
    case (PKG_respawn):
        do_respawn ((struct pkg_respawn *) packet, addr);
        break;
    case (PKG_updateinfo):
        do_updateinfo ((struct pkg_updateinfo *) packet, addr);
        break;
    case (PKG_teamdata):
        do_teamdata ((struct pkg_teamdata *) packet, addr);
        break;
    default:
        send_error (addr, "BomberClone: unknown data packet");
        break;
    }
};
