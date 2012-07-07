/* $Id: configuration.c,v 1.85 2009-11-28 13:11:51 stpohle Exp $
 * configuration */

#include <SDL.h>

#include "basic.h"
#include "bomberclone.h"
#include "network.h"
#include "packets.h"
#include "gfx.h"
#include "chat.h"
#include "sound.h"
#include "menu.h"
#include "keyb.h"
#include "player.h"


/*
 * try to find the datapath and set the variable bman.datapath
 * test: 1) PACKAGE_DATA_DIR
 *       2) ./data
 *       3) ../data
 */
void
config_get_datapath ()
{
    FILE *f;
    char filename[255];

    sprintf (bman.datapath, PACKAGE_DATA_DIR);
    sprintf (filename, "%s/gfx/logo.png", bman.datapath);
    f = fopen (filename, "r");

    if (!f) {
        sprintf (bman.datapath, "data");
        sprintf (filename, "%s/gfx/logo.png", bman.datapath);
        f = fopen (filename, "r");
        if (!f) {
            sprintf (bman.datapath, "../data");
            sprintf (filename, "%s/gfx/logo.png", bman.datapath);
            f = fopen (filename, "r");
            if (!f) {
                printf ("Can't find Datafiles.\n");
                exit (1);
            }
        }
    }
    fclose (f);
}


/*
 * reset all variables and load all configs.
 */
void
config_init (int argc, char **argv)
{
    SDL_Surface *icon_img;
    char text[255],
      icon[255];
    int i,
      j;

    config_get_datapath ();

    srand (((int) time (NULL))); // initialize randomgenerator

    for (i = 0; i < MAX_TEAMS; i++) {
        teams[i].col = i;
        sprintf (teams[i].name, "Team %d", i + 1);
        for (j = 0; j < MAX_PLAYERS; j++)
            teams[i].players[j] = NULL;
    }

    stonelist_del ();
    chat.oldscreen = NULL;
    chat.active = 0;
    chat.curline = 0;
    keyb_config_reset ();
    keybinput_new (&chat.input, KEYBI_text, 255);

    for (i = 0; i < CHAT_MAX_LINES; i++)
        chat.lines[i].text[0] = 0;

    bman.maxplayer = MAX_PLAYERS;
    bman.net_ai_family = PF_INET;
    bman.sock = -1;
    bman.p_nr = -1;
    bman.p2_nr = -1;
    bman.gamename[0] = 0;
    bman.playnum = 0;
    sprintf (bman.playername, "Player1");
    sprintf (bman.player2name, "Player2");
    sprintf (bman.port, "%d", DEFAULT_UDPPORT);
    sprintf (bman.ogcserver, DEFAULT_GAMECACHE);
    sprintf (bman.ogc_port, DEFAULT_GAMECACHEPORT);
    rscache.count = 0;
    bman.notifygamemaster = 1;
    bman.broadcast = 1;
    bman.autostart = AUTOSTART;
    bman.askplayername = 0;
    debug = 0;
    gfx.res.x = 640;
    gfx.res.y = 480;
    gfx.bpp = 16;
    gfx.players = NULL;
    bman.password[0] = 0;
    bman.passwordenabled = 0;
    map.tileset[0] = 0;
    map.random_tileset = 1;
    map.size.x = 25;
    map.size.y = 17;
    map.map[0] = 0;
    map.map_selection = 2;
    map.type = 1;
    bman.firewall = 0;
    bman.init_timeout = GAME_TIMEOUT;
    bman.ai_players = 1;
    bman.minplayers = 0;
    snd.inited = 0;
    snd.audio_rate = 22050;
    snd.audio_format = AUDIO_S16;
    snd.audio_channels = 2;
    snd.playmusic = 1;
    snd.playsound = 1;
    map.bombs = GAME_SPECIAL_ITEMBOMB;
    map.fire = GAME_SPECIAL_ITEMFIRE;
    map.shoes = GAME_SPECIAL_ITEMSHOE;
    map.mixed = GAME_SPECIAL_ITEMMIXED;
    map.death = GAME_SPECIAL_ITEMDEATH;
    map.sp_trigger = GAME_SPECIAL_ITEMSTRIGGER;
    map.sp_row = GAME_SPECIAL_ITEMSROW;
    map.sp_push = GAME_SPECIAL_ITEMSPUSH;
    map.sp_kick = GAME_SPECIAL_ITEMSKICK;

    bman.start_bombs = START_BOMBS;
    bman.start_speed = START_SPEED;
    bman.start_range = START_RANGE;
    bman.bomb_tickingtime = BOMB_TIMEOUT;
    bman.dropitemsondeath = 0;
    d_printf ("\n\n ***** Bomberclone Version %s \n\n", VERSION);
    config_read ();

    ReadPrgArgs (argc, argv);

    gfx_init ();
    draw_logo ();
    if (bman.askplayername)
        playernamemenu ();

    snd_init ();
    gfx_blitdraw ();

    SDL_Flip (gfx.screen);

    sprintf (text, "Bomberclone %s", VERSION);
    sprintf (icon, "%s/pixmaps/bomberclone.png", bman.datapath);
    SDL_WM_SetCaption (text, NULL);
    icon_img = IMG_Load (icon);
    if (icon_img == NULL)
        d_printf ("could not load icon. (%s)\n", icon);

#ifdef _WIN32
    {
        SDL_Surface *tmp = icon_img;
        icon_img = scale_image (tmp, 32, 32);
        SDL_FreeSurface (tmp);
    }
#endif
    SDL_WM_SetIcon (icon_img, NULL);

    ReadPrgArgs_Jump (argc, argv);
};


