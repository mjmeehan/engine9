#ifndef _FIRE_H_
#define _FIRE_H_


void fire_loop ();

void spawn_fire (int x, int y, int intensity);
void spread_fire ();
void fire_draw ();
extern void fire_clear ();
extern void fire_init ();
float fire_spread_time();
void collide_fire ();

#endif
