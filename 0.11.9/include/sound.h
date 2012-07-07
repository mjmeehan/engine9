/* $Id: sound.h,v 1.4 2003-12-24 02:38:15 stpohle Exp $ */
/* include file for the sound */

#ifndef _SOUND_H_
#define _SOUND_H_

#include "bomberclone.h"

#if HAVE_SDL_MIXER
#include <SDL_mixer.h>
#else
#define Mix_Chunk void
#define Mix_Music void
#endif

enum _soundsample {
	SND_dead = 0,
	SND_explode,
	SND_bombdrop,
	
	SND_max
};

struct __snd {
	unsigned char inited;

	Mix_Chunk *sample[SND_max]; // henqvist
	Mix_Music *music; // henqvist

    int audio_rate;
    Uint16 audio_format;
    int audio_channels;
	int playmusic;
	int playsound;
	
} typedef _snd;

extern _snd snd;

void snd_play(int samplenr);
void snd_music_start();
void snd_music_stop();
void snd_init();
void snd_load(char *tilesetname);
void snd_free ();
void snd_options ();
void snd_shutdown ();

#endif
