#include "level.h"
#include "main.h"
#include "player.h"
#include <SDL3/SDL.h>
#include <string.h>

// ---------- world storage ----------
static int cur = 0;
static SDL_FRect solids[MAX_SOLIDS];
static int nsol = 0;
static SDL_FRect oneways[MAX_ONEWAY];
static int noneway = 0;
static Mover movers[MAX_MOVERS];
static int nmover = 0;
static Vanish vanishes[MAX_VANISH];
static int nvan = 0;
static SDL_FRect spikes[MAX_SPIKES];
static int nspike = 0;
static Box boxes[MAX_BOXES];
static int nbox = 0;

static SDL_FRect keyRect;
static bool keyExists = false, keyTaken = false;
static SDL_FRect exitRect;
static SDL_FRect buttonRect;
static bool buttonExists = false, buttonPressed = false;
static SDL_FRect gateRect;
static bool gateExists = false;
static SDL_FRect portals[MAX_PORTALS];
static bool portalExists = false;
static SDL_FRect flipZones[MAX_FLIPZ];
static int nflip = 0;
static float flipCool = 0;
static SDL_FRect spawnRect;

static bool cleared = false;

static void add_solid(float x, float y, float w, float h) {
  if (nsol < MAX_SOLIDS)
    solids[nsol++] = (SDL_FRect){x, y, w, h};
}
static void add_oneway(float x, float y, float w) {
  if (noneway < MAX_ONEWAY)
    oneways[noneway++] = (SDL_FRect){x, y, w, 16};
}
static void add_mover(float x, float y, float w, float h, int axis, float mn,
                      float mx, float speed) {
  if (nmover >= MAX_MOVERS)
    return;
  movers[nmover].rect = (SDL_FRect){x, y, w, h};
  movers[nmover].axis = axis;
  movers[nmover].ax_min = mn;
  movers[nmover].ax_max = mx;
  movers[nmover].speed = speed;
  movers[nmover].t = 0;
  movers[nmover].dx = 0;
  movers[nmover].dy = 0;
  nmover++;
}
static void add_vanish(float x, float y, float w) {
  if (nvan >= MAX_VANISH)
    return;
  vanishes[nvan].rect = (SDL_FRect){x, y, w, 18};
  vanishes[nvan].active = true;
  vanishes[nvan].standT = 0;
  vanishes[nvan].respawnT = 0;
  nvan++;
}
static void add_spike(float x, float y, float w, float h) {
  if (nspike < MAX_SPIKES)
    spikes[nspike++] = (SDL_FRect){x, y, w, h};
}
static void add_box(float x, float y) {
  if (nbox >= MAX_BOXES)
    return;
  boxes[nbox].rect = (SDL_FRect){x, y, 44, 44};
  boxes[nbox].vx = 0;
  boxes[nbox].vy = 0;
  boxes[nbox].onGround = false;
  nbox++;
}

static void build_level_0(void) {
  // Tutorial: move/jump + key + door. One gap, one oneway.
  spawnRect = (SDL_FRect){60, 400, 36, 56};
  add_solid(0, 560, 700, 80);     // ground A
  add_solid(780, 560, 720, 80);   // ground B
  add_solid(1500, 560, 900, 80);  // ground C
  add_solid(-40, -40, 40, 760);   // left wall
  add_solid(0, -40, 2440, 40);    // ceiling
  add_solid(850, 480, 120, 80);   // step block
  add_oneway(300, 440, 170);      // practice oneway jump
  add_oneway(1100, 440, 170);
  keyRect = (SDL_FRect){1240, 500, 28, 28};
  keyExists = true;
  exitRect = (SDL_FRect){2280, 440, 50, 120};
}

static void build_level_1(void) {
  // Boxes + button/gate + mover + vanish + key.
  spawnRect = (SDL_FRect){60, 400, 36, 56};
  add_solid(0, 560, 1400, 80);    // ground A (up to pit)
  add_solid(1700, 560, 700, 80);  // ground B (after pit)
  add_solid(-40, -40, 40, 760);
  add_solid(0, -40, 2440, 40);
  // gate wall blocking path (solid only while button not pressed)
  gateRect = (SDL_FRect){1100, 400, 30, 160};
  gateExists = true;
  buttonRect = (SDL_FRect){400, 540, 56, 20};
  buttonExists = true;
  add_box(700, 500); // push this onto the button to hold gate open
  // spike pit with mover bridge
  add_spike(1400, 600, 300, 24);
  add_mover(1380, 470, 130, 20, 0, 1380, 1590, 110);
  // vanish steps up to high ledge with key
  add_solid(2000, 360, 220, 20); // high ledge
  add_vanish(1760, 470, 95);
  add_vanish(1880, 410, 95);
  keyRect = (SDL_FRect){2080, 310, 28, 28};
  keyExists = true;
  exitRect = (SDL_FRect){2290, 440, 50, 120};
  add_oneway(950, 440, 120);
}

