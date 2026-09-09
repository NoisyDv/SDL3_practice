#include "particles.h"
#include <stdlib.h>

#define MAX_P 256

typedef struct {
  float x, y, vx, vy, life, maxLife, size;
  Uint8 r, g, b;
  bool alive;
} P;

static P pool[MAX_P];
static int cursor = 0;

void particles_init(void) {
  for (int i = 0; i < MAX_P; i++)
    pool[i].alive = false;
  cursor = 0;
}

void particles_burst(float x, float y, int count, float spread, float up,
                     Uint8 r, Uint8 g, Uint8 b, float life) {
  for (int i = 0; i < count; i++) {
    P *p = &pool[cursor];
    cursor = (cursor + 1) % MAX_P;
    p->alive = true;
    p->x = x;
    p->y = y;
    p->vx = ((float)(rand() % 2000) / 1000.0f - 1.0f) * spread;
    p->vy = -((float)(rand() % 1000) / 1000.0f) * up - 40.0f;
    p->maxLife = p->life = life * (0.6f + (rand() % 1000) / 1000.0f * 0.7f);
    p->size = 3.0f + (rand() % 1000) / 1000.0f * 5.0f;
    p->r = r;
    p->g = g;
    p->b = b;
  }
}

void particles_update(float dt) {
  for (int i = 0; i < MAX_P; i++) {
    P *p = &pool[i];
    if (!p->alive)
      continue;
    p->life -= dt;
    if (p->life <= 0) {
      p->alive = false;
      continue;
    }
    p->vy += 1400.0f * dt;
    p->x += p->vx * dt;
    p->y += p->vy * dt;
  }
}

void particles_draw(SDL_Renderer *renderer, float camX, float camY) {
  for (int i = 0; i < MAX_P; i++) {
    P *p = &pool[i];
    if (!p->alive)
      continue;
    float a = p->life / p->maxLife; // 1 -> 0
    Uint8 alpha = (Uint8)(a * 255);
    SDL_SetRenderDrawColor(renderer, p->r, p->g, p->b, alpha);
    SDL_FRect r = {p->x - camX - p->size / 2, p->y - camY - p->size / 2,
                   p->size * (0.5f + a * 0.5f), p->size * (0.5f + a * 0.5f)};
    SDL_RenderFillRect(renderer, &r);
  }
}

// ---- screen shake ----
static float shakeMag = 0, shakeT = 0, shakeDur = 1;

void shake_add(float mag, float time) {
  if (mag > shakeMag)
    shakeMag = mag;
  if (time > shakeT) {
    shakeT = time;
    shakeDur = time > 0.001f ? time : 0.001f;
  }
}

void shake_update(float dt) {
  if (shakeT > 0) {
    shakeT -= dt;
    if (shakeT <= 0) {
      shakeT = 0;
      shakeMag = 0;
    }
  }
}

void shake_offset(float *ox, float *oy) {
  if (shakeT <= 0 || shakeMag <= 0) {
    *ox = 0;
    *oy = 0;
    return;
  }
  float k = shakeT / shakeDur; // decay 1 -> 0
  float m = shakeMag * k;
  *ox = ((float)(rand() % 2000) / 1000.0f - 1.0f) * m;
  *oy = ((float)(rand() % 2000) / 1000.0f - 1.0f) * m;
}
