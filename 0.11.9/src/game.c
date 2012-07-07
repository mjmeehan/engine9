/* $Id: game.c,v 1.120 2009-05-11 20:51:25 stpohle Exp $ 
  game.c - procedures for the game. */

#include <string.h>
#include <SDL.h>

#include "bomberclone.h"
#include "gfx.h"
#include "network.h"
#include "packets.h"
#include "chat.h"
#include "flyingitems.h"
#include "menu.h"
#include "keyb.h"
#include "single.h"

extern int blitdb_nr,
  blitrects_nr;

Uint32 game_timediff,
  game_timediff1;
static float hurrywarn_to;
static int hurrywarn_state;
static _menu *menu;

void
game_draw_info ()
{
    int i,
      x,
      j,
      col;
    char text[255];
    char scrtext[255];
    SDL_Rect src,
      dest;

    if (GT_MP && (chat.oldscreen == NULL || chat.window.x != 4 || chat.window.y != 4.5 * 16)) {
        chat_show (4, 4.5 * 16, gfx.res.x - 8, gfx.offset.y - 4.5 * 16);
        chat_setactive (0, 0);
    }

    if (bman.updatestatusbar) {
        redraw_logo (0, 0, gfx.res.x, (4.5 * 16));
        dest.x = dest.y = 0;
        dest.h = 4.5 * 16;
        dest.w = gfx.res.x;
        gfx_blitupdaterectadd (&dest);

        /* In Multiplayer mode draw Player names and 
           count the players who are still alife. */
        for (x = 0, j = 0, i = 0; i < MAX_PLAYERS; i++)
            if ((players[i].state & PSFM_used) != 0) {

                if (players[i].gfx_nr != -1 && PS_IS_used (players[i].state)) {
                    src.x = 0;
                    src.y = 0;
                    src.w = dest.w = players[i].gfx->small_image->w;
                    src.h = dest.h = players[i].gfx->small_image->h;

                    dest.x = x;
                    dest.y = j - 4;

                    SDL_BlitSurface (players[i].gfx->small_image, &src, gfx.screen, &dest);
                }

                sprintf (scrtext, "%10s:%d %1d %1d", players[i].name, players[i].wins, players[i].points, players[i].nbrKilled);
                if (!PS_IS_alife (players[i].state)) { // Player is dead
                    if ((players[i].state & PSF_used) != PSF_used)
                        col = 4;
                    else
                        col = 3;
                }
                else {          // player is alife
                    if (bman.gametype == GT_team)
                        col = teams[players[i].team_nr].col;
                    else
                        col = 0;
                }

                font_draw (x, j, scrtext, 0, col);

                x = x + 180;
                if (x >= gfx.res.x - (120 + 170)) {
                    x = 0;
                    j = j + 1.5 * font[0].size.x;
                }
            }

        x = gfx.res.x - 120;
        sprintf (text, "Bombs: %2d", players[bman.p_nr].bombs_n);
        font_draw (x, 0, text, 0, 0);
        sprintf (text, "Range: %2d", players[bman.p_nr].range);
        font_draw (x, 16, text, 0, 0);
        sprintf (text, "Speed: %1.1f", players[bman.p_nr].speed * 10);
        font_draw (x, 32, text, 0, 0);
        if (players[bman.p_nr].special.type != 0) {
            col = players[bman.p_nr].special.type + FT_sp_trigger - 1;
            dest.x = x - 32;
            dest.y = 16;

            dest.w = gfx.menu_field[col]->w;
            dest.h = gfx.menu_field[col]->h;

            SDL_BlitSurface (gfx.menu_field[col], NULL, gfx.screen, &dest);
        }

        if (bman.state == GS_ready && GT_MP_PTPM)
            font_gfxdraw (100, 32, "Press F4 to start the game", 0, COLOR_yellow, 0xFFFF);
        else if (bman.state == GS_ready)
            font_gfxdraw (100, 32, "Waiting for the Server to Start", 0, COLOR_yellow, 0xFFFF);

    }

    /* draw the warning part */
    if (map.state != MS_normal) {
        hurrywarn_to -= timediff;

        if (bman.updatestatusbar || hurrywarn_to <= 0.0 || hurrywarn_to > HURRYWARN_TO_BLINKING) {
            hurrywarn_to = HURRYWARN_TO_BLINKING;
            hurrywarn_state = !hurrywarn_state;

            if (hurrywarn_state) {
                font_drawbold ((gfx.res.x - strlen ("HURRY HURRY") * font[1].size.x) / 2, 40,
                               "HURRY HURRY", 1, 0, 2);
                font_draw ((gfx.res.x - strlen ("HURRY HURRY") * font[1].size.x) / 2, 40,
                           "HURRY HURRY", 1, 1);
            }
            else {
                font_drawbold ((gfx.res.x - strlen ("HURRY HURRY") * font[1].size.x) / 2, 40,
                               "HURRY HURRY", 1, 1, 2);
                font_draw ((gfx.res.x - strlen ("HURRY HURRY") * font[1].size.x) / 2, 40,
                           "HURRY HURRY", 1, 0);
            }
            dest.x = dest.y = 0;
            dest.h = 4.5 * 16;
            dest.w = gfx.res.x;
            gfx_blitupdaterectadd (&dest);
        }
    }

    if (debug)
        debug_ingameinfo ();

    bman.updatestatusbar = 0;
};


