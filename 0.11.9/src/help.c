/* $Id: help.c,v 1.14 2004-12-26 04:19:20 stpohle Exp $ 
 * Display complex help text and information screen about the game
 */

#include "bomberclone.h"
#include "menu.h"
#include "player.h"

/*
 * show the manual pages
 */
void help (int showpage) {
	int page = showpage, menuselect = 2, y = 0;
	char title[255];
	_menu *menu;
	
    menu_displaytext ("Please Wait", "Loading GFX Data");

	if (page < 0 || page >= HP_max)
		page = 0;
	
	while (menuselect != -1 && menuselect != 1 && bman.state != GS_quit) {
		if (page == HP_howto0) {
			sprintf (title, "How To Play (%d/%d)", page + 1, HP_max);
			menu = menu_new (title, 500, 400);
			menu_create_text (menu, "help", 5, 55, 53, 20, COLOR_brown,
			"The goal of the game is to be the last one, "
			"who is alive. You can drop bombs which will explode after "
			"a certain time and destroy everything in horizontal and vertical "
			"direction. So you can remove stones or kill other players. But take care. "
			"Don't kill yourself otherwise the game will be over for you. During the "
			"game you will find diffrenent powerups to raise your skills. If you are "
			"running faster than your opponent and you have many bombs, you can catch "
			"him within lots of bombs and he has no chance to escape.");

			menu_create_image (menu, "img", 450, 255, 0, gfx.players[0].menu_image, NULL);
			
			menu_create_text (menu, "help", 5, 255, 45, 10, COLOR_brown, 
			"You will get points for every player you have killed. "
			"If you win the game, you can earn additional points "
			"depending on how many players played the game. ");
		}
		else if (page == HP_powerup0) {
			sprintf (title, "Powerups (%d/%d)", page + 1, HP_max);
			menu = menu_new (title, 500, 400);
			
			y = 50;
			
			menu_create_text (menu, "help", 5, y, 53, 10, COLOR_brown,
			"In the game you will find some diffend kind of powerups. "
			"There are the powerups who give you more power for the whole game "
			"and the special powerups which will hold only for a certain time.");
			y += 75;
			
			menu_create_label (menu, "Permanent Powerups", -1, y, 2, COLOR_yellow);
			y += (5 + font[2].size.y);
			
			menu_create_image (menu, "bomb", 5, y, 0, gfx.menu_field[FT_bomb], NULL);
			menu_create_text (menu, "help", 55, y, 45, 10, COLOR_brown,
			"Give you another bomb to drop. Maximum number of bombs is %d.", MAX_BOMBS);
			y += 40;
			
			menu_create_image (menu, "fire", 5, y, 1, gfx.menu_field[FT_fire], NULL);
			menu_create_text (menu, "help", 55, y, 45, 10, COLOR_brown,
			"The range of your bombs will be increased. Maximum range is %d.", MAX_RANGE);
			y += 40;

			menu_create_image (menu, "shoe", 5, y, 1, gfx.menu_field[FT_shoe], NULL);
			menu_create_text (menu, "help", 55, y, 45, 10, COLOR_brown,
			"This will make your player run faster. The maximum speed will be %1.2f.", MAX_SPEED);
			y += 40;

			menu_create_text (menu, "help", 5, y, 53, 10, COLOR_brown,
			"Depends how the game is set up, you'll lose "
			"these powerups if you die. Other players can collect them. "
			"In the deathmatch mode you can keep the powerups you collected, "
			"but this depends on the Game if you drop them or not.");
			y += 40;
		}
		else if (page == HP_powerup1) {
			sprintf (title, "Powerups (%d/%d)", page + 1, HP_max);
			menu = menu_new (title, 500, 400);
			
			y = 45;
			
			menu_create_label (menu, "Special Powerups", -1, y, 2, COLOR_yellow);
			y += (5 + font[2].size.y);
			
			menu_create_image (menu, "kick", 5, y, 1, gfx.menu_field[FT_sp_kick], NULL);
			menu_create_text (menu, "help", 55, y, 48, 10, COLOR_brown,
			"Allowes you to kick some bombs around the level. This will hold "
			"just a short time of %d seconds. The maximum distance you can "
			"kick the bombs is %d fields.", SPECIAL_KICK_TIME, SPECIAL_KICK_MAXDIST);
			y += 70;
			
			menu_create_image (menu, "push", 5, y, 1, gfx.menu_field[FT_sp_push], NULL);
			menu_create_text (menu, "help", 55, y, 48, 10, COLOR_brown,
			"Push bombs one field, as long as nothing is behind this bomb.");
			y += 40;
			
			menu_create_image (menu, "droprow", 5, y, 1, gfx.menu_field[FT_sp_row], NULL);
			menu_create_text (menu, "help", 55, y, 48, 10, COLOR_brown,
			"You can drop a row of that many bombs you have still left to drop.");
			y += 40;
			
			menu_create_image (menu, "dropliquid", 5, y, 1, gfx.menu_field[FT_sp_liquid], NULL);
			menu_create_text (menu, "help", 55, y, 48, 10, COLOR_brown,
			"The bomb you push now won't stop moving untill they explode.");
			y += 40;
			
			menu_create_image (menu, "dropliquid", 5, y, 1, gfx.menu_field[FT_sp_moved], NULL);
			menu_create_text (menu, "help", 55, y, 45, 10, COLOR_brown,
			"The bomb you push will stop moving on the next border or bomb.");
			y += 40;

			menu_create_image (menu, "dropltrigger", 5, y, 1, gfx.menu_field[FT_sp_trigger], NULL);
			menu_create_text (menu, "help", 55, y, 45, 10, COLOR_brown,
			"You will be able to drop triggered bombs. Use "
			"the special key to let all your bombs explode. "
			"at the time where you want it.");
			y += 40;
		}
		else if (page == HP_powerup2) {
			sprintf (title, "Powerups (%d/%d)", page + 1, HP_max);
			menu = menu_new (title, 500, 400);
			
			y = 45;
			
			menu_create_label (menu, "Death Item", -1, y, 2, COLOR_yellow);
			y += (5 + font[2].size.y);
			
			menu_create_text (menu, "help", 5, y, 53, 10, COLOR_brown,
			"In the game you will find another type of item to collect. "
			"This item is not a powerup at all. If you collect it you will "
			"get a random illness. This illness will hold for %dseconds. "
			"If you have contact to another player the other player will "
			"get all the illnesses you have too.", ILL_TIMEOUT);
			y += 110;
			
			menu_create_image (menu, "pwdeath", 12, y+8, 1, gfx.menu_field[FT_death], NULL);
			menu_create_text (menu, "help", 55, y, 45, 10, COLOR_brown,
			"This will make your player ill. We have at the moment %d diffrent "
			"types of illnesses for you to collect. To make the game more", PI_max);
			y += 3*font[0].size.y;
			menu_create_text (menu, "help", 5, y, 53, 10, COLOR_brown,
			"interesting we won't put here a list of all types there are.");
		}
		else if (page == HP_keyboard0) {
			sprintf (title, "Keyboard (%d/%d)", page + 1, HP_max);
			menu = menu_new (title, 500, 400);
			
			y = 50;
			
			menu_create_label (menu, "During a Game", -1, y, 2, COLOR_yellow);
			y += font[2].size.y;
			
			menu_create_image (menu, "img", 450, 100, 0, gfx.players[7].menu_image, NULL);
			
			menu_create_text (menu, "help", 5, y, 53, 10, COLOR_brown,
			"Arrow Keys - Moving of the Player\n"
			"STRG/CTRL  - Dropping bombs\n"
			"Shift      - Special Use Key\n"
			"F4         - Start the Game (only Server)\n"
			"F8         - Fullscreen (not in Windows)\n"
			"Return     - Send Entered Text Message\n"
			"ESC        - Exit Game\n");
			y += 7*font[0].size.y;
			
			menu_create_label (menu, "Player Selection", -1, y, 2, COLOR_yellow);
			y += font[2].size.y;
			
			menu_create_text (menu, "help", 5, y, 53, 10, COLOR_brown,
			"Left/Right - Select and Deselect a Player\n"
			"F1	        - Mini Help Screen\n"
			"F2         - Player Screen\n"
			"F3         - Map and Game Settings\n"
			"F4         - Start the Game if at last 2 Players\n"
			"             are selected. (only Server)\n"
			"ESC        - Exit Game\n");
		}
		else if (page == HP_credit0) {
			sprintf (title, "About BomberClone (%d/%d)", page + 1, HP_max);
			menu = menu_new (title, 500, 400);
			
			menu_create_image (menu, "img", 15, 60, 0, gfx.players[4].menu_image, NULL);
			menu_create_text (menu, "help", 75, 50, 45, 10, COLOR_brown,
			"If you have any problems or questions with the game you can send your questions "
			"to the mailinglist or directly to me. Bugfixes should be send to the SourceForge "
			"Projects page about BomberClone.");
			
			menu_create_label (menu, "WWW",-1, 140, 2, COLOR_yellow);
			menu_create_text (menu, "help", -1, 165, 53, 10, COLOR_brown, "http://www.bomberclone.de");

			menu_create_label (menu, "EMail",-1, 185, 2, COLOR_yellow);
			menu_create_text (menu, "help", -1, 210, 53, 10, COLOR_brown, "steffen@bomberclone.de");

			menu_create_label (menu, "Project Page",-1, 230, 2, COLOR_yellow);
			menu_create_text (menu, "help", -1, 255, 53, 10, COLOR_brown, "http://sourceforge.net/projects/bomberclone");
		}			
		else if (page == HP_credit1) {
			sprintf (title, "People (%d/%d)", page + 1, HP_max);
			menu = menu_new (title, 500, 400);
			
			menu_create_image (menu, "img", 250, 100, 0, gfx.players[6].menu_image, NULL);
			
			y = 50;
			menu_create_label (menu, "Coding:", 5, y, 2, COLOR_yellow);
			y += font[2].size.y;
			menu_create_text (menu, "help", 50, y, 53, 10, COLOR_brown,
			"  Steffen Pohle\n"
			"Patrick Wilczek\n");

			y = 100;
			menu_create_label (menu, "GFX:", 425, y, 2, COLOR_yellow);
			y += font[2].size.y;
			menu_create_text (menu, "help", 325, y, 53, 10, COLOR_brown,
			"TekkRat\n"
			"Martijn de Boer\n"
			"Steffen Pohle\n"
			"Patrick Wilczek\n");

			y = 140;
			menu_create_label (menu, "Sound/Music:", 5, y, 2, COLOR_yellow);
			y += font[2].size.y;
			menu_create_text (menu, "help", 50, y, 53, 10, COLOR_brown,
			"Henrik_Enqvist\n"
			"Cerror\n"
			"Martijn de Boer\n");
			
			y = 240;
			menu_create_label (menu, "Thanks To:", -1, y, 2, COLOR_yellow);
			y += font[2].size.y;
			menu_create_text (menu, "help", -1, y, 53, 10, COLOR_brown,
			"kitutou(coding/fixing), thaphool(tilesets), ob1kenewb(coding/fixing), "
			"TeKkraT(website,gfx), caccola(tilesets), Digital_D(music), "
			"dcdillon(coding), Psycho(music),\nNiklas Sj\xf6sv\xe4rd(music)");
			
		}
		else break;			
		
		if (page > 0) menu_create_button (menu, "Previous Page", 20, 370, 150, 0);
		else if (menuselect == 0)
			menuselect = 2;
		menu_create_button (menu, "Main Menu", -1, 370, 150, 1);
		if (page < HP_max-1) menu_create_button (menu, "Next Page", 350, 370, 150, 2);
		
		menu_focus_id (menu, menuselect);
		menuselect = menu_loop (menu);
		if (menuselect == 0 && page > 0)
			page--;
		if (menuselect == 2 && page < HP_max - 1)
			page++;
		menu_delete (menu);
	}
};