/* read the configuration file
 * return -1 if something went wrong and 0 if no problem */
int
config_read ()
{
    FILE *config;
    char buf[1024];
    char *findit,
     *keyword,
     *value;
    int i;
    char filename[512];

#ifdef _WIN32
    sprintf (filename, "%sbomberclone.cfg", s_gethomedir ());
#else
    sprintf (filename, "%s.bomberclone.cfg", s_gethomedir ());
#endif

    config = fopen (filename, "r");
    if (config == NULL) {
        d_printf ("Error: Config file not found!\n");
        return -1;
    }
    d_printf ("Reading Config-file: %s", filename);

    while (fgets (buf, sizeof (buf), config) != NULL) {
        findit = strchr (buf, '\n');
        if (findit)
            findit[0] = '\0';
        if (buf[0] == '\0')
            continue;

        keyword = buf;
        while (isspace (*keyword))
            keyword++;

        value = strchr (buf, '=');
        if (value == NULL)
            continue;
        *value = 0;
        value++;
        while (*value == ' ')
            value++;
        while (keyword[strlen (keyword) - 1] == ' ')
            keyword[strlen (keyword) - 1] = 0;
        while (value[strlen (value) - 1] == ' ')
            value[strlen (value) - 1] = 0;
        if (strlen (value) == 0)
            continue;
        for (i = 0; i < (int) strlen (keyword); i++)
            keyword[i] = tolower (keyword[i]);

        if (!strcmp (keyword, "playername")) {
            if (strlen (value) > LEN_PLAYERNAME) {
                d_printf
                    ("*** Error - playername too long (maximum size permitted is %d characters)!\n\n",
                     LEN_PLAYERNAME);
            }
            value[LEN_PLAYERNAME - 1] = 0;
            strcpy (bman.playername, value);
        }

        if (!strcmp (keyword, "player2name")) {
            if (strlen (value) > LEN_PLAYERNAME) {
                d_printf
                    ("*** Error - playername too long (maximum size permitted is %d characters)!\n\n",
                     LEN_PLAYERNAME);
            }
            value[LEN_PLAYERNAME - 1] = 0;
            strcpy (bman.player2name, value);
        }

        if (!strcmp (keyword, "gamename")) {
            if (strlen (value) > LEN_GAMENAME) {
                d_printf
                    ("*** Error - servername too long (maximum size permitted is %d characters)!\n\n",
                     LEN_GAMENAME);
            }
            value[LEN_GAMENAME - 1] = 0;
            strcpy (bman.gamename, value);
        }
        if (!strcmp (keyword, "askplayername")) {
            bman.askplayername = atoi (value);
        }
        if (!strcmp (keyword, "password")) {
            if (strlen (value) > LEN_PASSWORD) {
                d_printf
                    ("*** Error - Password is too long (maximum size permitted is %d characters)!\n\n",
                     LEN_PASSWORD);
            }
            value[LEN_PASSWORD - 1] = 0;
            strcpy (bman.password, value);
        }
        if (!strcmp (keyword, "passwordenabled")) {
            bman.passwordenabled = atoi (value);
        }
        if (!strcmp (keyword, "resolutionx")) {
            gfx.res.x = atoi (value);
        }
        if (!strcmp (keyword, "resolutiony")) {
            gfx.res.y = atoi (value);
        }
        if (!strcmp (keyword, "tileset")) {
            strcpy (map.tileset, value);
        }
        if (!strcmp (keyword, "mapname")) {
            if (strlen (value) > LEN_PATHFILENAME) {
                d_printf
                    ("*** Error - fieldpath too long (maximum size permitted is %d characters)!\n\n",
                     LEN_PATHFILENAME);
            }
            value[511] = 0;
            strcpy (map.map, value);
        }
        if (!strcmp (keyword, "udpport")) {
            if (strlen (value) > LEN_PORT) {
                d_printf
                    ("*** Error - servername too long (maximum size permitted is %d characters)!\n\n",
                     LEN_PORT);
            }
            value[LEN_PORT - 1] = 0;
            strcpy (bman.port, value);
        }
        if (!strcmp (keyword, "ai_players")) {
            bman.ai_players = atoi (value);
        }
        if (!strcmp (keyword, "fieldsizex")) {
            map.size.x = atoi (value);
        }
        if (!strcmp (keyword, "fieldsizey")) {
            map.size.y = atoi (value);
        }
        if (!strcmp (keyword, "fullscreen")) {
            gfx.fullscreen = atoi (value);
        }
        if (!strcmp (keyword, "bitsperpixel")) {
            gfx.bpp = atoi (value);
        }
        if (!strcmp (keyword, "ai_family")) {
            bman.net_ai_family = atoi (value);
        }
        if (!strcmp (keyword, "debug")) {
            debug = atoi (value);
        }
        if (!strcmp (keyword, "notify")) {
            bman.notifygamemaster = atoi (value);
        }
        if (!strcmp (keyword, "broadcast")) {
            bman.broadcast = atoi (value);
        }
        if (!strcmp (keyword, "ogcserver")) {
            strcpy (bman.ogcserver, value);
        }
        if (!strcmp (keyword, "ogc_port")) {
            strcpy (bman.ogc_port, value);
        }
        if (!strcmp (keyword, "maxplayer")) {
            bman.maxplayer = atoi (value);
        }
        if (!strcmp (keyword, "mapselection")) {
            map.map_selection = atoi (value);
        }
        if (!strcmp (keyword, "maptype")) {
            map.type = atoi (value);
        }
        if (!strcmp (keyword, "randomtileset")) {
            map.random_tileset = atoi (value);
        }
        if (!strcmp (keyword, "gametimeout")) {
            bman.init_timeout = atoi (value);
        }
        if (!strcmp (keyword, "gametype")) {
            bman.gametype = atoi (value);
        }
        if (!strcmp (keyword, "sndrate")) {
            snd.audio_rate = atoi (value);
        }
        if (!strcmp (keyword, "sndchannels")) {
            snd.audio_channels = atoi (value);
        }
        if (!strcmp (keyword, "sndformat")) {
            snd.audio_format = atoi (value);
        }
        if (!strcmp (keyword, "sndplaymusic")) {
            snd.playmusic = atoi (value);
        }
        if (!strcmp (keyword, "sndplaysound")) {
            snd.playsound = atoi (value);
        }
        if (!strcmp (keyword, "start_bombs")) {
            bman.start_bombs = atoi (value);
        }
        if (!strcmp (keyword, "start_range")) {
            bman.start_range = atoi (value);
        }
        if (!strcmp (keyword, "start_speed")) {
            sscanf (value, "%f", &bman.start_speed);
        }
        if (!strcmp (keyword, "special_itembombs")) {
            sscanf (value, "%d", &map.bombs);
        }
        if (!strcmp (keyword, "special_itemfire")) {
            sscanf (value, "%d", &map.fire);
        }
        if (!strcmp (keyword, "special_itemshoes")) {
            sscanf (value, "%d", &map.shoes);
        }
        if (!strcmp (keyword, "special_itemmixed")) {
            sscanf (value, "%d", &map.mixed);
        }
        if (!strcmp (keyword, "special_itemdeath")) {
            sscanf (value, "%d", &map.death);
        }
        if (!strcmp (keyword, "special_trigger")) {
            sscanf (value, "%d", &map.sp_trigger);
        }
        if (!strcmp (keyword, "special_row")) {
            sscanf (value, "%d", &map.sp_row);
        }
        if (!strcmp (keyword, "special_push")) {
            sscanf (value, "%d", &map.sp_push);
        }
        if (!strcmp (keyword, "special_kick")) {
            sscanf (value, "%d", &map.sp_kick);
        }
        if (!strcmp (keyword, "bomb_ticking")) {
            sscanf (value, "%f", &bman.bomb_tickingtime);
        }
        if (!strcmp (keyword, "dropitemsondeath")) {
            bman.dropitemsondeath = atoi (value);
        }

        for (i = 0; i < MAX_TEAMS; i++) {
            char txt[255];
            sprintf (txt, "teamcol%d", i);
            if (!strcmp (keyword, txt)) {
                teams[i].col = atoi (value);
            }
            sprintf (txt, "teamname%d", i);
            if (!strcmp (keyword, txt)) {
                strncpy (teams[i].name, value, LEN_PLAYERNAME);
            }
        }

        /*
         * keyboard config, i will give names to the keys insteed of the numbers,
         * this is done to add more keys to the game without destroying the config.
         */
        if (!strcmp (keyword, "key_p1_up"))
            keyb_gamekeys.keycode[BCPK_up] = atoi (value);
        if (!strcmp (keyword, "key_p1_down"))
            keyb_gamekeys.keycode[BCPK_down] = atoi (value);
        if (!strcmp (keyword, "key_p1_left"))
            keyb_gamekeys.keycode[BCPK_left] = atoi (value);
        if (!strcmp (keyword, "key_p1_right"))
            keyb_gamekeys.keycode[BCPK_right] = atoi (value);
        if (!strcmp (keyword, "key_p1_bomb"))
            keyb_gamekeys.keycode[BCPK_drop] = atoi (value);
        if (!strcmp (keyword, "key_p1_special"))
            keyb_gamekeys.keycode[BCPK_special] = atoi (value);

        if (!strcmp (keyword, "key_p2_up"))
            keyb_gamekeys.keycode[BCPK_max + BCPK_up] = atoi (value);
        if (!strcmp (keyword, "key_p2_down"))
            keyb_gamekeys.keycode[BCPK_max + BCPK_down] = atoi (value);
        if (!strcmp (keyword, "key_p2_left"))
            keyb_gamekeys.keycode[BCPK_max + BCPK_left] = atoi (value);
        if (!strcmp (keyword, "key_p2_right"))
            keyb_gamekeys.keycode[BCPK_max + BCPK_right] = atoi (value);
        if (!strcmp (keyword, "key_p2_bomb"))
            keyb_gamekeys.keycode[BCPK_max + BCPK_drop] = atoi (value);
        if (!strcmp (keyword, "key_p2_special"))
            keyb_gamekeys.keycode[BCPK_max + BCPK_special] = atoi (value);

        if (!strcmp (keyword, "key_help"))
            keyb_gamekeys.keycode[BCK_help] = atoi (value);
        if (!strcmp (keyword, "key_playermenu"))
            keyb_gamekeys.keycode[BCK_playermenu] = atoi (value);
        if (!strcmp (keyword, "key_mapmenu"))
            keyb_gamekeys.keycode[BCK_mapmenu] = atoi (value);
        if (!strcmp (keyword, "key_chat"))
            keyb_gamekeys.keycode[BCK_chat] = atoi (value);
        if (!strcmp (keyword, "key_pause"))
            keyb_gamekeys.keycode[BCK_pause] = atoi (value);
        if (!strcmp (keyword, "key_fullscreen"))
            keyb_gamekeys.keycode[BCK_fullscreen] = atoi (value);
        /*
         * joypad config 
         */
        if (!strcmp (keyword, "joy_1_drop"))
   	        joy_keys[0].drop = atoi (value);
        if (!strcmp (keyword, "joy_1_special"))
   	        joy_keys[0].special = atoi (value);
        if (!strcmp (keyword, "joy_2_drop"))
   	        joy_keys[1].drop = atoi (value);
        if (!strcmp (keyword, "joy_2_special"))
   	        joy_keys[1].special = atoi (value);
    }
    fclose (config);
    return 0;
};


