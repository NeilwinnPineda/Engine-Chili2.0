# Studio Documentation

## Overview

`engine_studio` is now a native-window-hosted editor shell for Engine-Chili2.0.
The studio keeps the existing native engine window and now creates CoreTools
through the engine's reusable web dialog API.

Current architecture:

```text
engine_studio.exe
|-- native studio window
|-- native engine/main area
`-- left-docked engine-owned web dialog for CoreTools
```

This milestone is Windows-only and intentionally focused:

- the native outer window stays authoritative
- `CoreTools` is the first-party embedded tool surface
- the left dock is fixed-width for now
- transport and command scaffolding remain in the repo, but they are not the runtime center of this milestone
- Studio now proves the engine-level web dialog feature instead of owning its own WebView implementation

## Current Structure

```text
apps/studio
|-- CMakeLists.txt
|-- README.md
|-- config/
|-- coretools/
|   |-- angular/
|   |   |-- README.md
|   |   |-- src/
|   |   |   |-- app/
|   |   |   `-- assets/
|   |   `-- public/
|   |-- runtime/
|   |   |-- top-bar/
|   |   |-- left-bar/
|   |   |-- right-bar/
|   |   |-- bottom-bar/
|   |   |-- dialogs/
|   |   `-- shared/
|   `-- README.md
|-- runtime_test/
`-- src/
    |-- main.cpp
    |-- studio_app.hpp
    |-- studio_app.cpp
    |-- studio_host.hpp
    |-- studio_host.cpp
    |-- bridge/
    |   |-- engine_bridge.hpp
    |   `-- engine_bridge.cpp
    |-- commands/
    |   |-- command_policy.hpp
    |   |-- command_policy.cpp
    |   |-- command_protocol.hpp
    |   |-- command_protocol.cpp
    |   |-- command_router.hpp
    |   `-- command_router.cpp
    |-- transport/
    |   |-- http_server.hpp
    |   |-- http_server.cpp
    |   |-- websocket_server.hpp
    |   `-- websocket_server.cpp
