# Engine Revival Plan

## Purpose

This document is the active execution order for reviving Engine-Chili2.0.

The immediate goal is to restore one coherent, testable product path:

```text
project package
    -> project runtime DLL
    -> engine.dll
    -> Studio preview or thin game preview host
```

The order is intentional. Testing and runtime truth must be stable before an AI
control surface is allowed to mutate projects.

## Current Reality

- the working tree contains an unfinished Studio runtime-directory reorganization
- Pong DLL/preview targets and wrapper aliases have been restored in the working tree, pending wrapper verification
- the Windows GitHub Actions lane is pinned to VS2022 x64, pending a green CI verification run
- a first headless CTest contract suite is registered and wired into CI
- current validation is mostly manual, diagnostic, or trial-app driven
- Studio runs an active localhost HTTP bridge for its embedded HTML tools
- `CommandRouter` now has a read-first permission policy and an active localhost HTTP ingress; WebSocket remains incomplete scaffolding
- a dependency-free read-only MCP adapter now fronts the validated command ingress; there is no embedded model client
- a versioned create/tick/destroy runtime ABI, Studio DLL loader, and generic preview host are implemented in the working tree
- generated projects now build a runtime DLL and preview host, and export rewrites the package manifest to the adjacent artifact paths

## Non-Negotiable Contracts

- preserve active local work; do not restore build or runtime files blindly
- use wrapper scripts as the documented configure/build entry points
- keep `launcher/preview host -> engine.dll -> project runtime DLL` ownership explicit
- Studio may control and inspect runtime truth, but must not duplicate gameplay truth
- Studio Preview and exported output must execute the same project-owned runtime implementation
- tests must exercise public or owner-level boundaries rather than backend internals
- automation and AI commands must route through the same Studio project/runtime systems as the UI
- do not create an AI-only scene, project, render, or gameplay state path

## Phase 1 - Recover A Stable Baseline

1. inventory all modified, deleted, moved, and untracked files
2. reconstruct and complete the unfinished Studio runtime folder reorganization
3. repair includes and CMake source paths without discarding local additions
4. reconcile the canonical targets:
   - `EngineRuntime`
   - `Studio`
   - `PongPreview`
   - `PongRuntime`
   - `HotBuildTool`
5. align wrapper aliases, output folders, CI expectations, and documentation
6. update the canonical GitHub remote owner when repository maintenance is authorized
7. keep generated runtime history out of useful CI diagnostic artifacts
8. run the required wrapper build lane when build execution is available

Exit criteria:

- source and build wiring describe the same target graph
- all newly referenced sources are tracked
- wrapper builds produce the documented artifacts
- CI uses the wrapper scripts and reaches a green baseline

## Phase 2 - Establish Automated Testing

1. add a dedicated test target and CTest registration
2. keep test code out of production targets
3. provide deterministic seams that do not require a visible DX11 window where possible
4. add focused tests for:
   - scene serialization round trips
   - `RuntimeWorld` entity/component behavior
   - project manifest parsing
   - project path confinement
   - prototype resolution
   - input context priority and consumption
   - runtime play/pause/step/stop transitions
5. add integration coverage for:
   - create -> open -> save -> reload project
   - restoring the edit world after Stop
   - export-package content validation
6. make CI run the test suite after the wrapper build
7. retain a small Windows runtime smoke pass for Studio and Pong

Exit criteria:

- `ctest` is a real supported lane behind repository wrappers
- core project, scene, and runtime contracts have regression coverage
- CI fails on test regressions rather than only missing binaries

## Phase 3 - Make Preview And Export Share Runtime Truth

1. define a versioned project-runtime ABI with explicit create/tick/destroy entry points
2. implement runtime DLL load, validation, handoff, unload, and failure reporting
3. move Pong gameplay ownership completely into `PongRuntime`
4. keep `PongPreview` limited to process startup, DLL loading, and handoff
5. make Studio load the project runtime artifact produced by the project build
6. make Build App and Preview App consume the same export package
7. remove the temporary in-process preview runtime as project gameplay truth
8. test ABI mismatch, missing DLL, load failure, shutdown, and reload behavior

Exit criteria:

- Studio Preview and the exported game execute the same project runtime DLL
- Studio contains no Pong-specific gameplay implementation
- preview success cannot hide export/package failure

## Phase 4 - Create One Studio Command Service

1. extract route behavior from `StudioHost` into focused command/query services
2. define typed requests, typed results, request IDs, and stable error codes
3. separate read-only queries from mutations
4. route commands through current owners such as:
   - `StudioProjectSystem`
   - `StudioRuntimeHost`
   - `StudioInteractionController`
   - scene and prototype services
5. replace hand-written JSON field extraction with a real parser
6. use `GET` for queries and `POST` for mutations
7. add a per-session secret, origin restrictions, path validation, and audit records
8. represent builds and other long work as asynchronous jobs with status queries

Exit criteria:

- embedded Studio UI and automation share one command implementation
- `StudioHost` is not the permanent owner of every HTTP action
- unauthenticated cross-origin requests cannot mutate a running Studio session

## Phase 5 - Add The AI Bridge

1. define a small versioned tool protocol over the Studio command service
2. expose read-only tools first:
   - workspace status
   - project listing and inspection
   - scene/entity listing
   - entity inspection
   - runtime and build status
3. expose the tool set through a local MCP server or equivalent adapter
4. add validated mutation tools only after read-only coverage is stable:
   - create prototype instance
   - rename entity
   - change transform
   - save scene
   - start/stop preview
   - request project build
5. add undo checkpoints or transactions around mutations
6. require explicit confirmation for destructive operations
7. record tool calls, authorization decisions, and outcomes in an audit log
8. add protocol, authorization, concurrency, and mutation integration tests

Exit criteria:

- the AI layer has no privileged bypass around Studio owners
- read and write authority are explicit
- failed or rejected mutations leave project/runtime truth coherent
- the bridge is covered by automated tests

## Phase 6 - Documentation And Handoff

1. update the README, API map, architecture TODO, build lanes, and Studio docs
2. document test commands and expected results
3. document the AI tool surface, security model, and destructive-action policy
4. maintain a manual acceptance checklist for:
   - Studio startup and shutdown
   - project create/open/save/reload
   - scene editing and viewport interaction
   - play/pause/step/stop
   - Pong preview
   - exported Pong
   - AI read and mutation tools
5. checkpoint each verified phase in a focused commit

## Recommended Checkpoints

- A: coherent source tree and green wrapper/CI build
- B: automated project, scene, and runtime tests
- C: Studio preview/export runtime parity
- D: secure read-only automation and AI bridge
- E: validated write-capable AI bridge

Phases 1 through 3 take priority. The AI bridge must be built on the tested real
runtime path, not used as a second implementation of that path.

## Working-Tree Progress (Pending Runtime Verification)

- restored `PongRuntime` and the thin `PongPreview` loader target
- restored wrapper aliases and documented output ownership
- added `EngineContractTests`, `test.cmd`, and the CI test step
- added contract checks for scene round trips, malformed-scene rollback,
  entity lifecycle, render presets, Pong simulation, input consumption, and
  Studio command authorization
- pinned CI to the stable VS2022 x64 toolchain image
- added a read-only-by-default Studio command endpoint
- extracted the read-only workspace/project/entity/runtime query JSON into a dedicated Studio command-query service so `StudioHost` no longer owns that query assembly directly
- extracted build/export route orchestration into `StudioBuildRequestService`
- extracted workspace layout save and visibility-toggle route orchestration into `StudioWorkspaceRequestService`
- hardened the localhost HTTP transport against cross-site ingress and
  unbounded request headers
- added a per-session Studio cookie and same-origin WebView navigation
- added the versioned runtime ABI, validated Studio DLL loader, generic preview
  host, and project-template DLL generation
- made Studio consume the frame produced by the loaded artifact and preserve it
  while paused
- made Build App create a clean package containing the preview host,
  project-runtime DLL, `engine.dll`, manifest, config, scenes, and assets
- added a fixed-allowlist read-only MCP adapter with request-id validation,
  response limits, and audit records
- added contract coverage for project artifact-state reporting through both the
  native Studio test target and the MCP adapter test

These changes are deliberately marked pending: repository rules require the
wrapper build lane before they can be treated as verified checkpoints.
