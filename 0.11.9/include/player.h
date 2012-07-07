/* $Id: player.h,v 1.12 2009-05-11 20:51:25 stpohle Exp $
 * playerinclude file
 */

#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "bomb.h"

enum _specials {
	SP_nothing=0,				// 0 player has no special
	SP_trigger,					// 1 triggered bomb
	SP_row,						// 2 bomb row
	SP_push,					// 3 push bombs
	SP_moved,					// 4 moved bombs
	SP_liquid,					// 5 liquid bombs
	SP_kick,					// 6 kick bombs
	
	SP_max,						// 7 just to know how many types there are
	SP_clear					// 8 needed to let the server know we removed the special
};



enum _playerillnestype {
    PI_keys = 0,                // switch keys
    PI_range,                   // set exploding range to 1
    PI_slow,                    // sets speed to 6
    PI_fast,                    // sets speed to 150
    PI_bomb,                    // player is dropping bombs permanently
    PI_nobomb,                  // player cannot drop a bomb or only 1 bomb

    PI_max                      // just to know what is the last number
};


enum _playerstateflags {		//     not Set    |   Set
    PSF_used = 1,               // Player Unused  | Player Used
    PSF_net = 2,                // Local Player   | Network Player
    PSF_alife = 4,              // Player is Dead | Player is Alife
    PSF_playing = 8,            // Watching Player| Playing Player -- as long as one don't delete
	PSF_ai = 16,				//                | AI Player
	PSF_respawn = 32			//                | Player is Respawning
};


#define PSFM_used (PSF_used + PSF_playing)
#define PSFM_alife (PSF_used + PSF_alife + PSF_playing)
#define PS_IS_dead(__ps) (((__ps) & (PSFM_alife + PSF_respawn)) == (PSFM_used))
#define PS_IS_respawn(__ps) (((__ps) & (PSFM_alife + PSF_respawn)) == (PSFM_used + PSF_respawn))
#define PS_IS_alife(__ps) (((__ps) & (PSFM_alife)) == (PSFM_alife))
#define PS_IS_netplayer(__ps) (((__ps) & (PSF_net)) != 0)
#define PS_IS_playing(__ps) (((__ps) & (PSFM_used)) == (PSFM_used))
#define PS_IS_used(__ps) (((__ps) & (PSFM_used)) != 0)
#define PS_IS_aiplayer(__ps) ((((__ps) & (PSFM_used)) != 0) && (((__ps) & (PSF_ai)) == PSF_ai))

struct {
	int killedBy[MAX_PLAYERS];
	int killed;
	int unknown;
	int isaplayer;
} typedef _gamestats;

struct {
	float to;  // if (to > 0) the ilness is still working
	int datai;		// hold a integer data (like number of something..)
	float dataf;	// hold a float data (speed and so on)
} typedef _playerilness;


struct {
	int type; 					// type of the special
	float to;						// timeout
	int numuse;					// num of uses left
	int use;					/* currently used set by special_use 
								   and deleted in special_loop */
	int clear;					// do we need to clear this special
} typedef _special;


struct {
	_gfxplayer *gfx;			// pointer to the gfx information
	int gfx_nr;					// number of the player GFX
	
    float frame;                // step of the animation (only integer part will shown)
	float illframe;

    _pointf pos;                // position on the field
    _pointf old;				// the old position
	float tunnelto;				/* timeout for dont show and move player
							   	   needed on the tunnel effect */
	
    signed char d;              // direction
    signed char m;              // player is moving ?
    signed char old_m;          // to save the old state..
	signed char keyf_bomb;		// flag for the bomb key
	signed char keyf_special;	// flag for the special key

    int bombs_n;                // maximal number of bombs for the player
	int bomb_lastex;			// number of the bomb which explode the last time
    _bomb bombs[MAX_BOMBS];     // number of bombs who are ticking.
    int range;                  // range of the bombs
	float speed;				// how fast we can go (0 = slow, 1 = normal... 3 = fastest)
	float stepsleft;			// distance to walk on the next stepmove_player
	int collect_shoes;
	_playerilness ill[PI_max];  // all possible types
	_special special;			// special the player has
	
    char name[LEN_PLAYERNAME];  // name oder name[0] == 0
	int team_nr;				// number of the team we are in or -1
    unsigned char state;        // status of the player
	int ready;					// only used in net games
    signed char in_nr;          // number of the connected player entry

    int points;                 // points
	int nbrKilled;				// number of player killed during a round
	int wins;					// wins
    signed char dead_by;        // player who killed this player

	_net_player net;			// holds all important network data
	_gamestats gamestats;
} typedef _player;


struct __team {
	_player *players[MAX_PLAYERS];
	char name[LEN_PLAYERNAME];
	int col;		// color of the Teamname
	int wins;
	int points;
} typedef _team;


// everything what is declared in players.c
extern _player *players;
extern _team *teams;

extern void dead_playerani ();
extern void draw_player (_player * player);
extern void restore_players_screen ();
extern void player_move (int pl_nr);
extern float stepmove_player (int pl_nr);
extern void player_drop_bomb (int pl_nr);
extern void get_player_on (float x, float y, int pl_nr[]);
extern void player_died (_player * player, signed char dead_by, int network);
extern void draw_players ();
extern void player_animation (_player * player);
extern int check_field (short int x, short int y);
extern int check_exfield (short int x, short int y);
extern void player_calcpos ();
extern void player_set_ilness (_player * p, int t, float new_to);
extern void player_clear_ilness (_player *p, int type);
extern void player_ilness_loop (int plnr);
extern void player_check_powerup (int p_nr);
extern void player_set_gfx (_player *p, signed char gfx_nr);
extern int player_findfreebomb (_player *player);
extern int player_checkpos (int x, int y);
extern void player_checkdeath (int pnr);
extern void player_check (int pl_nr);
extern void player_delete (int pl_nr);

extern void player2_join ();
extern void player2_add (int pl_nr);

extern void team_update ();
extern void team_choose (_player *pl);

// for the playerinput handling
extern void playerinput_loop (int pl_nr);
extern void playerinput_keyb_loop (int pl_nr);
extern inline void playerinput_keyb_read (int pk_offset, int pl_nr);

/* playermenu.c */
extern void playernamemenu ();
extern void playermenu ();
extern int playermenu_selgfx (int pl_nr);
extern void playermenu_getflags (char *text, _player *player);

extern int teammenu_select (_player *pl);
extern void teammenu ();

#endif
