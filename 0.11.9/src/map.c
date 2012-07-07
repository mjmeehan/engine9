/* $Id: map.c,v 1.28 2009-05-11 20:51:25 stpohle Exp $ */
/* map handling, like generate and load maps. */

#include "bomberclone.h"
#include "player.h"

_map map;

// put the special items into the field
void
map_fillitems (int fieldtype, int num)
{
    int nb_try = 100,
        d,
        x,
        y;
    /* this is the item factor we multiply it with this so we know
       how much items we want in the game */
    float fkt = ((float) (map.size.x * map.size.y)) / (25.0 * 17.0);

    for (d = 0; d < num * fkt; d++) {
        x = y = 0;
        while (map.field[x][y].type != FT_stone && map.field[x][y].special != FT_nothing) {
            x = ((float) rand () / (float) RAND_MAX) * (map.size.x - 1);
            y = ((float) rand () / (float) RAND_MAX) * (map.size.y - 1);
            nb_try--;
            if (nb_try < 0)
                break;
        }
        if (map.field[x][y].type != FT_tunnel)
            map.field[x][y].special = fieldtype;
    }
}


// loads or generate an map
void
map_new (char *filename)
{
    int x,
      y;
    FILE *fmap;
    signed char old_maptype = map.type;
    int pl_cnt, pl;
    
    /* initialize the start_point array in the _map struct */
    
    map_init_start_points();

    if (filename) {
        fmap = fopen (filename, "r");

        /* if we can't open the given filename for any reason, reverting 
           to default value else, load the file map */
        if (fmap)
            map_load (fmap);
    }
    else
        fmap = NULL;

    // Clean and create the field //
    if (fmap == NULL)
		map_genrandom ();
	
	/* Generate a More random map if requested */
    if (map.map_selection == MAPS_morerand)
        map_genmorerandom();
    
    if (map.type == -1)
        map.type = s_random (MAPT_max);

    if (map.type == MAPT_tunnel) {
        /* insert tunnels */
        for (x = 0; x < GAME_MAX_TUNNELS; x++)
            map.tunnel[x].x = map.tunnel[x].y = -1;
        map.field[3][3].type = FT_tunnel;
        map.field[3][3].special = 0;
        map.field[map.size.x - 4][map.size.y - 4].type = FT_tunnel;
        map.field[map.size.x - 4][map.size.y - 4].special = 1;

        if (map.size.y > 12) {
            map.field[map.size.x - 4][3].type = FT_tunnel;
            map.field[map.size.x - 4][3].special = 2;
            map.field[3][map.size.y - 4].type = FT_tunnel;
            map.field[3][map.size.y - 4].special = 3;

            map.tunnel[0].x = map.size.x - 4;
            map.tunnel[0].y = 3;
            map.tunnel[1].x = 3;
            map.tunnel[1].y = map.size.y - 4;
            map.tunnel[2].x = map.size.x - 4;
            map.tunnel[2].y = map.size.y - 4;
            map.tunnel[3].x = 3;
            map.tunnel[3].y = 3;
        }
        else {
            map.tunnel[0].x = map.size.x - 4;
            map.tunnel[0].y = map.size.y - 4;
            map.tunnel[1].x = 3;
            map.tunnel[1].y = 3;
        }
    }


    /* delete the bfield data */
    for (x = 0; x < MAX_FIELDSIZE_X; x++)
        for (y = 0; y < MAX_FIELDSIZE_Y; y++)
            map.bfield[x][y] = 0;

    /* count the number of players on this map so we know how many starting points
     * to find
     */
     
    pl_cnt = 0;
    for (pl = 0; pl < MAX_PLAYERS; pl++) {
        if (PS_IS_used (players[pl].state)) {
            pl_cnt++;
        }
    }
    
    
    /* identify possible starting positions for players and store them in the
     * start_point array in the _map struct.  This will always succeed.  If
     * it cannot find starting points within the tolerance, it first attempts
     * to create start points within the tolerence and otherwise lowers the
     * tolerence until it can satisfy the proper number of start points.
     * eventually the tolerence reaches 0, so it can, in the worst case, start
     * all players at the same start point.
     */
     
    map_find_and_add_start_points(pl_cnt - map_num_defined_start_points(), MAP_POSITION_TOLERENCE);

    /* Set the Playerinformation */
    map_set_playerposition (fmap != NULL);

    /* put the fire powerups in the field */
    map_fillitems (FT_fire, map.fire);
    /* put the bomb powerups in the field */
    map_fillitems (FT_bomb, map.bombs);
    /* put the shoe powerup in the field */
    map_fillitems (FT_shoe, map.shoes);
    /* put the death ?powerups? in the field */
    map_fillitems (FT_death, map.death);
    /* put the mixed powerrup in the field */
    map_fillitems (FT_mixed, map.mixed);
    /* put the trigger special in the field */
    map_fillitems (FT_sp_trigger, map.sp_trigger);
    /* put the row special in the field */
    map_fillitems (FT_sp_row, map.sp_row);
    /* put the push special in the field */
    map_fillitems (FT_sp_push, map.sp_push);
    map_fillitems (FT_sp_liquid, map.sp_push);
    map_fillitems (FT_sp_moved, map.sp_push);
    /* put the push special in the field */
	map_fillitems(FT_sp_kick,map.sp_kick);

    map.type = old_maptype;
}

