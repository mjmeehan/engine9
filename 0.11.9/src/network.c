/* $Id: network.c,v 1.79 2007-01-12 22:42:31 stpohle Exp $ */
/*
	network routines.
*/

#include "bomberclone.h"
#include "network.h"
#include "chat.h"
#include "packets.h"
#include "ogcache-client.h"
#include "menu.h"

int
network_server_port (char *server, char *host, int hostlen, char *port, int portlen)
{
    char *pos,
     *pos2;

    if (host == NULL)
        return -1;

    pos2 = pos = strchr (server, ':');

    if (pos != NULL)
        while (pos2 != NULL) {
            pos = pos2;
            pos2 = strchr (pos + 1, ':');
        }

    if (pos != NULL) {
        // : für Portangabe gefunden
        if (pos - server < hostlen) {
            strncpy (host, server, pos - server);
            host[pos - server] = 0;
            if (pos[1] == 0)
                sprintf (port, "11000");
            else
                strcpy (port, pos + 1);
        }
        else {
            return -1;
        }
    }
    else {
        // Portangabe wurde nicht gefunden und wird auf 0 gesetzt
        strncpy (host, server, hostlen);
        sprintf (port, "11000");
    }

    return 0;
};


/*
	try to work better with the network packet option
*/
void
net_dyn_pkgoption ()
{
    int p;
    _net_pkgopt *npkg;

    for (p = 0; p < MAX_PLAYERS; p++)
        if (PS_IS_netplayer (players[p].state) && !PS_IS_aiplayer (players[p].state)) {
            npkg = &players[p].net.pkgopt;

            if (npkg->to_2sec > DYN_PKG_MAX_MISSING) {
                if (npkg->send_set < 10)
                    npkg->send_set++;
                npkg->to_2sec = 0;
                npkg->to_timestamp = timestamp;
            }

            if ((timestamp - npkg->to_timestamp > 2000)
                && npkg->to_2sec <= DYN_PKG_MIN_MISSING) {
                if (npkg->send_set > PKG_SENDSETOPT)
                    npkg->send_set--;
                npkg->to_2sec = 0;
                npkg->to_timestamp = timestamp;
            }
        }
};




/*
	setup everything for the network loop
*/
int
network_init ()
{
    char host[LEN_SERVERNAME];
    char port[LEN_PORT];

/*   we need it for the windows winsock */
#ifdef _WIN32
    WSADATA wsaData;

    if (WSAStartup (MAKEWORD (1, 1), &wsaData) != 0) {
        d_printf ("WSAStartup failed.\n");
        exit (1);
    }
#endif

    if (bman.net_ai_family == PF_INET)
        sprintf (host, "IPv4");
#ifndef _WIN32
    else if (bman.net_ai_family == PF_INET6)
        sprintf (host, "IPv6");
#endif
    else
        sprintf (host, "IPv (unknown)");
    d_printf ("Network Init with %s.\n", host);

    bman.sock = -1;
    timestamp = SDL_GetTicks ();

    // start the udp server
    bman.sock = udp_server (bman.port, bman.net_ai_family);

    if (bman.sock < 0) {
#ifdef _WIN32
        WSACleanup ();
#endif
        return -1;
    }

    if (bman.notifygamemaster) {
        network_server_port (bman.ogcserver, host, LEN_SERVERNAME, port, LEN_PORT);
        if (ogc_init (bman.ogc_port, host, port, "BomberClone", bman.net_ai_family) == 0)
            bman.notifygamemaster = 0;
    }

    // we have got our socket..
	rscache_init ();
	
    return 0;
};


