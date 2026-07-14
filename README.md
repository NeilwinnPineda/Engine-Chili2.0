# Engine-Chili2.0
Game Engine from Scratch

Current targets:

- `EngineRuntime` - shared library target that outputs `engine.dll`
- `PongRuntime` - Pong-owned shared library target under `apps/pong` that outputs `pong_runtime.dll`
- `Studio` - native studio host executable, output as `engine_studio.exe`
- `PongPreview` - Pong-owned thin executable under `apps/pong` that loads `pong_runtime.dll`
- `HotBuildTool` - standalone external tool stub under `tools/hotbuild`

Current state:

- the active revival plan is documented in `docs/engine/REVIVAL_PLAN.md`
- the current engine/studio/runtime/API seam map is documented in `docs/engine/API_MAP.md`
- a first CTest contract suite now covers scene/world behavior, Pong simulation, runtime loading failure handling, input priority/consumption, Studio command permissions, Studio project artifact-state reporting, and extracted Studio build/workspace request services
- Studio's localhost bridge exposes validated read-only inspection commands over a versioned envelope
- `tools/ai_bridge/studio_mcp.py` provides a dependency-free, read-only MCP adapter with manifest/capability validation, project artifact-state inspection coverage, and audit logging; no model client is embedded in the application
- project templates now generate a runtime DLL plus thin preview host, and Studio can load that runtime artifact into its viewport
- `StudioCommandQueryService`, `StudioBuildRequestService`, and `StudioWorkspaceRequestService` now hold the first extracted Studio-side command/build/workspace route seams instead of leaving that orchestration entirely inside `StudioHost`
- generic WebSocket transport and write-capable AI tools remain intentionally disabled

- the old sandbox executable is no longer part of the top-level build; rapid Pong testing now moves through `apps/pong`'s `PongPreview`
- the front-facing app architecture is being iterated through real game trials, currently using `apps/pong`, future game runtime DLLs, and `apps/_template` to pressure-test what app authors and players see and do
- older one-off sandbox variants have been stripped down; `hex_observation` remains in archive while its debug types/logic migrate into `src/modules/diagnostics/`
- frame submission now flows through `FramePrototype` from app-facing code into the renderer/GPU path
- prototype families now live under:
  - `src/prototypes/presentation/`
  - `src/prototypes/entity/appearance/`
  - `src/prototypes/entity/geometry/`
  - `src/prototypes/entity/object/`
  - `src/prototypes/entity/scene/`
  - `src/prototypes/math/`
- `RenderModule` now translates prototype requests into render-owned `RenderFrameData`
- `GpuModule` and backend internals now consume render-owned data instead of prototype structs directly
- the DX11 backend now realizes submitted frame contents into visible geometry
- the active DX11 sandbox path now supports:
  - point-light shading
  - cubemap shadow scaffolding for the primary point light
  - albedo texture sampling on built-in meshes
  - prototype-driven material tint blending
- `ViewPrototype` can carry light-ray prototypes into render-owned frame data, but that is not the active sandbox lane
- in-window overlay text is currently a separate Win32 paint path, not renderer-owned text

Architecture rule:

- prototype definition:
  - in this engine, a prototype is a reusable construction object, not a passive data-only throwaway
  - prototypes may include methods, lifecycle hooks, and chaining/composition logic
  - prototypes exist to implement reusable construction behavior once and reuse it later
- shared engine-side prototypes and built-in material defaults must stay generic
- sandbox experiments, look-dev tweaks, and scene-specific authoring belong in sandbox-owned configuration or dedicated sandbox-specific prototype variants
- app-side experimentation should not silently rewrite shared engine defaults just to prove a feature path

Architecture snapshot:

- `PlatformModule` owns the OS window and render surface
- `GpuModule` owns the backend, device-facing presentation path, and generic GPU resources
- `RenderModule` owns render submission flow, prototype-to-render translation, and frame orchestration
- `ResourceModule` owns engine-facing asset/resource state
- `JobModule` owns worker execution
- `MemoryModule` owns allocation policy and tracking

Render path snapshot:

- sandbox/app code builds a `FramePrototype`
- `EngineCore` forwards it to `RenderModule`
- `RenderModule` translates it into render-owned `RenderFrameData`
- `RenderModule` submits that through `GpuModule`
- the DX11 backend uploads and draws submitted frame items from render-owned data

Build instructions:

Preferred local commands:

```powershell
.\configure.cmd
.\build.cmd
.\build.cmd studio
.\build.cmd pong
.\build.cmd engine
.\test.cmd
```

