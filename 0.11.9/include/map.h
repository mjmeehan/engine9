/* $Id: map.h,v 1.16 2006-07-30 11:44:57 stpohle Exp $ */
/* map.h */

#ifndef _MAP_H_
#define _MAP_H_

#define FIELDCHECK_TIMEOUT 2.0 /* timeout for the field check function */
#define FIELDHURRYWARN 5.0  /* hurry warning */
#define FIELDHURRYDROPTO 0.5 /* timeout when to put in some special items */
#define FIELDHURRYSIZE 0.25    /* timeout forresizeing the game */
#define FIELDHURRYSIZEMIN 5 /* min size for the field */
#define FIELDHURRYTIMEOUT 120.0	// game timeout for hurry and dropping mode (1min)

#define ANI_STONETIMEOUT 1.0	// factor for the animation frame je seconds
#define ANI_POWERUPTIMEOUT 0.5  // factor for powerup animations

/* this is the minimum distance that starting points should be set apart from each
 * other.  If the given tolerence cannot be satisfied on the map, it will be decremented
 * by 1 until it can be satisfied.  this is just our initial value
 */
 
#define MAP_POSITION_TOLERENCE 10

struct __ex_field {
	unsigned char count;
	float frame;
	int bomb_b;			// BombID from the last know
	int bomb_p;			// explosion on this field
} typedef _ex_field;


struct __field {
    unsigned char type;
	signed char mixframe;		// data for the mixed frame
    float frame;                // frame (frame > 0 && FS_stone)
    unsigned char special;      // to save special stones, or the tunnel number
    _ex_field ex[4];          	// count up every explosion there is on this field for ever direction
    Sint32 ex_nr;               // number to identify the explosion.
} typedef _field;


/* holds the locatition of a start point and the use count */

struct __start_point {
	_point pos;
	int used;
} typedef _start_point;


struct __map {
	_point size;			// dimension of the field

    _field field[MAX_FIELDSIZE_X][MAX_FIELDSIZE_Y];
	_point tunnel[GAME_MAX_TUNNELS];  // save the destination of the tunnel
	unsigned char bfield[MAX_FIELDSIZE_X][MAX_FIELDSIZE_Y]; // will hold informations if ther is a bomb

	char tileset [LEN_TILESETNAME];
	signed char random_tileset;
	char map [LEN_PATHFILENAME];
	signed char map_selection;
	signed char type;		// type of the map (MAPT_*);
	int bombs;
	int fire;
	int shoes;
	int mixed;
	int death;
	int sp_trigger;
	int sp_push;
	int sp_row;
	int sp_kick;
	unsigned char state; // state of the map
	_start_point start_point[MAX_PLAYERS];  // array of starting points for this map
} typedef _map;


extern _map map;

// mapmenu.c
extern void mapmenu ();
extern void mapinfo ();
extern void mapgamesetting ();

// map.c
extern void map_random ();
extern void map_genrandom ();
extern void map_genmorerandom ();
extern void init_map_tileset();
extern void map_new (char *filename);
extern void map_load (FILE * fmap);
extern void map_set_playerposition (int usermap);
/* new functions for player positioning */

extern int map_ensure_corner_start_points();
extern int map_init_start_points();
extern int map_set_start_point(int idx, int x, int y);
extern int map_add_start_point(int x, int y);
extern int map_is_start_point(int x, int y);
extern int map_num_defined_start_points();
extern int map_check_start_point(int x, int y, int tol);
extern int map_check_and_add_start_point(int x, int y, int tol);
extern int map_find_and_add_start_points(int num, int tol);
extern int map_is_possible_start_point(int x, int y);
extern int map_create_and_add_start_point(int tol);
extern int map_is_removable_field(int x, int y);
extern int map_place_player(int pl);
extern int map_respawn_player(int pl);

#endif