int
config_write ()
{
    FILE *config;
    int i;
    char filename[512];

#ifdef _WIN32
    sprintf (filename, "%sbomberclone.cfg", s_gethomedir ());
#else
    sprintf (filename, "%s.bomberclone.cfg", s_gethomedir ());
#endif
    if ((config = fopen (filename, "w")) == NULL)
        return -1;
    fprintf (config, "resolutionx=%d\n", gfx.res.x);
    fprintf (config, "resolutiony=%d\n", gfx.res.y);
    fprintf (config, "fullscreen=%d\n", gfx.fullscreen);
    fprintf (config, "tileset=%s\n", map.tileset);
    fprintf (config, "mapname=%s\n", map.map);
    fprintf (config, "udpport=%s\n", bman.port);
    fprintf (config, "ai_players=%d\n", bman.ai_players);
    fprintf (config, "fieldsizex=%d\n", map.size.x);
    fprintf (config, "fieldsizey=%d\n", map.size.y);
    fprintf (config, "notify=%d\n", bman.notifygamemaster);
    fprintf (config, "broadcast=%d\n", bman.broadcast);
    fprintf (config, "ai_family=%d\n", bman.net_ai_family);
    fprintf (config, "ogcserver=%s\n", bman.ogcserver);
    fprintf (config, "ogc_port=%s\n", bman.ogc_port);
    fprintf (config, "gamename=%s\n", bman.gamename);
    fprintf (config, "gametimeout=%d\n", bman.init_timeout);
    fprintf (config, "gametype=%d\n", bman.gametype);
    fprintf (config, "maxplayer=%d\n", bman.maxplayer);
    fprintf (config, "debug=%d\n", debug);
    fprintf (config, "password=%s\n", bman.password);
    fprintf (config, "passwordenabled=%d\n", bman.passwordenabled);
    fprintf (config, "askplayername=%d\n", bman.askplayername);
    fprintf (config, "playername=%s\n", bman.playername);
    fprintf (config, "player2name=%s\n", bman.player2name);
    fprintf (config, "bitsperpixel=%d\n", gfx.bpp);
    fprintf (config, "randomtileset=%d\n", map.random_tileset);
    fprintf (config, "mapselection=%d\n", map.map_selection);
    fprintf (config, "maptype=%d\n", map.type);
    fprintf (config, "sndrate=%d\n", snd.audio_rate);
    fprintf (config, "sndchannels=%d\n", snd.audio_channels);
    fprintf (config, "sndformat=%d\n", snd.audio_format);
    fprintf (config, "sndplaymusic=%d\n", snd.playmusic);
    fprintf (config, "sndplaysound=%d\n", snd.playsound);
    fprintf (config, "start_bombs=%d\n", bman.start_bombs);
    fprintf (config, "start_range=%d\n", bman.start_range);
    fprintf (config, "start_speed=%f\n", bman.start_speed);
    fprintf (config, "special_itembombs=%d\n", map.bombs);
    fprintf (config, "special_itemfire=%d\n", map.fire);
    fprintf (config, "special_itemshoes=%d\n", map.shoes);
    fprintf (config, "special_itemmixed=%d\n", map.mixed);
    fprintf (config, "special_itemdeath=%d\n", map.death);
	fprintf (config, "special_trigger=%d\n", map.sp_trigger);
	fprintf (config, "special_row=%d\n", map.sp_row);
	fprintf (config, "special_push=%d\n", map.sp_push);
	fprintf (config, "special_kick=%d\n", map.sp_kick);

    fprintf (config, "bomb_ticking=%f\n", bman.bomb_tickingtime);
	fprintf (config, "dropitemsondeath=%d\n",bman.dropitemsondeath);

    for (i = 0; i < MAX_TEAMS; i++) {
        fprintf (config, "teamcol%d=%d\n", i, teams[i].col);
        fprintf (config, "teamname%d=%s\n", i, teams[i].name);
    }

    /*
     * keyboard config
     */
    fprintf (config, "key_p1_up=%d\n", keyb_gamekeys.keycode[BCPK_up]);
    fprintf (config, "key_p1_down=%d\n", keyb_gamekeys.keycode[BCPK_down]);
    fprintf (config, "key_p1_left=%d\n", keyb_gamekeys.keycode[BCPK_left]);
    fprintf (config, "key_p1_right=%d\n", keyb_gamekeys.keycode[BCPK_right]);
    fprintf (config, "key_p1_bomb=%d\n", keyb_gamekeys.keycode[BCPK_drop]);
    fprintf (config, "key_p1_special=%d\n", keyb_gamekeys.keycode[BCPK_special]);
    fprintf (config, "key_p2_up=%d\n", keyb_gamekeys.keycode[BCPK_max + BCPK_up]);
    fprintf (config, "key_p2_down=%d\n", keyb_gamekeys.keycode[BCPK_max + BCPK_down]);
    fprintf (config, "key_p2_left=%d\n", keyb_gamekeys.keycode[BCPK_max + BCPK_left]);
    fprintf (config, "key_p2_right=%d\n", keyb_gamekeys.keycode[BCPK_max + BCPK_right]);
    fprintf (config, "key_p2_bomb=%d\n", keyb_gamekeys.keycode[BCPK_max + BCPK_drop]);
    fprintf (config, "key_p2_special=%d\n", keyb_gamekeys.keycode[BCPK_max + BCPK_special]);
    fprintf (config, "key_help=%d\n", keyb_gamekeys.keycode[BCK_help]);
    fprintf (config, "key_fullscreen=%d\n", keyb_gamekeys.keycode[BCK_fullscreen]);
    fprintf (config, "key_chat=%d\n", keyb_gamekeys.keycode[BCK_chat]);
    fprintf (config, "key_mapmenu=%d\n", keyb_gamekeys.keycode[BCK_mapmenu]);
    fprintf (config, "key_pause=%d\n", keyb_gamekeys.keycode[BCK_pause]);
    fprintf (config, "key_playermenu=%d\n", keyb_gamekeys.keycode[BCK_playermenu]);
    /*
     * joypad config
     */
    fprintf (config, "joy_1_drop=%d\n", joy_keys[0].drop);
    fprintf (config, "joy_1_special=%d\n", joy_keys[0].special);
    fprintf (config, "joy_2_drop=%d\n", joy_keys[1].drop);
    fprintf (config, "joy_2_special=%d\n", joy_keys[1].special);

    fclose (config);
    return 0;
}