/*
	shutdown the network part
*/
void
network_shutdown ()
{
    int i;
    int new_server = bman.p_servnr;
    d_printf ("network_shutdown\n");

    if (GT_MP_PTPM) {
        d_printf ("Server Quit Send information\n");

        /* find new server */
        for (i = 0; i < MAX_PLAYERS; i++) {
            if (PS_IS_used (players[i].state) && PS_IS_netplayer (players[i].state)
                && i != bman.p_servnr && players[i].net.flags == 0)
                new_server = i;
        }

        d_printf ("netword_shutdown: new server set to: %d\n", new_server);

        for (i = 0; i < MAX_PLAYERS; i++)
            if (i != bman.p_servnr && PS_IS_netplayer (players[i].state)
                && !PS_IS_aiplayer (players[i].state))
                send_quit (&players[i].net.addr, bman.p_servnr, new_server);
    }
    else if (players[bman.p_servnr].net.addr.host[0] != 0) {
        send_quit (&players[bman.p_servnr].net.addr, bman.p_nr, bman.p_servnr);
        if (IS_LPLAYER2)
            send_quit (&players[bman.p_servnr].net.addr, bman.p2_nr, bman.p_servnr);
    }

    if (bman.notifygamemaster) {
        ogc_sendgamequit (bman.sock);
        ogc_shutdown ();
    }

    udp_close (bman.sock);

    bman.p_nr = -1;
    bman.sock = -1;
#ifdef _WIN32
    WSACleanup ();
#endif
};


int
net_check_timeout (int pl_nr)
{
    int timeout = UDP_TIMEOUT;

    if ((players[pl_nr].state & (PSF_net + PSF_used + PSF_ai)) == (PSF_used + PSF_net)
        && ((players[pl_nr].net.flags & NETF_local2) == 0)
        && timestamp - players[pl_nr].net.timestamp > timeout
        && players[pl_nr].net.pingreq != players[pl_nr].net.pingack) {
        d_printf ("net_check_timeout pl_nr=%d, ack=%d, req=%d, timediff=%d\n", pl_nr,
                  players[pl_nr].net.pingack, players[pl_nr].net.pingreq,
                  timestamp - players[pl_nr].net.timestamp);
        players[pl_nr].net.timestamp = timestamp;
        players[pl_nr].net.pingack = players[pl_nr].net.pingreq;
        send_ping (&players[pl_nr].net.addr, players[pl_nr].net.pingack + 100, PKG_pingreq);
    }
    if ((players[pl_nr].state & (PSF_net + PSF_used + PSF_ai)) == (PSF_used + PSF_net)
        && ((players[pl_nr].net.flags & NETF_local2) == 0)
        && timestamp - players[pl_nr].net.timestamp > timeout
        && players[pl_nr].net.pingreq == players[pl_nr].net.pingack) {
        d_printf ("net_check_timeout pl_nr=%d, ack=%d, req=%d, timediff=%d\n", pl_nr,
                  players[pl_nr].net.pingack, players[pl_nr].net.pingreq,
                  timestamp - players[pl_nr].net.timestamp);
        return 1;
    }
    return 0;
};



/*
	Read data from the network and work with it
*/
int
network_loop ()
{
    char data[MAX_UDPDATA];
    struct pkg *packet = (struct pkg *) data;
    int inlen,
      i;

    _net_addr addr;

    if (bman.state != GS_running && bman.state != GS_ready)
        timestamp = SDL_GetTicks ();

    /* 
       as long as we get any new data, work with them
     */
    inlen = udp_get (bman.sock, data, MAX_UDPDATA, &addr.sAddr, bman.net_ai_family);
    addr.port[0] = addr.host[0] = 0;
    if (inlen > 0)
        dns_filladdr (addr.host, LEN_SERVERNAME, addr.port, LEN_PORT, bman.net_ai_family,
                      &addr.sAddr);

    while (inlen > 0) {
        do_pkg (packet, &addr, inlen);

        //  printf ("Network : inlen (%d) typ (%d) Size (%d)\n", inlen, packet->typ, pkglen);
        inlen = udp_get (bman.sock, data, MAX_UDPDATA, &addr.sAddr, bman.net_ai_family);
        addr.port[0] = addr.host[0] = 0;
        if (inlen > 0)
            dns_filladdr (addr.host, LEN_SERVERNAME, addr.port, LEN_PORT,
                          bman.net_ai_family, &addr.sAddr);
    }

    /*
       check here for old connections who aren't answering
     */
    if (bman.state == GS_wait || bman.state == GS_ready || bman.state == GS_running) {
        if (GT_MP_PTPS) {
            if (net_check_timeout (bman.p_servnr)) {
                d_printf ("Server Timed Out\n");
                bman.state = GS_startup;
            }
        }
        else if (GT_MP_PTPM) {
            for (i = 1; i < MAX_PLAYERS; i++)
                if (i != bman.p_nr && net_check_timeout (i)) {
                    d_printf ("Player %d Timed Out\n", i);
                    player_delete (i);
                }
        }
    }

    /*
       resend_cache.... 
     */
    rscache_loop ();

    /*
       dynamic calibration of the network traffic option
     */
    net_dyn_pkgoption ();

    return 0;
};


