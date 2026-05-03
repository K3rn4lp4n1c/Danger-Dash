# Danger Dash

Danger Dash is a 32-bit Linux terminal runner built as a hybrid **C + NASM x86 assembly** project for class. The game combines:

- **C** for runtime systems, ncurses rendering, threading, audio, scoring, and environment tests
- **Assembly** for entrypoint control flow, command parsing, movement rules, and collision access into shared game state

The project is designed to explore low-level systems programming, terminal game development, and C/Assembly interoperability in one codebase.

---

## Features

- 32-bit Linux target using **GCC** and **NASM**
- Terminal-based UI using **ncursesw**
- Hybrid C and Assembly architecture
- Character-based runner gameplay
- Moving obstacle map
- HUD with score and player state display
- Audio support through **miniaudio**
- Local score persistence
- Built-in runtime self-test mode
- GitHub Actions CI for dependency, binary, and runtime validation

---

## Tech Stack

- **Languages:** C, NASM x86 Assembly
- **Platform:** Linux
- **Architecture target:** 32-bit ELF
- **UI library:** ncursesw
- **Audio library:** miniaudio
- **Build system:** Make
- **CI:** GitHub Actions

---

## Architecture Overview

### C responsibilities
The C runtime handles:

- ncurses initialization
- window creation and rendering
- game HUD drawing
- obstacle map rendering
- audio setup and playback
- thread creation and synchronization
- score tracking and persistence
- runtime environment testing
- overall game lifecycle coordination

### Assembly responsibilities
The assembly runtime handles:

- `asm_main` entrypoint logic
- command-line dispatch
- player argument parsing
- movement rule calculation
- collision reads into game memory
- bridging calls into C runtime functions

### Interop model
`src/driver.c` provides a standard C `main()` that forwards control to:

```c
int asm_main(int argc, char* argv[]);
````

From there, Assembly decides which mode to run and calls into C through wrappers exposed in `src/asm_io.asm`.

---

## Current Modes

### Default run

Starts the game with a default player profile.

```bash id="2i3gp2"
./game.out
```

### Help

```bash id="6g1ji3"
./game.out help
```

### Version

```bash id="diy0gn"
./game.out version
```

### Runtime test

Runs the internal self-test used by CI and manual environment checking.

```bash id="ppv2bz"
./game.out test
```

### Players mode

Starts a game with explicit player definitions.

```bash id="4d4p8h"
./game.out players 2 "Alice:B" "Bob:E"
```

Supported character tags:

* `B` = Benjamin
* `E` = Ethan
* `M` = Muhammad
* `Y` = Youssef

---

## Gameplay Summary

Danger Dash is a side-scrolling terminal runner. The game world moves horizontally while the player dodges incoming obstacles and survives as long as possible to increase score.

The C runtime controls rendering and timing. The Assembly routines currently participate in movement and collision-related decision flow.

The display is divided into three ncurses windows:

* **Status window** for score, state, and frame information
* **Game window** for the scrolling playfield
* **Info window** for players, controls, and rankings

---

## Controls

### Start screen

* `Space` to start
* `q` to quit

### In game

* `Arrow keys` to move
* `Space` maps to jump/upward movement
* `Backspace` to pause/resume
* `ESC` to end the run

### Experimental multiplayer input routing

The current code contains in-progress multi-input routing:

* Player 1: Arrow keys
* Player 3: `I` through `P`
* Player 4: numeric keys
* Player 2: fallback route for remaining non-arrow input

This logic exists in the implementation, but the multiplayer input design is still experimental.

---

## Build Requirements

This project targets **32-bit Linux** and requires multilib support.

### Packages used by `make install`

* nasm
* gcc
* make
* gcc-multilib
* libc6-dev-i386
* lib32gcc-s1
* lib32ncurses-dev
* libasound2t64:i386
* libpulse0:i386

---

## Build Instructions

### Install dependencies

```bash id="v2l3vt"
make install
```

### Build development binary

```bash id="7qbug4"
make
```

Output:

```text id="9n3t3l"
game.out
```

### Build production-named binary

```bash id="b90mzx"
make PROD=true
```

Output:

```text id="mis494"
danger-dash
```

### Clean build artifacts

```bash id="rtm8vl"
make clean
```

---

## Test Instructions

### Makefile test target

```bash id="o7mn5l"
make test
```

The current `test` target validates:

* the target binary exists
* the target binary is executable
* the binary is a **32-bit ELF**
* the binary is **dynamically linked**
* shared runtime libraries resolve through `ldd`
* the program’s internal runtime test passes

### Manual runtime test

```bash id="ho85ry"
./game.out test
```

This test is intended to verify that the environment has what the game needs to run, such as terminal assumptions, library availability, and required runtime assets.

---

## CI Overview

The GitHub Actions workflow currently:

1. Checks out the repository
2. Installs required build packages
3. Runs `make test`
4. Builds the project

This gives the project an automated verification path for binary format, runtime dependency resolution, and the internal self-test.

---

## Project Structure

```text id="v7u9ke"
Danger-Dash/
├── .github/
│   └── workflows/
│       └── testgame.yml
├── assets/
│   └── audio assets used by the game
├── inc/
│   ├── asm_io.inc
│   ├── cdecl.h
│   ├── game.h
│   └── miniaudio.h
├── src/
│   ├── asm_io.asm
│   ├── driver.c
│   ├── game.asm
│   └── game.c
├── LICENSE
├── Makefile
└── README.md
```

---

## File Guide

### `Makefile`

Builds either the development binary or the production-named binary, installs the 32-bit dependency set, and runs the runtime-aware `test` target.

### `.github/workflows/testgame.yml`

GitHub Actions workflow that installs dependencies, runs `make test`, and builds the game.

### `src/driver.c`

Minimal C entrypoint that forwards program execution into Assembly.

### `src/game.asm`

Primary assembly game control file. Handles:

* command-line parsing
* mode dispatch
* default launch behavior
* test mode dispatch
* player parsing
* movement rule logic
* collision routine

### `src/asm_io.asm`

Assembly bridge and utility layer. Provides:

* wrappers for C stdio
* wrappers for game lifecycle functions
* runtime glue from NASM into C
* debug dump helper routines

### `src/game.c`

Main C runtime implementation. Handles:

* curses setup
* rendering
* player and HUD display
* scrolling map updates
* input thread runtime
* audio support
* runtime test logic
* score and ranking behavior

### `inc/game.h`

Core game declarations, enums, structs, asset paths, constants, and function prototypes.

### `inc/cdecl.h`

Calling-convention compatibility helpers for C and Assembly interop.

### `inc/asm_io.inc`

NASM include file exposing bridge functions and dump helper macros.

### `inc/miniaudio.h`

Bundled miniaudio dependency for runtime audio support.

### `LICENSE`

MIT license for the project.

---

## Runtime Files

The game uses local files during runtime:

* `danger_dash.log` for log output
* `danger_dash.bin` for ranking persistence

---

## Audio Assets

The current code expects these runtime assets.

### Music

* `assets/benjamin.mp3`
* `assets/ethan.mp3`
* `assets/muhammad.mp3`
* `assets/test.wav`

### Sound Effects

* `assets/victory.mp3`
* `assets/smokeweed.mp3`
* `assets/kaboom.mp3`
* `assets/allahuakbar.mp3`

These files should be present relative to the executable’s working directory when the game is run.

---

## Core Data Structures

### `Player`

Stores:

* name
* x / y position
* selected character
* current state
* mutex
* worker thread handle

### `Environment`

Stores:

* status window
* game window
* info window
* map buffer
* frame rate

### `Game`

Stores:

* player count
* score
* environment pointer
* game state
* input thread
* global lock
* player pointers
* audio state
* predecessor ranking records

### `Record`

Stores:

* player name
* score
* character selection

---

## Rendering Model

The playfield scrolls by shifting the backing map buffer left each update. Obstacles are inserted at the right edge, and players are drawn on top using wide-character glyphs.

The C runtime is currently responsible for:

* panel borders
* score display
* player info display
* map painting
* character rendering
* full-screen refresh sequencing

---

## Audio Model

Audio is handled with **miniaudio**.

The runtime currently initializes an engine, loads character-based music for the active player, and can trigger sound effects during end-of-run handling.

---

## Status

This repository is no longer just a planning skeleton. It currently contains:

* implemented game runtime code
* implemented assembly entry and movement logic
* a 32-bit Linux build path
* CI testing
* a runtime self-test path
* working file layout for a hybrid C/Assembly game project

---

## Known Limitations

The latest codebase is functional but still has a few important rough edges:

* **Linux-only and 32-bit only.** The project is tightly tied to a 32-bit Linux toolchain and runtime environment.
* **Dynamic runtime dependencies are required.** The current build is dynamically linked, so the target system must have compatible runtime libraries available.
* **The runtime test is environment-aware, not a full gameplay integration test.** It is meant to verify that the environment is suitable for running the game, not to prove every gameplay path is correct.
* **The assembly CLI parser is still limited.** It currently treats player count as a single-character numeric argument and stores player data in fixed-size assembly buffers.
* **Assembly/C layout coupling is brittle.** Collision code in Assembly depends on hard-coded struct offsets from the C side, so changing struct layouts can break the assembly logic.
* **Multiplayer controls are still experimental.** The routing logic exists, but the input scheme is not yet polished.
* **Threading is not fully hardened yet.** The current runtime uses an input thread and detached player-effect threads, which makes cleanup and shutdown more delicate than a single-threaded design.
* **Documentation and metadata still need some cleanup.** Version strings and a few developer-facing details may still need alignment across files.
* **Obstacle logic is simple and still evolving.** The obstacle generation path works as a prototype and may not yet reflect the final gameplay design.

---

## Collaborators

* Oluwajuwon Adedowole
* Muhammad Essa

---

## License

This project is licensed under the **MIT License**. See the [`LICENSE`](./LICENSE) file for details.