void
map_genrandom ()
{
    int x,
      y,
      d;

    /* if we can't load the map check first the fieldsize settings */
    if (map.size.x < MIN_FIELDSIZE_X)
        map.size.x = MIN_FIELDSIZE_X;
    if (map.size.x > MAX_FIELDSIZE_X)
        map.size.x = MAX_FIELDSIZE_X;

    for (x = 0; x < map.size.x; x++)
        for (y = 0; y < map.size.y; y++) {
            if ((y == 0) || (y == map.size.y - 1))
                map.field[x][y].type = FT_block;
            else if ((x == 0) || (x == map.size.x - 1))
                map.field[x][y].type = FT_block;
            else if (((x & 1) == 0) && ((y & 1) == 0))
                map.field[x][y].type = FT_block;
            else {
                // create random field
                if ((s_random (256) & 3) == 0)
                    map.field[x][y].type = FT_nothing;
                else
                    map.field[x][y].type = FT_stone;
            }

            for (d = 0; d < 4; d++)
                map.field[x][y].ex[d].frame = map.field[x][y].ex[d].count = 0;
            map.field[x][y].ex_nr = -1;
            map.field[x][y].frame = 0.0f;
            map.field[x][y].special = FT_nothing;
        }
	
	/* set the corners of the map to be valid start points */
	
	// map_ensure_corner_start_points();
}

void
map_genmorerandom ()
{
    int x,
      y,
      d,
      ra;

    /* This is an enhanced version of genrandom() used by "more random" */
    d_printf("genmorerandom: *** init ***\n");
    /* if we can't load the map check first the fieldsize settings */
    if (map.size.x < MIN_FIELDSIZE_X)
        map.size.x = MIN_FIELDSIZE_X;
    if (map.size.x > MAX_FIELDSIZE_X)
        map.size.x = MAX_FIELDSIZE_X;

    for (x = 0; x < map.size.x; x++)
        for (y = 0; y < map.size.y; y++) {
            if ((y == 0) || (y == map.size.y - 1))
                map.field[x][y].type = FT_block;
            else if ((x == 0) || (x == map.size.x - 1))
                map.field[x][y].type = FT_block;
            else {
                // create random field
                ra = s_random (256) & 3;
                d_printf("genmorerandom: ra = %i\n", ra);
                
                if (ra == 0)
                    map.field[x][y].type = FT_nothing;
                else if (ra == 1)
                    map.field[x][y].type = FT_block;
                else
                    map.field[x][y].type = FT_stone;
            }

            for (d = 0; d < 4; d++)
                map.field[x][y].ex[d].frame = map.field[x][y].ex[d].count = 0;
            map.field[x][y].ex_nr = -1;
            map.field[x][y].frame = 0.0f;
            map.field[x][y].special = FT_nothing;
        }
	
	d_printf("genmorerandom: *** exit ***\n");
	/* set the corners of the map to be valid start points */
	
	// map_ensure_corner_start_points();
}