/*
   this is needed to draw the whole uppdate of everything 
*/
void
draw_netupdatestate (char st)
{
    char text[255];
    unsigned char b;
    int y = 0,
        b1,
        z,
        zx = 200,
        i,
        j,
        s = map.size.y + MAX_PLAYERS + GAME_MAX_TUNNELS;
    SDL_Rect src,
      dest;
    z = gfx.res.x - zx - 30 - 8;
    for (i = 0; i < MAX_PLAYERS; i++)
        if (PS_IS_used (players[i].state)) {
            y += 50;
            if (st) {
                redraw_logo (0, y, gfx.res.x, y + 50);

                if (players[i].gfx_nr != -1) {
                    dest.w = src.w = players[i].gfx->small_image->w;
                    dest.h = src.h = players[i].gfx->small_image->h;
                    src.x = players[i].gfx->small_image->w * down;
                    src.y = 0;

                    dest.x = 50;
                    dest.y = y;

                    SDL_BlitSurface (players[i].gfx->small_image, &src, gfx.screen, &dest);
                    gfx_blitupdaterectadd (&dest);
                }

                dest.x = zx;
                dest.y = y;
                dest.w = menulistimages[1][0]->w;
                dest.h = menulistimages[1][0]->h;
                gfx_blit (menulistimages[1][0], NULL, gfx.screen, &dest, 10000);
                dest.x = z + zx + 4;
                gfx_blit (menulistimages[1][2], NULL, gfx.screen, &dest, 10000);
                // draw the bottom left and right of the list
                dest.y = y + 29;
                gfx_blit (menulistimages[1][8], NULL, gfx.screen, &dest, 10000);
                dest.x = zx;
                gfx_blit (menulistimages[1][6], NULL, gfx.screen, &dest, 10000);
                //top & bottom
                for (j = 4; j < z + 4; j += 4) {
                    dest.x = j + zx;
                    dest.y = y;
                    gfx_blit (menulistimages[1][1], NULL, gfx.screen, &dest, 10000);
                    dest.y = y + 29;
                    gfx_blit (menulistimages[1][7], NULL, gfx.screen, &dest, 10000);
                }
                //left &right
                for (j = 4; j < 29; j += 4) {
                    dest.x = zx;
                    dest.y = y + j;
                    gfx_blit (menulistimages[1][3], NULL, gfx.screen, &dest, 10000);
                    dest.x = z + zx + 4;
                    gfx_blit (menulistimages[1][5], NULL, gfx.screen, &dest, 10000);
                }
                sprintf (text, "%s", players[i].name);
                font_draw (80, y, text, 0, 4);
            }
            // calc percentage, this a range from 0 to 255)   
            switch (players[i].net.net_istep) {
            case 3:
                sprintf (text, "Getting Tunnel Data %d.", players[i].net.net_status);
                b = (players[i].net.net_status + 1) * 255 / s;
                break;
            case 2:
                sprintf (text, "Getting Field Data %d of %d.",
                         players[i].net.net_status, map.size.y);
                b = (players[i].net.net_status + 1 + GAME_MAX_TUNNELS) * 255 / s;
                break;
            case 1:
                sprintf (text, "Getting Player Data %d of %d.",
                         players[i].net.net_status, MAX_PLAYERS);
                b = (players[i].net.net_status + 1 + GAME_MAX_TUNNELS + map.size.y) * 255 / s;
                break;
            default:
                sprintf (text, "Ready");
                b = 255;
                break;
            }

            //draw bar
            if (b > 0) {
                b1 = b * z / 255;
                dest.x = zx + 4;
                dest.y = y + 4;
                dest.w = menubuttonimages[2][0]->w;
                dest.h = menubuttonimages[2][0]->h;
                gfx_blit (menubuttonimages[2][0], NULL, gfx.screen, &dest, 10000);
                dest.x = zx + 4 + b1 - menubuttonimages[1][2]->w;
                if (dest.x < zx + 4)
                    dest.x = zx + 4;
                dest.w = menubuttonimages[2][2]->w;
                dest.h = menubuttonimages[2][2]->h;
                gfx_blit (menubuttonimages[2][2], NULL, gfx.screen, &dest, 10000);
                if (b1 > menubuttonimages[2][0]->w + menubuttonimages[2][2]->w) {
                    dest.w = menubuttonimages[2][1]->w;
                    dest.h = menubuttonimages[2][1]->h;
                    for (j = menubuttonimages[2][0]->w;
                         j < b1 - menubuttonimages[2][2]->w; j += menubuttonimages[2][1]->w) {
                        dest.x = j + zx + 4;
                        gfx_blit (menubuttonimages[2][1], NULL, gfx.screen, &dest, 10000);
                    }
                }
            }
            // draw old status in case of debug
            if (!players[i].net.net_istep)
                font_draw (80, y + 20, text, 0, 4);
            else if (debug) {
                redraw_logo (80, y + 35, gfx.res.x - 80, 15);
                font_draw (80, y + 35, text, 0, 4);
            }
        }
    gfx_blitdraw ();
    return;
}

