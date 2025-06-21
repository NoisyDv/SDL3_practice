#pragma once
#include <SDL3/SDL.h>
void load_player(SDL_Renderer *renderer);
void movement();
void draw_player(SDL_Renderer *renderer, Uint8 red, Uint8 green, Uint8 blue);
bool coliderrect(SDL_FRect rect1, SDL_FRect rect2);
void destroy_player();
