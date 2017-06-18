#ifndef _SOUND_H_
#define _SOUND_H_

#include "common.h"
#include "nvp.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sound_s {
	struct sound_s *prev;
	struct sound_s *next;
	char* file;
	char* token;
	ALenum format;
	ALsizei freq;
	ALuint id;
	char* data;
	int d_len;
} sound_t;

typedef struct sound_instance_s {
	struct sound_instance_s *prev;
	struct sound_instance_s *next;
	ALuint id;
	float volume;
	float fade;
} sound_instance_t;

/* defined in sound.c */
int sound_init(void);
void sound_exit(void);
void sound_step(float dtime, v3_t *pos, v3_t *at, v3_t *up);
int sound_load_effect(char* file, char* token);
int sound_load_music(char* file, char* token);
void sound_free_effect(char* token);
void sound_free_music(char* token);
uint32_t sound_play_effect(char* token, float volume, v3_t *pos);
uint32_t sound_play_music(char* token, float volume);
void sound_stop_effects(int fade);
void sound_stop_music(int fade);
void sound_stop_single(uint32_t id);
void sound_stop(int fade);
int sound_master_setter(char* value);
int sound_effects_setter(char* value);
int sound_music_setter(char* value);

#ifdef _VL_SOUND_EXPOSE_INTERNAL
/* defined in sound.c */
void sound_process(float dtime);
float sound_volume_master(float v);
float sound_volume_effects(float v);
float sound_volume_music(float v);

/* defined in sound_ogg.c */
int sound_is_ogg(file_t *f);
int sound_load_ogg(file_t *f, sound_t *e);

/* defined in sound_wav.c */
int sound_is_wav(file_t *f);
int sound_load_wav(file_t *f, sound_t *e);
#endif

#ifdef __cplusplus
}
#endif

#endif