/* will set the playerposition but in a way that we won't start on a block */
/* i am just too lazy to write this all again and again */
void
map_set_playerposition (int usermap)
{
    int pl;
	
	d_printf ("map_set_playerposition\n");
    
    /* This is the new code that will set every player in a starting point
     * It should never fail, but if it does, it will fall through to the old method
     */
    
    for (pl = 0; pl < MAX_PLAYERS; pl++) {
        if (PS_IS_used(players[pl].state)) {
            map_place_player(pl);
        }
    }
}
    
/* load a random map */
void
map_random ()
{
    _direntry *destart,
     *de,
     *desel;
    char path[LEN_PATHFILENAME];
    int max,
      sel;

    sprintf (path, "%s/maps", bman.datapath);
    desel = destart = s_getdir (path);

    for (max = 0, de = destart; de != NULL; de = de->next)
        if ((de->flags & DF_file) == DF_file)
            max++;

    sel = s_random (max);
    for (max = 0, de = destart; max <= sel && de != NULL; de = de->next)
        if ((de->flags & DF_file) == DF_file) {
            desel = de;
            max++;
        }

    d_printf ("Random Map %s (%d on %d)\n", desel->name, sel, max);

    if (desel != NULL)
        sprintf (map.map, "%s/maps/%s", bman.datapath, desel->name);
}


// Init the game according to options
void
init_map_tileset ()
{
	if (GT_MP_PTPM || GT_SP) {
		switch (map.map_selection) {
    		case (0):
        		map_new (map.map);
        		break;
    		case (1):
        		map_random ();
        		map_new (map.map);
        		break;
    		case (2):
        		map_new (NULL);
            case (3):
				map_new (NULL); /* for more random */
        	break;
		}
		if (map.random_tileset)
    	   	tileset_random ();
	}
}


/* read from an open file map, determine field.x and field.y 
   and fill the field. 
   (# correspond to a bloc and @ correspond to a stone,
    an espace is nothing ' '
    % are commentary at the beginning of the map */
void
map_load (FILE * fmap)
{
    size_t length;
    char *currentline;
    char tmp[MAX_FIELDSIZE_X];
    int sizex = 0;
    int sizey = 0;
    int i;
    int d;

    while ((currentline = fgets (tmp, MAX_FIELDSIZE_X, fmap))) {
        length = strlen (currentline);
        if (currentline[0] == '%')
            continue;
        /* now each line correspond to the field */
        else if (strstr (currentline, "bombs") == currentline) // bombs
            map.bombs = atoi (strchr (currentline, '=') + 1);
        else if (strstr (currentline, "fire") == currentline) // fire
            map.fire = atoi (strchr (currentline, '=') + 1);
        else if (strstr (currentline, "shoes") == currentline) // shoes
            map.shoes = atoi (strchr (currentline, '=') + 1);
        else if (strstr (currentline, "mixed") == currentline) // mixed
            map.mixed = atoi (strchr (currentline, '=') + 1);
        else if (strstr (currentline, "death") == currentline) // death
            map.death = atoi (strchr (currentline, '=') + 1);
        else if (strstr (currentline, "sp_trigger") == currentline) // trigger special
            map.sp_trigger = atoi (strchr (currentline, '=') + 1);
        else if (strstr (currentline, "sp_push") == currentline) // push special
            map.sp_push = atoi (strchr (currentline, '=') + 1);
        else if (strstr (currentline, "sp_row") == currentline) // row special
            map.sp_row = atoi (strchr (currentline, '=') + 1);
        else if (currentline[0] == '#') { /* the map itself */
            for (i = 0; i < length; i++) {
                switch (currentline[i]) {
                case '#':
                    map.field[i][sizey].type = FT_block;
                    break;
                case '@':
                    map.field[i][sizey].type = FT_stone;
                    break;
                case ' ':
                    map.field[i][sizey].type = FT_nothing;
                default:
                    break;
                }
                for (d = 0; d < 4; d++)
                    map.field[i][sizey].ex[d].frame = map.field[i][sizey].ex[d].count = 0;
                map.field[i][sizey].ex_nr = -1;
                map.field[i][sizey].frame = 0.0f;
                map.field[i][sizey].special = FT_nothing;
            }
            sizey++;
            if (sizex < length)
                sizex = length;
        }
    }

    map.size.x = sizex - 1;
    map.size.y = sizey;

    /* darw the border so we know everything is right */
    for (i = 0; i < map.size.x; i++)
        map.field[i][0].type = map.field[i][map.size.y - 1].type = FT_block;
    for (i = 0; i < map.size.y; i++)
        map.field[0][i].type = map.field[map.size.x - 1][i].type = FT_block;

    fclose (fmap);
};


