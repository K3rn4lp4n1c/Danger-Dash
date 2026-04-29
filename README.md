# Danger Dash Assembly/C Game Project

A Danger Dash-inspired 2D runner being planned for an Assembly class project.

## Collaborators

- Oluwajuwon Adedowole
- Muhammad Essa

## Project Status

This project is currently in the planning and architecture stage. No gameplay code has been written yet.

The current direction is to build a simple 2D runner where C handles the systems-facing parts of the program, while assembly handles the core game logic and decisions.

## Current Vision

The game is planned as a 2D runner with a moving world, player actions, obstacles, collision handling, and score/state progression.

At a high level, the design goal is:

- **C** for setup, timing, input, rendering, and general system coordination
- **Assembly** for gameplay rules, movement decisions, collision logic, obstacle updates, and state transitions

This split keeps operating-system and library concerns in C while preserving the main logic in assembly, which is the most interesting part for the course.

## Current Architecture Plan

### 1. Main control stays in C

C will likely be responsible for:

- program startup and shutdown
- frame loop control
- timing and pacing
- input collection
- rendering / presentation
- passing shared state into assembly routines

### 2. Game logic lives in assembly

Assembly will likely be responsible for:

- player state updates
- movement rules
- gravity / jumping behavior
- obstacle movement and obstacle-related decisions
- collision detection
- score updates
- game-over and state-transition decisions

### 3. Shared state in memory

The current plan is to use a **shared state block in memory** that both C and assembly understand.

Important planning decision:

- persistent game data should live in **memory**, not in registers
- registers should only be used as temporary working space during computation

C can pass a pointer to the shared state into assembly. Assembly can then read fields, update them, and write the results back.

This is preferred over trying to dedicate registers to long-term game state, because registers are not stable shared storage across normal C code and function calls.

## Threading Direction

The original idea considered using multiple threads for input, world movement, obstacle generation, and decision-making.

The current plan is **not** to split the game across many threads.

Instead, the project will most likely use:

- a **single main game loop** in C
- possibly an **optional input thread later**, only if blocking input becomes a real problem

Why:

- fewer race conditions
- easier debugging
- simpler ownership of game state
- more deterministic behavior

For now, the safest assumption is that gameplay state updates should happen in a single coordinated flow.

## Rendering and Visual Assets

The current plan is to keep **visual assets and rendering information on the C side**.

Assembly should deal with gameplay meaning, such as:

- player position and velocity
- obstacle positions
- object types
- animation state IDs
- score and flags

C should deal with presentation, such as:

- loading assets
- mapping IDs to visuals
- drawing sprites, text, or terminal output
- displaying score / UI

In other words:

- assembly decides **what the world is**
- C decides **how the world looks**

## Provided Support Files

The repository currently includes a starter support setup that appears to be intended for 32-bit NASM projects linked with C.

### `template/driver.c`

Provides a C `main()` that calls an assembly entry point named `asm_main()`.

This means the assembly program is expected to expose:

```c
int asm_main(void);
```

### `template/template.asm`

A basic NASM starter file that defines `asm_main` and includes `asm_io.inc`.

### `template/asm_io.inc`

Assembly-side declarations and helper macros for support routines such as register or stack dumps.

### `template/asm_io.asm`

Assembly wrappers around C standard library functions such as `printf`, `scanf`, `getchar`, and `putchar`.

These wrappers demonstrate how assembly can interface with C functions by:

- preparing arguments
- pushing them onto the stack
- calling the external C routine
- cleaning up the stack afterward

### `template/asm_io.h`

A C-side header that exposes helper routines such as `print_int`, `print_string`, and related functions.

These helpers appear to use macros that move values into registers before calling the assembly wrapper.

### `template/cdecl.h`

Used to keep calling convention declarations consistent between C and assembly.

### `template/asm_io.o`

Prebuilt object file for the assembly I/O support layer.

### `template/Makefile`

A simple template build file.

### `newasm`

A project generator script that creates a new project directory, copies in a starter assembly file, and writes a Makefile that links:

- the project assembly object
- `driver.c`
- `asm_io.o`

This script currently targets a 32-bit build flow using NASM and GCC.

## What We Understand So Far About the Assembly/C Interface

The current support code suggests a standard 32-bit x86 interop pattern:

- arguments to C functions are pushed onto the stack by assembly
- integer / character return values come back in `EAX`
- wrapper functions sometimes save returned values into a local stack slot before restoring registers

For example:

- `read_int` uses local stack space because `scanf` writes the input into memory through a pointer
- `read_char` stores the C function's return value temporarily before restoring saved registers
- print functions do not need local stack storage, so they use a zero-byte local frame

## Notes on `enter` and `leave`

The starter assembly uses `enter` and `leave` for function setup and teardown.

Current understanding:

- `enter 4, 0` creates a stack frame and reserves 4 bytes of local storage
- `enter 0, 0` creates a stack frame with no local storage
- `leave` tears down the frame, roughly like restoring `esp` from `ebp` and then restoring the old `ebp`

This explains why the read routines reserve local storage while the print routines do not.

## Likely Early Design Priorities

These are the planning items that still need to be nailed down:

1. the exact shared game-state layout
2. what fields C may write directly
3. what fields assembly owns exclusively
4. how player movement will be represented
5. how obstacles will be represented
6. how collision will be modeled
7. whether the first version will be terminal-based or use a graphical 2D rendering library
8. what fixed tick/frame model the game will use

## Early Design Principles

For now, the project should follow these rules:

- keep ownership clear between C and assembly
- do not let both sides update the same gameplay logic in conflicting ways
- keep persistent state in memory
- use registers only for temporary work
- keep rendering concerns out of the assembly logic as much as possible
- avoid unnecessary threads

## Toolchain Notes

The provided files suggest the current environment expects:

- NASM
- GCC
- 32-bit compilation / linking support
- Linux-style build flow

The generated Makefile currently uses options including:

- `-m32`
- `-no-pie`
- `-znoexecstack`

So the project setup appears to assume a 32-bit x86 target.

## Next Steps

The next major planning step should be to define the shared state layout on paper before writing gameplay code.

That should include:

- player-related fields
- obstacle-related fields
- input flags
- score / game flags
- render-facing IDs or hints
- ownership rules for each part of the state

---

This README is a living document and will be updated as the architecture, file layout, and implementation become more concrete.
