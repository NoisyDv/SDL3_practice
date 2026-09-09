#include "audio.h"
#include "level.h"
#include "main.h"
#include "particles.h"
#include "player.h"
#include <SDL3/SDL.h>
#include <math.h>
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
static Enemy enemies[MAX_ENEMIES];
static int nenemy = 0;
static Shot shots[MAX_SHOTS];
static Turret turrets[MAX_TURRETS];
static int nturret = 0;

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
// Walker: runs on ground between x0..x1. Flyer: hovers around (x,y).
static void add_walker(float x, float y, float x0, float x1, float speed) {
  if (nenemy >= MAX_ENEMIES)
    return;
  Enemy *e = &enemies[nenemy++];
  e->rect = (SDL_FRect){x, y, 44, 36};
  e->type = 0;
  e->dir = -1;
  e->speed = speed;
  e->baseY = y;
  e->phase = 0;
  e->vy = 0;
  e->alive = true;
  e->x_min = x0;
  e->x_max = x1;
}
static void add_flyer(float x, float y, float x0, float x1, float speed) {
  if (nenemy >= MAX_ENEMIES)
    return;
  Enemy *e = &enemies[nenemy++];
  e->rect = (SDL_FRect){x, y, 40, 30};
  e->type = 1;
  e->dir = -1;
  e->speed = speed;
  e->baseY = y;
  e->phase = 0;
  e->vy = 0;
  e->alive = true;
  e->x_min = x0;
  e->x_max = x1;
}
static void add_turret(float x, float y, float dir, float interval) {
  if (nturret >= MAX_TURRETS)
    return;
  Turret *t = &turrets[nturret++];
  t->rect = (SDL_FRect){x, y, 36, 44};
  t->dir = dir;
  t->cool = 1.0f;
  t->interval = interval;
}
static void fire_shot(float x, float y, float vx) {
  for (int i = 0; i < MAX_SHOTS; i++) {
    if (shots[i].alive)
      continue;
    shots[i].alive = true;
    shots[i].rect = (SDL_FRect){x, y, 12, 12};
    shots[i].vx = vx;
    shots[i].vy = 0;
    shots[i].life = 4.0f;
    break;
  }
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
  // duck tunnel: 44px gap (stand 56 blocked, duck 30 fits)
  add_solid(950, 470, 220, 46);
  add_walker(1700, 524, 1600, 2200, 90); // stompable, or dash through
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
  add_walker(800, 524, 700, 1000, 80); // patrols before the gate
  add_flyer(1450, 400, 1400, 1650, 110); // over the spike pit
  add_turret(2120, 316, -1, 2.2f);      // on the high ledge, fires left
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
  add_flyer(1350, 330, 1280, 1750, 120); // harasses the oneway hops
  add_turret(2150, 516, -1, 2.0f);       // guards the key: duck or dash
  keyRect = (SDL_FRect){2080, 500, 28, 28};
  keyExists = true;
  exitRect = (SDL_FRect){2280, 440, 50, 120};
}

static void build_level_3(void) {
  // Dash Gap: jump then SHIFT mid-air to cross the wide pit.
  spawnRect = (SDL_FRect){60, 400, 36, 56};
  add_solid(0, 560, 900, 80);     // ground A
  add_solid(1100, 560, 1300, 80); // ground B (200px pit: needs jump+dash)
  add_solid(-40, -40, 40, 760);
  add_solid(0, -40, 2440, 40);
  add_oneway(1150, 440, 130); // key platform
  add_walker(1500, 524, 1300, 2100, 100);
  keyRect = (SDL_FRect){1190, 390, 28, 28};
  keyExists = true;
  exitRect = (SDL_FRect){2280, 440, 50, 120};
}

static void build_level_4(void) {
  // Mover Ride: bridge the spike pit, then oneway hops.
  spawnRect = (SDL_FRect){60, 400, 36, 56};
  add_solid(0, 560, 700, 80);     // ground A
  add_solid(1200, 560, 1200, 80); // ground B
  add_solid(-40, -40, 40, 760);
  add_solid(0, -40, 2440, 40);
  add_spike(700, 600, 500, 24); // pit floor
  add_mover(700, 470, 140, 20, 0, 700, 1060, 120);
  add_oneway(1500, 440, 130);
  add_flyer(800, 400, 700, 1150, 120); // over the pit
  add_walker(1800, 524, 1600, 2200, 110);
  keyRect = (SDL_FRect){1540, 390, 28, 28};
  keyExists = true;
  exitRect = (SDL_FRect){2280, 440, 50, 120};
}

