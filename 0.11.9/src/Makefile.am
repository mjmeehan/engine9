
INCLUDES = -I../include

bin_PROGRAMS = bomberclone

bomberclone_SOURCES = \
	main.c\
	bomb.c\
	debug.c\
	packets.c\
	font.c\
	gfx.c\
	gfxengine.c\
	gfxpixelimage.c\
	player.c\
	game.c\
	netmenu.c\
	network.c\
	netsrvlist.c\
	udp.c\
	menu.c\
	menubools.c\
	menubuttons.c\
	menuentrys.c\
	menulabels.c\
	menulists.c\
	menuimages.c\
	map.c\
	mapmenu.c\
	tileset.c\
	field.c\
	sysfunc.c\
	configuration.c\
	pkgcache.c\
	chat.c \
	multiwait.c \
	keybinput.c \
	single.c \
	special.c \
	sound.c\
	ogcache-client.c\
	flyingitems.c\
	help.c\
	playerinput.c\
	keyb.c\
	playermenu.c\
	broadcast.c

if MINGW32
bomberclone_LDADD = bc-res.o


bc-res.o: bc-res.rc
	ln -sf ../data/pixmaps/bomberclone.ico ./
	windres -i bc-res.rc -o bc-res.o
	rm bomberclone.ico
endif