/*
	used to update settings at startup
*/
void
net_send_playerid (int pl_nr)
{
    int i;

    d_printf ("net_send_playerid pl_nr:%d\n", pl_nr);

    if (GT_MP_PTPM) {
        /*
           Send to all connected clients the update
         */
        for (i = 1; i < MAX_PLAYERS; i++)
            if (NET_CANSEND (i))
                send_playerid (&players[i].net.addr, players[pl_nr].name,
                               players[pl_nr].net.addr.host,
                               players[pl_nr].net.addr.port, pl_nr,
                               players[pl_nr].gfx_nr, players[pl_nr].team_nr,
                               players[pl_nr].net.flags);
    }
    else {
        /*
           Send only to the Server the update and only if pn_nr == bman.p_nr
         */
        if (pl_nr == bman.p_nr || (IS_LPLAYER2 && pl_nr == bman.p2_nr))
            send_playerid (&players[bman.p_servnr].net.addr, players[pl_nr].name,
                           players[pl_nr].net.addr.host, players[pl_nr].net.addr.port,
                           pl_nr, players[pl_nr].gfx_nr, players[pl_nr].team_nr,
                           players[pl_nr].net.flags);
    }

    if ((players[pl_nr].gfx_nr >= 0 && players[pl_nr].gfx != &gfx.players[players[pl_nr].gfx_nr])
        || (players[pl_nr].gfx_nr == -1 && players[pl_nr].gfx != NULL))
        player_set_gfx (&players[pl_nr], players[pl_nr].gfx_nr);
};