/* This is called for randomly generated maps.  It clears out each corner of the map
 * to make sure that the 4 corners are legal start points.
 */

int
map_ensure_corner_start_points ()
{

    /* make sure all the corners are empty as well as the 1 field in the Y direction
     * and one in the X direction
     */
    
    /* top left corner is safe start point */
    map.field[1][1].type = FT_nothing;
    map.field[1][2].type = FT_nothing;
    map.field[2][1].type = FT_nothing;
    
    map_add_start_point(1, 1);

    /* bottom left corner is safe start point */
    map.field[1][map.size.y - 2].type = FT_nothing;
    map.field[1][map.size.y - 3].type = FT_nothing;
    map.field[2][map.size.y - 2].type = FT_nothing;
    
    map_add_start_point(1, map.size.y - 2);
    
    /* top right corner is safe start point */
    map.field[map.size.x - 2][1].type = FT_nothing;
    map.field[map.size.x - 3][1].type = FT_nothing;
    map.field[map.size.x - 2][2].type = FT_nothing;
    
    map_add_start_point(map.size.x - 2, 1);
    
    /* bottom right corner is safe start point */
    map.field[map.size.x - 2][map.size.y - 2].type = FT_nothing;
    map.field[map.size.x - 2][map.size.y - 3].type = FT_nothing;
    map.field[map.size.x - 3][map.size.y - 2].type = FT_nothing;
    
    map_add_start_point(map.size.x - 2, map.size.y - 2);
    
    return 1;
}


/* initializes all the start points for the map to (-1,-1) and their used flag to 0 */

int
map_init_start_points()
{
    int i;
    
    for (i = 0; i < MAX_PLAYERS; i++) {
        map.start_point[i].pos.x = -1;
        map.start_point[i].pos.y = -1;
        map.start_point[i].used = 0;
    }
    
    return 0;
}

/* blindly sets (x,y) as the idx'th start point in the map */

int
map_set_start_point(int idx, int x, int y)
{
    map.start_point[idx].pos.x = x;
    map.start_point[idx].pos.y = y;
    return idx;
}

/* checks to see if all start points have been set, if not, sets (x,y) as a possible start point
 * returns 0 on successful set, -1 on failure
 */

int
map_add_start_point(int x, int y)
{
    int i;
    
    /* find the first unset start point */
    for (i = 0; i < MAX_PLAYERS; i++) {
        
        if ((map.start_point[i].pos.x == -1) && (map.start_point[i].pos.y == -1)) {
            map_set_start_point(i, x, y);
            return 0;
        }
    }
    
    /* if all start points are already set, do nothing and return -1 */
    
    return -1;
}


/* returns 0 if (x,y) is not already set as a start point in the current map, nonzero otherwise */

int
map_is_start_point(int x, int y)
{
    int i;
    
    for (i = 0; i < MAX_PLAYERS; i++) {
        
        if ((map.start_point[i].pos.x == x) && (map.start_point[i].pos.y == y))
            return 1;
    }
    
    return 0;
}

/* returns the number of start points set in the current map */

int
map_num_defined_start_points()
{
    int pts = 0;
    int i;
    
    for (i = 0; i < MAX_PLAYERS; i++) {
        
        if ((map.start_point[i].pos.x != -1) && (map.start_point[i].pos.y != -1))
            pts++;
    }
    
    return pts;
}


/* checks if the start point (x, y) is far enough away from all the other start points 
 * returns 1 if it is far enough, 0 otherwise
 */

int
map_check_start_point(int x, int y, int tol)
{
    int i;
    int dx, dy;
    float dist;
    
    for (i = 0; i < MAX_PLAYERS; i++) {
        
        if ((map.start_point[i].pos.x != -1) && (map.start_point[i].pos.y != -1)) {
            
            dx = map.start_point[i].pos.x - x;
            dy = map.start_point[i].pos.y - y;
            
            dist = sqrt(dx * dx + dy * dy);
            
            if (dist < tol)
                return 0;
        }
    }
    
    return 1;
}