void
config_video ()
{
    int done = 0,
        menuselect,
        x,
        y;
    _charlist screenres[] = {
        {"640x480", NULL},
        {"800x600", NULL},
        {"1024x768", NULL},
        {"1280x800", NULL},
        {"1280x1024", NULL},
		{"1600x1200", NULL},
        {"1920x1080", NULL},
        {"1920x1200", NULL},        
		{"1280x800", NULL}
    };
    _charlist screenbpp[] = {
        {"16", NULL},
        {"24", NULL},
        {"32", NULL}
    };
    _charlist *selres = NULL;
    _charlist *selbpp = NULL;
    char text[100];

    _menu *menu;

    /* set all pointers in this array */
    charlist_fillarraypointer (screenres, 6);
    charlist_fillarraypointer (screenbpp, 3);

    /* select the current settings */
    sprintf (text, "%dx%d", gfx.res.x, gfx.res.y);
    selres = charlist_findtext (screenres, text);
    sprintf (text, "%d", gfx.bpp);
    selbpp = charlist_findtext (screenbpp, text);

    while (!done && bman.state != GS_quit) {
        menu = menu_new ("Video Setup", 325, 300);
        menu_create_label (menu, "Resolution", 25, 70, 0, COLOR_brown);
        menu_create_list (menu, "res", 155, 55, 150, 85, screenres, &selres, 1);
        menu_create_label (menu, "Colors", 65, 170, 0, COLOR_brown);
        menu_create_list (menu, "bpp", 195, 155, 50, 55, screenbpp, &selbpp, 2);
        menu_create_bool (menu, "Fullscreen", -1, 220, 150, &gfx.fullscreen, 3);
        menu_create_button (menu, "OK", -1, 260, 100, 0);
        menuselect = menu_loop (menu);
        menu_delete (menu);

        switch (menuselect) {
        case (0):
            done = 1;
            gfx_shutdown ();
            gfx_init ();
            break;
        case (1):              // new resolution
            gfx_shutdown ();
            sscanf (selres->text, "%dx%d", &x, &y);
            gfx.res.x = x;
            gfx.res.y = y;
            gfx_init ();
            draw_logo ();
            break;
        case (2):              // new color depth
            gfx_shutdown ();
            sscanf (selbpp->text, "%d", &x);
            gfx.bpp = x;
            gfx_init ();
            draw_logo ();
            break;
        default:
            done = 1;
            break;
        }
    };
    draw_logo ();
    SDL_Flip (gfx.screen);
};





