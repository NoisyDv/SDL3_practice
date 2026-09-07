#pragma once
#include <SDL3/SDL.h>
#include <stdbool.h>

typedef struct {
  float x, y;     // top-left in world coords
  float vx, vy;
  float w, h;     // hitbox size
  bool onGround;
  bool flip;      // facing left
  bool hasKey;
  bool dead;
  float coyote;       // seconds left
  float buffer;       // jump buffer seconds left
  float portalCool;   // portal teleport cooldown
  int gravityDir;     // 1 = down, -1 = up (gravity flip puzzle)
  bool jumpHeld;
} Player;

extern Player player;

void load_player(SDL_Renderer *renderer);
void reset_player(float sx, float sy);
void update_player(float dt);
void draw_player(SDL_Renderer *renderer, float camX, float camY);
void destroy_player(void);
SDL_FRect player_rect(void);

// Fixed AABB overlap test (old coliderrect had a typo)
bool coliderrect(SDL_FRect a, SDL_FRect b);
