# Engine-Chili2.0
Game Engine from Scratch

Current targets:

- `engine_core` - reusable engine library
- `engine_sandbox` - active sandbox app under `apps/sandbox/`
- `engine_studio` - native studio host with an embedded WebView2 CoreTools surface

Current state:

- `engine_sandbox` is now a thin harness under `apps/sandbox/src/`
- the front-facing app architecture is being iterated through real game trials, currently using `apps/pong`, `apps/space_invaders`, and `apps/_template` to pressure-test what app authors and players see and do
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

Run instructions:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G Ninja
cmake --build build
```

Codex note:

- In this repo, direct CMake commands are the reliable path.
- Codex may need to run `cmake -S . -B build -G Ninja` and `cmake --build build` outside the sandbox because CMake try-compile/build subprocesses can stall or fail under sandboxed execution.

GitHub build note:

- the GitHub Actions Windows build should use the Visual Studio/MSVC toolchain with Ninja
- this repo is heavily Win32/DX11/WebView2-oriented, so MSVC is the intended CI compiler lane rather than MSYS2/MinGW

Sanitizer build:

```powershell
Remove-Item -Recurse -Force build\sanitize -ErrorAction SilentlyContinue
cmake -S . -B build\sanitize -G Ninja -DENABLE_SANITIZERS=ON
cmake --build build\sanitize
```

Studio status:

- `engine_studio` keeps the existing native engine window and hosts an embedded WebView2 sidebar
- The first embedded tool surface lives under `apps/studio/coretools`
- The current milestone is Windows-only and focused on a fixed left-docked CoreTools surface
- Future HTTP and WebSocket transport code remains under `apps/studio/src/transport`, but it is not the active runtime path for this milestone

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

See [docs/README.md](docs/README.md) for the current architecture, feature list, and API inventory.
