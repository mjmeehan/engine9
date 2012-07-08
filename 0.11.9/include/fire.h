#ifndef _FIRE_H_
#define _FIRE_H_


void fire_loop ();

void spawn_fire (int x, int y, int intensity);
void spread_fire ();
void draw_fire ();
extern void fire_clear ();
extern void fire_init ();
void collide_fire ();

#endif