/* Configuration Menu */
void
config_menu ()
{
    int menuselect = 0;
    _menu *menu;

    while (menuselect != -1 && bman.state != GS_quit) {
        menu = menu_new ("Configuration", 400, 300);
        menu_create_label (menu, "General Option", -1, 50, 1, COLOR_brown);
        menu_create_button (menu, "Playernames", 25, 85, 150, 1);
        menu_create_button (menu, "Keyboard", 250, 85, 150, 2);
        menu_create_button (menu, "Joypad", 25, 120, 150, 4);
        menu_create_button (menu, "Video Setup", 250, 120, 150, 3);
        menu_create_label (menu, "Sound", 25, 154, 0, COLOR_brown);
        menu_create_bool (menu, "ON", 100, 150, 50, &snd.playsound, 4);
        menu_create_label (menu, "Music", 250, 154, 0, COLOR_brown);
        menu_create_bool (menu, "ON", 325, 150, 50, &snd.playmusic, 5);
        menu_create_label (menu, "Extended Option", -1, 200, 1, COLOR_brown);
        menu_create_bool (menu, "Debug", 25, 230, 150, &debug, 6);
        menu_create_bool (menu, "Ask Playername", 250, 230, 150, &bman.askplayername, 7);
        menu_create_button (menu, "Ok", -1, 270, 150, 0);
        if (bman.playername[0] == '\0')
            menu_focus_id (menu, 1);
        else
            menu_focus_id (menu, 0);

        menu_reload (menu);
        menuselect = menu_loop (menu);

        menu_delete (menu);
        switch (menuselect) {
        case (0):              // Back to the Main Menu
            if (bman.playername[0] == '\0')
                menuselect = 0;
            else
                menuselect = -1;
            break;
        case (1):              // player screen 
            playernamemenu ();
            break;
        case (2):              // keyboard settings
            keyb_config ();
            break;
        case (3):              // Screen Options
            config_video ();
            break;
       case (4):              // joypad Options
            joypad_config ();
            break;

        }
    }
    config_write ();
};


