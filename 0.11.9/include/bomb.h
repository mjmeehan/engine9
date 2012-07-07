/* $Id: bomb.h,v 1.8 2009-05-11 20:51:25 stpohle Exp $
 * bomb include file
 */

#ifndef _BOMB_H_
#define _BOMB_H_


enum _bombstate {
        BS_off = 0,
        BS_ticking,
        BS_exploding,
        BS_trigger
};


enum _bombmode {
        BM_normal = 0,
        BM_pushed,
        BM_moving,
        BM_liquid,
        BM_kicked
};


struct {
        _pointf pos;            // position of the bomb.
		_pointf oldpos;			// old position of the bomb.
        struct __bomb_id {      // save the bomb id
                signed char p;  // playernumber of this bomb
                signed char b;  // bombnumber of this bomb
				signed char pIgnition; // playernumber of ignition explode
        } id;
        float firer[4];         // range of the fire for the fire for each direction
        int firemaxr[4];        // max range reached?
        float to;               // timeout in ms after dropping the bomb. (loops * 0.0005sec)
        float frame;            // frame of the animation
        unsigned char r;        // range of the bomb
        unsigned char state;    // state of the bomb BS_*
        unsigned char mode;     // mode of the bomb BM_*
        int ex_nr;              // explosion number
		_pointf source;			// start of a kicked bomb
        _pointf dest;            // destination to move the bomb to
		float fdata;			// float data: speed (moving bombs), pos (kicked bombs)
} typedef _bomb;



// for the bomb..
extern void bomb_loop ();
extern void bomb_explode (_bomb * bomb, int net);
extern inline void bomb_action (_bomb * bomb);
extern void bomb_move (_bomb * bomb);
extern void bomb_kicked (_bomb * bomb);
extern void get_bomb_on (float x, float y, _point bombs[]);
extern void explosion_do (_bomb * bomb);
extern void explosion_restore (_bomb * bomb);
extern int explosion_check_field (int x, int y, _bomb * bomb);


#endif
