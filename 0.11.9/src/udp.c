/* $Id: udp.c,v 1.18 2009-12-18 11:05:37 stpohle Exp $ */
/* udp.c code for the network
	File Version 0.2
*/

#include "udp.h"

extern char *dns_net_getip (char *host);
extern int dns_filladdr (char *host, int hostlen, char *port, int portlen, int ai_family,
                         struct _sockaddr *sAddr);
extern int udp_get (int sock, char *text, int len, struct _sockaddr *sAddr, int ai_family);
extern int udp_server (char *port, int ai_family);
extern void udp_send (int sock, char *text, int len, struct _sockaddr *sAddr, int ai_family);
extern void udp_close (int sock);

char dnsip[UDP_LEN_HOSTNAME];

extern void d_printf (char *fmt,...);


/* closes an existing udp server */
void
udp_close (int sock)
{
    if (sock != -1)
#ifdef _WIN32
		closesocket(sock);
#else
        close (sock);
#endif
    sock = -1;
};


int
dns_filladdr (char *host, int hostlen, char *port, int portlen, int ai_family,
              struct _sockaddr *sAddr)
{
#ifdef _WIN32
    struct hostent *he;
    char txt[255];

    if (host[0] == 0 || port[0] == 0) {
        /* we have to complete server and port from the sAddr */

        strncpy (host, inet_ntoa (((struct sockaddr_in *) sAddr)->sin_addr), hostlen);
        sprintf (txt, "%d", ntohs (((struct sockaddr_in *) sAddr)->sin_port));
        strncpy (port, txt, portlen);
    }
    else {
        /* we have to complete the sAddr struct */

        if ((he = gethostbyname (host)) == NULL) { // get the host info
            perror ("dns_filladdr (gethostbyname)");
            return -1;
        }

        ((struct sockaddr_in *) sAddr)->sin_family = ai_family; // host byte order
        ((struct sockaddr_in *) sAddr)->sin_port = htons (atoi (port)); // short, network byte order
        ((struct sockaddr_in *) sAddr)->sin_addr = *((struct in_addr *) he->h_addr);
        memset (&(((struct sockaddr_in *) sAddr)->sin_zero), '\0', 8); // zero the rest of the struct
    }

#else
    struct addrinfo hints,
     *res;
    int err, i,
      addrlen;

    if (host[0] == 0 || port[0] == 0) {
        /* we have to complete server and port from the sAddr */
        if (ai_family == PF_INET)
            addrlen = sizeof (struct sockaddr_in);
        else
            addrlen = sizeof (struct sockaddr_in6);

        memset (host, '\0', hostlen);
        memset (port, '\0', portlen);

        if ((err =
             getnameinfo ((struct sockaddr *) sAddr, addrlen, host, hostlen, port, portlen,
                          NI_NUMERICHOST | NI_NUMERICSERV)) < 0) {
            
		    d_printf ("dns_filladdr (getnameinfo): %s\n", gai_strerror (err));
            return -1;
        }

		if (strstr (host, "::ffff:") != NULL) {
			for (i = 0; host[i + 7] != 0; i++)
				host[i] = host[i+7];
			host[i] = 0;
		}
    }
    else {
        /* we have to complete the sAddr struct */
        memset (&hints, '\0', sizeof (struct addrinfo));
        hints.ai_family = ai_family;
        hints.ai_socktype = SOCK_DGRAM;

        if ((err = getaddrinfo (host, port, &hints, &res)) < 0) {
            d_printf ("dns_filladdr (getaddrinfo):%s\n", gai_strerror (err));
            return -1;
        }

        // i hope it's enough to copy only sizeof (struct sockaddr) ?
        memcpy (sAddr, res->ai_addr, res->ai_addrlen);

        freeaddrinfo (res);
    }

#endif

    return 0;
}


/* send text to someone */
void
udp_send (int sock, char *text, int len, struct _sockaddr *sAddr, int ai_family)
{
    int addrlen = sizeof (struct sockaddr_in);

#ifndef _WIN32
    if (ai_family == PF_INET)
        addrlen = sizeof (struct sockaddr_in);
    else
        addrlen = sizeof (struct sockaddr_in6);
#endif

    if (sendto (sock, text, len, 0, (struct sockaddr *) sAddr, addrlen) == -1)
        perror ("udp_send :");
};


