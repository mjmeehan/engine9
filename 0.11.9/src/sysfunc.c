/* $Id: sysfunc.c,v 1.26 2007-12-09 22:13:03 stpohle Exp $
    sysfunc.c - this file hold some routines for the system functions..
    like d_delay
*/

#include "bomberclone.h"
#include "sysfunc.h"

/* i have no banned usleep completly, even usleep would 
   bring the cpu usage more down. but i found out that 
   usleep is not as excact as select is. */
void
s_delay (int ms)
{
#ifdef _WIN32
	Sleep (ms);   /* i hope this works on windows.. on MSVC it
	                 should but with MINGW i don't know yet */
#else
    fd_set selectset;
    struct timeval tval;

	FD_ZERO (&selectset);
	tval.tv_sec = 0;
	tval.tv_usec = ms * 1000;
	select (1, &selectset, NULL, NULL, &tval);
#endif
};


int
s_fetchevent (SDL_Event *event)
{
	if (SDL_PollEvent (event))
		return 1;
  	s_delay (20);
	return 0;
}


int
s_random (int maxnr)
{
#if ( defined _WIN32 ) || ( RAND_MAX < 2147483647UL )
    return ((rand () * maxnr) / RAND_MAX);
#else
    int i;

    i = (((rand () >> 16) * (maxnr + 1)) / (RAND_MAX >> 16));
    if (i >= maxnr)
        i = 0;
    return i;
#endif
};


#if !defined(HAVE_RINTF) && !defined(HAVE_RINT)
inline float rintf (float f) {
       if (CUTINT (f) < 0.5f)
               return (floorf (f));
       else
               return (floorf (f + 1.0f));
};
#endif



static char homedir[255];

char *
s_gethomedir ()
{
    char *hd;

    if ((hd = getenv ("HOME")) == NULL) {
        /* Homedir konnte nicht ermittelt werden. */
        homedir[0] = 0;
        d_printf ("Variable HOME could not be found\n");
    }
    else {
        strncpy (homedir, hd, 253);
        homedir[strlen (homedir) + 1] = 0;
#ifdef _WIN32
        if (homedir[strlen (homedir) - 1] != '\\')
            homedir[strlen (homedir)] = '\\';
#else
        if (homedir[strlen (homedir) - 1] != '/')
            homedir[strlen (homedir)] = '/';
#endif
    }
    return homedir;
};

static _direntry direntrys[MAX_DIRENTRYS];

_direntry *
s_getdir (char *path)
{
    int entrynr = 0;

#ifdef _WIN32
    WIN32_FIND_DATA fdata;
    HANDLE fhandle;
    struct stat fstat;
    char filename[LEN_PATHFILENAME];

	sprintf (filename, "%s\\*.*", path);
	d_printf ("Reading Dir [%s]\n", filename);
    if ((fhandle = FindFirstFile (filename, &fdata)) != INVALID_HANDLE_VALUE) {
        do {
			d_printf ("  Got Somthing [%s]\n",fdata.cFileName);

            direntrys[entrynr].next = NULL;
            strncpy (direntrys[entrynr].name, fdata.cFileName, LEN_FILENAME - 1);
            if (strlen (fdata.cFileName) >= LEN_FILENAME)
                direntrys[entrynr].name[LEN_FILENAME - 1] = 0;
            sprintf (filename, "%s\\%s", path, direntrys[entrynr].name);
            stat (filename, &fstat);
            if (S_ISREG (fstat.st_mode)) {
                direntrys[entrynr].flags = DF_file;
                direntrys[entrynr].next = &direntrys[entrynr + 1];
                entrynr++;
            }
            else if (S_ISDIR (fstat.st_mode)) {
                direntrys[entrynr].flags = DF_dir;
                direntrys[entrynr].next = &direntrys[entrynr + 1];
                entrynr++;
            }
        } while (FindNextFile (fhandle, &fdata) && entrynr < MAX_DIRENTRYS);
		
        FindClose (fhandle);
    }

#else
    DIR *dp;
    struct dirent *ep;
    struct stat fstat;
    char filename[LEN_PATHFILENAME];

    dp = opendir (path);
    if (dp != NULL) {
        while ((ep = readdir (dp)) != NULL && entrynr < MAX_DIRENTRYS) {
            direntrys[entrynr].next = NULL;
            strncpy (direntrys[entrynr].name, ep->d_name, LEN_FILENAME - 1);
            if (strlen (ep->d_name) >= LEN_FILENAME)
                direntrys[entrynr].name[LEN_FILENAME - 1] = 0;

            sprintf (filename, "%s/%s", path, direntrys[entrynr].name);
            stat (filename, &fstat);
            if (S_ISREG (fstat.st_mode)) {
                direntrys[entrynr].flags = DF_file;
                direntrys[entrynr].next = &direntrys[entrynr + 1];
                entrynr++;
            }
            else if (S_ISDIR (fstat.st_mode)) {
                direntrys[entrynr].flags = DF_dir;
                direntrys[entrynr].next = &direntrys[entrynr + 1];
                entrynr++;
            }
        }
        closedir (dp);
    }
#endif
	d_printf ("Readin %d Entrys in the Directory\n", entrynr);
    if (entrynr == 0)
        return NULL;
    direntrys[entrynr - 1].next = NULL;

    return &direntrys[0];
};

