/* $Id: basic.h,v 1.39 2010-12-11 20:55:37 steffen Exp $ */
/* basic types which we need everywhere */

#ifndef _BC_BASIC_H_

#define _BC_BASIC_H_

#define GAME_SPECIAL_ITEMBOMB 20
#define GAME_SPECIAL_ITEMFIRE 20
#define GAME_SPECIAL_ITEMSHOE 20
#define GAME_SPECIAL_ITEMDEATH 40
#define GAME_SPECIAL_ITEMMIXED 20
#define GAME_SPECIAL_ITEMSTRIGGER 3
#define GAME_SPECIAL_ITEMSROW 3
#define GAME_SPECIAL_ITEMSPUSH 3
#define GAME_SPECIAL_ITEMSKICK 3
#define GAME_MAX_TUNNELS 4      // number of tunnel entrys
#define GAME_TIMEOUT 600.0      // game timeout 10min)
#define GAME_OVERTIMEOUT 5.0    // second of remaining the last player
#define GAME_TUNNEL_TO 0.5      // wait 0.5 seconds
#define HURRYWARN_TO_BLINKING 0.10

#define EXPLOSION_SAVE_DISTANCE 0.25
#define EXPLOSION_GROW_SPEED 0.75f
#define SPECIAL_TRIGGER_TIMEOUT 15
#define SPECIAL_TRIGGER_NUMUSE 5 // 0=unlimited
#define SPECIAL_TRIGGER_TIME 25
#define SPECIAL_ROW_TIME 30
#define SPECIAL_PUSH_TIME 50
#define SPECIAL_KICK_TIME 30
#define SPECIAL_KICK_MAXDIST 8  // maximum distance allowed

#define START_BOMBS 1
#define START_RANGE 2
#define START_SPEED 0.07
#define SPEEDMUL 1.2

#define MAX_PLAYERS 16
#define MAX_TEAMS	4
#define MAX_BOMBS 12
#define MAX_RANGE 10
#define MAX_SPEED 0.4
#define MAX_STONESTODRAW 2048
#define MAX_SERVERENTRYS 8      /* number of entrys in the server tab */
#define MAX_GAMESRVENTRYS 255   /* number of entry which can be get */
#define MAX_FIELDSIZE_X 51
#define MAX_FIELDSIZE_Y 31
#define MIN_FIELDSIZE_X 15
#define MIN_FIELDSIZE_Y 9
#define MAX_FIELDANIMATION 2048 /* number of points on the field to be animated exploding 
                                   stoned or powerups */

#define EXPLOSIONTIMEOUT 0.5
#define ANI_FIRETIMEOUT 2
#define ANI_BOMBTIMEOUT 1
#define ANI_PLAYERTIMEOUT 0.66
#define ANI_PLAYERILLTIMEOUT 1.0

#define BOMB_TIMEOUT 4.0
#define ILL_TIMEOUT 20
#define ILL_SLOWSPEED 0.03
#define ILL_FASTSPEED 0.5

#define LEN_PLAYERNAME 16
#define LEN_SERVERNAME 41
#define LEN_PORT 6
#define LEN_GAMENAME 32
#define LEN_PATHFILENAME 512
#define LEN_FILENAME 64
#define LEN_TILESETNAME 32
#define LEN_CHARENTRY 256
#define LEN_PASSWORD 16
#define LEN_VERSION 20

#define DEFAULT_UDPPORT 11000
#define DEFAULT_GAMECACHEPORT "11111"
#define DEFAULT_GAMECACHE "ogc.gulpe.de:11111"
#define GAMESRV_TIMEOUT 2000    /* Timeout of the GameSrv_GetEntry */

#define UDP_TIMEOUT 15000
#define BUF_SIZE 1024

#define AUTOSTART 20            /* server autostart */

#define MW_IS_GFX_SELECT(__gfx_nr,__result) for (__result = (MAX_PLAYERS-1); (__result >= 0) && (players[__result].gfx_nr != __gfx_nr); __result--);

#define CUTINT(__x) (__x-floorf(__x)) // cut the integer part off
#define postofield(__x) ((int)(rintf(__x))) // position to int with rounding

#define UINT16_HALF 32767

#include <SDL.h>

enum _backgound {               // to load some diffrent logos.. 
        BG_start = 0,
        BG_net,
        BG_conf
};


enum _gametype {
        GT_bomberman = 0,
        GT_deathmatch,
        GT_team
};


enum _gamestate {
        GS_startup = 0,
        GS_quit,
        GS_wait,                // waiting for players to join
        GS_update,
        GS_ready,
        GS_running
};


enum _maptype {
        MAPT_random = -1,       // random map
        MAPT_normal = 0,        // a normal map
        MAPT_tunnel,            // a map with tunnels

        MAPT_max
};


enum _fieldtype {
        FT_nothing = 0,         // Nothing in here
        FT_stone,               // Stones you can bomb away
        FT_block,               // Stones which can't bomb away
        FT_tunnel,              // the tunnel item
        FT_death,               // The bad Powerup
        FT_fire,                // The fire Powerup
        FT_bomb,                // The bomb Powerup
        FT_shoe,                // The shoe Powerup
        FT_mixed,               // The mixed Powerup
        FT_sp_trigger,          // The Triggered bomb Special
        FT_sp_row,              // The bomb-row special
        FT_sp_push,             // The push-boms special
        FT_sp_moved,            // The moved-boms special
        FT_sp_liquid,           // The liquid-bomb special
        FT_sp_kick,             // The kick-bomb special
        FT_max                  // just to know how many types there are
};


extern const char *ft_filenames[]; // declared in tileset.c

enum _poweruptypes {
        PWUP_good = 0,
        PWUP_bad,
        PWUP_special,
        PWUP_max
};


enum _direction {               // to handle directions better
        left = 0,
        right,
        up,
        down
};

enum _mapselection {
        MAPS_select = 0,
        MAPS_randmap,
        MAPS_randgen,
        MAPS_morerand
};

enum _mstatus {
        MS_normal = 0,
        MS_hurrywarn,
        MS_hurry,               // mapsize will go down
        MS_dropitems,           // alot of items will be droppen randomly into the game

        MS_max
};

enum _help_page {
        HP_howto0 = 0,
        HP_powerup0,
        HP_powerup1,
        HP_powerup2,
        HP_keyboard0,
        HP_credit0,
        HP_credit1,
        HP_max
};


struct {
        Sint16 x;
        Sint16 y;
} __attribute__((packed)) typedef _point;


struct {
        float x;
        float y;
} typedef _pointf;


struct __charlist {
        char text[LEN_CHARENTRY];
        struct __charlist *next;
} typedef _charlist;

extern _point dir_change[];

#endif