/* send udp broadcasted message */
void udp_sendbroadcast (int sock, char *text, int len, struct _sockaddr *sAddr, int ai_family)
{
	char value;
	
	value = 1;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &value, sizeof (value));
	udp_send (sock, text, len, sAddr, ai_family);
	value = 0;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &value, sizeof (value));
};

int
udp_server (char *port, int ai_family)
{
#ifdef _WIN32

    int sock;
    struct sockaddr_in sAddr;   // my address information

    if ((sock = socket (ai_family, SOCK_DGRAM, 0)) == -1) {
        perror ("udp_server: socket");
        return -1;
    }

    sAddr.sin_family = AF_INET; // host byte order
    sAddr.sin_port = htons (atoi (port)); // short, network byte order
    sAddr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP

    memset (&(sAddr.sin_zero), '\0', 8); // zero the rest of the struct

    if (bind (sock, (struct sockaddr *) &sAddr, sizeof (struct sockaddr)) == -1) {
        perror ("udp_server: bind");
        return -1;
    }

#else
    struct addrinfo hints,
     *res,
     *sres;
    int err,
      sock,
      ai_family_;

    memset (&hints, '\0', sizeof (struct addrinfo));

    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_DGRAM;

    ai_family_ = 0;

    if ((err = getaddrinfo (NULL, port, &hints, &res)) == 0) {
        sres = res;
        while ((ai_family_ == 0) && (sres)) {
            if (sres->ai_family == ai_family || ai_family == PF_UNSPEC)
                ai_family_ = sres->ai_family;
            else
                sres = sres->ai_next;
        }

        if (sres == NULL)
            sres = res;

        ai_family_ = sres->ai_family;
        if (ai_family_ != ai_family && ai_family != PF_UNSPEC) {
            // ai_family is not identic
            freeaddrinfo (res);
            return -1;
        }

        if ((sock = socket (sres->ai_family, SOCK_DGRAM, 0)) < 0) {
            perror ("UDP_Server (socket):");
            freeaddrinfo (res);
            return -1;
        }

        if ((err = bind (sock, sres->ai_addr, sres->ai_addrlen)) < 0) {
            perror ("UDP_Server (bind):");
            close (sock);
            freeaddrinfo (res);
            return -1;
        }

        freeaddrinfo (res);
    }
    else {
        sock = -1;
        d_printf ("UDP_Server (getaddrinfo):%s\n", gai_strerror (err));
    }
#endif

    return sock;
};

/*
   gets some text
   RESULT: 0 for nothing on there 
*/
int
udp_get (int sock, char *text, int len, struct _sockaddr *sAddr, int ai_family)
{
#ifdef _WIN32
	int clen;
#else
	unsigned int clen;
#endif	
	unsigned int msglen;
    fd_set sockset;
    struct timeval tval;

	if (sock == -1)
		return -1;
	
    /* what version of tcp/ip we're using */
    if (ai_family == AF_INET)
        clen = sizeof (struct sockaddr_in);
#ifndef _WIN32
    else
        clen = sizeof (struct sockaddr_in6);
#endif

    memset (text, '\0', len);

    // check if we have got any data
    FD_ZERO (&sockset);
    FD_SET (sock, &sockset);

    tval.tv_sec = 0;
	tval.tv_usec = 100;
    msglen = 0;
    if (select (sock + 1, &sockset, NULL, NULL, &tval)) if (FD_ISSET (sock, &sockset)) {
        msglen = recvfrom (sock, text, len, MSG_DONTWAIT, (struct sockaddr *) sAddr, &clen);
        if (msglen < 0)
            return 0;

        if ((msglen >= 0) && (msglen < len))
            text[msglen] = 0;
    }
    return msglen;
};


char *
dns_net_getip (char *host)
{
    struct hostent *hAddr;

	hAddr = gethostbyname (host);
    if (hAddr == NULL)
        return NULL;
    strncpy (dnsip, inet_ntoa (*((struct in_addr *) hAddr->h_addr)), UDP_LEN_HOSTNAME);

	return dnsip;
};
