# Revival Acceptance Checklist

Use this checklist only after the wrapper configure/build/test lane succeeds.
Record failures against the owning layer rather than adding alternate preview
or rendering paths.

## Build And Tests

- [ ] `configure.cmd` succeeds
- [ ] `build.cmd studio` produces `build/bin/studio/engine_studio.exe`
- [ ] `build.cmd pong` produces `PongPreview.exe` and `pong_runtime.dll`
- [ ] `build.cmd engine` produces `build/bin/engine/engine.dll`
- [ ] `test.cmd` passes `engine.contracts`
- [ ] `test.cmd` passes `studio.ai_bridge_contract` when Python is available

## Studio

- [ ] Studio starts and all embedded panels load from `127.0.0.1:37620`
- [ ] project create/open/save/reload succeeds
- [ ] a newly created project manifest declares `native_artifact`
- [ ] scene selection, object selection, and inspector state remain centralized
- [ ] `Alt + LMB`, `MMB`, wheel, held `RMB`, movement keys, and `F` work
- [ ] play loads the configured project runtime DLL into the Studio viewport
- [ ] pause preserves the last project-runtime frame
- [ ] step advances exactly one runtime tick
- [ ] stop unloads the DLL and restores the edit-world snapshot

## Preview And Export

- [ ] Build App creates a clean `User/<id>/Export` package
- [ ] the package contains `<id>.exe`, `<id>_runtime.dll`, `engine.dll`, and `project.enginegame`
- [ ] config, scenes, and assets are copied from the real project
- [ ] the packaged manifest points to adjacent exported binaries
- [ ] Preview App launches `User/<id>/Export/<id>.exe`
- [ ] Studio Play and Preview App visibly execute the same project runtime behavior
- [ ] missing DLL and ABI-version mismatch produce explicit errors
- [ ] Pong preview loads `pong_runtime.dll` and runs without Studio-owned Pong logic

## Command And AI Bridge

- [ ] cross-site or incorrect-Host HTTP requests are rejected
- [ ] normal Studio routes reject requests without the session cookie
- [ ] `list_capabilities` reports read and mutation authority separately
- [ ] workspace, project, entity, selection, and runtime inspection commands return owner-backed state
- [ ] `exit` remains denied while mutations are disabled
- [ ] the MCP adapter lists only read-only tools
- [ ] MCP responses echo and validate request ids
- [ ] AI tool calls append authorization outcomes to `User/studio/logs/ai_bridge_audit.jsonl`
- [ ] no destructive or write-capable AI tool is exposed
