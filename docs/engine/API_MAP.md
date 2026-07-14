# Engine API Map

This is the current practical map of the main engine, Studio, runtime, and AI
bridge seams in the working tree.

Use this file when you need to answer:

- where gameplay/runtime truth lives
- how Studio loads and controls that runtime truth
- which layer owns project/package/build behavior
- which automation surface is real versus provisional

This is a code-scan map, not a runtime-certified verification report. Wrapper
configure/build/test validation is still required before the in-flight revival
work can be treated as a verified checkpoint.

## Runtime Stack

Target direction and current revived path:

```text
preview host or Studio
    -> engine.dll
    -> project runtime DLL
```

The important ownership split is:

- host executable owns process startup and DLL handoff only
- `EngineRuntime` owns reusable engine systems
- project/game runtime DLL owns gameplay state and rules
- Studio owns editor control and inspection, not duplicate gameplay truth

Primary files:

- [include/engine/game_runtime_api.hpp](../../include/engine/game_runtime_api.hpp)
- [apps/pong/CMakeLists.txt](../../apps/pong/CMakeLists.txt)
- [tools/game_preview/main.cpp](../../tools/game_preview/main.cpp)
- [apps/studio/src/runtime/api/game_runtime_library.hpp](../../apps/studio/src/runtime/api/game_runtime_library.hpp)
- [apps/studio/src/runtime/host/studio_runtime_host.hpp](../../apps/studio/src/runtime/host/studio_runtime_host.hpp)

## 1. Engine Runtime Surface

`EngineCore` remains the main engine-facing host surface.

Primary file:

- [src/core/engine_core.hpp](../../src/core/engine_core.hpp)

Use it when:

- a host executable needs to initialize, tick, and shut down engine systems
- app or Studio code needs engine-owned services without reaching into module internals

Common calls:

- `Initialize()`
- `Run()`
- `Tick()`
- `Shutdown()`
- `RequestShutdown()`
- `SetFrameCallback(...)`
- `SubmitRenderFrame(...)`

Related owner-facing capability surface:

- [src/app/app_capabilities.hpp](../../src/app/app_capabilities.hpp)

That capability surface is the preferred app/runtime seam for:

- runtime pressure state
- sound service access
- app-facing engine services during ticks

## 2. Game Runtime ABI

Gameplay/runtime truth now has a versioned DLL ABI.

Primary file:

- [include/engine/game_runtime_api.hpp](../../include/engine/game_runtime_api.hpp)

Main types:

- `studio_runtime::ProjectRuntimeDesc`
- `studio_runtime::RuntimeInput`
- `studio_runtime::RuntimeFrame`
- `studio_runtime::IGameRuntime`
- `GameRuntimeApi`
- `get_game_api()`

Use it when:

- a project runtime DLL must be created and driven by Studio or a preview host
- a host needs a stable create/tick/destroy boundary instead of linking gameplay directly

Current contract:

- API version is `2`
- the DLL exports `get_game_api()`
- the host validates `apiVersion`, `structSize`, and function pointers
- the runtime implementation owns gameplay state and writes a `RuntimeFrame`
- the frame carries `FramePrototype` output back to the host

Current limitation:

- this ABI still carries STL types across the DLL boundary, so runtime
  verification is still a meaningful risk item until the wrapper lane is run

## 3. Preview Host Ownership

The generic preview host is intentionally tiny.

Primary files:

- [tools/game_preview/main.cpp](../../tools/game_preview/main.cpp)
- [apps/pong/CMakeLists.txt](../../apps/pong/CMakeLists.txt)

Use it when:

- a project or sample app needs a launchable executable without re-owning gameplay logic

Current Pong shape:

- `PongRuntime` builds `pong_runtime.dll`
- `PongPreview` loads that DLL and hands control through `get_game_api()`
- `PongPreview` also carries `engine.dll` beside it at packaging/build output time

The intended contract is:

- no Pong-specific gameplay logic compiled into Studio
- no duplicate preview-only Pong implementation

## 4. Studio Runtime Loading

Studio runtime loading is now centered in the runtime host and DLL loader
helpers, not in ad hoc sample-runtime ownership.

Primary files:

- [apps/studio/src/runtime/host/studio_runtime_host.hpp](../../apps/studio/src/runtime/host/studio_runtime_host.hpp)
- [apps/studio/src/runtime/host/studio_runtime_host.cpp](../../apps/studio/src/runtime/host/studio_runtime_host.cpp)
- [apps/studio/src/runtime/api/game_runtime_library.hpp](../../apps/studio/src/runtime/api/game_runtime_library.hpp)
- [apps/studio/src/runtime/api/game_runtime_library.cpp](../../apps/studio/src/runtime/api/game_runtime_library.cpp)

