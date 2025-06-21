#include "player.h"
#include "main.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

int velocity = 2;
bool move = false;
bool flip = false;
// load texture
SDL_FRect srcidle[5] = {{0, 0, 32, 32},
                        {32, 0, 32, 32},
                        {64, 0, 32, 32},
                        {96, 0, 32, 32},
                        {0, 32, 32, 32}};
SDL_FRect srcwalk[6] = {{32, 32, 32, 32}, {64, 32, 32, 32}, {96, 32, 32, 32},
                        {0, 64, 32, 32},  {32, 64, 32, 32}, {64, 64, 32, 32}};

// define texture size and position
SDL_FRect dstplayer = {10, 10, 64, 64};
int idle_frame;
int walk_frame;
int framedelay_idle = 15;
int framedelay_walk = 8;
int idle_framecount = 0;
int walk_framecount = 0;
SDL_Texture *player = NULL;

// for  initial load player image
void load_player(SDL_Renderer *renderer) {

  player = IMG_LoadTexture(renderer, "asset/stick.png");
  SDL_SetTextureScaleMode(player, SDL_SCALEMODE_NEAREST);
}

// for idle animation
void idle(SDL_Renderer *renderer) {
  idle_framecount++;
  if (idle_framecount >= framedelay_idle) {
    idle_frame = (idle_frame + 1) % 4;
    idle_framecount = 0;
  }
  SDL_RenderTexture(renderer, player, &srcidle[idle_frame], &dstplayer);
}

// for work animation
void walk(SDL_Renderer *renderer) {
  walk_framecount++;
  if (walk_framecount >= framedelay_walk) {
    walk_frame = (walk_frame + 1) % 6;
    walk_framecount = 0;
  }
  if (flip) {
    SDL_RenderTextureRotated(renderer, player, &srcwalk[walk_frame], &dstplayer,
                             0, NULL, SDL_FLIP_HORIZONTAL);
  } else {
    SDL_RenderTexture(renderer, player, &srcwalk[walk_frame], &dstplayer);
  }
}

// for draw player on renderer
void draw_player(SDL_Renderer *renderer, Uint8 red, Uint8 green, Uint8 blue) {
  if (!move) {
    idle(renderer);
  } else {
    walk(renderer);
  }
}

// keyboard movement for player
void movement() {
  move = false;
  const bool *keystate = SDL_GetKeyboardState(NULL);
  if (keystate[SDL_SCANCODE_D]) {
    dstplayer.x += velocity;
    move = true;
    flip = false;
  }
  if (keystate[SDL_SCANCODE_A]) {
    dstplayer.x -= velocity;
    move = true;
    flip = true;
  }
  if (keystate[SDL_SCANCODE_W]) {
    dstplayer.y -= velocity;
    move = true;
  }
  if (keystate[SDL_SCANCODE_S]) {
    dstplayer.y += velocity;
    move = true;
  }

  // check limit of window
  if (dstplayer.x >= WINDOW_HEIGHT - dstplayer.w) {
    dstplayer.x = WINDOW_HEIGHT - dstplayer.w;
  }
  if (dstplayer.x < 0) {
    dstplayer.x = 0;
  }
  if (dstplayer.y >= WINDOW_WIDTH - dstplayer.h) {
    dstplayer.y = WINDOW_WIDTH - dstplayer.h;
  }
  if (dstplayer.y < 0) {
    dstplayer.y = 0;
  }
}
// collision check function
bool coliderrect(SDL_FRect rect1, SDL_FRect rect2) {
  if (rect1.x < rect2.x + rect2.w && rect1.x + rect1.w > rect2.x &&
      rect1.y < rect2.y + rect2.h && rect1.y + rect1.y > rect2.y) {
    SDL_Log("coliderect");
    return true;
  }
  return false;
}
