#include "bomberclone.h"
#include "fire.h"


void fire_loop()
{
	spread_fire ();
}

void fire_init()
{
	int x, y, i;
	fire_clear();
	d_printf("Initializing fires\n");
	for(i = 0; i < 6; i++) {
		x = 1 + s_random(map.size.x - 1);
    	y = 1 + s_random(map.size.y - 1);
    	spawn_fire (x, y, FIRE_high);
	}
}


void spawn_fire(int x, int y, int intensity) 
{
	static int fire_count = 0;

	/* fire only catches on combustables */
	if(FT_stone == map.field[x][y].type || FT_block == map.field[x][y].type ) {
    	d_printf ("Spawning fire at (%d, %d) with intensity\n", x, y, intensity);
		if(map.field[x][y].fire.intensity + intensity > FIRE_high) {
			map.field[x][y].fire.intensity = FIRE_high;
		} else {
			map.field[x][y].fire.intensity += intensity;
		}
		map.field[x][y].fire.spread = 4 + s_random(3);
		map.field[x][y].fire.frame=0.0f;
		fire_count++;
		stone_drawfire (x, y, -1);
	}
}

void spread_fire()
{
	int x, y, direction, intensity;
	_point target;
	for(x = 0; x < map.size.x; x++) {
		for( y = 0; y < map.size.y; y++) {
			/* update frames */
			map.field[x][y].fire.frame += timediff;
			if(map.field[x][y].fire.frame > gfx.fire.frames)
					map.field[x][y].fire.frame = 0.0f;
			map.field[x][y].fire.spread -= timediff;

			if(map.field[x][y].fire.intensity > FIRE_low && map.field[x][y].fire.spread < 0) {
				d_printf("Spreading fire at (%d, %d)\n", x, y);
				intensity = map.field[x][y].fire.intensity -1;
				for(direction = 0; direction < 4; direction++) {
					target.x = x;
					target.y = y;
					map_rel_direction(&target, direction, 1);
					spawn_fire(target.x, target.y, intensity);
				}
			}
		}
	}
}

void fire_clear () 
{
	int x, y;
	for (x = 0; x < map.size.x; x++) 
		for (y = 0; y < map.size.y; y++) 
			map.field[x][y].fire.intensity = FIRE_none;
			map.field[x][y].fire.spread = 0;
}



void collide_fire() 
{
	/* if players are near fire, make them sick */
	/* if players are in fire, kill them */
	/* if fire touches any stone, burn them and spawn more fire */
	/* if fire touches any blocks, burn them and subtract from the score */
}