Use it when:

- Studio Play should load the current project runtime artifact
- Studio Pause/Step/Stop should operate on the real runtime path
- Studio must unload and restore edit state cleanly

Key responsibilities:

- load the runtime DLL
- validate `get_game_api()`
- create and destroy the runtime object
- feed `RuntimeInput`
- consume `RuntimeFrame`
- preserve the last runtime frame while paused
- unload on stop and restore the edit-world snapshot

Legacy note:

- `hello_game_runtime.*` still exists as an optional sample-runtime lane behind
  `ENGINE_BUILD_STUDIO_SAMPLE_RUNTIMES`
- that path is not the long-term runtime-truth owner

## 5. Studio Project And Manifest Ownership

Project identity and package-level metadata live in the Studio project system.

Primary files:

- [apps/studio/src/studio/studio_project_system.hpp](../../apps/studio/src/studio/studio_project_system.hpp)
- [apps/studio/src/studio/studio_project_system.cpp](../../apps/studio/src/studio/studio_project_system.cpp)
- [apps/studio/src/studio/file_proxy.hpp](../../apps/studio/src/studio/file_proxy.hpp)

Use it when:

- creating, opening, saving, and resolving projects inside `User/`
- reading or writing manifest fields that define runtime/build/package behavior

Important project/manfiest fields currently in play:

- `runtime_kind`
- `runtime_artifact`
- `build_configure`
- `build_command`
- `build_output`
- `preview_output`
- `default_scene`

Current native-artifact expectation for generated projects:

- `runtime_kind = native_artifact`
- runtime DLL output and preview host output are declared explicitly in the manifest

Workspace helpers:

- `GetProjectWorkspacePath(...)`
- `GetProjectSourcePath(...)`
- `GetProjectBuildPath(...)`
- `GetProjectCachePath(...)`
- `GetProjectLogsPath(...)`

## 6. Studio Build And Export Ownership

Studio build/export behavior currently lives in the build system layer.

Primary files:

- [apps/studio/src/studio/studio_build_system.hpp](../../apps/studio/src/studio/studio_build_system.hpp)
- [apps/studio/src/studio/studio_build_system.cpp](../../apps/studio/src/studio/studio_build_system.cpp)
- [apps/studio/src/studio/studio_build_request_service.hpp](../../apps/studio/src/studio/studio_build_request_service.hpp)
- [apps/studio/src/studio/studio_build_request_service.cpp](../../apps/studio/src/studio/studio_build_request_service.cpp)
- [apps/studio/src/studio/studio_workspace_request_service.hpp](../../apps/studio/src/studio/studio_workspace_request_service.hpp)
- [apps/studio/src/studio/studio_workspace_request_service.cpp](../../apps/studio/src/studio/studio_workspace_request_service.cpp)

Use it when:

- Build App needs to configure/build the currently opened project
- Export needs to assemble a clean package under `User/<id>/Export`
- Preview App needs a launchable packaged executable

Main request/result types:

- `BuildAndRunProjectRequest`
- `BuildAndRunProjectResult`
- `StudioBuildSystem::BuildAndRunProject(...)`

Current behavior:

- `StudioBuildRequestService` owns the project-open check, build/export invocation, result narration, and project artifact-state recording for the HTTP build route
- `StudioWorkspaceRequestService` owns workspace layout save and visibility-toggle route orchestration for the HTTP workspace routes
- parses manifest build/runtime fields
- falls back to a direct raw-CMake invocation when the manifest does not define build commands
- validates that the configured runtime artifact exists
- preserves both the built runtime artifact path and the packaged export artifact path in the result/state model
- exports `project.enginegame`, config, scenes, assets, and scripts when present
- for `native_artifact` projects:
  - copies the runtime DLL into the export root
  - copies the preview host as `<project_id>.exe`
  - copies adjacent `engine.dll`
  - rewrites packaged manifest fields to the exported adjacent filenames

Current project/runtime status now distinguishes:

- configured runtime artifact path from the source manifest
- last built runtime artifact path
- last packaged runtime artifact path
- active runtime artifact path Studio would currently load

Important revival note:

- this is still a transitional backend
- repo-level policy says wrapper scripts remain the canonical configure/build lane
- Studio should eventually delegate to the same shared builder contract as wrappers and CI

## 7. Studio Command Service

Studio now has a versioned command ingress with explicit read-versus-mutation
classification.

Primary files:

