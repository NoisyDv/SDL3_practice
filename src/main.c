#define SDL_MAIN_USE_CALLBACKS
#include "player.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>


SDL_FRect rect0={100,100,100,100};
SDL_Window *window;
SDL_Renderer *renderer;


//Global initialize 
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {

  //create window and renderer
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("unable to initialize %s", SDL_GetError());
  }
  window = SDL_CreateWindow("hahah", 800, 800, NULL);
  if (!window) {
    SDL_Log("unable to create window %s", SDL_GetError());
  }
  renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    SDL_Log("unable to create rederer %s", SDL_GetError());
  }
   load_player(renderer);   
  return SDL_APP_CONTINUE;
}

//Global Event function
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }

  return SDL_APP_CONTINUE;
}
//
//Game quit function
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  //delete window and renderer from memory
  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
}

//Game loop function 
SDL_AppResult SDL_AppIterate(void *appstate) {
  SDL_Delay(8);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  movement();
  draw_player(renderer, 255, 0, 0);

  SDL_RenderPresent(renderer);

  return SDL_APP_CONTINUE;
}