/*
 * keyboard handling for keys which have nothing to do with the playerkeys
 * before calling this function make sure keyb_loop (); was called.
 *
 * chat mode: the chatmode should only be disabled in the game mode
 *   in the GS_wait mode the chat will always be active.
 */
void
game_keys_loop ()
{

    if (menu != NULL) {
        /* delete all movement keys */

        int i;
        for (i = 0; i < BCPK_max * 2; i++)
            keyb_gamekeys.state[i] = 0;
    }
    else {

        /* don't go into the game_keys if there is no menu displayed */

        if (GT_MP_PTPM && bman.state == GS_ready && keyb_gamekeys.state[BCK_pause]
            && !keyb_gamekeys.old[BCK_pause]) {
            /* Server is starting the game
             * check in multiplayer if all players are ready for the game
             */
            int i,
              ready = 1;

            for (i = 0; i < MAX_PLAYERS; i++)
                if (NET_CANSEND (i) && !players[i].ready)
                    ready = 0;

            if (ready)
                bman.state = GS_running;
            else
                d_printf ("game_keys_loop: not all players are ready\n");

            net_send_servermode ();
            bman.updatestatusbar = 1; // force an update
        }

        if (keyb_gamekeys.state[BCK_fullscreen] && !keyb_gamekeys.old[BCK_fullscreen]) {
            /* Switch Fullscreen */
            SDL_WM_ToggleFullScreen (gfx.screen);
            gfx.fullscreen = !gfx.fullscreen;
            bman.updatestatusbar = 1; // force an update
        }

		
        if (keyb_gamekeys.state[BCK_esc] && !keyb_gamekeys.old[BCK_esc]) {
            if (chat.active && (bman.state == GS_ready || bman.state == GS_running) && IS_LPLAYER2) {
                chat.active = 0;
                d_printf ("Chatmode Disabled\n");
            }
            else
                game_menu_create ();
        }

        if ((GT_MP_PTPM || GT_MP_PTPS) && keyb_gamekeys.state[BCK_chat]
            && !keyb_gamekeys.old[BCK_chat]) {
            chat_setactive (1, 0);
            chat.changed = 1;
            d_printf ("Chatmode Enabled\n");
        }
    }
};



