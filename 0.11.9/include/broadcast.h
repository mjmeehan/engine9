/* $Id: broadcast.h,v 1.2 2005-03-28 16:38:29 stpohle Exp $ */

#define BC_MAXENTRYS 64
#define BC_REQUESTTIME 2500
#define BC_REQUESTTIMEOUT 7500
#define BC_MAXREQUEST 3

struct broadcast_entry {
	char port[LEN_PORT];
	char host[LEN_SERVERNAME];
	char gamename[LEN_GAMENAME];
	char version[LEN_VERSION];
	int ping;
	int password;
	int curplayers;
	int maxplayers;
	int timestamp;
	int try;
	int lan;				// the broadcasted packet indicates a lan game
};

extern struct broadcast_entry broadcast_list [];
	
extern void broadcast_send (char *host, char *port);
void broadcast_init ();
void broadcast_loop ();
int broadcast_find (char *host, char *port);
int broadcast_findfree ();
void broadcast_check ();