/*
    sets up everything for the network game..
	on servers the game field will be created and the clients will wait for the game data
	within the network loop
*/
void
net_transmit_gamedata ()
{
    int done = 0,
        keypressed = 0,
        x,
        y,                      // network upload status for one step
        p,
        i,
        net_istep;              // network init step
    SDL_Event event;
    Uint8 *keys;
    Uint32 downtimestamp = 0;

    draw_logo ();

    if (GT_MP_PTPM)
        font_draw (100, 0, "Waiting for the Clients", 1, 0);
    else
        font_draw (100, 0, "Downloading Data", 1, 0);

    /* 
       prepare everything for the loop 
     */
    for (x = 0; x < MAX_PLAYERS; x++) {
        players[x].net.timestamp = 0;
        players[x].net.net_status = -1;
        if ((PS_IS_aiplayer (players[x].state)) || (x == bman.p_servnr) || (x == bman.p2_nr)
            || (players[x].net.flags & NETF_local2) == NETF_local2)
            players[x].net.net_istep = 0;
        else
            players[x].net.net_istep = 3;
    }

    y = -1;
    if (GT_MP_PTPM)
        net_istep = 0;
    else
        net_istep = 3;

    draw_netupdatestate (1);
    SDL_Flip (gfx.screen);

    downtimestamp = timestamp;
    while (!done && (bman.state == GS_update || (GT_MP_PTPS && net_istep != 0))) {
        /* the network thing */

        network_loop ();

        /* if PTPM check if all players are ready */
        if (GT_MP_PTPM) {
            if (timestamp - downtimestamp > TIME_UPDATEINFO) {
                downtimestamp = timestamp;
                net_send_updateinfo ();
            }
            for (p = 0, i = 1; p < MAX_PLAYERS; p++)
                if (PS_IS_playing (players[p].state)
                    && players[p].net.net_istep != 0)
                    i = 0;
            if (i == 1) {       /* all players are ready */
                done = 1;
                bman.state = GS_ready;
            }
        }

        /* if PTPS get all data */
        if (GT_MP_PTPS) {
            if (net_istep == 3) {
                /* 
                   get tunneldata 
                 */
                if ((y < GAME_MAX_TUNNELS - 1 && y == players[bman.p_nr].net.net_status)
                    || y == -1) {
                    y++;
                    downtimestamp = timestamp;
                    send_tunneldata (&players[bman.p_servnr].net.addr, y, -1, -1);
                }
                else if (y < GAME_MAX_TUNNELS
                         && y != players[bman.p_nr].net.net_status && y >= 0
                         && timestamp - downtimestamp > DOWNLOAD_TIMEOUT) {
                    /* we have got no tunnel data */
                    y--;
                }
                else if (y == GAME_MAX_TUNNELS - 1 && players[bman.p_nr].net.net_status == y) {
                    /* we have got all tunnel data */
                    y = -1;
                    players[bman.p_nr].net.net_istep = --net_istep;
                    players[bman.p_nr].net.net_status = -1;
                }
            }

            if (net_istep == 2) {
                /* 
                   get field data
                 */
                if ((y < map.size.y - 1 && y == players[bman.p_nr].net.net_status)
                    || y == -1) {
                    /* send field data req */
                    y++;
                    downtimestamp = timestamp;
                    send_getfield (&players[bman.p_servnr].net.addr, y);
                }
                else if (y < map.size.y && y != players[bman.p_nr].net.net_status
                         && y >= 0 && timestamp - downtimestamp > DOWNLOAD_TIMEOUT) {
                    /* getdata timed out - we have got no field data */
                    y--;
                }
                else if (y == map.size.y - 1 && players[bman.p_nr].net.net_status == y) {
                    /* we have got all field data */
                    y = -1;
                    players[bman.p_nr].net.net_istep = --net_istep;
                    players[bman.p_nr].net.net_status = -1;
                }
            }

            if (net_istep == 1) {
                /*
                   get player data
                 */
                if ((y < MAX_PLAYERS - 1 && y == players[bman.p_nr].net.net_status)
                    || y == -1) {
                    /* send player date req */
                    y++;
                    downtimestamp = timestamp;
                    send_getplayerdata (&players[bman.p_servnr].net.addr, y);
                }
                if (y < MAX_PLAYERS && y != players[bman.p_nr].net.net_status
                    && y >= 0 && timestamp - downtimestamp > DOWNLOAD_TIMEOUT) {
                    /* we have got no player data */
                    y--;
                }
                if (y == MAX_PLAYERS - 1 && players[bman.p_nr].net.net_status == y) {
                    /* we have got all playerdata */
                    y = -1;
                    players[bman.p_nr].net.net_istep = --net_istep;
                    players[bman.p_nr].net.net_status = -1;
                    downtimestamp = timestamp;
                    send_playerstatus (&players[bman.p_servnr].net.addr, bman.p_nr, 0, 0);
                }
            }

            if (net_istep == 0 && players[bman.p_nr].net.net_status == -1
                && timestamp - downtimestamp > DOWNLOAD_TIMEOUT) {
                /* server did not send informations back */
                downtimestamp = timestamp;
                send_playerstatus (&players[bman.p_servnr].net.addr, bman.p_nr, 0, 0);
            }
        }

        /* do the grafik work */
        draw_netupdatestate (0);

        if (s_fetchevent (&event) != 0)
            switch (event.type) {
            case (SDL_QUIT):
                bman.state = GS_quit;
                bman.p_nr = -1;
                done = 1;
            }

        keys = SDL_GetKeyState (NULL);

        if (keys[SDLK_ESCAPE] && event.type == SDL_KEYDOWN) {
            done = 1;
            bman.p_nr = -1;
            keypressed = 1;
            bman.state = GS_startup;
        }

        if (event.type == SDL_KEYUP)
            keypressed = 0;
    }

    timestamp = SDL_GetTicks (); // needed for time sync.
    SDL_Delay (1);              // we don't need here anything better

    /* player is only watching so just go after we have got everything
       go to show the field */
    if (GT_MP_PTPS && bman.state == GS_update && net_istep == 0 && players[bman.p_nr].gfx_nr == -1) {
        done = 1;
        bman.state = GS_running;
    }
};