static void build_level_2(void) {
  // Portals + gravity flip + precision over spikes.
  spawnRect = (SDL_FRect){60, 400, 36, 56};
  add_solid(0, 560, 2400, 80);
  add_solid(-40, -40, 40, 760);
  add_solid(0, -40, 2440, 40);
  add_solid(2360, 0, 40, 640); // right wall
  // tall wall forces portal use
  add_solid(800, 180, 30, 380);
  portals[0] = (SDL_FRect){640, 450, 44, 110};
  portals[1] = (SDL_FRect){890, 450, 44, 110};
  portalExists = true;
  // gravity flip corridor: ceiling walk
  add_solid(1200, 60, 800, 24); // ceiling path
  flipZones[0] = (SDL_FRect){1200, 380, 110, 180};
  flipZones[1] = (SDL_FRect){1890, 80, 110, 180};
  nflip = 2;
  // spike strip on ground -> use oneways above
  add_spike(1250, 540, 600, 20);
  add_oneway(1280, 440, 130);
  add_oneway(1460, 360, 130);
  add_oneway(1640, 440, 130);
  add_vanish(1830, 400, 95);
  keyRect = (SDL_FRect){2080, 500, 28, 28};
  keyExists = true;
  exitRect = (SDL_FRect){2280, 440, 50, 120};
}

void level_load(int idx) {
  cur = idx;
  nsol = noneway = nmover = nvan = nspike = nbox = 0;
  keyExists = keyTaken = false;
  buttonExists = buttonPressed = false;
  gateExists = false;
  portalExists = false;
  nflip = 0;
  flipCool = 0;
  cleared = false;
  if (idx == 0)
    build_level_0();
  else if (idx == 1)
    build_level_1();
  else
    build_level_2();
  reset_player(spawnRect.x, spawnRect.y);
}

void init_obstruc(void) { level_load(0); }
void destroy_obstruc(void) {}

int level_index(void) { return cur; }
const char *level_name(void) {
  if (cur == 0)
    return "Level 1/3: First Jumps";
  if (cur == 1)
    return "Level 2/3: Boxes & Buttons";
  return "Level 3/3: Portals & Gravity";
}
const char *level_hint(void) {
  if (cur == 0)
    return "A/D move  SPACE jump  Get KEY, reach DOOR  (R restart)";
  if (cur == 1)
    return "Push BOX onto BUTTON to open GATE  Ride platform, beware gaps";
  return "Enter PORTAL  Touch ORANGE zone to flip gravity  N next when open";
}

bool level_cleared(void) { return cleared; }
bool level_player_dead(void) { return player.dead; }
void level_respawn(void) {
  // reset boxes to initial level layout (simplest: reload level but keep key? no - full reload)
  bool hadKey = false;
  (void)hadKey;
  int idx = cur;
  level_load(idx);
}

int level_num_solids(void) {
  // gate counts as solid only while closed
  if (gateExists && !buttonPressed)
    return nsol + 1;
  return nsol;
}
SDL_FRect *level_solids(void) {
  static SDL_FRect combined[MAX_SOLIDS + 1];
  for (int i = 0; i < nsol; i++)
    combined[i] = solids[i];
  if (gateExists && !buttonPressed)
    combined[nsol] = gateRect;
  return combined;
}
bool level_is_oneway(int i, SDL_FRect *out) {
  if (i < 0 || i >= noneway)
    return false;
  *out = oneways[i];
  return true;
}
int level_num_oneways(void) { return noneway; }
Mover *level_movers(int *n) {
  *n = nmover;
  return movers;
}
Vanish *level_vanishes(int *n) {
  *n = nvan;
  return vanishes;
}
Box *level_boxes(int *n) {
  *n = nbox;
  return boxes;
}
SDL_FRect level_spawn(void) { return spawnRect; }

