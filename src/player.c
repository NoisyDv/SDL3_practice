#include "audio.h"
#include "player.h"
#include "level.h"
#include "main.h"
#include "particles.h"
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
static float idleT = 0, walkT = 0;
static bool moving = false;
static float squashT = 0; // >0 = landing squash pulse

// Time-based animation: same speed on 60/144/240Hz.
// Bigger number = slower. Tune here.
#define IDLE_FRAME_TIME 0.15f // ~6.6 fps idle
#define WALK_FRAME_TIME 0.09f // ~11 fps walk

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
  player.h = STAND_H;
  player.onGround = false;
  player.flip = false;
  player.hasKey = false;
  player.dead = false;
  player.coyote = 0;
  player.buffer = 0;
  player.portalCool = 0;
  player.gravityDir = 1;
  player.jumpHeld = false;
  player.ducking = false;
  player.dashT = 0;
  player.dashCD = 0;
  player.invulnT = 0;
  player.dashDir = 1;
  player.dashHeld = false;
  idle_frame = 0;
  walk_frame = 0;
  idleT = 0;
  walkT = 0;
  squashT = 0;
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

  // ---- duck: hold S/Down -> short hitbox, slow, dodges high shots ----
  bool duckKey = k[SDL_SCANCODE_S] || k[SDL_SCANCODE_DOWN];
  if (duckKey && !player.ducking) {
    player.ducking = true;
    player.y += player.h - DUCK_H;
    player.h = DUCK_H;
  } else if (!duckKey && player.ducking) {
    // try to stand: blocked by a ceiling? stay ducked
    float newY = player.y - (STAND_H - DUCK_H);
    SDL_FRect stand = {player.x, newY, player.w, STAND_H};
    bool blocked = false;
    SDL_FRect *ss = level_solids();
    for (int i = 0; i < level_num_solids() && !blocked; i++)
      if (coliderrect(stand, ss[i]))
        blocked = true;
    int nm0 = 0;
    Mover *mv0 = level_movers(&nm0);
    for (int i = 0; i < nm0 && !blocked; i++)
      if (coliderrect(stand, mv0[i].rect))
        blocked = true;
    if (!blocked) {
      player.y = newY;
      player.h = STAND_H;
      player.ducking = false;
    }
  }

  // ---- dodge-dash: Shift/L edge-trigger, burst + i-frames ----
  bool dashKey = k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT] ||
                 k[SDL_SCANCODE_L];
  bool dashPressed = dashKey && !player.dashHeld;
  player.dashHeld = dashKey;
  if (dashPressed && player.dashCD <= 0 && player.dashT <= 0) {
    player.dashDir = (dir != 0) ? dir : (player.flip ? -1.0f : 1.0f);
    player.dashT = DASH_TIME;
    player.dashCD = DASH_COOLDOWN;
    if (player.invulnT < DASH_INVULN)
      player.invulnT = DASH_INVULN;
    player.vy = 0;
    particles_burst(player.x + player.w / 2, player.y + player.h / 2, 10, 200,
                    120, 120, 220, 255, 0.35f);
    audio_dash();
  }

  float spd = MOVE_SPEED * (player.ducking ? DUCK_SPEED_MULT : 1.0f);
  if (player.dashT > 0)
    player.vx = player.dashDir * DASH_SPEED;
  else
    player.vx = dir * spd;

  // ---- time-based sprite animation (dt, not frame count) ----
  if (moving) {
    walkT += dt;
    if (walkT >= WALK_FRAME_TIME) {
      walkT -= WALK_FRAME_TIME;
      walk_frame = (walk_frame + 1) % 6;
    }
    idleT = 0; // reset so idle resumes cleanly when stopping
  } else {
    idleT += dt;
    if (idleT >= IDLE_FRAME_TIME) {
      idleT -= IDLE_FRAME_TIME;
      idle_frame = (idle_frame + 1) % 5;
    }
    walkT = 0;
    walk_frame = 0; // restart walk cycle on next move for consistency
  }

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
  if (player.dashCD > 0)
    player.dashCD -= dt;
  if (player.invulnT > 0)
    player.invulnT -= dt;
  bool dashing = player.dashT > 0;
  if (player.dashT > 0) {
    player.dashT -= dt;
    // afterimage trail
    particles_burst(player.x + player.w / 2, player.y + player.h / 2, 2, 60,
                    40, 120, 200, 255, 0.3f);
  }

  // gravity (skipped while dashing: dash hovers)
  if (!dashing) {
    player.vy += GRAVITY * player.gravityDir * dt;
    if (player.ducking && !player.onGround)
      player.vy += GRAVITY * player.gravityDir * dt; // fast-fall while ducking
    if (player.vy > MAX_FALL)
      player.vy = MAX_FALL;
    if (player.vy < -MAX_FALL)
      player.vy = -MAX_FALL;
  } else {
    player.vy = 0;
  }

  // jump (normal or flipped); weaker from a duck
  if (player.buffer > 0 && (player.onGround || player.coyote > 0)) {
    float jv = JUMP_VEL * (player.ducking ? DUCK_JUMP_MULT : 1.0f);
    player.vy = (player.gravityDir == 1) ? jv : -jv;
    player.onGround = false;
    player.coyote = 0;
    player.buffer = 0;
    // jump dust at feet
    particles_burst(player.x + player.w / 2, player.y + player.h - 4, 8, 160,
                    220, 200, 200, 210, 0.4f);
    audio_jump();
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
  float fallVy = player.vy; // speed at impact, for landing dust/shake
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

  if (squashT > 0)
    squashT -= dt;

  // ---- enemies: stomp from above kills, side touch hurts ----
  {
    int nen = 0;
    Enemy *ens = level_enemies(&nen);
    for (int i = 0; i < nen; i++) {
      if (!ens[i].alive || player.dead)
        continue;
      SDL_FRect e = ens[i].rect;
      SDL_FRect es = {e.x + 4, e.y + 4, e.w - 8, e.h - 4}; // forgiving
      if (!coliderrect(pr, es))
        continue;
      bool stomp = false;
      if (player.gravityDir == 1 && fallVy > 60 && prevBottom <= e.y + 16)
        stomp = true;
      if (player.gravityDir == -1 && fallVy < -60 &&
          prevTop >= e.y + e.h - 16)
        stomp = true;
      if (stomp) {
        level_kill_enemy(i);
        player.vy = (player.gravityDir == 1 ? JUMP_VEL : -JUMP_VEL) *
                    STOMP_BOUNCE_MULT;
        landed = false;
        player.onGround = false;
        if (player.invulnT < 0.15f)
          player.invulnT = 0.15f;
        squashT = 0.1f;
        particles_burst(e.x + e.w / 2, e.y + e.h / 2, 16, 260, 280, 255, 120,
                        80, 0.5f);
        shake_add(4.0f, 0.12f);
        audio_stomp();
        pr = player_rect();
      } else if (player.invulnT > 0 || player.dashT > 0) {
        // i-frames: dash straight through unharmed
      } else {
        player.dead = true;
        particles_burst(pr.x + pr.w / 2, pr.y + pr.h / 2, 22, 300, 320, 220,
                        50, 50, 0.7f);
        shake_add(8.0f, 0.25f);
        audio_death();
      }
    }
  }

  // ---- fireballs: duck under high ones, jump low ones, dash through ----
  {
    int nsh = 0;
    Shot *shs = level_shots(&nsh);
    for (int i = 0; i < nsh; i++) {
      if (!shs[i].alive || player.dead)
        continue;
      SDL_FRect s = shs[i].rect;
      s.x += 3;
      s.y += 3;
      s.w -= 6;
      s.h -= 6;
      if (!coliderrect(pr, s))
        continue;
      if (player.invulnT > 0 || player.dashT > 0) {
        // dashing through a fireball destroys it with a spark
        shs[i].alive = false;
        particles_burst(s.x + s.w / 2, s.y + s.h / 2, 8, 200, 160, 255, 200,
                        100, 0.35f);
        audio_stomp();
      } else {
        player.dead = true;
        particles_burst(pr.x + pr.w / 2, pr.y + pr.h / 2, 22, 300, 320, 255,
                        120, 40, 0.7f);
        shake_add(8.0f, 0.25f);
        audio_death();
      }
    }
  }

  if (landed) {
    player.onGround = true;
    player.coyote = COYOTE_TIME;
    if (!wasGround) {
      // just landed: squash + dust, harder impact = more dust + tiny shake
      squashT = 0.14f;
      float impact = fallVy < 0 ? -fallVy : fallVy;
      int n = impact > 600 ? 14 : 7;
      particles_burst(player.x + player.w / 2, player.y + player.h - 2, n,
                      200, 160, 190, 190, 200, 0.45f);
      audio_land(impact > 600);
      if (impact > 700)
        shake_add(4.0f, 0.12f);
    }
  } else if (wasGround) {
    // just walked off: keep small coyote
    if (player.coyote <= 0)
      player.coyote = COYOTE_TIME;
  }

  // fell out of world
  if (!player.dead && (player.y > LEVEL_H + 80 || player.y + player.h < -200)) {
    player.dead = true;
    particles_burst(player.x + player.w / 2, LEVEL_H - 40, 16, 260, 300, 220,
                    60, 60, 0.6f);
    shake_add(6.0f, 0.2f);
    audio_death();
  }
}