/*
 * send informations about a player too all connected players
 * last_change: the player will not be send if (send_to_p_nr == bman.p_nr)
 */
void
net_game_send_player (int p_nr)
{
    int p;

    if (GT_MP_PTPM) {
        for (p = 0; p < MAX_PLAYERS; p++)
            if (PS_IS_netplayer (players[p].state) && p != bman.p_nr && p != bman.p2_nr
                && p != p_nr)
                send_playerdata (&players[p].net.addr, p_nr, &players[p_nr]);
    }
    else if (p_nr == bman.p_nr || p_nr == bman.p2_nr) {
        for (p = 0; p < MAX_PLAYERS; p++)
            if (NET_CANSEND (p))
                send_playerdata (&players[p].net.addr, p_nr, &players[p_nr]);
    }
};


void
net_game_send_playermove (int p_nr, int mustsend)
{
    int p;
    _player *pl;

    for (p = 0; p < MAX_PLAYERS; p++)
        if (NET_CANSEND (p)) {
            pl = &players[p_nr];

            pl->net.pkgopt.send_to--;
            if ((pl->net.pkgopt.send_to <= 0 || mustsend))
                send_playermove (&players[p].net.addr, p_nr, pl);

            /* network packet send control */
            if (pl->net.pkgopt.send_to <= 0 || pl->net.pkgopt.send_to > pl->net.pkgopt.send_set)
                pl->net.pkgopt.send_to = pl->net.pkgopt.send_set;
        }
};

void
net_game_send_bomb (int p, int b)
{
    int pl;

    /* check if we are slave and send something else as dropping a bomb */
    if (GT_MP_PTPS && players[p].bombs[b].state != BS_ticking
        && players[p].bombs[b].state != BS_trigger)
        return;

    d_printf ("Send BombData %d, %d\n", p, b);

    if (p < 0 || p >= MAX_PLAYERS || b < 0 || b >= MAX_BOMBS)
        return;

    for (pl = 0; pl < MAX_PLAYERS; pl++)
        if (NET_CANSEND (pl))
            send_bombdata (&players[pl].net.addr, p, b, &players[p].bombs[b]);
};


