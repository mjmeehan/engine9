/* $id:$ */

#ifndef _FLYINGITEMS_H_
#define _FLYINGITEMS_H_

/* maximum number of items which are saved in the list */
#define FLITEMS_MAXITEMS 250

struct __flyingitem {
	_pointf pos;	// current position
	_pointf from;	// position from where the items comes
	_pointf to;		// position to where it is going
	float step;		// step 0.0 is start 1.0 is end position
	unsigned char type;	// type
	struct __flyingitem *next;
} typedef _flyingitem;

extern void flitems_loop ();
extern _flyingitem *flitems_findfree ();
extern void flitems_dropitems (int p_nr, _pointf from, int cnt_speed, int cnt_bombs, int cnt_range);
extern void flitems_draw (_flyingitem *flitem);
extern _flyingitem *flitems_additem (_pointf from, _point to, signed char type);
extern void flitems_reset ();
extern _point flitems_randompos (int p_nr);
extern int flitems_checkfreepos (_point to);

#endif
