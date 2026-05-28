# Thunder Fighter Refactoring and Migration Plan

## Goal

The long-term goal is to make Thunder Fighter easier to migrate to SDL, WebAssembly, and mobile platforms.

The current goal is not to add SDL yet.

The current goal is behavior-preserving refactoring only.

## Current Stage

Stage 1: Behavior-preserving cleanup.

In this stage, the game should remain a Windows terminal / FTXUI game.

No gameplay behavior should change.

## Current Restrictions

The current stage must not:

- Introduce SDL
- Introduce WebAssembly
- Introduce mobile input
- Add new gameplay
- Add new rendering style
- Change enemy behavior
- Change bullet behavior
- Change collision behavior
- Change score logic
- Change leaderboard logic
- Change menu flow
- Change key bindings

## Allowed Work

The current stage may:

- Split large source files
- Split large header files
- Extract type definitions
- Extract config constants
- Extract console input code
- Extract console rendering code
- Extract high score storage code
- Improve file organization
- Improve naming when behavior remains identical
- Update CMake to include newly split files

## Planned Stages

### Stage 0: Safety Baseline

- Create a refactor branch
- Tag the current working version
- Confirm the original project builds and runs

### Stage 1: Documentation and Rules

- Add `.clinerules`
- Add `MIGRATION_PLAN.md`
- Do not modify source code yet

### Stage 2: Extract Type Definitions

- Move existing enums and structs into `game_types.h`
- Do not change enum values or meanings

### Stage 3: Extract Config Constants

- Move repeated fixed values into `game_config.h`
- Do not change any numeric value

### Stage 4: Extract Console Input

- Move Windows keyboard input code into `input_console.h/.cpp`
- Keep key bindings unchanged

### Stage 5: Extract High Score Logic

- Move leaderboard file I/O into `highscore.h/.cpp`
- Keep `highscore.txt` behavior unchanged

### Stage 6: Extract Console Rendering

- Move terminal rendering code into `render_console.h/.cpp`
- Keep visual output unchanged

### Stage 7: Clean ThunderFighter Class Layout

- Reorder and group methods
- Split long methods only when behavior remains identical

### Stage 8: Prepare for Later Architecture Work

- Only after behavior-preserving cleanup is stable
- Start separating platform-dependent and platform-independent code

### Stage 9: Add SDL

- Not part of the current stage
- Add only after the existing logic is cleanly separated