- [apps/studio/src/commands/command_policy.hpp](../../apps/studio/src/commands/command_policy.hpp)
- [apps/studio/src/commands/command_policy.cpp](../../apps/studio/src/commands/command_policy.cpp)
- [apps/studio/src/commands/command_protocol.hpp](../../apps/studio/src/commands/command_protocol.hpp)
- [apps/studio/src/commands/command_protocol.cpp](../../apps/studio/src/commands/command_protocol.cpp)
- [apps/studio/src/commands/command_router.hpp](../../apps/studio/src/commands/command_router.hpp)
- [apps/studio/src/commands/command_router.cpp](../../apps/studio/src/commands/command_router.cpp)

Use it when:

- embedded Studio UI or local automation needs owner-backed Studio/project/runtime queries
- command authorization should be explicit instead of route-specific accident

Current command categories:

- read-only:
  - `hello`
  - `ping`
  - `get_status`
  - `list_capabilities`
  - `get_workspace_status`
  - `get_project_status`
  - `list_entities`
  - `get_selected_entity`
  - `get_runtime_status`
- mutating:
  - `exit`

Protocol shape:

- versioned envelope with:
  - `protocolVersion`
  - `kind`
  - `command`
  - `sender`
  - `requestId`

Current policy:

- unknown commands are denied
- mutating commands are denied unless explicitly enabled
- current bridge use is deliberately read-first

## 8. Studio HTTP Transport

HTTP is the active transport for embedded CoreTools pages and the command
endpoint.

Primary files:

- [apps/studio/src/transport/http_server.hpp](../../apps/studio/src/transport/http_server.hpp)
- [apps/studio/src/transport/http_server.cpp](../../apps/studio/src/transport/http_server.cpp)

Use it when:

- first-party embedded panels need same-origin access to Studio routes
- local automation needs the read-only command endpoint

Current transport/security behavior:

- binds loopback on `127.0.0.1:37620`
- validates `Host`
- rejects cross-site ingress
- avoids wildcard CORS
- limits request header size
- issues a per-session cookie for normal Studio routes

Important exception:

- `/studio/bridge/command` is exempt from the normal session cookie because its
  external entry point is permanently forced to read-only handling

## 9. AI Bridge

The AI bridge is external to the application and intentionally dependency-free.

Primary files:

- [tools/ai_bridge/studio_mcp.py](../../tools/ai_bridge/studio_mcp.py)
- [tools/ai_bridge/README.md](../../tools/ai_bridge/README.md)

Use it when:

- an MCP client needs a small, explicit read-only Studio inspection surface

Current tool set:

- `studio_status`
- `workspace_status`
- `project_status`
- `list_entities`
- `selected_entity`
- `runtime_status`

Current guardrails:

- fixed allowlist only
- no arbitrary command forwarding
- request-id validation
- timeout and response-size limits
- manifest-to-Studio descriptor validation, including bridge tool names and descriptions
- audit records written to `User/studio/logs/ai_bridge_audit.jsonl`

There is currently:

- no embedded model client
- no write-capable AI tool surface
- no undo/transaction layer for AI mutations

## 10. Test Surface

The current revival test surface is registered under CTest.

Primary files:

- [test.cmd](../../test.cmd)
- [tests/CMakeLists.txt](../../tests/CMakeLists.txt)
- [tests/engine_contract_tests.cpp](../../tests/engine_contract_tests.cpp)
- [tests/test_studio_mcp.py](../../tests/test_studio_mcp.py)

Use it when:

- checking the revived contract suite after the wrapper build lane succeeds

Current registered tests:

- `engine.contracts`
- `studio.ai_bridge_contract` when Python is available

Current coverage themes:

- scene serialization round trip and malformed rollback behavior
- runtime-world lifecycle behavior
- Pong simulation/runtime lifecycle
- input priority and consumption
- Studio command classification/authorization
- DLL loader failures and empty path handling
- AI bridge allowlist behavior

## 11. Current Caveats

Keep these in mind while reading the codebase:

- wrapper configure/build/test verification has not been performed in this session
- the Studio runtime-directory reorganization is still an active working-tree transition
- Studio build/export behavior still contains a transitional direct-CMake backend
- `apps/_template` still trails the revived Pong/runtime-DLL model
- current docs should be treated as implementation guidance until the wrapper lane confirms the revival checkpoints

## 12. Follow-Up Docs

- [README](../../README.md)
- [Build Lanes](../build/BUILD_LANES.md)
- [Revival Plan](./REVIVAL_PLAN.md)
- [Acceptance Checklist](./ACCEPTANCE_CHECKLIST.md)
- [Architecture TODO](./TODO.md)
