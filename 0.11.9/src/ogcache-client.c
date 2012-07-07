/* $Id: ogcache-client.c,v 1.7 2005-03-28 16:26:40 stpohle Exp $
 * OpenGameCache-Client: this file will hold the protocol for the gameserver communication
 */

#include "basic.h"
#include "bomberclone.h"
#include "udp.h"
#include "ogcache-client.h"

int ogc_sock = -1;
struct game_entry ogc_array[MAX_OGC_ENTRYS];
char ogc_host[LEN_OGCHOST+1];
char ogc_port[LEN_OGCPORT+1];
char ogc_game[LEN_GAME+1];
int ogc_ai_family;
struct _sockaddr ogc_addr;
int ogc_browsing = 0;
int ogc_lasttimeout = 0;

/* fill out the whole gameentry */
static void fill_gameentry (char *pos, struct game_entry *ge) {
	char *pos_next;		/* pointer to the next seperator */
	int i = 0;

	while (pos != NULL)
	{
		/* search and test the next seperator */
		pos_next = strchr (pos, '\t');
		if (pos_next == NULL)
			pos_next = strchr (pos, '\n');
		/* if we found a return delete it.. 
		 * if we found the tab delete it */
		if (pos_next != NULL)
		{
			if (pos_next[0] == '\n')
			{	/* RETURN found */
				pos_next[0] = '\0';
				pos_next = NULL;
			}
			else
				pos_next[0] = '\0';
		}

		/* fill the struct with our data */
		switch (i)
		{
		case (0):	/* serial */
			ge->serial = atoi (pos);
			break;
		case (1):	/* host */
			strncpy (ge->host, pos, LEN_OGCHOST);
			break;
		case (2):	/* port */
			strncpy (ge->port, pos, LEN_OGCPORT);
			break;
		case (3):	/* game */
			strncpy (ge->game, pos, LEN_GAME);
			break;
		case (4):	/* version */
			strncpy (ge->version, pos, LEN_VERSION);
			break;
		case (5):	/* nettype */
			if (strcmp (pos, "IPv6") == 0)
				ge->ai_family = PF_INET6;
			else
				ge->ai_family = PF_INET;
			break;
		case (6):	/* netname */
			strncpy (ge->gamename, pos, LEN_GAMENAME);
			break;
		case (7):	/* state */
			strncpy (ge->status, pos, LEN_STATUS);
			break;
		case (8):	/* curplayers */
			ge->curplayers = atoi (pos);
			break;
		case (9):	/* maxplayers */
			ge->maxplayers = atoi (pos);
			break;
		default:
			return;
		}

		/* increase i for the next entry */
		i++;
		if (pos_next != NULL)
			pos = pos_next + 1;
		else
			pos = NULL;
	}
	
	d_printf ("fill_gameentry [Serial:%d, Addr:%s:%s, Game:%s, Gamename:%s\n", ge->serial, ge->host, ge->port, ge->game, ge->gamename);
};


/* add a game to the list or update one */
static int ogc_listadd (char *data) {
	struct game_entry ge;
	int i;
	
	/* fill out the entry and check if there was a problem */
	fill_gameentry (data, &ge);
	if (ge.serial == -1) return 0;
	
	for (i = 0; (i < MAX_OGC_ENTRYS && ogc_array[i].serial != -1 && ogc_array[i].serial != ge.serial); i++);
	
	if (i < MAX_OGC_ENTRYS && ogc_array[i].serial == ge.serial)
		ogc_array[i] = ge;
	else if (i < MAX_OGC_ENTRYS && ogc_array[i].serial == -1)
		ogc_array[i] = ge;
	
	return 1;
};


/* delete the game from the list */
static int ogc_listdel (char *data) {
	int i, serial, res = 0;
	
	serial = atoi (data);
	
	for (i = 0; i < MAX_OGC_ENTRYS; i++)
		if (ogc_array[i].serial == serial) {
			ogc_array[i].serial = -1;
			res = 1;
		}

	return res;
};


/* work with the incoming packet */
static int ogc_do_inpacket (char *in, int len, struct _sockaddr *addr) {
	char *param_start = strchr (in, ':');
	int i = 0;
	
	/* set param_start at the place where the first parameter is
	 * and if there is a : delete this one */
	if (param_start == NULL) {
		if ((param_start = strchr (in, '\n')) != NULL) {
			param_start[0] = '\0';
			param_start = NULL;
		}
	}
	else {
		param_start[0] = '\0';
		param_start++;
	}
	
	/*
     *	work with the incoming data
	 */
	if (strcmp (in, "ENTRY") == 0)
		i = ogc_listadd (param_start);
	
	else if (strcmp (in, "DELENTRY") == 0) 
		i = ogc_listdel (param_start);
	
	return i;
};



