#include "audio.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdbool.h>

static SDL_AudioStream *stream = NULL;
static bool ready = false;

void audio_init(void) {
  SDL_AudioSpec spec;
  SDL_zero(spec);
  spec.format = SDL_AUDIO_F32;
  spec.channels = 1;
  spec.freq = 22050;
  stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec,
                                     NULL, NULL);
  if (!stream) {
    SDL_Log("audio: no device (%s), SFX muted", SDL_GetError());
    ready = false;
    return;
  }
  SDL_ResumeAudioStreamDevice(stream);
  ready = true;
}

void audio_quit(void) {
  if (stream) {
    SDL_DestroyAudioStream(stream);
    stream = NULL;
  }
  ready = false;
}

// sweep f0->f1 Hz over dur seconds, square-ish blip with decay envelope
static void tone(float f0, float f1, float dur, float vol, bool square) {
  if (!ready || !stream)
    return;
  int n = (int)(22050 * dur);
  static float buf[22050]; // max 1s
  if (n > (int)(sizeof(buf) / sizeof(buf[0])))
    n = sizeof(buf) / sizeof(buf[0]);
  float phase = 0;
  for (int i = 0; i < n; i++) {
    float k = (float)i / n;
    float f = f0 + (f1 - f0) * k;
    phase += 2.0f * 3.14159265f * f / 22050.0f;
    float s = square ? ((sinf(phase) > 0 ? 1.0f : -1.0f) * 0.6f +
                        sinf(phase) * 0.4f)
                     : sinf(phase);
    float env = (1.0f - k) * (1.0f - k); // fast decay, no clicks
    // tiny fade-in to avoid pop
    if (i < 32)
      env *= (float)i / 32.0f;
    buf[i] = s * vol * env;
  }
  SDL_PutAudioStreamData(stream, buf, n * sizeof(float));
}

void audio_jump(void) { tone(300, 620, 0.12f, 0.35f, true); }
void audio_land(bool hard) {
  if (hard)
    tone(220, 90, 0.14f, 0.4f, true);
  else
    tone(200, 140, 0.07f, 0.2f, false);
}
void audio_key(void) {
  tone(660, 660, 0.07f, 0.35f, true);
  tone(990, 990, 0.12f, 0.35f, true);
}
void audio_death(void) { tone(320, 70, 0.35f, 0.45f, true); }
void audio_portal(void) { tone(900, 300, 0.18f, 0.35f, false); }
void audio_flip(void) { tone(250, 700, 0.16f, 0.35f, false); }
void audio_win(void) {
  tone(523, 523, 0.1f, 0.35f, true);
  tone(659, 659, 0.1f, 0.35f, true);
  tone(784, 784, 0.2f, 0.4f, true);
}
void audio_dash(void) { tone(200, 800, 0.12f, 0.3f, false); }
void audio_stomp(void) { tone(500, 150, 0.14f, 0.4f, true); }
void audio_shoot(void) { tone(800, 400, 0.09f, 0.22f, true); }
