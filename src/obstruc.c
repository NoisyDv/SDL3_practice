#include "main.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_pixels.h>
#include <stdlib.h>
typedef struct {
  int red;
  int green;
  int blue;
} RGB;

float rand_x() { return (float)SDL_rand(WINDOW_HEIGHT); }
float rand_y() { return (float)SDL_rand(WINDOW_WIDTH); }
SDL_FRect *ob_rect;
RGB color[10];
void init_obstruc() {
  ob_rect = malloc(10 * sizeof(SDL_FRect));
  for (int i = 0; i < 10; i++) {
    ob_rect[i].w = 50;
    ob_rect[i].h = 50;
    ob_rect[i].x = rand_x();
    ob_rect[i].y = rand_y();
  }
  for (int j = 0; j < 10; j++) {
    color[j].red = SDL_rand(256);
    color[j].green = SDL_rand(256);
    color[j].blue = SDL_rand(256);
  }
}

void add_obstruc(SDL_Renderer *renderer) {

  for (int i = 0; i < 10; i++) {
    SDL_SetRenderDrawColor(renderer, color[i].red, color[i].green,
                           color[i].blue, SDL_ALPHA_OPAQUE);

    SDL_RenderFillRect(renderer, &ob_rect[i]);
  }
}
void destroy_obstruc() { free(ob_rect); }