/* checks to see if there is an available start point and if (x, y) is sufficiently far from all
 * defined start points and adds (x, y) as a start point if the conditions are met.
 * returns 1 on success, 0 on failure
 */
 
int
map_check_and_add_start_point(int x, int y, int tol)
{
    if ((map_num_defined_start_points() < MAX_PLAYERS) && (map_check_start_point(x, y, tol))) {
        
        map_add_start_point(x, y);
        return 1;
    }
    
    return 0;
}


/* locates and adds num start points to the current map.  returns the number of start points added. */

int
map_find_and_add_start_points(int num, int tol)
{
    int x;
    int y;
    int i;
    int added = 0;
    
    while ((added < num) && (tol >= 0)) {
        for (x = 0; x < map.size.x; x++) {
            
            for (y = 0; y < map.size.y; y++) {
                
                if (map_is_possible_start_point(x, y)) {
                    
                    added += map_check_and_add_start_point(x, y, tol);
                }
            }
        }
        
        for (i = 0; i < num - added; i++) {
            
            added += map_create_and_add_start_point(tol);
        }
        
        tol--;
    }
    
    /* printf("Minimum Tolerance: %d\n", tol + 1); */
    
    return added;
}

/* returns 1 if (x,y) is a feasible start point such that the player can safely lay a bomb and not die
 * otherwise returns 0.  Note: this is a quick check only checking immediately adjacent fields.  It will not
 * check secondary adjacencies, however, the idea is that when looking for possible start poitns, it will be run
 * on every FT_nothing field so it will catch the secondary adjacencies when they become primary adjacencies
 */

int
map_is_possible_start_point(int x, int y)
{
    
    int x_ok_pos;
    int x_ok_neg;
    int y_ok_pos;
    int y_ok_neg;
    
    int x_adj = 0;
    int y_adj = 0;
    int i;
    
    /* if (x, y) is not FT_nothing, this is not a valid start point */
    
    if (map.field[x][y].type != FT_nothing) {
        
        return 0;
    }
    
    x_ok_pos = (x < map.size.x - 2) ? 1:0;
    x_ok_neg = (x > 1) ? 1:0;
    
    y_ok_pos = (y < map.size.y - 2) ? 1:0;
    y_ok_neg = (y > 1) ? 1:0;
    
    /* calculate the number of adjacent FT_nothing fields in the X and Y directions */
    
    for (i = 1; i < bman.start_range + 2; i++) {
        
        if (x_ok_pos) {
            
            if (map.field[x+i][y].type == FT_nothing) {
                
                x_adj++;
            } else {
                
                x_ok_pos = 0;
            }
        }
        
        if (x_ok_neg) {
            
            if (map.field[x-i][y].type == FT_nothing) {
                
                x_adj++;
            } else {
                
                x_ok_neg = 0;
            }
        }
        
        if (y_ok_pos) {
            
            if (map.field[x][y+i].type == FT_nothing) {
                
                y_adj++;
            } else {
                
                y_ok_pos = 0;
            }
        }
        
        if (y_ok_neg) {
            
            if (map.field[x][y-i].type == FT_nothing) {
                
                y_adj++;
            } else {
                y_ok_neg = 0;
            }
        }
    }
    
    if ((x_adj >= bman.start_range + 1) || (y_adj >= bman.start_range + 1)) {
        
        return 1;
    }
    
    if ((x_adj >= 1) && (y_adj >= 1)) {
        
        return 1;
    }
    
    return 0;
}


/* alters the map to create another start point at least tol units from any other
 * start point.  returns 1 on success, 0 on failure
 */