// ---- per-frame world update ----
void level_update(float dt) {
  if (dt > 1.0f / 30.0f)
    dt = 1.0f / 30.0f;

  // movers patrol (sine)
  for (int i = 0; i < nmover; i++) {
    Mover *m = &movers[i];
    float prevx = m->rect.x, prevy = m->rect.y;
    m->t += dt * m->speed / 120.0f;
    float range = m->ax_max - m->ax_min;
    float ph = (m->t - (int)m->t); // 0..1 sawtooth -> pingpong
    // use triangle wave for back-and-forth
    float tri = ph < 0.5f ? ph * 2 : 2 - ph * 2;
    if (m->axis == 0)
      m->rect.x = m->ax_min + tri * range;
    else
      m->rect.y = m->ax_min + tri * range;
    m->dx = m->rect.x - prevx;
    m->dy = m->rect.y - prevy;
  }

  // vanish logic: if player stands on it, count down then disappear
  SDL_FRect pr = player_rect();
  for (int i = 0; i < nvan; i++) {
    Vanish *v = &vanishes[i];
    if (!v->active) {
      v->respawnT -= dt;
      if (v->respawnT <= 0) {
        v->active = true;
        v->standT = 0;
      }
      continue;
    }
    SDL_FRect feet = {pr.x, pr.y + pr.h - 4, pr.w, 8};
    if (coliderrect(feet, v->rect) && player.vy >= 0) {
      v->standT += dt;
      if (v->standT > 0.7f) {
        v->active = false;
        v->respawnT = 2.2f;
      }
    } else {
      v->standT = 0;
    }
  }

  // boxes: gravity + collide with solids (Y), block on X
  for (int i = 0; i < nbox; i++) {
    Box *b = &boxes[i];
    b->vy += GRAVITY * dt;
    if (b->vy > MAX_FALL)
      b->vy = MAX_FALL;
    b->rect.y += b->vy * dt;
    b->onGround = false;
    for (int s = 0; s < nsol; s++) {
      if (coliderrect(b->rect, solids[s])) {
        if (b->vy > 0) {
          b->rect.y = solids[s].y - b->rect.h;
          b->vy = 0;
          b->onGround = true;
        } else if (b->vy < 0) {
          b->rect.y = solids[s].y + solids[s].h;
          b->vy = 0;
        }
      }
    }
    if (gateExists && !buttonPressed && coliderrect(b->rect, gateRect)) {
      if (b->vy > 0) {
        b->rect.y = gateRect.y - b->rect.h;
        b->vy = 0;
        b->onGround = true;
      }
    }
    // box vs box vertical separate (simple)
    for (int j = 0; j < nbox; j++) {
      if (j == i)
        continue;
      if (coliderrect(b->rect, boxes[j].rect) && b->vy > 0 &&
          b->rect.y + b->rect.h - boxes[j].rect.y < 20) {
        b->rect.y = boxes[j].rect.y - b->rect.h;
        b->vy = 0;
        b->onGround = true;
      }
    }
    if (b->rect.y > LEVEL_H + 100) { // fell out: respawn at spawn x
      b->rect.x = 700 + i * 60;
      b->rect.y = 400;
      b->vy = 0;
    }
  }

  // button: pressed if player or any box overlaps
  if (buttonExists) {
    bool p = coliderrect(pr, buttonRect);
    if (!p) {
      for (int i = 0; i < nbox; i++) {
        if (coliderrect(boxes[i].rect, buttonRect)) {
          p = true;
          break;
        }
      }
    }
    buttonPressed = p;
  }

  if (flipCool > 0)
    flipCool -= dt;
  // gravity flip zones
  for (int i = 0; i < nflip; i++) {
    if (flipCool <= 0 && coliderrect(pr, flipZones[i])) {
      player.gravityDir *= -1;
      player.vy = 0;
      player.onGround = false;
      flipCool = 0.8f;
      SDL_Log("gravity flipped: %d", player.gravityDir);
    }
  }

  // portals
  if (portalExists && player.portalCool <= 0) {
    if (coliderrect(pr, portals[0])) {
      player.x = portals[1].x + portals[1].w + 4;
      player.y = portals[1].y + portals[1].h - player.h - 8;
      player.portalCool = 0.7f;
    } else if (coliderrect(pr, portals[1])) {
      player.x = portals[0].x - player.w - 4;
      player.y = portals[0].y + portals[0].h - player.h - 8;
      player.portalCool = 0.7f;
    }
  }

  // key pickup
  if (keyExists && !keyTaken && coliderrect(pr, keyRect)) {
    keyTaken = true;
    player.hasKey = true;
    SDL_Log("key collected");
  }

  // spikes kill (forgiving shrink)
  for (int i = 0; i < nspike; i++) {
    SDL_FRect s = spikes[i];
    s.x += 6;
    s.w -= 12;
    if (s.w < 4)
      continue;
    if (coliderrect(pr, s))
      player.dead = true;
  }

  // exit / door
  if (coliderrect(pr, exitRect)) {
    if (!keyExists || keyTaken)
      cleared = true;
  }
}