void
game_loop ()
{
    SDL_Event event;
    int done = 0,
        eventstate;

    if (GT_MP)
        net_game_fillsockaddr ();
	if ( SDL_InitSubSystem ( SDL_INIT_JOYSTICK ) < 0 )
        {
                fprintf ( stderr, "Unable to initialize Joystick: %s\n", SDL_GetError() );
        }
	 printf ( "%i joysticks found\n", SDL_NumJoysticks () );

    menu = NULL;
    bman.updatestatusbar = 1;   // force an update
    timestamp = SDL_GetTicks (); // needed for time sync.
    d_gamedetail ("GAME START");

    gfx_blitupdaterectclear ();
    draw_logo ();
    draw_field ();
    SDL_Flip (gfx.screen);
    draw_players ();

    if (bman.p_nr >= 0 && bman.p_nr < MAX_PLAYERS) {
        players[bman.p_nr].ready = 1;
        if (GT_MP_PTPS)
            send_playerdata (&players[bman.p_servnr].net.addr, bman.p_nr, &players[bman.p_nr]);
    }
    if (bman.p2_nr >= 0 && bman.p2_nr < MAX_PLAYERS) {
        players[bman.p2_nr].ready = 1;
        if (GT_MP_PTPS)
            send_playerdata (&players[bman.p_servnr].net.addr, bman.p2_nr, &players[bman.p2_nr]);
    }

    while (!done && (bman.state == GS_running || bman.state == GS_ready)) {
	SDL_JoystickUpdate ();
        if ((eventstate = SDL_PollEvent (&event)) != 0)
            switch (event.type) {
            case (SDL_QUIT):
                done = 1;
                bman.state = GS_quit;
            }

        /*
         * input handling 
         */
        keyb_loop (&event);

        game_keys_loop ();

        if (GT_MP)
            chat_loop (&event);

        if ((!IS_LPLAYER2) && (!chat.active))
            chat_setactive (1, 1);

        restore_players_screen ();

        player_check (bman.p_nr);
        if (IS_LPLAYER2)
            player_check (bman.p2_nr);

        dead_playerani ();

        special_loop ();

        player_move (bman.p_nr);
        if (IS_LPLAYER2)
            player_move (bman.p2_nr);

        if (GT_MP) {
            player_calcpos ();
            network_loop ();
        }

        if (bman.state == GS_running)
            single_loop ();

        bomb_loop ();
        field_loop ();
        flitems_loop ();

        draw_players ();
        game_draw_info ();      // will set the var bman.player_nr

        /* check if there is only one player left and the game is in multiplayer mode
           and if there the last dieing animation is done */
        if (game_check_endgame () && bman.timeout >= 0.0f)
            bman.timeout = 0.0f;

        if ((GT_SP || GT_MP_PTPM) && bman.timeout < -GAME_OVERTIMEOUT) {
            d_printf ("GAME: Game Over\n");
            done = 1;
        }

        stonelist_draw ();

        /* if there is any menu displayed do so */
        if (menu != NULL)
            game_menu_loop (&event, eventstate);

        gfx_blitdraw ();

        s_calctimesync ();
        bman.timeout -= timediff;

    }

    if (menu != NULL) {
        menu_delete (menu);
		menu = NULL;
	}
	
    gfx_blitdraw ();

    chat_show (-1, -1, -1, -1);
    draw_logo ();
    gfx_blitupdaterectclear ();
    SDL_Flip (gfx.screen);

    d_gamedetail ("GAME END");
    d_printf ("done = %d\n", done);
};


/*
 * check if we only one player left or only ai players are left.
 * check also if we there is only one team alife
 */
#define ENDGAME_CHECK_AGAIN 1.0f
int
game_check_endgame ()
{
    int res = 0;
    static float loop;

    loop -= timediff;

    if (loop > 0.0f && loop < ENDGAME_CHECK_AGAIN)
        return 0;
    loop = ENDGAME_CHECK_AGAIN;

    if (bman.gametype == GT_team) {
        /*
         * Team Mode Calculation
         */
        int t_nr;               // teamnumber
        int p_nr;               // playernumber
        int h_team = 0;         // how many human teams are alife
        int ateam = 0;          // teams which are alife
        int h_team_last = -1;   // last human team which was alife
        int team_last = -1;     // last teams which was alift
        _player *p;

        for (t_nr = 0; t_nr < MAX_TEAMS; t_nr++)
            for (p_nr = 0; p_nr < MAX_PLAYERS; p_nr++)
                if (teams[t_nr].players[p_nr] != NULL) {
                    p = teams[t_nr].players[p_nr];
                    if (PS_IS_used (p->state) && PS_IS_alife (p->state)) {
                        if (team_last != t_nr) {
                            team_last = t_nr;
                            ateam++;
                        }
                        if ((!PS_IS_aiplayer (p->state)) && h_team_last != t_nr) {
                            h_team++;
                            h_team_last = t_nr;
                        }
                    }
                }

        if (h_team < 1 || ateam < 2)
            res = 1;
    }
    else if ((bman.gametype == GT_bomberman)
             || (map.state != MS_normal && bman.gametype == GT_deathmatch)) {
        int p_nr;               // playernumber
        int h_alife = 0;        // human players who are alife
        int alife = 0;          // ai players who are alife
        _player *p;

        for (p = &players[0], p_nr = 0; p_nr < MAX_PLAYERS; p_nr++, p++) {
            if (PS_IS_used (p->state) && PS_IS_alife (p->state)) {
                alife++;
                if (!PS_IS_aiplayer (p->state))
                    h_alife++;
            }
        }

        if ((h_alife < 1) || (alife < 2))
            res = 1;
    }

    return res;
};

