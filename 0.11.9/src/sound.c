/* $Id: sound.c,v 1.8 2005-07-06 13:11:56 stpohle Exp $ */
/* sound */

#include "sound.h"

_snd snd;


/* display the sound options */
void
snd_options () {
/*    int menuselect = 0;
    _menu menu[] = {
        {1, "Music Play"},
        {1, "Sound Play"},
        {2, "Return To Main Menu"},
        {-1, ""}
    };

    while (menuselect != -1 && bman.state != GS_quit) {
		if (snd.playmusic)
	        sprintf (menu[0].text, "Music Play ON");
		else
			sprintf (menu[0].text, "Music Play OFF");
		
		if (snd.playsound)
	        sprintf (menu[1].text, "Sound Play ON");
		else
			sprintf (menu[1].text, "Sound Play OFF");
		
        menuselect = menu_loop ("Audio Options", menu, menuselect);
        switch (menuselect) {
        case (0):              // Play Music
            snd.playmusic = 1-snd.playmusic;
            break;
        case (1):              // Play Sound
            snd.playsound = 1-snd.playsound;
            break;
		case (2):
			menuselect = -1;
			break;
		}
	} */
};


/* play an soundsample SND_* */
void
snd_play (int samplenr)
{
#if HAVE_SDL_MIXER
    if (samplenr < SND_max && snd.inited && snd.sample[samplenr] != NULL && snd.playsound)
        Mix_PlayChannel (-1, snd.sample[samplenr], 0);
#endif
};


/* start playing music */
void
snd_music_start ()
{
#if HAVE_SDL_MIXER
    if (snd.inited && snd.music != NULL && snd.playmusic)
        Mix_PlayMusic (snd.music, -1);
#endif
};

/* stop playing music */
void
snd_music_stop ()
{
#if HAVE_SDL_MIXER
    if (snd.inited)
        Mix_HaltMusic ();
#endif
};

/* init audio, sdl_mixer */
void
snd_init ()
{
#if HAVE_SDL_MIXER
    if (Mix_OpenAudio (snd.audio_rate, snd.audio_format, snd.audio_channels, 1024) < 0) {
        d_printf ("Couldn't open audio mixer: %s\n", SDL_GetError ());
        snd.inited = 0;
        return;
    }

    Mix_QuerySpec (&snd.audio_rate, &snd.audio_format, &snd.audio_channels);
    d_printf ("Opened audio at %d Hz %d bit %s\n",
              snd.audio_rate, (snd.audio_format & 0xFF), (snd.audio_channels > 1 ? "stereo" : "mono"));

    Mix_Volume (-1, 96);
    Mix_VolumeMusic (48);

    snd.inited = 1;
#endif
    return;
};


void
snd_shutdown () {
#if HAVE_SDL_MIXER
	if (snd.inited) {
		snd.inited = 0;
		Mix_CloseAudio ();
	}
#endif
	return;
};


/* load the audio files */
void
snd_load (char *tilesetname)
{
#if HAVE_SDL_MIXER
    char fullname[LEN_PATHFILENAME];
    char filename[LEN_FILENAME];
	_direntry *destart, *de, *desel;
    int i, max, sel;

    // load samples
    d_printf ("Loading Audioset\n");

    for (i = 0; i < SND_max; i++) {
        switch (i) {
        case (SND_dead):
            sprintf (filename, "dead.wav");
            break;
        case (SND_explode):
            sprintf (filename, "explode.wav");
            break;
        default:
            sprintf (filename, "drop.wav");
            break;
        }

        /* try loading the sample from the tileset or the default */
        sprintf (fullname, "%s/tileset/%s/%s", bman.datapath, tilesetname, filename);
        if ((snd.sample[i] = Mix_LoadWAV (fullname)) == NULL) {
            sprintf (fullname, "%s/tileset/default/%s", bman.datapath, filename);
            if ((snd.sample[i] = Mix_LoadWAV (fullname)) == NULL)
                d_printf ("Couldn't load %s: %s\n", fullname, SDL_GetError ());
        }
    }


	/* random selection of an sound file */	
	sprintf (fullname, "%s/music", bman.datapath);
	desel = destart = s_getdir (fullname);
	
	for (max = 0, de = destart; de != NULL; de = de->next) 
		if (de->name[0] != '.' && (de->flags & DF_file) == DF_file)
			max++;
	
	sel = s_random (max);
	for (max = 0, de = destart; max <= sel && de != NULL; de = de->next)
		if (de->name[0] != '.' && (de->flags & DF_file) == DF_file) {
			desel = de;
			max++;
	}
	
    /* try loading the music from the tileset or the default */
	if (desel != NULL) {
		sprintf (fullname, "%s/music/%s", bman.datapath, desel->name);
		if ((snd.music = Mix_LoadMUS (fullname)) == NULL)
        	d_printf ("Couldn't load %s: %s\n", fullname, SDL_GetError ());
	}

#endif
    return;
};


void
snd_free ()
{
#if HAVE_SDL_MIXER
    int i;

    for (i = 0; i < SND_max; i++)
        if (snd.sample[i] != NULL)
            Mix_FreeChunk (snd.sample[i]);

    if (snd.music != NULL)
        Mix_FreeMusic (snd.music);
#endif
};
