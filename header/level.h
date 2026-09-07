#pragma once
#include <SDL3/SDL.h>
#include <stdbool.h>

#define MAX_SOLIDS 80
#define MAX_ONEWAY 24
#define MAX_MOVERS 8
#define MAX_VANISH 8
#define MAX_SPIKES 16
#define MAX_BOXES 4
#define MAX_PORTALS 2
#define MAX_FLIPZ 2

typedef struct {
  SDL_FRect rect;
  float ax_min, ax_max; // patrol range on one axis
  float speed;
  int axis; // 0 = x, 1 = y
  float t;  // phase
  float dx, dy; // per-frame delta (computed in update)
} Mover;

typedef struct {
  SDL_FRect rect;
  bool active;     // false = fallen/disappeared
  float standT;    // time player stood on it
  float respawnT;  // countdown to reappear
} Vanish;

typedef struct {
  SDL_FRect rect; // hitbox in world coords
  float vx, vy;
  bool onGround;
} Box;

void init_obstruc(void);   // kept for compat: loads level 0
void destroy_obstruc(void); // frees nothing dynamic now

// New level API
#define NUM_LEVELS 3
void level_load(int idx);
void level_update(float dt);
void level_draw(SDL_Renderer *renderer, float camX, float camY);
bool level_cleared(void);      // player touched exit with key (or exit if no key)
bool level_player_dead(void);
void level_respawn(void);      // reset player+boxes to spawn, keep level layout
int level_index(void);
const char *level_name(void);
const char *level_hint(void);

// Collision helpers used by player + boxes
int level_num_solids(void);
SDL_FRect *level_solids(void); // includes closed gates
bool level_is_oneway(int i, SDL_FRect *out);
int level_num_oneways(void);
Mover *level_movers(int *n);
Vanish *level_vanishes(int *n);
Box *level_boxes(int *n);
SDL_FRect level_spawn(void);
