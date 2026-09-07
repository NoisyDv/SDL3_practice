#include <SDL3/SDL_video.h>
#define SDL_MAIN_USE_CALLBACKS
#include "level.h"
#include "main.h"
#include "player.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

static Uint64 lastTick = 0;
static float camX = 0, camY = 0;
static float deadTimer = 0;
static bool won = false;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  (void)appstate;
  (void)argc;
  (void)argv;
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("unable to initialize %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  window = SDL_CreateWindow("Puzzle Jumper - A/D move, SPACE jump", WINDOW_WIDTH,
                            WINDOW_HEIGHT, 0);
  if (!window) {
    SDL_Log("unable to create window %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    SDL_Log("unable to create renderer %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_Log("renderer: %s", SDL_GetRendererName(renderer));

  load_player(renderer);
  init_obstruc(); // loads level 0
  lastTick = SDL_GetTicks();
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  (void)appstate;
  if (event->type == SDL_EVENT_QUIT)
    return SDL_APP_SUCCESS;
  if (event->type == SDL_EVENT_KEY_DOWN) {
    switch (event->key.scancode) {
    case SDL_SCANCODE_R:
      level_respawn();
      deadTimer = 0;
      won = false;
      break;
    case SDL_SCANCODE_N:
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
      if (level_cleared()) {
        int n = level_index() + 1;
        if (n >= NUM_LEVELS) {
          won = true;
        } else {
          level_load(n);
          deadTimer = 0;
        }
      } else if (won) {
        level_load(0);
        won = false;
      }
      break;
    case SDL_SCANCODE_1:
      level_load(0);
      won = false;
      deadTimer = 0;
      break;
    case SDL_SCANCODE_2:
      level_load(1);
      won = false;
      deadTimer = 0;
      break;
    case SDL_SCANCODE_3:
      level_load(2);
      won = false;
      deadTimer = 0;
      break;
    case SDL_SCANCODE_ESCAPE:
      return SDL_APP_SUCCESS;
    default:
      break;
    }
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  (void)appstate;
  (void)result;
  destroy_player();
  destroy_obstruc();
  if (renderer)
    SDL_DestroyRenderer(renderer);
  if (window)
    SDL_DestroyWindow(window);
}

static void update_camera(float dt) {
  (void)dt;
  float target = player.x + player.w / 2 - WINDOW_WIDTH / 2;
  if (target < 0)
    target = 0;
  if (target > LEVEL_W - WINDOW_WIDTH)
    target = LEVEL_W - WINDOW_WIDTH;
  // smooth follow
  camX += (target - camX) * 0.12f;
  camY = 0; // levels fit vertically
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  (void)appstate;
  Uint64 now = SDL_GetTicks();
  float dt = (now - lastTick) / 1000.0f;
  lastTick = now;
  if (dt > 0.05f)
    dt = 0.05f;
  if (dt < 0)
    dt = 0;

  if (!won) {
    level_update(dt);
    update_player(dt);
    update_camera(dt);

    if (player.dead) {
      deadTimer += dt;
      if (deadTimer > 1.0f) {
        level_respawn();
        deadTimer = 0;
      }
    } else {
      deadTimer = 0;
    }
    if (level_cleared()) {
      // auto-advance hint handled via N; if last level, mark won
      if (level_index() >= NUM_LEVELS - 1) {
        // wait for N/Enter, but also show win text
      }
    }
  }

  // draw world + player (level_draw clears screen)
  level_draw(renderer, camX, camY);
  draw_player(renderer, camX, camY);

  // HUD via SDL3 debug text (no TTF needed)
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderDebugText(renderer, 10, 10, level_name());
  SDL_RenderDebugText(renderer, 10, 26, level_hint());
  char buf[128];
  SDL_snprintf(buf, sizeof(buf), "KEY:%s  LV:%d/3  1/2/3 jump  R retry",
               player.hasKey ? "YES" : "NO", level_index() + 1);
  SDL_RenderDebugText(renderer, 10, 42, buf);

  if (player.dead)
    SDL_RenderDebugText(renderer, WINDOW_WIDTH / 2 - 90, WINDOW_HEIGHT / 2,
                        "YOU DIED - respawning...");
  if (level_cleared() && level_index() < NUM_LEVELS - 1)
    SDL_RenderDebugText(renderer, WINDOW_WIDTH / 2 - 130, 80,
                        "DOOR OPEN! Press N for next level");
  if (level_cleared() && level_index() >= NUM_LEVELS - 1 && !won)
    SDL_RenderDebugText(renderer, WINDOW_WIDTH / 2 - 150, 80,
                        "ALL LEVELS CLEAR! Press N to win");
  if (won)
    SDL_RenderDebugText(renderer, WINDOW_WIDTH / 2 - 110, WINDOW_HEIGHT / 2,
                        "YOU WIN! Press N to replay");

  // gravity indicator
  if (player.gravityDir == -1)
    SDL_RenderDebugText(renderer, 10, 58, "GRAVITY: UP ^");

  SDL_RenderPresent(renderer);
  SDL_Delay(1);
  return SDL_APP_CONTINUE;
}
