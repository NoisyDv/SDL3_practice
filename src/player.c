#include "player.h"
#include "level.h"
#include "main.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

// Sprite sheets (stick.png is 128x96 of 32px cells)
static SDL_FRect srcidle[5] = {{0, 0, 32, 32},
                               {32, 0, 32, 32},
                               {64, 0, 32, 32},
                               {96, 0, 32, 32},
                               {0, 32, 32, 32}};
static SDL_FRect srcwalk[6] = {{32, 32, 32, 32},
                               {64, 32, 32, 32},
                               {96, 32, 32, 32},
                               {0, 64, 32, 32},
                               {32, 64, 32, 32},
                               {64, 64, 32, 32}};

Player player = {0};
static SDL_Texture *tex = NULL;
static int idle_frame = 0, walk_frame = 0;
static int idle_count = 0, walk_count = 0;
static bool moving = false;

void load_player(SDL_Renderer *renderer) {
  tex = IMG_LoadTexture(renderer, "asset/stick.png");
  if (tex)
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
  else
    SDL_Log("load_player: IMG_LoadTexture failed: %s (will use rect fallback)",
            SDL_GetError());
  reset_player(60, 400);
}

void reset_player(float sx, float sy) {
  player.x = sx;
  player.y = sy;
  player.vx = 0;
  player.vy = 0;
  player.w = 36;
  player.h = 56;
  player.onGround = false;
  player.flip = false;
  player.hasKey = false;
  player.dead = false;
  player.coyote = 0;
  player.buffer = 0;
  player.portalCool = 0;
  player.gravityDir = 1;
  player.jumpHeld = false;
  moving = false;
}

SDL_FRect player_rect(void) {
  SDL_FRect r = {player.x, player.y, player.w, player.h};
  return r;
}

bool coliderrect(SDL_FRect a, SDL_FRect b) {
  return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h &&
          a.y + a.h > b.y);
}

// Try to push a box horizontally; returns true if box moved the full dx.
static bool push_box(Box *boxes, int nboxes, int idx, float dx) {
  Box *b = &boxes[idx];
  SDL_FRect nb = {b->rect.x + dx, b->rect.y, b->rect.w, b->rect.h};
  // blocked by solids?
  int ns = level_num_solids();
  SDL_FRect *s = level_solids();
  for (int i = 0; i < ns; i++) {
    if (coliderrect(nb, s[i]))
      return false;
  }
  // blocked by other boxes?
  for (int j = 0; j < nboxes; j++) {
    if (j == idx)
      continue;
    if (coliderrect(nb, boxes[j].rect))
      return false;
  }
  // blocked by world bounds
  if (nb.x < 0 || nb.x + nb.w > LEVEL_W)
    return false;
  b->rect.x = nb.x;
  return true;
}