_direntry *
s_dirfilter (_direntry * dirstart, signed char dirflags)
{
    _direntry *newstart = NULL,
     *pos = NULL,
     *old = NULL;

    for (pos = dirstart; pos != NULL; pos = pos->next)
        if (pos->name[0] != '.' && (pos->flags & dirflags) != 0) {
            if (newstart == NULL) {
                newstart = pos;
                old = pos;
            }
            else {
                old->next = pos;
                old = pos;
            }
        }

    if (old != NULL)
        old->next = NULL;

    return newstart;
};


/* count the bits ... for directions and so on */
int s_countbits (int bits, int nr) {
	int i, r = 0;

	for (i = nr-1; i >= 0; i--)
		if ((bits & (1 << i)) != 0)
			r++;
		
	return r;
}


// Return only the file name
char* getfilename(char* path)
{
	int i;
	for(i=strlen(path);i>=0;i--)
		if(path[i] == '\\' || path[i] == '/')
			return path+i+1;
	return path;
}


/* swap 16bit integers, needed for
   little and big endian convert */
inline Sint16 s_swap16 (Sint16 i) {
	Sint16 r;
	char *z1 = (char *)&i;
	char *z2 = (char *)&r;
	
	*(z2+1) = *z1;
	*z2 = *(z1+1);
	
	return r;
};


/* swap 32bit integers, needed for
   little and big endian convert */
inline Sint32 s_swap32 (Sint32 i) {
	Sint32 r;
	int j;
	char *z1 = (char *) &i;
	char *z2 = (char *) &r;
	
	for (j = 0; j < 4; j++)
		*(z2+j) = *(z1+(3-j));
	
	return r;
};


extern Uint32 game_timediff, game_timediff1;

inline void s_calctimesync () {
	Uint32 timeloop1;
	
    // calculate time sync.
    timeloop1 = SDL_GetTicks ();
    game_timediff = timeloop1 - timestamp; // only for debugging needed

	while (timeloop1 - timestamp >= 3 && timeloop1 - timestamp < 20) {
        s_delay (20 - (timeloop1 - timestamp) - 1);
        timeloop1 = SDL_GetTicks ();
    }

    game_timediff1 = timeloop1 - timestamp;
    timestamp = timeloop1;
		
	timefactor = ((float)game_timediff1) / 20.0f;
	timediff = ((float)game_timediff1) / 1000.0f;
}


/* clipping of a rectangle before blitting
 * src    - Source Image
 * dest   - Destination Image
 * window - Window in the Destination 
 */
void rect_clipping (SDL_Rect *src, SDL_Rect *dest, SDL_Rect *window, SDL_Rect *csrc, SDL_Rect *cdest) {
	/* X - Position */
	if (dest->x > window->x)
		cdest->x = dest->x;
	else
		cdest->x = window->x;
	if ((dest->x + dest->w)<(window->x + window->w))
		cdest->w = (dest->x + dest->w) - cdest->x;
	else
		cdest->w = (window->x + window->w) - cdest->x;

	/* X - Position */
	if (dest->y > window->y)
		cdest->y = dest->y;
	else
		cdest->y = window->y;
	if ((dest->y + dest->h)<(window->y + window->h))
		cdest->h = (dest->y + dest->h) - cdest->y;
	else
		cdest->h = (window->y + window->h) - cdest->y;

	/* setup the clipping source */	
	csrc->x = src->x + (cdest->x - dest->x);
	csrc->w = cdest->w;
	csrc->y = src->y + (cdest->y - dest->y);
	csrc->h = cdest->h;
};


/* fills the list->next pointer with the next element 
 * as it would be with an array */
void charlist_fillarraypointer (_charlist *list, int c) {
	int i = 0;
	
	if (c <= 0) return;
	do {
		if (i < c - 1)
			list[i].next = &list[i+1];
		else 
			list[i].next = NULL;
		i++;
	} while (i < c);
};



/* find the text in the list */
_charlist *charlist_findtext (_charlist *list, char *text) {
	_charlist *result;
	
	for (result = list; result != NULL && (strncmp (result->text, text, 255) != 0); result = result->next);
	
	return result;
};

float absol(float f) {
	if (f<0) f*=-1;
return f;
}