#undef ENDGAME_CHECK_AGAIN

/* check which player won and free all unnneded data */
void
game_end ()
{
    int i;
    int cnt_left = 0;

    gfx_free_players ();
    tileset_free ();
    snd_music_stop ();
    snd_free ();

    /* count the wins for the player, and if only one player 
     * left count the points too */
    cnt_left = 0;
    for (i = 0; i < MAX_PLAYERS; i++){
        // Update player statistics
        players[i].gamestats.killed += players[i].nbrKilled;
        if (PS_IS_used (players[i].state)) {
            if (PS_IS_alife (players[i].state)) {
                bman.lastwinner = i;
                cnt_left++;
                if ( GT_MP_PTPM )
                players[i].wins++;
            }
        }
    }
	
    if (cnt_left == 1 && GT_MP_PTPM ){
        players[bman.lastwinner].points += 1; // Bonus for victory
        players[bman.lastwinner].points += players[bman.lastwinner].nbrKilled;
    }
    else
        bman.lastwinner = -1;

    /* check which team was alife */
    if (bman.gametype == GT_team) {
        int t_nr;

        bman.lastwinner = -1;
        cnt_left = 0;

        for (t_nr = 0; t_nr < MAX_TEAMS; t_nr++)
            for (i = 0; i < MAX_PLAYERS; i++) {
                if (teams[t_nr].players[i] != NULL)
                    if (PS_IS_alife (teams[t_nr].players[i]->state)) {
                        if (bman.lastwinner != t_nr) {
                            teams[t_nr].wins++;
                            cnt_left++;
                            bman.lastwinner = t_nr;
                        }
                    }
            }

        if (cnt_left == 1) {
            /* set the points */
            cnt_left = 0;
            for (t_nr = 0; t_nr < MAX_TEAMS; t_nr++) {
                for (i = 0; (i < MAX_PLAYERS && teams[t_nr].players[i] != NULL); i++);
                if (i < MAX_PLAYERS && teams[t_nr].players[i] != NULL)
                    cnt_left++;
            }
            teams[bman.lastwinner].points += cnt_left;
        }
        else
            bman.lastwinner = -1;
    }

    if (GT_SP)
        game_showresult ();

    /* check which player is now free,i.e. disconnected during the game and was playing */
    for (i = 0; i < MAX_PLAYERS; i++)
        if ((players[i].state & PSF_used) == 0)
            players[i].state = 0;
}


/* load the images with the right scaleing */
void
game_start ()
{
    int p,
      i;

    menu_displaytext ("Loading..", "Please Wait");

    bman.players_nr_s = 0;
    bman.playnum += 1;
    for (p = 0; p < MAX_PLAYERS; p++) {
        if (PS_IS_used (players[p].state)) {
            bman.players_nr_s++;
            if (players[p].gfx_nr == -1) {
                players[p].gfx = NULL;
                players[p].state &= (0xff - (PSF_alife + PSF_playing));
            }
            else {
                players[p].state |= PSF_alife + PSF_playing;
                players[p].gfx = &gfx.players[players[p].gfx_nr];
            }
        }
        else
            players[p].state = 0;

        players[p].bombs_n = bman.start_bombs;
        players[p].range = bman.start_range;
        players[p].speed = bman.start_speed;
        players[p].collect_shoes = 0;
        players[p].special.type = SP_nothing;
		players[p].special.use = 0;
		players[p].special.clear = 0;
        players[p].m = 0;
        players[p].old.x = 0;
        players[p].old.y = 0;
        bman.updatestatusbar = 1;
        players[p].frame = 0.0f;
        players[p].d = 0;
        players[p].pos.x = 0.0;
        players[p].pos.y = 0.0;
        players[p].tunnelto = 0.0f;
        players[p].ready = 0;
        // add reset nbrKilled value
        players[p].nbrKilled = 0;

        /* all types of illnes turn them off */
        for (i = 0; i < PI_max; i++)
            players[p].ill[i].to = 0.0f;

        // reset bombs
        for (i = 0; i < MAX_BOMBS; i++) {
            players[p].bombs[i].state = BS_off;
            players[p].bombs[i].ex_nr = -1;
            players[p].bombs[i].fdata = 0;
            players[p].bombs[i].dest.x = 0;
            players[p].bombs[i].dest.y = 0;
            players[p].bombs[i].mode = BM_normal;
            players[p].bombs[i].id.p = p;
            players[p].bombs[i].id.pIgnition = p;
            players[p].bombs[i].id.b = i;
            players[p].pos.x = 0;
            players[p].pos.x = 0;
        }
    }

    flitems_reset ();
    init_map_tileset ();

    tileset_load (map.tileset, -1, -1);

    gfx_load_players (gfx.block.x, gfx.block.y);

    snd_load (map.tileset);
    snd_music_start ();

    map.state = MS_normal;
    bman.timeout = bman.init_timeout;
    s_calctimesync ();          // to clean up the timesyc 
    s_calctimesync ();          // data run this twice
    if (GT_MP_PTPM)
        net_send_servermode ();
};