void
net_game_send_field (int x, int y)
{
    int pl;

    d_printf ("Send FieldData %d, %d\n", x, y);

    if (x < 0 || x >= MAX_FIELDSIZE_X || y < 0 || y >= MAX_FIELDSIZE_Y)
        return;

    for (pl = 0; pl < MAX_PLAYERS; pl++)
        if (NET_CANSEND (pl))
            send_field (&players[pl].net.addr, x, y, &map.field[x][y]);
};


/*
 * send the information about the deleted player to all clients
 */
void
net_game_send_delplayer (int pl_nr)
{
    int i;
    int new_server = bman.p_servnr;

    d_printf ("net_game_send_delplayer (%d)\n", pl_nr);

    if (GT_MP_PTPM && (GS_WAITRUNNING || bman.state == GS_update)) {

        /* find new server, if needed */
        if (pl_nr == bman.p_servnr) {
            for (i = 0; i < MAX_PLAYERS; i++) {
                if (PS_IS_used (players[i].state)
                    && PS_IS_netplayer (players[i].state) && i != bman.p_servnr
                    && players[i].net.flags == 0)
                    new_server = i;
            }
            d_printf ("new_game_send_delplayer: new server set to: %d\n", new_server);
        }

        for (i = 0; i < MAX_PLAYERS; i++)
            if (NET_CANSEND (i) && i != bman.p_nr)
                send_quit (&players[i].net.addr, pl_nr, new_server);
        bman.updatestatusbar = 1;
    }
    /* we have to send that one if our own players quit */
    else if (pl_nr == bman.p_nr || pl_nr == bman.p2_nr) {
        send_quit (&players[bman.p_servnr].net.addr, pl_nr, -1);
    }

    inpkg_delplayer (pl_nr);

    if (GT_MP_PTPM) {
        if (bman.notifygamemaster)
            send_ogc_update ();

        if (new_server >= 0 && new_server < MAX_PLAYERS)
            bman.p_servnr = new_server;
    }
};


void
net_game_fillsockaddr ()
{
    /* Update all sockaddr before the game starts */
    int i;

    for (i = 0; i < MAX_PLAYERS; i++)
        if (!PS_IS_aiplayer (players[i].state) && players[i].net.addr.host[0] != 0
            && players[i].net.addr.host[0] != 0)
            dns_filladdr (players[i].net.addr.host, LEN_SERVERNAME,
                          players[i].net.addr.port, LEN_PORT, bman.net_ai_family,
                          &players[i].net.addr.sAddr);
};


void
net_send_servermode ()
{
    int i;

    for (i = 0; i < MAX_PLAYERS; i++)
        if (NET_CANSEND (i))
            send_servermode (&players[i].net.addr, i);

    if (bman.notifygamemaster && GT_MP_PTPM)
        send_ogc_update ();
};


/* sends to everyone an up to date playerlist*/
void
net_send_players ()
{
    int i,
      j;

    for (j = 0; j < MAX_PLAYERS; j++)
        if (NET_CANSEND (j))
            for (i = 0; i < MAX_PLAYERS; i++)
                send_playerid (&players[j].net.addr, players[i].name,
                               players[i].net.addr.host, players[i].net.addr.port,
                               i, players[i].gfx_nr, players[i].team_nr, players[i].net.flags);

};



/* sends to everyone the teamdata */
void
net_send_teamdata (int team_nr)
{
    int j;

    if (GT_MP_PTPS)
        return;

    for (j = 0; j < MAX_PLAYERS; j++)
        if (NET_CANSEND (j))
            send_teamdata (&players[j].net.addr, team_nr);
};



void
net_send_chat (char *text, signed char notigamesrv)
{
    int i;

    for (i = 0; i < MAX_PLAYERS; i++)
        if (NET_CANSEND (i))
            send_chat (&players[i].net.addr, text);
};