```

## Ownership

### `StudioApp`

- owns `StudioHost`
- app lifecycle only

### `StudioHost`

- owns `EngineBridge`
- keeps future-facing transport pieces secondary
- creates CoreTools through the engine-owned web dialog API
- keeps future-facing transport pieces secondary
- runs the studio loop while the engine itself owns dialog layout updates

### `EngineBridge`

- remains the native engine-facing authority
- initializes, ticks, and shuts down `EngineCore`
- exposes the local CoreTools content path and access to the underlying `EngineCore`

### `CommandRouter`

- provides the versioned command-envelope ingress
- classifies read-only and mutating commands through `command_policy`
- denies mutating commands unless the host explicitly enables them
- is available over `/studio/bridge/command?message=<url-encoded-json>`

### `StudioCommandQueryService`

- owns the read-only workspace/project/entity/runtime query assembly used by the command router
- keeps owner-backed inspection JSON out of `StudioHost`
- is the current extraction seam between transport/routing and Studio-layer owners

### `StudioBuildRequestService`

- owns the Build App / Preview App route orchestration that was previously inlined in `StudioHost`
- keeps build/export request handling, console narration, and recorded artifact-state updates in one Studio-layer seam
- leaves process execution and packaging mechanics in `StudioBuildSystem`

### `StudioWorkspaceRequestService`

- owns workspace layout save and visibility-toggle route orchestration that was previously inlined in `StudioHost`
- keeps project-session save behavior and workspace toggle messaging in one Studio-layer seam
- leaves raw panel/UI implementation details with the existing panel owners

### `transport/`

- HTTP is active on `127.0.0.1:37620` for first-party embedded tools and the command ingress
- when HTTP starts successfully, Studio WebViews navigate to that loopback origin so their API calls remain same-origin; local-file navigation is the display-only fallback
- HTTP validates Host/Origin/fetch-site metadata, limits request headers, and does not grant wildcard CORS
- WebSocket remains inactive scaffolding

### Automation / AI Boundary

The current bridge is deliberately read-first. Protocol version `1` accepts:

- `hello`
- `ping`
- `get_status`
- `list_capabilities`
- `get_workspace_status`
- `get_project_status`
- `list_entities`
- `get_selected_entity`
- `get_runtime_status`

`exit` is classified as mutating and denied by default. The read-only MCP
adapter under `tools/ai_bridge` exposes only the inspection subset and cannot
forward arbitrary commands. There is no model-provider client or write-capable
AI tool layer yet.

When Studio publishes structured command descriptors, it now includes the
bridge-facing MCP tool names for the read-only inspection subset so the
external adapter can verify that its manifest still matches Studio's declared
contract.

`/studio/bridge/command` itself is permanently forced through read-only command
handling even if Studio later enables mutation authority on other internal
paths.

Current MCP inspection tools include:

- `studio_status`
- `studio_capabilities`
- `workspace_status`
- `project_status`
- `list_entities`
- `selected_entity`
- `runtime_status`

Normal embedded routes require the random session cookie issued with the
first-party Studio page. The external command endpoint is exempt from that
cookie only because its command policy cannot authorize mutations.

## Runtime Behavior

Current startup path:

1. `main()` creates `StudioApp`
2. `StudioApp::Initialize()` initializes `StudioHost`
3. `StudioHost::Initialize()` initializes `EngineBridge`
4. `EngineBridge` initializes `EngineCore`
5. `StudioHost` creates docked web dialogs through `EngineCore`
6. the engine `WebViewModule` creates and owns the WebView2 host surface
7. the WebView2 surfaces load named CoreTools runtime files such as `top-bar/top-bar.html` and `left-bar/left-bar.html`
8. `StudioHost::Run()` keeps ticking the native engine while the engine updates docked dialog layout

Temporary stop paths:

- close the native window
- press `Escape`

## Initial Studio Interaction Controls (Current Milestone)

Studio interaction scope is intentionally minimal for this phase. The goal is to lock in stable viewport navigation and stable selection before adding advanced editor tooling.

### Camera Controls

| Action | Binding |
|--------|----------|
| Orbit Camera | `Alt + LMB Drag` |
| Pan Camera | `MMB Drag` |
| Zoom Camera | `Mouse Wheel` |
| Fly Camera | `Hold RMB` |
| Fly Forward | `W` |
| Fly Backward | `S` |
| Fly Left | `A` |
| Fly Right | `D` |
| Fly Up | `E` |
| Fly Down | `Q` |
| Fly Speed Boost | `Shift` |
| Focus Selection | `F` |

### Selection Controls

| Action | Binding |
|--------|----------|
| Select Object | `LMB` |
| Multi-select | `Shift + LMB` |
| Clear Selection | `LMB` on empty space |

### Behavior Rules

- Selection:
  - clicking an object selects it
  - centralized selection state updates
  - inspector selection data updates
  - visible selection highlight updates
- Empty-space click:
  - selection clears
  - highlight clears
  - inspector selection clears
- Orbit focus target priority:
  - selected object
  - hovered object
  - last focus point
  - world origin only if none of the above are valid
- Fly camera mode:
  - hold-to-use only while `RMB` is held
  - release `RMB` to exit immediately
  - no persistent fly toggle mode
- Camera feel:
  - responsive and direct
  - no cinematic smoothing, acceleration curves, inertia, or lag

### Architecture Rule

Viewport interaction must route through centralized interaction state/control. UI, viewport, selection, and inspector must not keep duplicate interaction state.

### Out Of Scope For This Milestone

Do not implement yet:

- transform tools
- snapping
- advanced inference
- procedural manipulation
- animation controls
- editor modes beyond navigation and selection
- complex gizmo systems

## Current Progress Reassessment

### Achieved

- Proxy Library migration baseline is now active:
  - `project.chili.json` supports editable `assetProxyFolder`
  - proxy folder structure is auto-provisioned (`prototypes/`, `meshes/`, `materials/`, `textures/`, `scenes/`, `prefabs/`, `audio/`, `metadata/`)
  - project-side registry is written to `.chili/asset_registry.json`
  - default reusable prototype definitions are seeded (`proto.default_cube`, `proto.default_light`)
  - RuntimeWorld object instances store `prototypeId`
  - scene serializer supports prototype instance shape (`prototype`, `values`, `overrides`)
  - startup default scene uses object-instance references to prototypes
  - runtime prototype resolution is now a dedicated path (resolver), not serializer hardcoding
  - Studio asset library view scans and displays proxy entries by category

- Studio runtime world foundation is active:
  - stable entity ids
  - scene load/save path
  - default scene + fallback scene behavior
- Interaction state is centralized:
  - tool state
  - selection state
  - runtime mode state
  - inspector feed/state updates
- One authoritative viewport layout path now drives:
  - renderer viewport
  - camera aspect
  - picking/raycasting bounds
- Play/Stop remains in the Studio host viewport (no separate Play window).
- Grid prototype is integrated through prototype lowering and rendered in Studio.
- Initial reusable `InputSystem` prototype is integrated for Studio:
  - raw input -> context/actions -> camera/selection behavior
  - `Studio` context and `GameSample` context both declared
  - Studio camera and selection bindings use named actions instead of direct button chords
  - exact modifier chords prevent `LMB`, `Shift + LMB`, and `Alt + LMB` from conflicting
  - `Shift + LMB` now routes through centralized multi-selection state

### Partial / In Progress

- Orbit focus priority is documented; ongoing verification is needed across all interaction edge cases.
- Input consumption and context priority are implemented in a first pass and should be hardened with more UI/console blocking scenarios.

### Next Practical TODO

- validate full control matrix across resize + panel visibility + play/edit transitions
- keep moving any remaining direct raw-input checks toward named actions
- expand the new CTest contract suite with runtime state-transition and project/export integration checks

## Default Scene Template

Studio now uses a centralized default scene template as the baseline editor/runtime starting scene.

Template elements:

- world origin marker
- X/Y/Z axis markers
- editor reference grid
- sample cube sanity object
- stable default camera framing
- basic preview light

Ownership notes:

- template composition lives in `src/runtime/scene/studio_default_scene_template.*`
- this template is reusable setup logic, not gameplay logic
- sandbox/demo scenes remain separate and should stay explicitly labeled as sandbox/sample lanes

## Preview/Build Behavior (Current Contract)

Studio now follows a single project-output contract for preview/build actions:

- `Build App`:
  - builds the opened project
  - exports output to `User/<project_id>/Export`
- `Preview App`:
  - builds the opened project
  - exports output to `User/<project_id>/Export`
  - launches the exported executable from that same path

Export package contract:

```text
User/<project_id>/Export/<project_id>.exe
User/<project_id>/Export/<project_id>_runtime.dll
User/<project_id>/Export/engine.dll
User/<project_id>/Export/project.enginegame
User/<project_id>/Export/config/*
User/<project_id>/Export/scenes/*
User/<project_id>/Export/assets/*
```

This prevents preview-vs-output drift caused by launching different binaries or missing project content.

Studio now also tracks these artifact states separately while a project stays
open:

- configured runtime artifact path from the source project manifest
- last built runtime artifact path
- last packaged runtime artifact path
- last packaged preview executable path

When a packaged artifact is available, Studio's project runtime handoff prefers
that packaged DLL so viewport play and exported preview keep converging on the
same runtime truth.

## CoreTools

`CoreTools` is the first-party tool surface for the studio.
For this milestone it is intentionally minimal and only proves that embedded web-authored UI is rendering correctly inside the native host.

Visible content includes:

- a CoreTools title
- `Hello from CoreTools`

## CoreTools Frontend Source

Angular is intended to belong to the `CoreTools` module itself, not to the studio host root.

The current split is:

- `apps/studio/coretools/angular`
  - framework source and frontend tooling for `CoreTools`
- `apps/studio/coretools/runtime`
  - runtime web content loaded by WebView2
- `apps/studio/coretools/README.md`
  - module-level notes for how source and runtime content relate

That keeps the native build separate while also keeping frontend implementation details owned by the `CoreTools` module boundary.

## Engine Web Dialog API

This studio milestone now depends on the engine's reusable web dialog feature.

Current engine-level capabilities include:

- dock-left, dock-right, dock-top, dock-bottom, fill, and manual child placement inside the engine window
- floating top-level web dialogs
- local file-backed WebView2 content loading
- runtime visibility, bounds, and content-path updates through `EngineCore`

## Build Notes

Build direction:

- Studio is moving toward the same DLL-loaded runtime shape as the rest of the engine
- the desired process shape is a thin launcher executable loading `engine.dll`, then Studio/CoreTools/runtime DLLs as needed
- Studio-specific behavior should not become permanent launcher logic
- engine systems should remain behind `engine.dll` boundaries so editor iteration does not require relinking the whole executable

Current transitional Studio target:

- `Studio`, output as `engine_studio.exe`

Current transitional build backend:

- `StudioBuildSystem` still shells direct `cmake -S/-B` and `cmake --build` when a project manifest does not override build commands
- that is implementation reality today, not the desired long-term contract
- repository policy still treats wrapper scripts as the canonical configure/build lane and expects Studio, wrappers, and CI to converge on one shared builder contract
- when Studio build wiring changes, wrapper scripts remain the documented verification lane

Sample runtime coupling note:

- `HelloGameRuntime` is now an optional legacy in-process sample runtime.
- Enable it only when needed with `-DENGINE_BUILD_STUDIO_SAMPLE_RUNTIMES=ON`.
- Default Studio direction is `StudioPreviewRuntime` plus project/runtime contract flows, not permanent sample-runtime ownership inside Studio.

Canonical local verification lane:

```powershell
.\configure.cmd
.\build.cmd studio
```

The underlying CMake target is still `Studio`; `engine_studio.exe` is the
output name kept for continuity.

The build copies `WebView2Loader.dll` from a known local Visual Studio path when that loader is available there.
If it is not copied automatically, place `WebView2Loader.dll` beside `engine_studio.exe`.

## Next Steps

- verify the engine-owned web dialog surface renders on the target machine
- add additional engine-level docking behaviors beyond the fixed presets
- decide how native main-area rendering should evolve alongside embedded tool surfaces
- reintroduce transport and command/event messaging as future editor-facing systems instead of browser-first runtime requirements
- replace temporary in-host demo runtime paths with project-owned runtime artifact loading in the center preview host