static void build_level_5(void) {
  // Vanish Chain: keep moving, platforms fall after 0.7s.
  spawnRect = (SDL_FRect){60, 400, 36, 56};
  add_solid(0, 560, 500, 80);      // ground A
  add_solid(1260, 560, 1140, 80);  // ground B
  add_solid(-40, -40, 40, 760);
  add_solid(0, -40, 2440, 40);
  add_vanish(560, 480, 95);
  add_vanish(700, 420, 95);
  add_vanish(840, 360, 95);
  add_vanish(980, 420, 95);
  add_vanish(1120, 480, 95);
  add_flyer(700, 300, 600, 1100, 130);
  keyRect = (SDL_FRect){865, 310, 28, 28}; // grab mid-hop over the peak
  keyExists = true;
  exitRect = (SDL_FRect){2280, 440, 50, 120};
}

static void build_level_6(void) {
  // Boxes II: two boxes, one button, spikes after the gate.
  spawnRect = (SDL_FRect){60, 400, 36, 56};
  add_solid(0, 560, 2400, 80);
  add_solid(-40, -40, 40, 760);
  add_solid(0, -40, 2440, 40);
  gateRect = (SDL_FRect){1200, 400, 30, 160};
  gateExists = true;
  buttonRect = (SDL_FRect){500, 540, 56, 20};
  buttonExists = true;
  add_box(800, 500);
  add_box(920, 500);
  add_spike(1400, 540, 300, 20);
  add_oneway(1420, 440, 140); // bridge over spikes
  add_walker(2000, 524, 1800, 2250, 100);
  keyRect = (SDL_FRect){1900, 500, 28, 28};
  keyExists = true;
  exitRect = (SDL_FRect){2280, 440, 50, 120};
}

static void build_level_7(void) {
  // Flyer Corridor: oneway hops harassed by two flyers + turret.
  spawnRect = (SDL_FRect){60, 400, 36, 56};
  add_solid(0, 560, 2400, 80);
  add_solid(-40, -40, 40, 760);
  add_solid(0, -40, 2440, 40);
  add_spike(900, 540, 600, 20);
  add_oneway(900, 440, 120);
  add_oneway(1060, 360, 120);
  add_oneway(1220, 440, 120);
  add_oneway(1380, 360, 120);
  add_flyer(950, 300, 900, 1200, 130);
  add_flyer(1250, 280, 1100, 1500, 140);
  add_turret(2000, 516, -1, 2.0f);
  keyRect = (SDL_FRect){2100, 500, 28, 28};
  keyExists = true;
  exitRect = (SDL_FRect){2280, 440, 50, 120};
}

static void build_level_8(void) {
  // Turret Gauntlet: tunnels block shots, duck through under fire.
  spawnRect = (SDL_FRect){60, 400, 36, 56};
  add_solid(0, 560, 2400, 80);
  add_solid(-40, -40, 40, 760);
  add_solid(0, -40, 2440, 40);
  add_solid(600, 470, 220, 46);  // duck tunnel 1 (44px gap)
  add_solid(1400, 470, 220, 46); // duck tunnel 2
  add_turret(350, 516, 1, 1.8f);   // fires right: tunnel blocks shots
  add_turret(1700, 516, -1, 1.8f); // fires left: tunnel blocks shots
  add_walker(1000, 524, 900, 1300, 90);
  keyRect = (SDL_FRect){1100, 500, 28, 28};
  keyExists = true;
  exitRect = (SDL_FRect){2280, 440, 50, 120};
}

static void build_level_9(void) {
  // Final Mix: portal + gravity flip + vanish + walker.
  spawnRect = (SDL_FRect){60, 400, 36, 56};
  add_solid(0, 560, 2400, 80);
  add_solid(-40, -40, 40, 760);
  add_solid(0, -40, 2440, 40);
  add_solid(2360, 0, 40, 640); // right wall
  add_solid(800, 180, 30, 380); // tall wall forces portal use
  portals[0] = (SDL_FRect){640, 450, 44, 110};
  portals[1] = (SDL_FRect){890, 450, 44, 110};
  portalExists = true;
  add_solid(1200, 60, 800, 24); // ceiling path
  flipZones[0] = (SDL_FRect){1200, 380, 110, 180};
  flipZones[1] = (SDL_FRect){1890, 80, 110, 180};
  nflip = 2;
  add_spike(1250, 540, 600, 20);
  add_oneway(1280, 440, 130);
  add_oneway(1460, 360, 130);
  add_oneway(1640, 440, 130);
  add_vanish(1830, 400, 95);
  add_walker(2000, 524, 1950, 2250, 120);
  add_turret(2150, 516, -1, 2.4f);
  keyRect = (SDL_FRect){2080, 500, 28, 28};
  keyExists = true;
  exitRect = (SDL_FRect){2280, 440, 50, 120};
}

void level_load(int idx) {
  cur = idx;
  nsol = noneway = nmover = nvan = nspike = nbox = nenemy = nturret = 0;
  for (int i = 0; i < MAX_SHOTS; i++)
    shots[i].alive = false;
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
  else if (idx == 2)
    build_level_2();
  else if (idx == 3)
    build_level_3();
  else if (idx == 4)
    build_level_4();
  else if (idx == 5)
    build_level_5();
  else if (idx == 6)
    build_level_6();
  else if (idx == 7)
    build_level_7();
  else if (idx == 8)
    build_level_8();
  else
    build_level_9();
  reset_player(spawnRect.x, spawnRect.y);
}