/*
 * Show results of the game 
 * show the diffrent screens one for players and one for teams
 */
/* Teamplay */
#define SHOWRESULT_TEAMHEAD 24
#define SHOWRESULT_TEAMPLAYER 16
#define SHOWRESULT_TEAMPLAYERWIDTH 150
void
game_showresultteam (int pos_x, int pos_y, int pos_w, int pos_h)
{
    int i,
      t_nr,
      p_nr;                     // counter for teams and players
    struct {
        _team *team;            // pointer to the team
        _player *pl[MAX_PLAYERS]; // players in the team (sorted)
        int cnt;
    } tdata[MAX_TEAMS];         // hold some team informations (sorted)
    int t_count = 0,
        p_maxcount = 0,
        p_sumcount = 0;
    int sx,
      sy,
      p_y,
      p_x,
      dx,
      dy,
      col,
      x = 0;
    SDL_Rect dest,
      src;
    char text[255];

    /* sort all teams */
    for (t_nr = 0; t_nr < MAX_TEAMS; t_nr++) {
        tdata[t_nr].team = NULL;
        tdata[t_nr].cnt = 0;
    }

    for (t_nr = 0; t_nr < MAX_TEAMS; t_nr++) {
        for (p_nr = 0; (p_nr < MAX_PLAYERS && teams[t_nr].players[p_nr] == NULL); p_nr++);

        if (p_nr < MAX_PLAYERS && teams[t_nr].players[p_nr] != NULL) {
            tdata[t_count].team = &teams[t_nr];
            i = t_count;

            while (i > 0 && (tdata[i - 1].team->wins < teams[t_nr].wins
                             || (tdata[i - 1].team->wins == teams[t_nr].wins
                                 && tdata[i - 1].team->points < teams[t_nr].points))) {
                tdata[i].team = tdata[i - 1].team;
                i--;
                tdata[i].team = &teams[t_nr];
            }
            t_count++;
        }
    }

    /* sort all players dependsing on the number of wins they have */
    for (t_nr = 0; t_nr < t_count; t_nr++)
        for (p_nr = 0, tdata[t_nr].cnt = 0; p_nr < MAX_PLAYERS; p_nr++) {
            if (t_nr < t_count) {
                if (tdata[t_nr].team->players[p_nr] != NULL
                    && PS_IS_used (tdata[t_nr].team->players[p_nr]->state)) {
                    tdata[t_nr].pl[tdata[t_nr].cnt] = tdata[t_nr].team->players[p_nr];
                    i = tdata[t_nr].cnt;

                    while (i > 0
                           && (tdata[t_nr].pl[i - 1]->wins < tdata[t_nr].team->players[p_nr]->wins
                               || (tdata[t_nr].pl[i - 1]->wins ==
                                   tdata[t_nr].team->players[p_nr]->wins
                                   && tdata[t_nr].pl[i - 1]->points <
                                   tdata[t_nr].team->players[p_nr]->points))) {
                        tdata[t_nr].pl[i] = tdata[t_nr].pl[i - 1];
                        i--;
                        tdata[t_nr].pl[i] = tdata[t_nr].team->players[p_nr];
                    }
                    tdata[t_nr].cnt++;
                }
            }
        }

    /* check the max number of players in one team and number of all players */
    for (t_nr = 0, p_maxcount = 0; t_nr < t_count; t_nr++) /* t_count + 1 */
        if (p_maxcount < tdata[t_nr].cnt)
            p_maxcount = tdata[t_nr].cnt;

    for (p_sumcount = 0, p_nr = 0; p_nr < MAX_PLAYERS; p_nr++)
        if (PS_IS_used (players[p_nr].state))
            p_sumcount++;

    /* calculate the best view */
    p_x = dx = dy = 0;
    do {
        p_x++;
        p_y = 0;                // calc. again for this setting
        for (t_nr = 0; t_nr < t_count; t_nr++) {
            p_y += ceil ((float) (((float) tdata[t_nr].cnt) / ((float) p_x)));
        }
        if (p_y == 0)
            p_y = 1;
        dy = (pos_h - (SHOWRESULT_TEAMHEAD * t_count)) / p_y;
    } while (dy < SHOWRESULT_TEAMPLAYER);

    if (dy > 2 * SHOWRESULT_TEAMPLAYER)
        dy = 2 * SHOWRESULT_TEAMPLAYER;

    /* draw everything */
    sy = (pos_h - (SHOWRESULT_TEAMHEAD * t_count + dy * p_y)) / 2;
    for (t_nr = 0; t_nr < t_count; t_nr++) {
        sprintf (text, "%s Victorys %d (%d)", tdata[t_nr].team->name, tdata[t_nr].team->wins,
                 tdata[t_nr].team->points);
        sx = (pos_w - strlen (text) * font[0].size.x) / 2;
        font_gfxdrawbold (10 + pos_x + sx, pos_y + sy + 3, text, 0, COLOR_brown, 1, 1);
        font_gfxdraw (10 + pos_x + sx, pos_y + sy + 3, text, 0, COLOR_yellow, 2);
        sy += SHOWRESULT_TEAMHEAD;
        dx = pos_w / p_x;
        sx = (dx - SHOWRESULT_TEAMPLAYERWIDTH) / 2;

        for (col = 0, p_nr = 0; p_nr < tdata[t_nr].cnt; p_nr++) {
            if (col == 0 || col >= p_x) {
                if (col >= p_x)
                    sy += dy;
                col = 0;
                x = sx;
            }

            if (tdata[t_nr].pl[p_nr]->gfx != NULL) {
                dest.x = pos_x + x;
                dest.y = pos_y + sy;
                src.w = dest.w = tdata[t_nr].pl[p_nr]->gfx->small_image->w;
                src.h = dest.h = tdata[t_nr].pl[p_nr]->gfx->small_image->h;
                src.x = 0;
                src.y = 0;
                gfx_blit (tdata[t_nr].pl[p_nr]->gfx->small_image, &src, gfx.screen, &dest, 1);
            }
            else {
                dest.x = pos_x + x;
                dest.y = pos_y + sy;
                src.w = dest.w = gfx.ghost_small->w;
                src.h = dest.h = gfx.ghost_small->h;
                src.x = 0;
                src.y = 0;
                gfx_blit (gfx.ghost_small, &src, gfx.screen, &dest, 1);
            }
            sprintf (text, "%s(%d/%d)", tdata[t_nr].pl[p_nr]->name, tdata[t_nr].pl[p_nr]->wins,
                     tdata[t_nr].pl[p_nr]->points);
            font_gfxdraw (10 + pos_x + x + GFX_SMALLPLAYERIMGSIZE_X * 2, pos_y + sy + 2, text, 0, 0,
                          2);
            x += dx;
            col++;
        }
        sy += dy;
    }
}