void
net_game_send_ill (int p_nr)
{
    int i;

    d_printf ("net_game_send_ill (%d)\n", p_nr);

    for (i = 0; i < MAX_PLAYERS; i++)
        if (NET_CANSEND (i))
            send_ill (&players[i].net.addr, p_nr, &players[p_nr]);
};



/* send to all players the drop item list */
void
net_game_send_dropitems (int pl_nr, _flyingitem ** fiptr, int cnt)
{
    int i;

    d_printf ("net_game_send_dropitems (%d): %d items droppped\n", pl_nr, cnt);

    for (i = 0; i < MAX_PLAYERS; i++)
        if (NET_CANSEND (i))
            send_dropitems (&players[i].net.addr, pl_nr, fiptr, cnt);
};



/*
   this routine will set up some things for the network game
   after this the data should be transfered to the other clients.
*/
void net_new_game () {
    int p;

    /* set all multiplayer depending datas */
    bman.players_nr = 0;
    bman.players_nr_s = 0;
    for (p = 0; p < MAX_PLAYERS; p++) {
        if (PS_IS_used (players[p].state))
            bman.players_nr_s++;
        else
            players[p].state = 0;
    }

    if (bman.p_nr != -1)
        players[bman.p_nr].state &= (0xFF - PSF_net); // we are the local player
    bman.last_ex_nr = 1;
};



/* send special use elements into the network, 
   to make sure nothing bad happens with explosions
   we send the ex_nr number too */
void net_game_send_special (int pl_nr, int ex_nr, int type) {
    int pl;

    d_printf ("Send Special Data pl_nr:%d ex_nr:%d type:%d\n", pl_nr, ex_nr, type);

    if (pl_nr < 0 || pl_nr >= MAX_PLAYERS)
        return;

    for (pl = 0; pl < MAX_PLAYERS; pl++)
        if (NET_CANSEND (pl))
            send_special (&players[pl].net.addr, pl_nr, type, ex_nr);
};


/* Send update informations to all clients */
void
net_send_updateinfo ()
{
    int i;

    if (GT_MP_PTPS)
        return;

    d_printf ("Send Updateinfo\n");

    for (i = 0; i < MAX_PLAYERS; i++)
        if (NET_CANSEND (i))
            send_updateinfo (&players[i].net.addr);
};


/* Send mapinformations to all clients */
void
net_send_mapinfo ()
{
    int i;

    if (GT_MP_PTPS)
        return;

    d_printf ("Send Mapinfo\n");

    for (i = 0; i < MAX_PLAYERS; i++)
        if (NET_CANSEND (i))
            send_mapinfo (&players[i].net.addr);
};



/* send an update about the game to all clients */
void
send_ogc_update ()
{
    int i,
      j;
    char status[10];

    if (!bman.notifygamemaster || GT_MP_PTPS)
        return;

    for (j = 0, i = 0; i < bman.maxplayer; i++)
        if (PS_IS_used (players[i].state))
            j++;

    switch (bman.state) {
    case (GS_running):
        sprintf (status, "running");
        break;
    case (GS_ready):
        sprintf (status, "ready");
        break;
    case (GS_wait):
        sprintf (status, "wait");
        break;
    case (GS_update):
        sprintf (status, "update");
        break;
    default:
        sprintf (status, "error");
        break;
    }
    ogc_sendgamestatus (bman.sock, "BomberClone", VERSION, bman.gamename, j, bman.maxplayer,
                        status);
};


/* send the respawn data to all clients and with the new position of the player */
void
net_game_send_respawn (int pl_nr)
{
    int pl;

    d_printf ("Send Respawn Data %d New Position (%d,%d)\n", pl_nr);

    if (pl_nr < 0 || pl_nr >= MAX_PLAYERS)
        return;

    for (pl = 0; pl < MAX_PLAYERS; pl++)
        if (NET_CANSEND (pl))
            send_respawn (&players[pl].net.addr, pl_nr);
};
