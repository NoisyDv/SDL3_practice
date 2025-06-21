#include "main.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_pixels.h>

float rand_x() { return (float)SDL_rand(WINDOW_HEIGHT); }
float rand_y() { return (float)SDL_rand(WINDOW_WIDTH); }

SDL_FRect ob_rect;
void add_obstruc(SDL_Renderer *renderer) {
  ob_rect.x = rand_x();
  ob_rect.y = rand_y();
  ob_rect.w = 100;
  ob_rect.h = 100;

  SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);

  SDL_RenderFillRect(renderer, &ob_rect);
}