#undef SHOWRESULT_TEAMHEAD
#undef SHOWRESULT_TEAMPLAYER
#undef SHOWRESULT_TEAMPLAYERWIDTH


/* Bomberman/Deathmatch Version Play */
#define SHOWRESULT_WIDTH 150
#define SHOWRESULT_HEIGHT 60
void
game_showresultnormal (int pos_x, int pos_y, int pos_w, int pos_h)
{
    char text[255];
    int i,
      p,
      x,
      y,
      pl_cnt = 0,
        pl_x,
        pl_y,                   // player in a row/col
        dx,
        dy,                     // distance
        sx,
        sy,
        px;                     // start view and position

    SDL_Rect dest,
      src;
    _player *pl[MAX_PLAYERS];


    /* Sort the playerlist */
    for (p = 0, pl_cnt = 0; p < MAX_PLAYERS; p++)
        if (PS_IS_used (players[p].state)) {
            // Set isplayer statistics for futur display
            players[p].gamestats.isaplayer = 1;
            // Sort player
            pl[pl_cnt] = &players[p];
            i = pl_cnt;

            while (i > 0 && (pl[i - 1]->points < players[p].points
                             || (pl[i - 1]->points == players[p].points
                                 && pl[i - 1]->wins < players[p].wins))) {
                pl[i] = pl[i - 1];
                i--;
                pl[i] = &players[p];
            }
            pl_cnt++;
        }

    if (pl_cnt == 0) {
        /* we still haven't joined the game */
    }

    /* calc the best view and start point */
    pl_x = 0;
    do {
        pl_x++;
        pl_y = ceil ((float) (((float) pl_cnt) / ((float) pl_x)));
        if (pl_y == 0)
            pl_y++;
        dy = pos_h / pl_y;
    } while (dy < SHOWRESULT_HEIGHT);
    dx = pos_w / pl_x;

    x = sx = (dx - SHOWRESULT_WIDTH) / 2;
    y = sy = (dy - SHOWRESULT_HEIGHT) / 2;
    px = 0;

    d_printf ("game_showresultnormal: pl_x:%d, pl_y:%d, dx:%d, dy:%d\n", pl_x, pl_y, dx, dy);

    /* draw the playerlist */
    for (i = 1, p = 0; p < pl_cnt; p++) {
        if (PS_IS_used (pl[p]->state)) {
            if (PS_IS_alife (pl[p]->state)) {
                font_gfxdrawbold (10 + pos_x + x + GFX_MENUPLAYERIMGSIZE_X + 8, pos_y + y - 10,
                                  pl[p]->name, 0, COLOR_brown, 1, 1);
                font_gfxdraw (10 + pos_x + x + GFX_MENUPLAYERIMGSIZE_X + 8, pos_y + y - 10,
                              pl[p]->name, 0, COLOR_yellow, 1);
            }
            else
                font_gfxdraw (10 + pos_x + x + GFX_MENUPLAYERIMGSIZE_X, pos_y + y - 10, pl[p]->name,
                              0, COLOR_gray, 1);

            sprintf (text, "%3d (%2d)", pl[p]->points, pl[p]->wins);
            font_gfxdraw (10 + pos_x + x + GFX_MENUPLAYERIMGSIZE_X, pos_y + y + 6, text, 0, 0, 1);

            if (pl[p]->gfx != NULL) {
                dest.x = pos_x + x;
                dest.y = pos_y + y - 16;
                src.w = dest.w = pl[p]->gfx->menu_image->w;
                src.h = dest.h = pl[p]->gfx->menu_image->h;
                src.x = 0;
                src.y = 0;
                gfx_blit (pl[p]->gfx->menu_image, &src, gfx.screen, &dest, 1);
            }
            else {
                dest.x = pos_x + x;
                dest.y = pos_y + y - 16;
                src.w = dest.w = gfx.ghost->w;
                src.h = dest.h = gfx.ghost->h;
                src.x = 0;
                src.y = 0;
                gfx_blit (gfx.ghost, &src, gfx.screen, &dest, 1);
            }
            /* setup the new position */
            y += (dy / pl_x);
            x += dx;
            px++;
            if (px >= pl_x) {
                px = 0;
                x = sx;
            }
        }
    }
}

