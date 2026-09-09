# Changelog

## Unreleased
### Added - 10 levels
- Roster grows 3 -> 10 (`NUM_LEVELS 10`): 4 Dash Gap (jump+dash a 200px pit), 5 Mover Ride (bridge + flyer), 6 Vanish Chain (5-platform run over void), 7 Boxes II (2 boxes, gate, spike bridge), 8 Flyer Corridor (oneways + 2 flyers + turret), 9 Turret Gauntlet (duck tunnels block shots), 10 Final Mix (portal + flip + vanish + walker + turret).
- Names/hints tables for all 10; palette and player aura cycle cyan/mint/violet by `cur % 3`; HUD shows `LV:x/10`; level jump keys `1-9` + `0` for level 10.
### Fixed - turret vs duck fairness
- Turrets aim at head height (`player.y + 8`, was chest) so standing shots (512..524) cleanly miss a ducked player (top 530); turrets hold fire while the player ducks (hide mechanic); verified headless: live-fire connects/fires, duck-hide suppresses, tunnels block shots.
- World goes near-black (`8,10,16` + per-level tint) with dimmed stars/hills/grid; all geometry is dark bodies with neon edges so the black sprite silhouettes against glowing shapes.
- `draw_solid` takes an edge color: solids cyan/mint/violet per level, movers amber, walkers red-rimmed black crabs, flyers violet-rimmed black bats, turrets gunmetal, boxes warm-rimmed dark, button/gate/exit-locked dark with red rims.
- Spikes: near-black teeth with hot-red tips + red outline edges; oneways dark with lime lip; vanish dark violet with white danger flash; flip zones dark amber with bright chevrons; shots hotter core.
- Character keeps `stick.png` but gets a spirit glow (3-layer backdrop lift tinted per-level neon) — a white color-mod halo was tried and discarded (tint multiplies: black x tint = black).
- Verified headless: pixel-assert test renders real frames (bg dark, lips neon, spike tips hot, enemy bodies dark, eyes bright, aura present) — ALL PASS.
### Changed - static SDL linking
- SDL3 (3.4.16) + SDL3_image (3.4.6) built from source into `third_party/sdl-static` and linked as explicit `.a` archives: `ldd out/game` shows no `libSDL3*`. Binary grew to ~5MB.
- System backends (X11/Wayland/ALSA/Pulse, image codecs) stay dynamic — SDL dlopens them at runtime; no static archives exist for them on this machine.
- `make` auto-bootstraps the prefix (`make setup-static`, pinned release tags, cmake+ninja); `.gitignore` keeps sources/build dirs out but tracks the built prefix for offline rebuilds.

## [0.4.0] - 2026-09-07
### Added - duck, dodge-dash, enemies
- Duck (hold `S/Down`): hitbox `56->30px`, feet planted, `0.4x` speed, `0.7x` jump, `2x` gravity fast-fall in air; stand-up blocked under ceilings; crouch pose in `draw_player`. Lv1 duck tunnel (44px gap) teaches it.
- Dodge-dash (`Shift/L`, edge-trigger): `650px/s` for `0.15s`, hovers (no gravity), `0.28s` i-frames, `0.9s` cooldown with pip above head, afterimage trail, i-frame blink, `audio_dash`.
- Walkers (red, gravity, wall+ledge turn) + flyers (purple, sine hover) in `level.h:Enemy` (`MAX 8`): stomp from above bounces (`0.6x`) + `audio_stomp`, side touch kills unless i-frames/dashing.
- Turrets (`MAX 4`) + fireballs (`MAX 16`, `260px/s` aimed at chest height): duck high shots, jump low ones, dash through to destroy; charge-blink eye, muzzle puff, `audio_shoot`.
- Placements: Lv1 tunnel+walker, Lv2 walker+pit flyer+ledge turret, Lv3 oneway flyer+exit turret; hints + HUD controls line updated.
- Tuning in `header/main.h` (`STAND_H/DUCK_H/DASH_*`); SFX `audio_dash/stomp/shoot`.

## [0.3.0] - 2026-09-07
### Fixed - character animation speed
- Anim was frame-counted in `draw_player` (8/15 frames) so 144/240Hz ran 2-4x fast. Now dt-based: `IDLE 0.15s`, `WALK 0.09s` in `update_player`, same speed everywhere.
- Enabled `SDL_SetRenderVSync(renderer,1)` + `SDL_INIT_AUDIO`.

### Added - look & feel polish
- Particles (`header/particles.h`, `src/particles.c`, 256 pool): jump/land dust, key/portal/flip bursts, death explosion.
- Screen shake (`shake_add/update/offset`): hard land 4.0, flip 3.0, death 8.0.
- Squash & stretch in `draw_player`: stretch by `|vy|`, 0.14s squash on landing.
- World readability (`src/level.c`): beveled solids + seams, triangle spikes via `SDL_RenderGeometry`, mover chevrons, depressing button, bobbing key, pulsing exit/portal/flip zones, parallax stars + hills + per-level palette.
- Procedural SFX (`header/audio.h`, `src/audio.c`, SDL3 streams, no mixer dep): jump/land/key/death/portal/flip/win.
- Camera: velocity look-ahead (+-90px) + dt-based smoothing.
- HUD: top bar, `TIME mm:ss`, `DEATHS`, fade 0.35s overlay on load/respawn/next.

## [0.2.0] - 2026-09-07
### Changed - 2D puzzle jumping game upgrade
- Fixed `WINDOW_WIDTH` / `WINDOW_HEIGHT` swap between `main.c` and `main.h`; window is now `960x640` with world `2400x640`.
- Replaced fixed-step `SDL_Delay(8)` + free-fly `WASD` movement with delta-time physics (`A/D` move, `SPACE/W/Up` jump).
- Fixed `coliderrect` typo (`rect1.y + rect1.y` -> `rect1.y + rect1.h`).

### Added
- Player physics: gravity (2300), jump velocity (-780), variable jump height, coyote-time (0.10s), jump buffer (0.12s), gravity-flip support.
- Level system (`header/level.h`, `src/level.c`, 3 levels): solids, one-way platforms, moving platforms (ride/carry), disappearing platforms (0.7s stand -> 2.2s respawn), spikes, pushable boxes with gravity, key + locked door/exit, hold-to-open button + gate, portal pairs, gravity-flip zones.
- Levels: 1/3 First Jumps (tutorial), 2/3 Boxes & Buttons, 3/3 Portals & Gravity.
- Game shell: follow camera, HUD via `SDL_RenderDebugText` (level name, hint, key state), states (play / dead-respawn 1s / door-open / win), shortcuts `R` retry, `N/Enter` next, `1/2/3` jump to level, `Esc` quit.
- Makefile: `-Wall -Wextra -g`, `-lm`, separate `run` target (no more auto-launch on build).

### Removed
- Old random obstacle system (`src/obstruc.c`); `header/obstruc.h` is now a compat shim over `level.h`.

### Controls
- `A/D` or arrows: move, `SPACE/W/Up`: jump, `R`: retry, `N`: next level, `1/2/3`: load level.
