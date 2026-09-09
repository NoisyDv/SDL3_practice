#pragma once
#include <SDL3/SDL.h>

void particles_init(void);
void particles_burst(float x, float y, int count, float spread, float up,
                     Uint8 r, Uint8 g, Uint8 b, float life);
void particles_update(float dt);
void particles_draw(SDL_Renderer *renderer, float camX, float camY);

// Tiny screen-shake helper (owned here so player+level can trigger it)
void shake_add(float mag, float time);
void shake_update(float dt);
void shake_offset(float *ox, float *oy);
