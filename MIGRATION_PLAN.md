# Thunder Fighter Refactoring and Migration Plan

## Goal

The long-term goal is to make Thunder Fighter easier to migrate to SDL, WebAssembly, and mobile platforms.

## Current State

Stage 2 (logic / platform separation) is **complete**.

- The terminal frontend still runs and keeps its original look and rules.
- A new SDL3 frontend shares the same game logic; it is a minimal playable build.
- Gameplay rules (score, collision, life, enemy behaviour, leaderboard) are unchanged.
- Two intentional behaviour changes were made and committed separately:
  pause is now edge-triggered, and `Esc` quits during play (matching the README).

### What Stage 2 produced

- `thunder_logic`: `GameWorld`, `GameSession`, `InputState` / `InputTracker`,
  `WorldSnapshot` / `SessionSnapshot`, `HighScore`, `ScoreStore` interface
- `thunder_io`: `FileScoreStore` (the only file IO implementation)
- `cell_grid` belongs to the terminal layer; the SDL frontend draws from snapshots directly
- Terminal frontend uses a single non-blocking loop driven by `GameSession`
- `GameSession` owns the only `InputTracker`; pause never calls `GameWorld::Tick`
- GoogleTest regression tests plus an isolation check for the logic layer
- SDL3 frontend: minimal playable build with a fixed 16 ms logic step

## Completed Stages

### Stage 0: Safety Baseline

- Refactor branch `refactor/logic-separation`
- Fresh CMake configure, baseline menu capture (`docx/baseline`)

### Stage 1: Documentation and Rules

- `.clinerules`, `MIGRATION_PLAN.md`, `docx/stage2_plan.md`

### Stage 2: Types, Config, Input, Storage, Rendering

- Types and constants moved into `inc/game/`
- Input isolated in `input_console` / `InputState`
- High score file IO isolated behind `ScoreStore`
- Terminal rendering split into `cell_grid` + `render_console`

### Stage 3: Logic Extraction

- `GameWorld` owns all mutable game state, `Tick(input, dt)` takes injected time
- `GameSession` owns the non-blocking screen flow and the `InputTracker`
- Terminal menus no longer block; the SDL frontend reuses the same session

### Stage 4: SDL Frontend

- `Thunder_fighter_jet_sdl` target, optional at configure time
- Fixed 16 ms logic step, rendering decoupled from display refresh
- Text via a built-in 5x7 bitmap font (English ASCII), no font library dependency

## Next Steps (not part of Stage 2)

- SDL3 is now installed through vcpkg (classic mode): `vcpkg install sdl3:x64-mingw-static`
- vcpkg Manifest Mode is still not adopted: vcpkg's own tool bootstrap (ninja / 7-Zip
  downloads through WinHTTP) is unstable on the development machine. Revisit when that
  download path is reliable; the project already works with classic mode.
- SDL frontend polish: sprites are in place; audio, menu animations and leaderboard
  parity checks are still open
- Separate platform services further if more frontends are added

## Notes for Future Work

- Keep the logic layer free of `windows.h`, FTXUI, SDL, `std::cout`, file IO and system clock
  (`tests/check_isolation.cmake` enforces this)
- Keep gameplay rules covered by tests before changing them
- Behaviour changes must be separate commits with their own tests