/* Read Programm parameter */
void
ReadPrgArgs (int argc, char **argv)
{
    int i = 0;
	char s;
    while (argv[++i] != NULL) {
        if (!strcmp (argv[i], "-h")||!strcmp (argv[i], "-help")||!strcmp (argv[i], "--help")) {
            printf ("BomberClone Version " VERSION "\n");
            printf (" WebPage       : http://www.bomberclone.de\n");
            printf (" Bug Report to :\n");
            printf ("   http://sourceforge.net/tracker/?group_id=79449&atid=556629\n");
            printf (" Other Comments: steffen@bomberclone.de\n");
            printf ("\nProgramm options:\n");
            printf (" -name PLAYERNAME   - set the Playername\n");
            printf (" -name2 PLAYERNAME  - set the Playername for the second player\n");
            printf (" -gamename GAMENAME - set the name of the game\n");
            printf (" -port PORT         - set the local BomberClone port\n");
            printf ("                      (Def.: 11000)\n");
            printf (" -ogcport PORT      - set the local OGC Port (Def.: 11100)\n");
            printf (" -ogc 0/1           - Enable/Disable OGC\n");
            printf (" -broadcast 0/1     - Enable/Disable broadcast requests.\n");
            printf (" -host              - start a network game\n");
            printf (" -join              - go into the join menu\n");
            printf (" -connect ADDRESS   - connect to a server\n");
            printf (" -debug 0/1         - enable/disable debug\n");
            printf (" -autostart SECONDS - time before a game starts\n");
            exit (0);
        }
	}
	i=0;
	while (argv[++i] != NULL) {
		if (argv[i][0]=='-') {
			if (argv[i+1] != NULL) 
				s=argv[i+1][0];
			else 
				s='-';
        	if (!strcmp (argv[i], "-port")) {
            	if (s!='-') 
					strncpy (bman.port, argv[++i], LEN_PORT);
					else {
						printf("Error: Parameter required for -port\n");
						exit(1);
					}
				}
        	if (!strcmp (argv[i], "-ogcport")) {
				if (s!='-')
					strncpy (bman.ogc_port, argv[++i], LEN_PORT);
					else {
						printf("Error: Parameter required for -ogcport\n");
						exit(1);
					}
				}
        	if (!strcmp (argv[i], "-name")) {
				if (s!='-')
					strncpy (bman.playername, argv[++i], LEN_PLAYERNAME);
					else {
						printf("Error: Parameter required for -name\n");
						exit(1);
					}
				}
    	    if (!strcmp (argv[i], "-name2")) {
				if (s!='-')
					strncpy (bman.player2name, argv[++i], LEN_PLAYERNAME);
					else {
						printf("Error: Parameter required for -name2\n");
						exit(1);
					}
				}
        	if (!strcmp (argv[i], "-gamename")) {
				if (s!='-') 
					strncpy (bman.gamename, argv[++i], LEN_GAMENAME);
					else {
						printf("Error: Parameter required for -gamename\n");
						exit(1);
					}
				}
			if (!strcmp (argv[i], "-autostart")) {
				if (s!='-')
    	    		bman.autostart = atoi (argv[++i]);
				else {
					printf("Error: Parameter required for -autostart\n");
					exit(1);
					}
				}
        	if (!strcmp (argv[i], "-ogc")) {
				if (s!='-')
            		bman.notifygamemaster = atoi (argv[++i]);
				else
					bman.notifygamemaster = 1;
			}
        	if (!strcmp (argv[i], "-broadcast")) {
				if (s!='-')
            		bman.broadcast = atoi (argv[++i]);
				else
					bman.broadcast = 1;
			}
        	if (!strcmp (argv[i], "-debug")) {
				if (s!='-')
            		debug = atoi (argv[++i]);
				else
					debug = 1;
	    	}
		}
	}
};