#undef SHOWRESULT_HEIGHT
#undef SHOWRESULT_WIDTH


void
game_showresult ()
{
    char text[255];
    SDL_Event event;
    Uint8 *keys;
    int done = 0,
        keypressed = 0,
        x,
        y;

    gfx_blitdraw ();

    draw_logo ();

    strcpy (text, "Game Result");
    x = (gfx.res.x - (font[2].size.x * strlen (text)) - 64) / 2;
    y = 0;
    font_drawbold (x, y, text, 2, 6, 2);
    font_draw (x, y, text, 2, 5);
    y += font[2].size.x;

    strcpy (text, "[CTRL],[RETURN] or [STRG] for another game");
    x = (gfx.res.x - (font[1].size.x * strlen (text)) - 64) / 2;
    font_drawbold (x, gfx.res.y - (2 * font[0].size.y) - 2, text, 0, COLOR_brown, 1);
    font_draw (x, gfx.res.y - (2 * font[0].size.y) - 2, text, 0, COLOR_yellow);

    strcpy (text, "or [ESC] to leave the game.");
    x = (gfx.res.x - (font[1].size.x * strlen (text)) - 64) / 2;
    font_drawbold (x, gfx.res.y - font[0].size.y - 2, text, 0, COLOR_brown, 1);
    font_draw (x, gfx.res.y - font[0].size.y - 2, text, 0, COLOR_yellow);

    if (bman.gametype == GT_team)
        game_showresultteam (10, 50, gfx.res.x - 20, gfx.res.y - 100);
    else
        game_showresultnormal (10, 50, gfx.res.x - 20, gfx.res.y - 100);

    gfx_blitdraw ();
    SDL_Flip (gfx.screen);

    while (!done && bman.state != GS_quit) {
        /* do the keyboard handling */
        if (s_fetchevent (&event) != 0)
            switch (event.type) {
            case (SDL_QUIT):
                bman.state = GS_quit;
                bman.p_nr = -1;
                done = 1;
            }

        keys = SDL_GetKeyState (NULL);

        if (keys[SDLK_ESCAPE] && event.type == SDL_KEYDOWN) {
            /* we want to quit */
            done = 1;
            bman.p_nr = -1;
            keypressed = 1;
            bman.state = GS_startup;
        }

        if ((keys[SDLK_RETURN] || keys[SDLK_LCTRL] || keys[SDLK_RCTRL]) && (!keypressed)
            && (event.type = SDL_KEYDOWN)) {
            done = 1;
            keypressed = 1;
        }

        if (keys[SDLK_F8] && event.type == SDL_KEYDOWN) {
            /* Switch Fullscreen */
            SDL_WM_ToggleFullScreen (gfx.screen);
            gfx.fullscreen = !gfx.fullscreen;
            bman.updatestatusbar = 1; // force an update
        }

        if (event.type == SDL_KEYUP)
            keypressed = 0;
        else if (event.type == SDL_KEYDOWN)
            keypressed = 1;

        s_delay (25);
    }

    if (bman.state != GS_quit && bman.state != GS_startup)
        bman.state = GS_running;
};