// ---- drawing ----
static void draw_world_rect(SDL_Renderer *r, SDL_FRect w, float cx, float cy,
                            Uint8 rr, Uint8 g, Uint8 b) {
  SDL_FRect s = {w.x - cx, w.y - cy, w.w, w.h};
  SDL_SetRenderDrawColor(r, rr, g, b, 255);
  SDL_RenderFillRect(r, &s);
}

void level_draw(SDL_Renderer *renderer, float camX, float camY) {
  // background: dark blue-grey
  SDL_SetRenderDrawColor(renderer, 24, 28, 44, 255);
  SDL_RenderClear(renderer);
  // subtle grid
  SDL_SetRenderDrawColor(renderer, 32, 38, 60, 255);
  for (int gx = -(int)camX % 80; gx < WINDOW_WIDTH; gx += 80) {
    SDL_FRect l = {(float)gx, 0, 1, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &l);
  }

  // solids
  for (int i = 0; i < nsol; i++)
    draw_world_rect(renderer, solids[i], camX, camY, 96, 125, 180);
  // oneways (green flat)
  for (int i = 0; i < noneway; i++)
    draw_world_rect(renderer, oneways[i], camX, camY, 90, 200, 120);
  // movers (orange)
  for (int i = 0; i < nmover; i++)
    draw_world_rect(renderer, movers[i].rect, camX, camY, 230, 150, 60);
  // vanish (blink when about to fall)
  for (int i = 0; i < nvan; i++) {
    if (!vanishes[i].active)
      continue;
    Uint8 r = 180, g = 140, b = 220;
    if (vanishes[i].standT > 0.3f) { // blink warning
      int bl = ((int)(vanishes[i].standT * 12) % 2) ? 80 : 255;
      r = bl;
      g = bl;
    }
    draw_world_rect(renderer, vanishes[i].rect, camX, camY, r, g, b);
  }
  // spikes red
  for (int i = 0; i < nspike; i++)
    draw_world_rect(renderer, spikes[i], camX, camY, 220, 50, 50);
  // boxes brown with border
  for (int i = 0; i < nbox; i++) {
    draw_world_rect(renderer, boxes[i].rect, camX, camY, 170, 120, 70);
    SDL_FRect s = {boxes[i].rect.x - camX + 4, boxes[i].rect.y - camY + 4,
                   boxes[i].rect.w - 8, boxes[i].rect.h - 8};
    SDL_SetRenderDrawColor(renderer, 120, 80, 40, 255);
    SDL_RenderRect(renderer, &s);
  }
  // button (red unpressed / green pressed)
  if (buttonExists) {
    if (buttonPressed)
      draw_world_rect(renderer, buttonRect, camX, camY, 80, 220, 100);
    else
      draw_world_rect(renderer, buttonRect, camX, camY, 220, 80, 80);
  }
  // gate (grey closed / outline open)
  if (gateExists) {
    if (!buttonPressed)
      draw_world_rect(renderer, gateRect, camX, camY, 150, 150, 160);
    else {
      SDL_FRect s = {gateRect.x - camX, gateRect.y - camY, gateRect.w,
                     gateRect.h};
      SDL_SetRenderDrawColor(renderer, 80, 220, 100, 255);
      SDL_RenderRect(renderer, &s);
    }
  }
  // key
  if (keyExists && !keyTaken)
    draw_world_rect(renderer, keyRect, camX, camY, 240, 210, 60);
  // exit door: red locked / green open
  if (!keyExists || keyTaken)
    draw_world_rect(renderer, exitRect, camX, camY, 70, 210, 110);
  else
    draw_world_rect(renderer, exitRect, camX, camY, 200, 70, 70);
  // portals purple / cyan
  if (portalExists) {
    draw_world_rect(renderer, portals[0], camX, camY, 180, 90, 250);
    draw_world_rect(renderer, portals[1], camX, camY, 80, 220, 230);
  }
  // flip zones orange translucent look (solid + border)
  for (int i = 0; i < nflip; i++) {
    draw_world_rect(renderer, flipZones[i], camX, camY, 240, 140, 40);
    SDL_FRect s = {flipZones[i].x - camX, flipZones[i].y - camY, flipZones[i].w,
                   flipZones[i].h};
    SDL_SetRenderDrawColor(renderer, 255, 220, 120, 255);
    SDL_RenderRect(renderer, &s);
  }
}

// old API compat
void add_obstruc(SDL_Renderer *renderer) { level_draw(renderer, 0, 0); }