/* Read Programm Parameter which will drop us in a special menu */
void
ReadPrgArgs_Jump (int argc, char **argv)
{
    int i = 0;

    while (argv[++i] != NULL) {
        /* check for commands which will put us into a certain menu */
        if (!strcmp (argv[i], "-host")) {
            host_multiplayer_game ();
        }
        else if (!strcmp (argv[i], "-join")) {
            join_multiplayer_game ();
        }
        else if (!strcmp (argv[i], "-connect")) {
            strncpy (bman.servername, argv[++i], LEN_SERVERNAME + LEN_PORT + 2);
            join_multiplayer_game ();
        }
    }
};


/* check the version number, return [ 0 =] [-1 <] [ 1 >] */
int
check_version (int ma, int mi, int su, char *ver)
{
    int v1,
      v2,
      v3,
      res = 0;

    sscanf (ver, "%d.%d.%d", &v1, &v2, &v3);
    if (v1 < ma)
        res = -1;
    else if (v1 > ma)
        res = 1;
    else if (v2 < mi)
        res = -1;
    else if (v2 > mi)
        res = 1;
    else if (v3 < su)
        res = -1;
    else if (v3 > su)
        res = 1;

    // d_printf ("version_check (%d.%d.%d, %s = %d\n" , ma, mi, su, ver, res);

    return res;
};