int
map_create_and_add_start_point(int tol)
{
    int x;
    int y;
    int dx;
    int dy;

    int init_x;
    int init_y;
    int end_x;
    int end_y;
    int step_x;
    int step_y;
    
    /* this changes how we traverse the map when looking for a place to put
     * a start point.  this is so all the start points don't get stuck in one
     * part of the map if the map is large enough
     */
    
    if (s_random(100) % 2) {
        
        init_x = 0;
        end_x = map.size.x;
        step_x = 1;
    } else {
        
        init_x = map.size.x - 1;
        end_x = -1;
        step_x = -1;
    }
    
    if (s_random(100) % 2) {
        
        init_y = 0;
        end_y = map.size.y;
        step_y = 1;
    } else {
        
        init_y = map.size.y - 1;
        end_y = -1;
        step_y = -1;
    }
    
    
    /* first try only FT_nothing fields as start points */
    
    for (x = init_x; x != end_x; x += step_x) {
        
        for (y = init_y; y != end_y; y+= step_y) {
            
            if ((map.field[x][y].type == FT_nothing) && (map_check_start_point(x, y, tol))) {
                
                dx = (x >= map.size.x - 2) ? -1:1;
                dy = (y >= map.size.y - 2) ? -1:1;
                
                if ((map_is_removable_field(x+dx, y)) && (map_is_removable_field(x, y+dy))) {
                    
                    /* printf("Creating Start Point (%d, %d).\n", x, y); */
                    
                    map.field[x][y].type = FT_nothing;
                
                    map.field[x+dx][y].type = FT_nothing;
                    map.field[x][y+dy].type = FT_nothing;
    
                    map_add_start_point(x, y);
                
                    return 1;
                }
            }
        }
    }
    
    /* if we get here we didn't find a useful FT_nothing field, so check the FT_stone
     * fields
     */
     
    for (x = init_x; x != end_x; x += step_x) {
        
        for (y = init_y; y != end_y; y+= step_y) {
            
            if ((map.field[x][y].type == FT_stone) && (map_check_start_point(x, y, tol))) {
                
                dx = (x >= map.size.x - 2) ? -1:1;
                dy = (y >= map.size.y - 2) ? -1:1;
                
                if ((map_is_removable_field(x+dx, y)) && (map_is_removable_field(x, y+dy))) {
                    
                    /* printf("Creating Start Point (%d, %d).\n", x, y); */
                    
                    map.field[x][y].type = FT_nothing;
                
                    map.field[x+dx][y].type = FT_nothing;
                    map.field[x][y+dy].type = FT_nothing;
    
                    map_add_start_point(x, y);
                
                    return 1;
                }
            }
        }
    }
    
    /* if we get to this point, we tried every field that we want to turn into a
     * start point, so we return 0 indicating failure
     */
    
    return 0;
}


/* checks the type of the field at (x, y).  if it is something we can remove without
 * drastically altering the map (that is to say, not a FT_tunnel or FT_block) returns
 * 1, returns 0 if this is not a field that we want to alter
 */
 
int
map_is_removable_field(int x, int y)
{
    if ((map.field[x][y].type == FT_nothing) || (map.field[x][y].type == FT_stone)) {
        
        return 1;
    } else {
        
        return 0;
    }
}


/* sets the players[pl] initial position to one of the start_points found in the map
 * randomly selects the start point so the same players don't always start near each
 * other
 */
 
int
map_place_player(int pl)
{
    int index;
    int start_points;
    int idx;
    int i;
    
    start_points = map_num_defined_start_points();
    index = (s_random(MAX_PLAYERS) + 1) % start_points;
    
    for (i = 0; i < start_points; i++) {
        
        idx = (index + i) % start_points;
        
        if ((!map.start_point[idx].used) && (map.start_point[idx].pos.x != -1)
            && (map.start_point[idx].pos.y != -1)) {
    
            players[pl].pos.x = map.start_point[idx].pos.x;
            players[pl].pos.y = map.start_point[idx].pos.y;
            map.start_point[idx].used = 1;
            
            return 1;
        }
    }
    
    return 0;
}


/* this will randomly select a start point, check to see if it is a safe place to
 * respawn the player, and then set their position.  It cannot fail unless there is
 * no start point possible on the map (which would prevent the game from ever starting)
 */

/* note: this is not used yet, but could be used to respawn players instead of the old
 * method
 */

int
map_respawn_player(int pl)
{
    int x;
    int y;
    
    do {
        
        x = s_random (map.size.x - 2) + 1;
        y = s_random (map.size.y - 2) + 1;
    } while (!map_is_possible_start_point(x, y));
    
    players[pl].pos.x = x;
    players[pl].pos.y = y;
    
    return 1;
}