/* start a new listen server to get the list */
int ogc_init (char *localport, char *server, char *port, char *game, int ai_family) {
	int i;
	
	ogc_sock = -1;

	ogc_sock = udp_server (localport, ai_family);
	d_printf ("ogc_init (Localport: %s, Server %s:%s Game %s socket=%d)\n", localport, server, port, game, ogc_sock);
	if (ogc_sock <= 0)
		return 0;

	strncpy (ogc_host, server, LEN_OGCHOST);
	strncpy (ogc_port, port, LEN_OGCPORT);
	strncpy (ogc_game, game, LEN_GAME);
	ogc_ai_family = ai_family;
	
	d_printf ("ogc_host:%s ogc_port:%s ogc_game:%s ogc_ai_family:%d\n", ogc_host, ogc_port, ogc_game, ogc_ai_family);
	if (dns_filladdr (ogc_host, LEN_OGCHOST, ogc_port, LEN_OGCPORT, ogc_ai_family, &ogc_addr) < 0) {
		udp_close (ogc_sock);
		ogc_sock = -1;
		return 0;
	}
	
	for (i = 0; i < MAX_OGC_ENTRYS; i++)
		ogc_array[i].serial = -1;
	
	return 1;
};



/* send the status of the curent game to the game cache */
int ogc_sendgamestatus (int sock, char *game, char *version, char *gamename,
						int curplayers, int maxplayers, char *status) {
	char data[BUF_SIZE];
	
	if (sock <= 0 || ogc_sock <= 0) return 0;
	sprintf (data, "GAME:%s\t%s\t%s", game, version, gamename);
	if (ogc_ai_family == PF_INET)
		sprintf (data, "%s\tIPv4", data);
	else
		sprintf (data, "%s\tIPv6", data);
	sprintf (data, "%s\t%d\t%d\t%s", data, curplayers, maxplayers, status);

	udp_send (sock, data, strlen (data), &ogc_addr, ogc_ai_family);
	
	return 1;
};


/* send to the gamecache that this game has quit */
int ogc_sendgamequit (int sock) {
	char data[BUF_SIZE];

	if (sock <= 0 || ogc_sock <= 0) return 0;
	
	sprintf (data, "QUIT:");
	udp_send (sock, data, strlen (data), &ogc_addr, ogc_ai_family);
	
	return 1;
};


/* send to the gamecache that this browser does not anymore exist */
void ogc_shutdown () {
	if (ogc_sock <= 0)
		return;
	
	d_printf ("ogc_shutdown\n");
	if (ogc_browsing)
		ogc_browsestop ();
	udp_close (ogc_sock);
	
	ogc_sock = -1;
};



/* check the socket for incoming data and work 
 * with them if there are any */
int ogc_loop () {
	int len, i = 0;
	char in[BUF_SIZE];
	struct _sockaddr inaddr;
	
	if (ogc_sock <= 0)
		return -1;
		
	len = udp_get (ogc_sock, in, BUF_SIZE, &inaddr, ogc_ai_family);	
	
	if (len > 0) {  /* we have got something */
		d_printf ("ogc_got: %s\n", in);
		i = ogc_do_inpacket (in, len, &inaddr);
	}
	
	if (ogc_browsing && (time (NULL) - ogc_lasttimeout) > 30)
		ogc_browsestart ();
	
	return i;
};


/* send to the gamecache that we want 
 * to browse through the open games */
void ogc_browsestart () {
	char data[BUF_SIZE];
	
	d_printf ("ogc_browsestart\n");
	
	if (ogc_sock <= 0)
		return;

	sprintf (data, "LISTGAMES:%s", ogc_game);

	udp_send (ogc_sock, data, strlen (data), &ogc_addr, ogc_ai_family);
	ogc_browsing = 1;

	ogc_lasttimeout = time (NULL);
};


/* send the gamecache that we don't want to watch the list no more */
void ogc_browsestop () {
	char data[BUF_SIZE];
	
	d_printf ("ogc_browsestop\n");

	ogc_browsing = 0;
	if (ogc_sock <= 0)
		return;

	sprintf (data, "LISTQUIT:");
	
	udp_send (ogc_sock, data, strlen (data), &ogc_addr, ogc_ai_family);
};