Target aliases:

- `studio` builds the `Studio` target and outputs `build/bin/studio/engine_studio.exe`
- `pong` builds `PongPreview` plus the Pong runtime DLL outputs under `build/bin/apps/pong`
- `engine` builds the reusable `EngineRuntime` target and outputs `build/bin/engine/engine.dll`
- no target argument builds the default CMake all target

Agent note:

- In Codex or similar sandboxed agents, run `configure.cmd`, `build.cmd`, direct `cmake`, and Ninja commands only with escalated execution.
- The wrapper commands write logs to `logs/cmake-configure.log` and `logs/cmake-build.log`.
- The test wrapper writes failures and results to `logs/ctest.log`.

Build lanes note:

- the repository currently supports multiple build lanes (CI, Codex, Claude, and human operator flows) while build policy is being converged
- see `docs/build/BUILD_LANES.md` for the current lane definitions and the unified-builder convergence target
- Studio build flows are currently transitional clients and should eventually delegate to the same unified builder contract

Build direction:

- the project is moving away from hot-building monolithic executables as the normal workflow
- the target runtime shape is:
  - launcher `.exe`
  - `engine.dll`
  - app/editor/game DLLs loaded after the engine
- the launcher should stay thin: process setup, DLL loading, and handoff only
- `engine.dll` should own reusable engine systems and stable module boundaries
- app/tool DLLs should own project-specific runtime/editor behavior without forcing the launcher or engine core to relink for every iteration
- future build work should prefer DLL-safe boundaries, explicit exported entry points, and reload-friendly ownership over direct executable coupling

Underlying CMake equivalent for the wrapper lane:

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

Use the wrapper scripts above as the canonical operator and CI entry points.
Do not treat the raw CMake pair as a separate primary workflow.

Current output layout:

```txt
build/bin/engine/engine.dll
build/bin/studio/engine_studio.exe
build/bin/apps/pong/PongPreview.exe
build/bin/apps/pong/pong_runtime.dll
build/bin/tools/hotbuild/HotBuildTool.exe
```

Codex note:

- In this repo, build commands must be run outside the normal CLI sandbox.
- Codex may need to run the wrapper lane outside the sandbox because CMake try-compile/build subprocesses can stall or fail under sandboxed execution.
- As the DLL build model lands, documentation and code changes should keep the launcher/engine/app-DLL split explicit instead of folding new behavior into one executable target.

GitHub build note:

- the GitHub Actions Windows build is pinned to the VS2022 x64 image and uses MSVC with Ninja
- this repo is heavily Win32/DX11/WebView2-oriented, so MSVC is the intended CI compiler lane rather than MSYS2/MinGW

Sanitizer note:

- no sanitizer-specific wrapper lane is documented as a stable repo contract yet
- if sanitizer support is revived, document the wrapper-backed flow before advertising it as supported

Studio status:

- `Studio` outputs `engine_studio.exe`, keeps the existing native engine window, and hosts an embedded WebView2 sidebar
- The first embedded tool surface lives under `apps/studio/coretools`
- The current milestone is Windows-only and focused on a fixed left-docked CoreTools surface
- the localhost HTTP transport is active for embedded tools and the read-only command ingress; WebSocket remains future scaffolding
- embedded panels use a per-session `HttpOnly`, `SameSite=Strict` cookie; cross-site requests cannot call normal Studio routes
- the external AI ingress remains read-only and is constrained by a fixed command allowlist

AI bridge:

```powershell
python tools/ai_bridge/studio_mcp.py
```

See `tools/ai_bridge/README.md` for MCP client configuration and the current
read-only tool list. Tool calls are audited under
`User/studio/logs/ai_bridge_audit.jsonl`.

Sandbox status:

- the active sandbox is a DX lighting/material lab under `apps/sandbox/`
- the sandbox app configures a lighting-lab scene preset, camera, and one primary point light
- the current sandbox scene is a meter-based room with a rotating cube, stucco-backed material prototypes, and live exposure/light controls
- current visible material support is:
  - albedo texture sampling
  - prototype color/tint blending over albedo
  - roughness/specular controls lowered into the DX11 shader path
- current shadow support is:
  - first-pass cubemap shadow generation for the primary point light
  - basic shadow sampling in the scene shader
- normal and height maps are declared in material prototypes but are not yet part of final shading

See [docs/README.md](docs/README.md) for the documentation map and
[docs/engine/REVIVAL_PLAN.md](docs/engine/REVIVAL_PLAN.md) for the active execution order.
