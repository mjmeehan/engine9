/* $Id: playerinput.c,v 1.3 2004-09-13 22:15:57 stpohle Exp $
 * playerinput 
 * 
 * the playerinput system will only set some values and flags of the player
 * not call a player_*() itself (i.e. for dropping a bomb). this will be
 * done with the player_*() functions which should be called only from the
 * game_loop();
 *
 * direction keys will set the moveing state (player->m) and the direction 
 * value (player->d) the bomb and special key will set a special flag this
 * the values will be used in player_loop () and the flags will be reset in 
 * there too.
 */


#include "bomberclone.h"
#include "player.h"
#include "keyb.h"
#include "chat.h"

/* 
 * handle the diffrent types of input devices and set the player informations
 * to use this function the function keyb_loop() have to be called first.
 */
void
playerinput_loop (int pl_nr)
{
	if ((!IS_LPLAYER2 )|| (!chat.active))
		playerinput_keyb_loop (pl_nr);
};


/*
 * keyboard handling read keyboard and set playervariables
 */
void
playerinput_keyb_loop (int pl_nr)
{
	int pk_offset;  // offset for the player keys
	
	if (bman.state != GS_running)
		return;

	if (!IS_LPLAYER2)				/* One Player per Screen */
		pk_offset = 0;
	else if (pl_nr == bman.p_nr)	/* Two Player per Screen - Player 1 */
		pk_offset = BCPK_max;
	else if (pl_nr == bman.p2_nr)	/* Two Player per Screen - Player 2 */
		pk_offset = 2*BCPK_max;
	else							/* not a local player */
		return;

	/* read the keys and set playervariables */
	playerinput_keyb_read (pk_offset, pl_nr);
};


inline void playerinput_keyb_read (int pk_offset, int pl_nr) {
	if (keyb_gamekeys.state[pk_offset + BCPK_up]) {
		players[pl_nr].d = up;
		players[pl_nr].m = 1;
	}

	if (keyb_gamekeys.state[pk_offset + BCPK_down]) {
		players[pl_nr].d = down;
		players[pl_nr].m = 1;
	}
	
	if (keyb_gamekeys.state[pk_offset + BCPK_right]) {
		players[pl_nr].d = right;
		players[pl_nr].m = 1;
	}
	
	if (keyb_gamekeys.state[pk_offset + BCPK_left]) {
		players[pl_nr].d = left;
		players[pl_nr].m = 1;
	}
	
	if (keyb_gamekeys.state[pk_offset + BCPK_drop] && !keyb_gamekeys.old[pk_offset + BCPK_drop])
		players[pl_nr].keyf_bomb = 1;
	
	if (keyb_gamekeys.state[pk_offset + BCPK_special])
		players[pl_nr].keyf_special = 1;
};