void update_player(float dt) {
  if (player.dead)
    return;
  if (dt <= 0)
    return;
  if (dt > 1.0f / 30.0f)
    dt = 1.0f / 30.0f;

  const bool *k = SDL_GetKeyboardState(NULL);
  float dir = 0;
  if (k[SDL_SCANCODE_A] || k[SDL_SCANCODE_LEFT])
    dir -= 1;
  if (k[SDL_SCANCODE_D] || k[SDL_SCANCODE_RIGHT])
    dir += 1;
  moving = (dir != 0);
  if (dir < 0)
    player.flip = true;
  else if (dir > 0)
    player.flip = false;

  player.vx = dir * MOVE_SPEED;

  bool jumpDown = k[SDL_SCANCODE_SPACE] || k[SDL_SCANCODE_W] ||
                  k[SDL_SCANCODE_UP] || k[SDL_SCANCODE_K];
  if (jumpDown && !player.jumpHeld)
    player.buffer = JUMP_BUFFER; // pressed this frame
  player.jumpHeld = jumpDown;

  if (player.buffer > 0)
    player.buffer -= dt;
  if (player.coyote > 0)
    player.coyote -= dt;
  if (player.portalCool > 0)
    player.portalCool -= dt;

  // gravity
  player.vy += GRAVITY * player.gravityDir * dt;
  if (player.vy > MAX_FALL)
    player.vy = MAX_FALL;
  if (player.vy < -MAX_FALL)
    player.vy = -MAX_FALL;

  // jump (normal or flipped)
  if (player.buffer > 0 && (player.onGround || player.coyote > 0)) {
    player.vy = (player.gravityDir == 1) ? JUMP_VEL : -JUMP_VEL;
    player.onGround = false;
    player.coyote = 0;
    player.buffer = 0;
  }
  // variable jump height: release space early -> cut velocity
  if (!jumpDown) {
    if (player.gravityDir == 1 && player.vy < JUMP_VEL * JUMP_CUT_MULT)
      player.vy = JUMP_VEL * JUMP_CUT_MULT;
    if (player.gravityDir == -1 && player.vy > -JUMP_VEL * JUMP_CUT_MULT)
      player.vy = -JUMP_VEL * JUMP_CUT_MULT;
  }

  bool wasGround = player.onGround;
  player.onGround = false;

  SDL_FRect *solids = level_solids();
  int nsol = level_num_solids();
  int nm = 0, nv = 0, nb = 0;
  Mover *movers = level_movers(&nm);
  Vanish *van = level_vanishes(&nv);
  Box *boxes = level_boxes(&nb);

  // ---- X axis ----
  player.x += player.vx * dt;
  SDL_FRect pr = player_rect();
  // solids + closed gates
  for (int i = 0; i < nsol; i++) {
    if (coliderrect(pr, solids[i])) {
      if (player.vx > 0)
        player.x = solids[i].x - player.w;
      else if (player.vx < 0)
        player.x = solids[i].x + solids[i].w;
      player.vx = 0;
      pr = player_rect();
    }
  }
  // movers are solid on X
  for (int i = 0; i < nm; i++) {
    if (coliderrect(pr, movers[i].rect)) {
      if (player.vx > 0)
        player.x = movers[i].rect.x - player.w;
      else if (player.vx < 0)
        player.x = movers[i].rect.x + movers[i].rect.w;
      player.vx = 0;
      pr = player_rect();
    }
  }
  // active vanish platforms solid on X
  for (int i = 0; i < nv; i++) {
    if (!van[i].active)
      continue;
    if (coliderrect(pr, van[i].rect)) {
      if (player.vx > 0)
        player.x = van[i].rect.x - player.w;
      else if (player.vx < 0)
        player.x = van[i].rect.x + van[i].rect.w;
      player.vx = 0;
      pr = player_rect();
    }
  }
  // boxes: push or block
  for (int i = 0; i < nb; i++) {
    if (coliderrect(pr, boxes[i].rect)) {
      float dx = player.vx * dt;
      if (dx != 0 && push_box(boxes, nb, i, dx)) {
        // pushed successfully, stay overlapping-free: keep player pos
      } else {
        if (player.vx > 0)
          player.x = boxes[i].rect.x - player.w;
        else if (player.vx < 0)
          player.x = boxes[i].rect.x + boxes[i].rect.w;
        player.vx = 0;
        pr = player_rect();
      }
    }
  }
  if (player.x < 0)
    player.x = 0;
  if (player.x + player.w > LEVEL_W)
    player.x = LEVEL_W - player.w;

  // ---- Y axis ----
  float prevBottom = pr.y + pr.h;
  float prevTop = pr.y;
  player.y += player.vy * dt;
  pr = player_rect();

  bool landed = false;
  // solids
  for (int i = 0; i < nsol; i++) {
    if (coliderrect(pr, solids[i])) {
      if (player.vy > 0) {
        player.y = solids[i].y - player.h;
        player.vy = 0;
        landed = (player.gravityDir == 1);
      } else if (player.vy < 0) {
        player.y = solids[i].y + solids[i].h;
        player.vy = 0;
        landed = (player.gravityDir == -1);
      }
      pr = player_rect();
    }
  }
  // movers: land + ride
  for (int i = 0; i < nm; i++) {
    if (coliderrect(pr, movers[i].rect)) {
      if (player.vy > 0) {
        player.y = movers[i].rect.y - player.h;
        player.vy = 0;
        landed = (player.gravityDir == 1);
        player.x += movers[i].dx; // carry
      } else if (player.vy < 0) {
        player.y = movers[i].rect.y + movers[i].rect.h;
        player.vy = 0;
        landed = (player.gravityDir == -1);
        player.x += movers[i].dx;
      }
      pr = player_rect();
    } else if (movers[i].dx != 0 || movers[i].dy != 0) {
      // standing on top without overlap (ride check)
      SDL_FRect feet = {player.x, player.y + player.h + 2, player.w, 4};
      if (player.gravityDir == 1 && coliderrect(feet, movers[i].rect)) {
        player.x += movers[i].dx;
        player.y += movers[i].dy;
        pr = player_rect();
      }
    }
  }
  // vanish (only when active)
  for (int i = 0; i < nv; i++) {
    if (!van[i].active)
      continue;
    if (coliderrect(pr, van[i].rect)) {
      if (player.vy > 0 && prevBottom <= van[i].rect.y + 8) {
        player.y = van[i].rect.y - player.h;
        player.vy = 0;
        landed = (player.gravityDir == 1);
      } else if (player.vy < 0 && prevTop >= van[i].rect.y + van[i].rect.h - 8) {
        player.y = van[i].rect.y + van[i].rect.h;
        player.vy = 0;
        landed = (player.gravityDir == -1);
      } else if (player.vy == 0) {
        // standing
        landed = true;
      }
      pr = player_rect();
    }
  }
  // one-way: only when falling and previously above
  int n1 = level_num_oneways();
  for (int i = 0; i < n1; i++) {
    SDL_FRect o;
    level_is_oneway(i, &o);
    if (player.gravityDir == 1 && player.vy >= 0) {
      if (coliderrect(pr, o) && prevBottom <= o.y + 10) {
        player.y = o.y - player.h;
        player.vy = 0;
        landed = true;
        pr = player_rect();
      }
    } else if (player.gravityDir == -1 && player.vy <= 0) {
      if (coliderrect(pr, o) && prevTop >= o.y + o.h - 10) {
        player.y = o.y + o.h;
        player.vy = 0;
        landed = true;
        pr = player_rect();
      }
    }
  }
  // boxes as landing surface
  for (int i = 0; i < nb; i++) {
    if (coliderrect(pr, boxes[i].rect)) {
      if (player.vy > 0 && prevBottom <= boxes[i].rect.y + 12) {
        player.y = boxes[i].rect.y - player.h;
        player.vy = 0;
        landed = (player.gravityDir == 1);
        pr = player_rect();
      } else if (player.vy < 0 && prevTop >= boxes[i].rect.y + boxes[i].rect.h - 12) {
        player.y = boxes[i].rect.y + boxes[i].rect.h;
        player.vy = 0;
        landed = (player.gravityDir == -1);
        pr = player_rect();
      }
    }
  }

  if (landed) {
    player.onGround = true;
    player.coyote = COYOTE_TIME;
  } else if (wasGround) {
    // just walked off: keep small coyote
    if (player.coyote <= 0)
      player.coyote = COYOTE_TIME;
  }

  // fell out of world
  if (player.y > LEVEL_H + 80 || player.y + player.h < -200)
    player.dead = true;
}

