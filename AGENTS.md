# Codex Instructions

## Build And Configure Commands

Do not run CMake, Ninja, configure, build, or test commands in the normal CLI sandbox for this repository.

Always request escalated execution before running commands such as:

- `cmake -S . -B build -G Ninja`
- `cmake --build build`
- `cmake --build build --target ...`
- `configure.cmd`
- `build.cmd`

Reason: this Windows/DX-oriented repo can stall or behave incorrectly when CMake/Ninja build subprocesses run inside the CLI sandbox.

Inspection commands such as `rg`, `Get-Content`, `git status`, and file listing commands are fine in the sandbox. Build/configure/test commands are not.

## Current Work Direction

The engine architecture is being fleshed out through real game trials. Treat `apps/pong`, `apps/space_invaders`, and `apps/_template` as front-facing pressure tests for what app authors and players actually see and do.

When game work exposes missing capabilities, awkward APIs, or presentation gaps, prefer improving the architecture in small reusable steps instead of hiding those issues inside one app.