void init_obstruc(void) { level_load(0); }
void destroy_obstruc(void) {}

int level_index(void) { return cur; }
static const char *level_names[NUM_LEVELS] = {
    "Level 1/10: First Jumps",   "Level 2/10: Boxes & Buttons",
    "Level 3/10: Portals & Gravity", "Level 4/10: Dash Gap",
    "Level 5/10: Mover Ride",    "Level 6/10: Vanish Chain",
    "Level 7/10: Boxes II",      "Level 8/10: Flyer Corridor",
    "Level 9/10: Turret Gauntlet", "Level 10/10: Final Mix",
};
static const char *level_hints[NUM_LEVELS] = {
    "A/D move SPACE jump S duck SHIFT dash Stomp reds (R restart)",
    "Push BOX onto BUTTON to open GATE Dash has i-frames vs shots",
    "Enter PORTAL Touch ORANGE zone to flip gravity N next when open",
    "Jump then SHIFT mid-air to cross the wide pit (R restart)",
    "Ride the platform over spikes Watch the flyer N next when open",
    "Keep moving: platforms fall after 0.7s Grab key mid-hop",
    "Two BOXES one BUTTON Bridge the spikes on the oneway",
    "Hop oneways under flyers Duck to hide from the TURRET",
    "Duck to hide from TURRETS Tunnels block shots N next when open",
    "Everything at once Portal flip vanish stomp Good luck",
};
const char *level_name(void) {
  if (cur < 0 || cur >= NUM_LEVELS)
    return "Level ?";
  return level_names[cur];
}
const char *level_hint(void) {
  if (cur < 0 || cur >= NUM_LEVELS)
    return "";
  return level_hints[cur];
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
Enemy *level_enemies(int *n) {
  *n = nenemy;
  return enemies;
}
Shot *level_shots(int *n) {
  *n = MAX_SHOTS;
  return shots;
}
Turret *level_turrets(int *n) {
  *n = nturret;
  return turrets;
}
void level_kill_enemy(int idx) {
  if (idx < 0 || idx >= nenemy)
    return;
  enemies[idx].alive = false;
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

  // ---- enemies ----
  for (int i = 0; i < nenemy; i++) {
    Enemy *e = &enemies[i];
    if (!e->alive)
      continue;
    if (e->type == 0) {
      // walker: gravity + patrol, turn at walls and ledges
      e->vy += GRAVITY * dt;
      if (e->vy > MAX_FALL)
        e->vy = MAX_FALL;
      e->rect.x += e->dir * e->speed * dt;
      bool hitWall = false;
      for (int s = 0; s < nsol; s++) {
        if (coliderrect(e->rect, solids[s])) {
          hitWall = true;
          break;
        }
      }
      if (e->rect.x < e->x_min || e->rect.x + e->rect.w > e->x_max)
        hitWall = true;
      if (hitWall) {
        e->dir *= -1;
        e->rect.x += e->dir * e->speed * dt; // step back out
      }
      e->rect.y += e->vy * dt;
      bool grounded = false;
      for (int s = 0; s < nsol; s++) {
        if (coliderrect(e->rect, solids[s])) {
          if (e->vy >= 0) {
            e->rect.y = solids[s].y - e->rect.h;
            e->vy = 0;
            grounded = true;
          } else {
            e->rect.y = solids[s].y + solids[s].h;
            e->vy = 0;
          }
        }
      }
      // ledge check: no ground ahead -> turn around
      if (grounded) {
        float aheadX = e->dir < 0 ? e->rect.x + 2 : e->rect.x + e->rect.w - 2;
        float probeY = e->rect.y + e->rect.h + 8;
        bool groundAhead = false;
        for (int s = 0; s < nsol; s++) {
          if (aheadX >= solids[s].x && aheadX <= solids[s].x + solids[s].w &&
              probeY >= solids[s].y && probeY <= solids[s].y + solids[s].h + 24) {
            groundAhead = true;
            break;
          }
        }
        if (!groundAhead)
          e->dir *= -1;
      }
      if (e->rect.y > LEVEL_H + 100)
        e->alive = false;
    } else {
      // flyer: horizontal pingpong + sine hover, no gravity
      e->phase += dt * 3.0f;
      e->rect.x += e->dir * e->speed * dt;
      if (e->rect.x < e->x_min || e->rect.x + e->rect.w > e->x_max)
        e->dir *= -1;
      e->rect.y = e->baseY + sinf(e->phase) * 30.0f;
    }
  }

  // ---- turrets + shots ----
  for (int i = 0; i < nturret; i++) {
    Turret *t = &turrets[i];
    t->cool -= dt;
    if (t->cool <= 0 && !player.dead) {
      if (player.ducking) {
        t->cool = 0.3f; // ducked prey: turret holds fire, re-check soon
        continue;
      }
      float pcx = player.x + player.w / 2, pcy = player.y + player.h / 2;
      float tcx = t->rect.x + t->rect.w / 2, tcy = t->rect.y + t->rect.h / 2;
      float dx = pcx - tcx;
      bool inDir = (t->dir < 0 && dx < 0) || (t->dir > 0 && dx > 0);
      if (inDir && dx < 700 && dx > -700 && pcy > tcy - 220 &&
          pcy < tcy + 220) {
        // aim at head height: standing players are hit, ducked (top 530)
        // cleanly dodge the 512..524 band by 6px
        float sy = player.y + 8;
        float sx = t->dir < 0 ? t->rect.x - 14 : t->rect.x + t->rect.w + 2;
        fire_shot(sx, sy, t->dir * 260.0f);
        particles_burst(sx, sy, 4, 80, 60, 255, 180, 80, 0.25f);
        audio_shoot();
        t->cool = t->interval;
      } else {
        t->cool = 0.2f; // re-check soon
      }
    }
  }
  for (int i = 0; i < MAX_SHOTS; i++) {
    Shot *sh = &shots[i];
    if (!sh->alive)
      continue;
    sh->life -= dt;
    if (sh->life <= 0) {
      sh->alive = false;
      continue;
    }
    sh->rect.x += sh->vx * dt;
    bool hit = false;
    for (int s = 0; s < nsol; s++) {
      if (coliderrect(sh->rect, solids[s])) {
        hit = true;
        break;
      }
    }
    if (hit) {
      sh->alive = false;
      particles_burst(sh->rect.x + 6, sh->rect.y + 6, 5, 120, 100, 255, 150,
                      60, 0.3f);
      continue;
    }
    if (sh->rect.x < -40 || sh->rect.x > LEVEL_W + 40)
      sh->alive = false;
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
      particles_burst(pr.x + pr.w / 2, pr.y + pr.h / 2, 16, 220, 260, 240,
                      150, 60, 0.5f);
      shake_add(3.0f, 0.1f);
      audio_flip();
      SDL_Log("gravity flipped: %d", player.gravityDir);
    }
  }

  // portals
  if (portalExists && player.portalCool <= 0) {
    if (coliderrect(pr, portals[0])) {
      player.x = portals[1].x + portals[1].w + 4;
      player.y = portals[1].y + portals[1].h - player.h - 8;
      player.portalCool = 0.7f;
      particles_burst(player.x + player.w / 2, player.y + player.h / 2, 14,
                      240, 240, 80, 220, 230, 0.5f);
      audio_portal();
    } else if (coliderrect(pr, portals[1])) {
      player.x = portals[0].x - player.w - 4;
      player.y = portals[0].y + portals[0].h - player.h - 8;
      player.portalCool = 0.7f;
      particles_burst(player.x + player.w / 2, player.y + player.h / 2, 14,
                      240, 240, 180, 90, 250, 0.5f);
      audio_portal();
    }
  }

  // key pickup
  if (keyExists && !keyTaken && coliderrect(pr, keyRect)) {
    keyTaken = true;
    player.hasKey = true;
    particles_burst(keyRect.x + keyRect.w / 2, keyRect.y + keyRect.h / 2, 18,
                    220, 260, 240, 210, 60, 0.6f);
    audio_key();
    SDL_Log("key collected");
  }

  // spikes kill (forgiving shrink) — burst only on transition to dead
  for (int i = 0; i < nspike; i++) {
    SDL_FRect s = spikes[i];
    s.x += 6;
    s.w -= 12;
    if (s.w < 4)
      continue;
    if (!player.dead && coliderrect(pr, s)) {
      player.dead = true;
      particles_burst(pr.x + pr.w / 2, pr.y + pr.h / 2, 22, 300, 320, 220,
                      50, 50, 0.7f);
      shake_add(8.0f, 0.25f);
      audio_death();
    }
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

// Dark body + neon edge (silhouette style for the black character):
// near-black fill, bright top lip, neon 1px outline, subtle seams.
static void draw_solid(SDL_Renderer *r, SDL_FRect w, float cx, float cy,
                       Uint8 br, Uint8 bg, Uint8 bb, Uint8 er, Uint8 eg,
                       Uint8 eb) {
  SDL_FRect s = {w.x - cx, w.y - cy, w.w, w.h};
  if (s.x + s.w < 0 || s.x > WINDOW_WIDTH || s.y + s.h < 0 ||
      s.y > WINDOW_HEIGHT)
    return;
  SDL_SetRenderDrawColor(r, br, bg, bb, 255);
  SDL_RenderFillRect(r, &s);
  // neon top lip
  SDL_FRect top = {s.x, s.y, s.w, 3};
  SDL_SetRenderDrawColor(r, er, eg, eb, 255);
  SDL_RenderFillRect(r, &top);
  // bottom shadow (darker body)
  SDL_FRect bot = {s.x, s.y + s.h - 4, s.w, 4};
  SDL_SetRenderDrawColor(r, (Uint8)(br * 0.5), (Uint8)(bg * 0.5),
                         (Uint8)(bb * 0.5), 255);
  SDL_RenderFillRect(r, &bot);
  // neon outline
  SDL_SetRenderDrawColor(r, er, eg, eb, 255);
  SDL_RenderRect(r, &s);
  // tile seams every 40px (slightly lighter body)
  Uint8 sr = br + 16 > 255 ? 255 : br + 16;
  Uint8 sg = bg + 16 > 255 ? 255 : bg + 16;
  Uint8 sb = bb + 16 > 255 ? 255 : bb + 16;
  SDL_SetRenderDrawColor(r, sr, sg, sb, 255);
  for (float lx = s.x + 40 - fmodf(w.x, 40.0f); lx < s.x + s.w; lx += 40) {
    SDL_FRect seam = {lx, s.y + 3, 1, s.h - 7};
    SDL_RenderFillRect(r, &seam);
  }
}

static void draw_spike_strip(SDL_Renderer *r, SDL_FRect w, float cx, float cy) {
  float sx = w.x - cx, sy = w.y - cy;
  if (sx + w.w < 0 || sx > WINDOW_WIDTH)
    return;
  // dark base
  SDL_FRect base = {sx, sy + w.h - 8, w.w, 8};
  SDL_SetRenderDrawColor(r, 10, 10, 14, 255);
  SDL_RenderFillRect(r, &base);
  // teeth as filled near-black triangles with hot-red tips + red outline
  const float tooth = 22.0f;
  int n = (int)(w.w / tooth);
  if (n < 1)
    n = 1;
  float tw = w.w / n;
  for (int i = 0; i < n; i++) {
    float x0 = sx + i * tw, x1 = sx + (i + 1) * tw, xm = (x0 + x1) / 2;
    SDL_Vertex v[3];
    v[0].position.x = x0;
    v[0].position.y = sy + w.h;
    v[1].position.x = x1;
    v[1].position.y = sy + w.h;
    v[2].position.x = xm;
    v[2].position.y = sy;
    for (int k = 0; k < 3; k++) {
      v[k].color.r = 0.08f;
      v[k].color.g = 0.08f;
      v[k].color.b = 0.10f;
      v[k].color.a = 1.0f;
      v[k].tex_coord.x = 0;
      v[k].tex_coord.y = 0;
    }
    // hot tip: bright red warning point
    v[2].color.r = 1.0f;
    v[2].color.g = 0.24f;
    v[2].color.b = 0.24f;
    SDL_RenderGeometry(r, NULL, v, 3, NULL, 0);
    SDL_SetRenderDrawColor(r, 255, 70, 70, 255);
    SDL_RenderLine(r, x0, sy + w.h, xm, sy);
    SDL_RenderLine(r, xm, sy, x1, sy + w.h);
  }
}

void level_draw(SDL_Renderer *renderer, float camX, float camY) {
  Uint32 ticks = SDL_GetTicks();
  float t = ticks / 1000.0f;

  // per-level dark palette: near-black bg + dark bodies + neon edge tint
  // cycles cyan -> mint -> violet so all 10 levels feel grouped in 3s
  Uint8 bgR = 8, bgG = 10, bgB = 16;
  Uint8 solR = 22, solG = 26, solB = 38;
  Uint8 edR = 0, edG = 230, edB = 255; // cyan neon
  if (cur % 3 == 1) {
    bgR = 8;
    bgG = 14;
    bgB = 12;
    solR = 20;
    solG = 30;
    solB = 26;
    edR = 0;
    edG = 255;
    edB = 180; // mint neon
  } else if (cur % 3 == 2) {
    bgR = 12;
    bgG = 9;
    bgB = 18;
    solR = 28;
    solG = 22;
    solB = 38;
    edR = 170;
    edG = 120;
    edB = 255; // violet neon
  }

  // background
  SDL_SetRenderDrawColor(renderer, bgR, bgG, bgB, 255);
  SDL_RenderClear(renderer);

  // parallax stars (far, 0.2x) — dim on near-black sky
  SDL_SetRenderDrawColor(renderer, 60, 70, 110, 255);
  for (int i = 0; i < 70; i++) {
    int wx = (i * 173 + 41) % 2400;
    int wy = (i * 97 + 13) % 640;
    float sx = wx - camX * 0.2f;
    while (sx < 0)
      sx += 2400;
    while (sx > WINDOW_WIDTH)
      sx -= 2400;
    if (sx < 0 || sx > WINDOW_WIDTH)
      continue;
    float tw = 0.5f + 0.5f * sinf(t * 2.0f + i);
    SDL_FRect st = {sx, (float)wy, 1 + tw, 1 + tw};
    SDL_RenderFillRect(renderer, &st);
  }
  // parallax hills (mid, 0.5x): dark silhouette bands
  SDL_SetRenderDrawColor(renderer, bgR + 6, bgG + 7, bgB + 10, 255);
  for (int i = 0; i < 12; i++) {
    float wx = i * 320.0f;
    float sx = wx - camX * 0.5f;
    if (sx + 260 < 0 || sx > WINDOW_WIDTH)
      continue;
    SDL_FRect hill = {sx, 470 + 20 * sinf(i * 1.7f), 260, 170};
    SDL_RenderFillRect(renderer, &hill);
  }
  // subtle grid (near, 1x)
  SDL_SetRenderDrawColor(renderer, bgR + 5, bgG + 6, bgB + 10, 255);
  int off = (int)camX % 80;
  if (off < 0)
    off += 80;
  for (int gx = -off; gx < WINDOW_WIDTH; gx += 80) {
    SDL_FRect l = {(float)gx, 0, 1, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &l);
  }

  // solids (dark bodies, neon edge)
  for (int i = 0; i < nsol; i++)
    draw_solid(renderer, solids[i], camX, camY, solR, solG, solB, edR, edG,
               edB);
  // oneways (dark flat + bright lime lip)
  for (int i = 0; i < noneway; i++) {
    draw_world_rect(renderer, oneways[i], camX, camY, 18, 30, 22);
    SDL_FRect o = {oneways[i].x - camX, oneways[i].y - camY, oneways[i].w, 4};
    SDL_SetRenderDrawColor(renderer, 150, 255, 150, 255);
    SDL_RenderFillRect(renderer, &o);
  }
  // movers (dark + amber neon + direction chevrons)
  for (int i = 0; i < nmover; i++) {
    draw_solid(renderer, movers[i].rect, camX, camY, 30, 24, 20, 255, 170, 40);
    // chevron arrows on top face
    SDL_SetRenderDrawColor(renderer, 255, 210, 130, 255);
    float mx = movers[i].rect.x - camX, my = movers[i].rect.y - camY;
    float mw = movers[i].rect.w, mh = movers[i].rect.h;
    for (float ax = mx + 12; ax < mx + mw - 6; ax += 18) {
      if (movers[i].axis == 0) {
        SDL_RenderLine(renderer, ax, my + mh / 2 - 5, ax + 6, my + mh / 2);
        SDL_RenderLine(renderer, ax + 6, my + mh / 2, ax, my + mh / 2 + 5);
      } else {
        SDL_RenderLine(renderer, mx + mw / 2 - 5, ax, mx + mw / 2, ax + 6);
        SDL_RenderLine(renderer, mx + mw / 2, ax + 6, mx + mw / 2 + 5, ax);
      }
    }
  }
  // vanish (dark + violet neon; danger blink inverts to bright)
  for (int i = 0; i < nvan; i++) {
    if (!vanishes[i].active)
      continue;
    Uint8 r = 28, g = 22, b = 36;
    Uint8 or_ = 190, og = 120, ob = 255;
    if (vanishes[i].standT > 0.3f) { // blink warning: flash bright
      int bl = ((int)(vanishes[i].standT * 12) % 2) ? 1 : 0;
      r = bl ? 220 : 60;
      g = bl ? 180 : 50;
      b = bl ? 255 : 70;
      or_ = 255;
      og = 255;
      ob = 255;
    }
    SDL_FRect v = {vanishes[i].rect.x - camX, vanishes[i].rect.y - camY,
                   vanishes[i].rect.w, vanishes[i].rect.h};
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &v);
    SDL_SetRenderDrawColor(renderer, or_, og, ob, 255);
    SDL_RenderRect(renderer, &v);
    SDL_FRect lip = {v.x, v.y, v.w, 2};
    SDL_RenderFillRect(renderer, &lip);
  }
  // spikes as triangles
  for (int i = 0; i < nspike; i++)
    draw_spike_strip(renderer, spikes[i], camX, camY);
  // enemies: walkers (red crabs) + flyers (purple bats)
  for (int i = 0; i < nenemy; i++) {
    Enemy *e = &enemies[i];
    if (!e->alive)
      continue;
    float ex = e->rect.x - camX, ey = e->rect.y - camY;
    if (ex + e->rect.w < 0 || ex > WINDOW_WIDTH)
      continue;
    if (e->type == 0) {
      // walker: near-black crab, red neon rim, white eyes
      draw_solid(renderer, e->rect, camX, camY, 18, 16, 20, 255, 70, 70);
      // feet nubs
      SDL_SetRenderDrawColor(renderer, 90, 20, 20, 255);
      float step = sinf(t * 10.0f + i * 2.0f) * 2.0f;
      SDL_FRect f1 = {ex + 6, ey + e->rect.h - 6 + step, 8, 6};
      SDL_FRect f2 = {ex + e->rect.w - 14, ey + e->rect.h - 6 - step, 8, 6};
      SDL_RenderFillRect(renderer, &f1);
      SDL_RenderFillRect(renderer, &f2);
      // eyes look toward walk dir
      float eo = e->dir < 0 ? -4 : 4;
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_FRect e1 = {ex + e->rect.w / 2 - 10 + eo, ey + 7, 8, 10};
      SDL_FRect e2 = {ex + e->rect.w / 2 + 2 + eo, ey + 7, 8, 10};
      SDL_RenderFillRect(renderer, &e1);
      SDL_RenderFillRect(renderer, &e2);
      SDL_SetRenderDrawColor(renderer, 20, 10, 10, 255);
      SDL_FRect p1 = {e1.x + 2 + eo * 0.3f, e1.y + 4, 4, 4};
      SDL_FRect p2 = {e2.x + 2 + eo * 0.3f, e2.y + 4, 4, 4};
      SDL_RenderFillRect(renderer, &p1);
      SDL_RenderFillRect(renderer, &p2);
    } else {
      // wings flap (dark violet)
      float flap = sinf(t * 10.0f + i * 1.7f) * 8.0f;
      SDL_SetRenderDrawColor(renderer, 70, 40, 130, 255);
      SDL_FRect w1 = {ex - 10, ey + 4 + flap, 14, 8};
      SDL_FRect w2 = {ex + e->rect.w - 4, ey + 4 - flap, 14, 8};
      SDL_RenderFillRect(renderer, &w1);
      SDL_RenderFillRect(renderer, &w2);
      // flyer: near-black bat, violet neon rim, yellow eyes
      draw_solid(renderer, e->rect, camX, camY, 22, 18, 30, 200, 130, 255);
      float eo = e->dir < 0 ? -3 : 3;
      SDL_SetRenderDrawColor(renderer, 255, 240, 120, 255);
      SDL_FRect e1 = {ex + 9 + eo, ey + 8, 8, 8};
      SDL_FRect e2 = {ex + 23 + eo, ey + 8, 8, 8};
      SDL_RenderFillRect(renderer, &e1);
      SDL_RenderFillRect(renderer, &e2);
    }
  }
  // turrets: gunmetal base + steel rim + barrel + charge blink
  for (int i = 0; i < nturret; i++) {
    Turret *tu = &turrets[i];
    draw_solid(renderer, tu->rect, camX, camY, 25, 28, 34, 150, 180, 220);
    float bx = tu->rect.x - camX, by = tu->rect.y - camY;
    SDL_FRect barrel;
    if (tu->dir < 0)
      barrel = (SDL_FRect){bx - 16, by + 14, 18, 10};
    else
      barrel = (SDL_FRect){bx + tu->rect.w - 2, by + 14, 18, 10};
    SDL_SetRenderDrawColor(renderer, 80, 85, 100, 255);
    SDL_RenderFillRect(renderer, &barrel);
    // eye: blinks faster as shot charges
    float charge = 1.0f - (tu->cool / tu->interval);
    if (charge < 0)
      charge = 0;
    if (charge > 1)
      charge = 1;
    int blink = ((int)(t * (4 + charge * 14)) % 2 == 0) ? 255 : 60;
    SDL_SetRenderDrawColor(renderer, blink, 50, 50, 255);
    SDL_FRect eye = {bx + tu->rect.w / 2 - 5, by + 6, 10, 10};
    SDL_RenderFillRect(renderer, &eye);
  }
  // shots: hot core, brighter on dark
  for (int i = 0; i < MAX_SHOTS; i++) {
    if (!shots[i].alive)
      continue;
    SDL_FRect s = {shots[i].rect.x - camX - 3, shots[i].rect.y - camY - 3,
                   shots[i].rect.w + 6, shots[i].rect.h + 6};
    SDL_SetRenderDrawColor(renderer, 200, 80, 20, 255);
    SDL_RenderFillRect(renderer, &s);
    SDL_FRect c = {shots[i].rect.x - camX, shots[i].rect.y - camY,
                   shots[i].rect.w, shots[i].rect.h};
    SDL_SetRenderDrawColor(renderer, 255, 210, 90, 255);
    SDL_RenderFillRect(renderer, &c);
  }
  // boxes dark with warm neon edge + cross plank
  for (int i = 0; i < nbox; i++) {
    draw_solid(renderer, boxes[i].rect, camX, camY, 30, 26, 22, 255, 190, 90);
    SDL_FRect s = {boxes[i].rect.x - camX + 4, boxes[i].rect.y - camY + 4,
                   boxes[i].rect.w - 8, boxes[i].rect.h - 8};
    SDL_SetRenderDrawColor(renderer, 150, 100, 55, 255);
    SDL_RenderRect(renderer, &s);
    SDL_RenderLine(renderer, s.x, s.y, s.x + s.w, s.y + s.h);
    SDL_RenderLine(renderer, s.x + s.w, s.y, s.x, s.y + s.h);
  }
  // button (depresses when pressed)
  if (buttonExists) {
    SDL_FRect b = buttonRect;
    if (buttonPressed)
      b.h = 10; // pressed flat
    if (buttonPressed)
      draw_world_rect(renderer, b, camX, camY, 80, 220, 100);
    else
      draw_solid(renderer, b, camX, camY, 40, 16, 16, 255, 80, 80);
  }
  // gate (dark steel closed / green outline open)
  if (gateExists) {
    if (!buttonPressed)
      draw_solid(renderer, gateRect, camX, camY, 28, 28, 34, 170, 180, 200);
    else {
      SDL_FRect s = {gateRect.x - camX, gateRect.y - camY, gateRect.w,
                     gateRect.h};
      SDL_SetRenderDrawColor(renderer, 80, 220, 100, 255);
      SDL_RenderRect(renderer, &s);
    }
  }
  // key (bob + glow pulse)
  if (keyExists && !keyTaken) {
    float bob = sinf(t * 3.0f) * 5.0f;
    SDL_FRect glow = {keyRect.x - 4 - camX, keyRect.y - 4 + bob - camY,
                      keyRect.w + 8, keyRect.h + 8};
    SDL_SetRenderDrawColor(renderer, 120, 100, 20, 255);
    SDL_RenderFillRect(renderer, &glow);
    SDL_FRect k = {keyRect.x - camX, keyRect.y + bob - camY, keyRect.w,
                   keyRect.h};
    SDL_SetRenderDrawColor(renderer, 240, 210, 60, 255);
    SDL_RenderFillRect(renderer, &k);
    SDL_SetRenderDrawColor(renderer, 120, 85, 10, 255);
    SDL_RenderRect(renderer, &k);
  }
  // exit door: frame + glow when open
  {
    bool open = !keyExists || keyTaken;
    SDL_FRect d = {exitRect.x - camX, exitRect.y - camY, exitRect.w,
                   exitRect.h};
    if (open) {
      float pulse = 0.5f + 0.5f * sinf(t * 4.0f);
      SDL_FRect glow = {d.x - 3, d.y - 3, d.w + 6, d.h + 6};
      SDL_SetRenderDrawColor(renderer, 30 + (int)(30 * pulse), 150, 90, 255);
      SDL_RenderFillRect(renderer, &glow);
    }
    if (open)
      draw_world_rect(renderer, exitRect, camX, camY, 70, 210, 110);
    else
      draw_solid(renderer, exitRect, camX, camY, 40, 16, 16, 255, 80, 80);
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderRect(renderer, &d);
  }
  // portals (pulsing core + border)
  if (portalExists) {
    for (int p = 0; p < 2; p++) {
      SDL_FRect pr = {portals[p].x - camX, portals[p].y - camY, portals[p].w,
                      portals[p].h};
      float pulse = 0.5f + 0.5f * sinf(t * 5.0f + p * 3.14f);
      float inset = 3 + 3 * pulse;
      if (p == 0) {
        draw_world_rect(renderer, portals[p], camX, camY, 180, 90, 250);
        SDL_FRect core = {pr.x + inset, pr.y + inset, pr.w - inset * 2,
                          pr.h - inset * 2};
        SDL_SetRenderDrawColor(renderer, 220, 180, 255, 255);
        SDL_RenderFillRect(renderer, &core);
      } else {
        draw_world_rect(renderer, portals[p], camX, camY, 80, 220, 230);
        SDL_FRect core = {pr.x + inset, pr.y + inset, pr.w - inset * 2,
                          pr.h - inset * 2};
        SDL_SetRenderDrawColor(renderer, 180, 255, 255, 255);
        SDL_RenderFillRect(renderer, &core);
      }
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_RenderRect(renderer, &pr);
    }
  }
  // flip zones (dark amber body + bright border + chevrons)
  for (int i = 0; i < nflip; i++) {
    SDL_FRect fz = {flipZones[i].x - camX, flipZones[i].y - camY,
                    flipZones[i].w, flipZones[i].h};
    float pulse = 0.5f + 0.5f * sinf(t * 3.0f + i);
    Uint8 r = (Uint8)(50 + 20 * pulse), g = (Uint8)(28 + 10 * pulse), b = 12;
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &fz);
    SDL_SetRenderDrawColor(renderer, 255, 180, 60, 255);
    SDL_RenderRect(renderer, &fz);
    // arrows pointing up (flip to ceiling)
    SDL_SetRenderDrawColor(renderer, 255, 200, 100, 255);
    for (float ay = fz.y + 14; ay < fz.y + fz.h - 6; ay += 22) {
      float ax = fz.x + fz.w / 2;
      float slide = sinf(t * 4.0f + ay * 0.05f) * 3.0f;
      SDL_RenderLine(renderer, ax - 12, ay + slide, ax, ay - 8 + slide);
      SDL_RenderLine(renderer, ax, ay - 8 + slide, ax + 12, ay + slide);
    }
  }
}

// old API compat
void add_obstruc(SDL_Renderer *renderer) { level_draw(renderer, 0, 0); }