void draw_player(SDL_Renderer *renderer, float camX, float camY) {
  // squash & stretch: stretch in air by |vy|, squash pulse on landing
  float sx = 1.0f, sy = 1.0f;
  if (squashT > 0) {
    float k = squashT / 0.14f; // 1 -> 0
    sx = 1.0f + 0.18f * k;
    sy = 1.0f - 0.18f * k;
  } else if (!player.onGround) {
    float v = player.vy < 0 ? -player.vy : player.vy;
    float k = v / 900.0f;
    if (k > 1)
      k = 1;
    sx = 1.0f - 0.10f * k;
    sy = 1.0f + 0.14f * k;
  }
  if (player.ducking) {
    // crouch pose: low and wide (also matches the short hitbox)
    sx *= 1.15f;
    sy *= 0.62f;
  }
  if (player.dashT > 0) {
    // horizontal speed streak
    sx = 1.3f;
    sy = 0.8f;
  }
  float dw = 64 * sx, dh = 64 * sy;
  // keep feet planted: grow/shrink around bottom-center
  float cx = player.x + player.w / 2;
  float feetY = player.y + player.h + 8;
  SDL_FRect dst = {cx - dw / 2 - camX, feetY - dh - camY, dw, dh};
  if (!tex) {
    SDL_SetRenderDrawColor(renderer, 220, 40, 40, 255);
    SDL_FRect hb = {player.x - camX, player.y - camY, player.w, player.h};
    SDL_RenderFillRect(renderer, &hb);
    return;
  }
  SDL_FRect *src;
  if (!moving) {
    if (idle_frame < 0 || idle_frame > 4)
      idle_frame = 0;
    src = &srcidle[idle_frame];
  } else {
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
  // i-frame blink
  if (player.invulnT > 0 && ((SDL_GetTicks() / 60) % 2 == 0))
    SDL_SetTextureAlphaMod(tex, 90);
  else
    SDL_SetTextureAlphaMod(tex, 255);
  // spirit glow: soft multi-layer aura so the black silhouette reads on dark
  // (a white color-mod halo can't work: tint multiplies, black x tint = black)
  // tint follows the level neon: cyan / mint / violet
  {
    Uint8 ar = 170, ag = 210, ab = 255;
    int li = level_index() % 3;
    if (li == 1) {
      ar = 170;
      ag = 255;
      ab = 210;
    } else if (li == 2) {
      ar = 200;
      ag = 170;
      ab = 255;
    }
    SDL_BlendMode oldBM;
    SDL_GetRenderDrawBlendMode(renderer, &oldBM);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    float gx = cx - camX, gy = feetY - dh / 2 - camY;
    // large-to-small: alpha accumulates toward the sprite = soft falloff.
    // kept faint: the tight layer hugs the silhouette, wide ones just lift
    // the backdrop so the black sprite never sits on pure black.
    const float sc[] = {1.5f, 1.25f, 1.08f};
    const Uint8 al[] = {8, 16, 40};
    for (int i = 0; i < 3; i++) {
      SDL_SetRenderDrawColor(renderer, ar, ag, ab, al[i]);
      SDL_FRect g = {gx - dw * sc[i] / 2.0f, gy - dh * sc[i] / 2.0f,
                     dw * sc[i], dh * sc[i]};
      SDL_RenderFillRect(renderer, &g);
    }
    SDL_SetRenderDrawBlendMode(renderer, oldBM);
  }
  if (fm == SDL_FLIP_NONE)
    SDL_RenderTexture(renderer, tex, src, &dst);
  else
    SDL_RenderTextureRotated(renderer, tex, src, &dst, 0, NULL, fm);
  SDL_SetTextureAlphaMod(tex, 255);
  // dash cooldown pip above head (shows when recharging)
  if (player.dashCD > 0 && !player.dead) {
    float w = 30.0f * (1.0f - player.dashCD / DASH_COOLDOWN);
    SDL_SetRenderDrawColor(renderer, 120, 200, 255, 255);
    SDL_FRect pip = {cx - 15 - camX, feetY - dh - 12 - camY, w, 3};
    SDL_RenderFillRect(renderer, &pip);
  }
}

void destroy_player(void) {
  if (tex) {
    SDL_DestroyTexture(tex);
    tex = NULL;
  }
}