/*
 * create the in game menu
 */
void
game_menu_create ()
{
    if (menu != NULL)
        return;

    menu = menu_new ("Gamemenu", 300, 150);

    menu_create_button (menu, "Back to the Game", -1, 50, 200, 1);
    if (GT_SP || GT_MP_PTPM)
        menu_create_button (menu, "End this Round", -1, 80, 200, 2);
    menu_create_button (menu, "Quit the Game", -1, 110, 200, 3);

    menu_focus_id (menu, 1);
    menu->looprunning = 1;
};


/*
 * in game menu_loop, will called every game loop.
 * As long as this menu is displayed the gamekeys will not work.
 *
 * the drawings will be done from the main loop. So we won't
 * have to draw anything on our own.
 *
 * Pressing ESC will bring you back to the game.
 */
void
game_menu_loop (SDL_Event * event, int eventstate)
{
    int done;

    if (menu == NULL)
        return;

    menu_draw (menu);

    done = menu_event_loop (menu, event, eventstate);
    /* 
     * check if one of the buttons was pressed
     */

    if (done != 0) {
        if (menu->focus->id == 2 && (GT_MP_PTPM || GT_SP)) { /* End Round */
            bman.timeout = -GAME_OVERTIMEOUT;
        }

        else if (menu->focus->id == 3) { /* End Game */
            /* send network update */
            if (GT_MP)
                net_game_send_delplayer (bman.p_nr);
            bman.state = GS_quit;
        }

        else {                  /* Quit Menu */
            menu_delete (menu);
            menu = NULL;
            gfx_blitdraw ();
            draw_field ();
        }
    }
};


/* reset all old gamedata, this is only needed on join, host or 
 * new singleplayer games. */
void game_resetdata () {
	int i;
	
	for (i = 0; i < MAX_PLAYERS; i++)
		memset (&players[i], 0, sizeof (_player));
	bman.p2_nr = bman.p_nr = -1;
};
