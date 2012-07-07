/* $Id: single.h,v 1.6 2004-11-07 04:13:09 stpohle Exp $
 * single player */
 
// single.c
extern void single_game_new ();
extern int single_create_ai (int num_players);
extern void	single_loop();
extern void single_playergame (int second_player, int ai_players);
extern void single_menu ();
extern int single_select_player ();
extern int ai_choosedir (int dir, int nearbomb, int oldpos);
extern inline int ai_invertdir (int dir);
extern inline int ai_checkpos (_player * pl, _point * pos);
extern int ai_findnearbombs (_point pos);
extern int ai_findbestbombdir (_point pos, int dir, int range);
extern int ai_bombpoints (_point pos, int range);
extern _airunaway ai_runawayfrom (_point p, int nearbomb, int range, signed char norecursive);
extern int ai_checkfield (int x, int y);
extern int ai_easyrunaway (_point p, int range);
extern void ai_team_choosegfx ();
