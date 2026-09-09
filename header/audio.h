#pragma once

// Procedural SFX via SDL3 audio streams (no SDL_mixer needed).
void audio_init(void);
void audio_quit(void);
void audio_jump(void);
void audio_land(bool hard);
void audio_key(void);
void audio_death(void);
void audio_portal(void);
void audio_flip(void);
void audio_win(void);
void audio_dash(void);
void audio_stomp(void);
void audio_shoot(void);