void draw_player(SDL_Renderer *renderer, float camX, float camY) {
  SDL_FRect dst = {player.x - 14 - camX, player.y - 8 - camY, 64, 64};
  if (!tex) {
    SDL_SetRenderDrawColor(renderer, 220, 40, 40, 255);
    SDL_FRect hb = {player.x - camX, player.y - camY, player.w, player.h};
    SDL_RenderFillRect(renderer, &hb);
    return;
  }
  SDL_FRect *src;
  if (!moving) {
    idle_count++;
    if (idle_count >= 15) {
      idle_frame = (idle_frame + 1) % 5;
      // idle has 5 entries but only 4 unique + 1; keep %5 safe
      idle_count = 0;
    }
    if (idle_frame < 0 || idle_frame > 4)
      idle_frame = 0;
    src = &srcidle[idle_frame];
  } else {
    walk_count++;
    if (walk_count >= 8) {
      walk_frame = (walk_frame + 1) % 6;
      walk_count = 0;
    }
    src = &srcwalk[walk_frame];
  }
  SDL_FlipMode fm = SDL_FLIP_NONE;
  if (player.flip)
    fm = SDL_FLIP_HORIZONTAL;
  if (player.gravityDir == -1) {
    // upside down when gravity flipped: combine flips
    fm = (player.flip) ? (SDL_FlipMode)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL)
                       : SDL_FLIP_VERTICAL;
  }
  if (fm == SDL_FLIP_NONE)
    SDL_RenderTexture(renderer, tex, src, &dst);
  else
    SDL_RenderTextureRotated(renderer, tex, src, &dst, 0, NULL, fm);
}

void destroy_player(void) {
  if (tex) {
    SDL_DestroyTexture(tex);
    tex = NULL;
  }
}
