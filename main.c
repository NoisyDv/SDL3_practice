#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <SDL3/SDL_main.h>
    

int velocity=6;
SDL_FRect rect;
SDL_Window *window;
SDL_Renderer *renderer;
void movement(void);
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv){
    if(!SDL_Init(SDL_INIT_VIDEO)){
      SDL_Log("unable to initialize %s",SDL_GetError());
    }
    window=SDL_CreateWindow("hahah",600,600,SDL_WINDOW_RESIZABLE);
    if(!window){
      SDL_Log("unable to create window %s",SDL_GetError());
    }
    renderer=SDL_CreateRenderer(window,NULL);
    if(!renderer){
      SDL_Log("unable to create rederer %s",SDL_GetError());
    }
    const float red=SDL_randf();
    const float green=SDL_randf();
    const float blue=SDL_randf();
    const float alpha=SDL_randf();
    rect.h = 50;
    rect.w = 50;
    rect.x = 200;
    rect.y = 200;
 
    return SDL_APP_CONTINUE;
    

}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    if(event->type==SDL_EVENT_QUIT){
        return SDL_APP_SUCCESS;

    }
  
   return SDL_APP_CONTINUE;

}

void SDL_AppQuit(void *appstate, SDL_AppResult result){
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);


}
SDL_AppResult SDL_AppIterate(void *appstate){
  movement();
  SDL_Delay(16);
    SDL_SetRenderDrawColorFloat(renderer,0.1,0.1,0.1,SDL_ALPHA_OPAQUE);

    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer,255,0,0,SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer,&rect);
    SDL_RenderPresent(renderer);


return SDL_APP_CONTINUE;

}
    
void movement(){
 const bool *keystate = SDL_GetKeyboardState(NULL);
  if(keystate[SDL_SCANCODE_D]){
    rect.x+=velocity;
  }
  if(keystate[SDL_SCANCODE_A]){
    rect.x-=velocity;
  }
  if(keystate[SDL_SCANCODE_W]){
    rect.y-=velocity;
  }
  if(keystate[SDL_SCANCODE_S]){
    rect.y+=velocity;
  }
 
}