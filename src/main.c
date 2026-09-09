#include <SDL3/SDL_video.h>
#define SDL_MAIN_USE_CALLBACKS
#include "audio.h"
#include "level.h"
#include "main.h"
#include "particles.h"
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
static float fadeT = 0; // black fade overlay timer (1 -> 0)
static int deaths = 0;
static float playT = 0;
static int lastLevel = -1;
static bool wasDead = false;

static void trigger_fade(void) { fadeT = 0.35f; }

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  (void)appstate;
  (void)argc;
  (void)argv;
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
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
  // Cap to display refresh so anim/physics timing is consistent
  // on 60/144/240Hz (anim is dt-based too, this saves CPU).
  if (!SDL_SetRenderVSync(renderer, 1)) {
    SDL_Log("vsync not enabled: %s (running uncapped)", SDL_GetError());
  }
  SDL_Log("renderer: %s", SDL_GetRendererName(renderer));

  load_player(renderer);
  particles_init();
  audio_init();
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
      trigger_fade();
      break;
    case SDL_SCANCODE_N:
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
      if (level_cleared()) {
        int n = level_index() + 1;
        if (n >= NUM_LEVELS) {
          won = true;
          audio_win();
        } else {
          level_load(n);
          deadTimer = 0;
          audio_portal();
          trigger_fade();
        }
      } else if (won) {
        level_load(0);
        won = false;
        trigger_fade();
      }
      break;
    case SDL_SCANCODE_1:
    case SDL_SCANCODE_2:
    case SDL_SCANCODE_3:
    case SDL_SCANCODE_4:
    case SDL_SCANCODE_5:
    case SDL_SCANCODE_6:
    case SDL_SCANCODE_7:
    case SDL_SCANCODE_8:
    case SDL_SCANCODE_9:
    case SDL_SCANCODE_0: {
      // 1..9 jump to levels 1..9, 0 jumps to level 10
      int idx = (event->key.scancode == SDL_SCANCODE_0)
                    ? NUM_LEVELS - 1
                    : (int)(event->key.scancode - SDL_SCANCODE_1);
      if (idx < 0)
        idx = 0;
      if (idx >= NUM_LEVELS)
        idx = NUM_LEVELS - 1;
      level_load(idx);
      won = false;
      deadTimer = 0;
      trigger_fade();
      break;
    }
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
  audio_quit();
  destroy_player();
  destroy_obstruc();
  if (renderer)
    SDL_DestroyRenderer(renderer);
  if (window)
    SDL_DestroyWindow(window);
}

static void update_camera(float dt) {
  // look-ahead by horizontal velocity + dt-based smoothing (fps independent)
  float look = player.vx * 0.22f;
  if (look > 90)
    look = 90;
  if (look < -90)
    look = -90;
  float target = player.x + player.w / 2 + look - WINDOW_WIDTH / 2;
  if (target < 0)
    target = 0;
  if (target > LEVEL_W - WINDOW_WIDTH)
    target = LEVEL_W - WINDOW_WIDTH;
  float k = dt * 6.0f;
  if (k > 1)
    k = 1;
  camX += (target - camX) * k;
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
    // per-level timer + death counter
    if (level_index() != lastLevel) {
      lastLevel = level_index();
      playT = 0;
    }
    playT += dt;

    level_update(dt);
    update_player(dt);
    particles_update(dt);
    shake_update(dt);
    update_camera(dt);
    if (fadeT > 0)
      fadeT -= dt;

    if (player.dead && !wasDead)
      deaths++;
    wasDead = player.dead;

    if (player.dead) {
      deadTimer += dt;
      if (deadTimer > 1.0f) {
        level_respawn();
        deadTimer = 0;
        trigger_fade();
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

  // apply screen shake to render camera
  float shX = 0, shY = 0;
  shake_offset(&shX, &shY);
  float rcX = camX + shX, rcY = camY + shY;

  // draw world + player + particles (level_draw clears screen)
  level_draw(renderer, rcX, rcY);
  draw_player(renderer, rcX, rcY);
  particles_draw(renderer, rcX, rcY);

  // fade overlay on transitions
  if (fadeT > 0) {
    float a = fadeT / 0.35f;
    if (a > 1)
      a = 1;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)(a * 255));
    SDL_FRect full = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &full);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  }

  // HUD: top bar + stats (SDL3 debug text, no TTF needed)
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);
  SDL_FRect bar = {0, 0, WINDOW_WIDTH, 74};
  SDL_RenderFillRect(renderer, &bar);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderDebugText(renderer, 10, 10, level_name());
  SDL_RenderDebugText(renderer, 10, 26, level_hint());
  char buf[160];
  int mm = (int)playT / 60, ss = (int)playT % 60;
  SDL_snprintf(buf, sizeof(buf), "KEY:%s  LV:%d/10  TIME:%02d:%02d  DEATHS:%d",
               player.hasKey ? "YES" : "NO", level_index() + 1, mm, ss,
               deaths);
  SDL_RenderDebugText(renderer, 10, 42, buf);
  SDL_RenderDebugText(renderer, 10, 58,
                      "S duck SHIFT dash Stomp foes 1-9,0 R N");

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

  // gravity indicator (moved below top bar)
  if (player.gravityDir == -1)
    SDL_RenderDebugText(renderer, 10, 74, "GRAVITY: UP ^");

  SDL_RenderPresent(renderer);
  SDL_Delay(1);
  return SDL_APP_CONTINUE;
}
