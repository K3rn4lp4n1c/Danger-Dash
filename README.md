# Danger Dash

Danger Dash is a 32-bit Linux terminal runner built as a hybrid C and NASM x86 assembly class project.

The project combines:

- **C** for runtime systems, ncurses rendering, threading, audio, scoring, persistence, and environment testing
- **Assembly** for entrypoint flow, command parsing, movement rules, and collision reads into shared game state

The goal of the project is to explore low-level systems programming, terminal game development, and C/Assembly interoperability in one codebase.

---

## Features

- 32-bit Linux target using **GCC** and **NASM**
- Terminal UI using **ncursesw**
- Hybrid C and Assembly architecture
- Side-scrolling runner gameplay
- HUD with score and player state display
- Character-based player selection
- Audio support through **miniaudio**
- Local score persistence
- Built-in runtime self-test mode
- GitHub Actions CI for dependency and runtime validation

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

The C runtime currently handles:

- ncurses initialization
- window creation and rendering
- game HUD drawing
- scrolling map updates
- audio initialization and playback
- thread creation and synchronization
- score tracking and ranking persistence
- runtime environment testing
- overall game lifecycle coordination

### Assembly responsibilities

The assembly runtime currently handles:

- `asm_main` entrypoint logic
- command-line dispatch
- help/version/test/players mode selection
- player argument parsing
- movement rule calculation
- collision reads into shared game memory
- bridging calls into C runtime functions

### Interop model

`src/driver.c` provides a C `main()` that forwards control to:

```c
int asm_main(int argc, char* argv[]);
````

From there, Assembly decides which mode to run and calls into C through wrapper functions exposed in `src/asm_io.asm`.

---

## Current Modes

### Default run

Starts the game with a default single player profile.

```bash
./game.out
```

### Help

```bash
./game.out help
```

### Version

```bash
./game.out version
```

### Runtime test

Runs the internal self-test used by CI and manual environment checking.

```bash
./game.out test
```

### Players mode

Starts a game with explicit player definitions.

```bash
./game.out players 2 "Alice:B" "Bob:E"
```

Supported character tags:

* `B` = Benjamin
* `E` = Ethan
* `M` = Muhammad
* `Y` = Youssef

---

## Gameplay Summary

Danger Dash is a side-scrolling terminal runner. The world scrolls horizontally while the player avoids obstacles and survives as long as possible to increase score.

The display is split into three ncurses windows:

* **Status window** for score, state, and frame information
* **Game window** for the scrolling playfield
* **Info window** for players, controls, and rankings

The C runtime controls drawing, timing, and presentation. The Assembly routines currently participate in movement and collision-related logic.

---

## Controls

### Start screen

* `Space` to start
* `q` to quit

### In game

* `Arrow keys` to move
* `Space` maps to jump or upward movement
* `Backspace` to pause or resume
* `ESC` to end the run

### Experimental multiplayer input routing

The current code contains in-progress multi-input routing:

* Player 1: Arrow keys
* Player 3: `I` through `P`
* Player 4: numeric keys
* Player 2: fallback route for other non-arrow input

This logic exists in the implementation, but multiplayer controls should still be considered experimental.

---

## Build Requirements

This project targets **32-bit Linux** and requires multilib support.

### Packages installed by `make install`

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

```bash
make install
```

### Build development binary

```bash
make
```

Output:

```text
game.out
```

### Build production-named binary

```bash
make PROD=true
```

Output:

```text
danger-dash
```

### Clean build artifacts

```bash
make clean
```

---

## Test Instructions

### Makefile test target

```bash
make test
```

The current `test` target validates:

* the target binary exists
* the target binary is executable
* the binary is a **32-bit ELF**
* the binary is **dynamically linked**
* shared runtime libraries resolve through `ldd`
* the program's internal runtime test passes

### Manual runtime test

```bash
./game.out test
```

This test is intended to verify that the environment has what the game needs to run, such as terminal assumptions, library availability, and required runtime assets.

If the game fails to start correctly in a given environment, run the runtime test first.

---

## CI Overview

The GitHub Actions workflow currently does the following:

1. Checks out the repository
2. Installs required build packages with `make install`
3. Runs `make test`

This gives the project an automated verification path for dependency setup, binary format validation, runtime library resolution, and the internal self-test.

---

## Project Structure

```text
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

GitHub Actions workflow that installs dependencies and runs `make test`.

### `src/driver.c`

Minimal C entrypoint that forwards program execution into Assembly.

### `src/game.asm`

Primary assembly game control file. Handles:

* command-line parsing
* mode dispatch
* default launch behavior
* help, version, test, and players modes
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

## Runtime Data

The game currently persists ranking data in:

* `danger_dash.bin`

There is no separate runtime log file documented by the current code.

---

## Audio Assets

The current code expects these runtime assets.

### Music

* `assets/benjamin.mp3`
* `assets/ethan.mp3`
* `assets/muhammad.mp3`
* `assets/test.wav`

### Sound effects

* `assets/victory.mp3`
* `assets/smokeweed.mp3`
* `assets/kaboom.mp3`
* `assets/allahuakbar.mp3`

These files should be present relative to the executable's working directory when the game is run.

---

## Core Data Structures

### `Player`

Stores:

* name
* x and y position
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

The C runtime is responsible for:

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
* a working file layout for a hybrid C/Assembly game project

---

## Known Limitations

The current codebase is functional, but still has a few rough edges:

* **Linux-only and 32-bit only.** The project is tightly tied to a 32-bit Linux toolchain and runtime environment.
* **Dynamic runtime dependencies are required.** The current build is dynamically linked, so the target system must have compatible runtime libraries available.
* **The runtime test is environment-aware, not a full gameplay integration test.** It is meant to verify that the environment is suitable for running the game, not to prove every gameplay path is correct.
* **The assembly player parser still uses fixed-size buffers for four players.** That matches the project limit, but it keeps the parsing logic tightly coupled to that fixed maximum.
* **Assembly/C layout coupling is brittle.** Collision logic in Assembly depends on hard-coded struct offsets from the C side, so changing struct layouts can break the assembly routine.
* **Multiplayer controls are still experimental.** The routing logic exists, but the input scheme is not yet polished.
* **Threading is not fully hardened yet.** The current runtime uses an input thread and detached player-effect threads, which makes cleanup and shutdown more delicate than a single-threaded design.
* **Obstacle logic is still prototype-level.** The current obstacle generation works, but it is still simple and may not represent the final gameplay design.

---

## Collaborators

* Oluwajuwon Adedowole
* Muhammad Essa

---

## License

This project is licensed under the **MIT License**. See the [`LICENSE`](./LICENSE) file for details.