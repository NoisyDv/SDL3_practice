# Changelog

## Unreleased

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
