# App Starter Template

This folder is a commented starter shape for new engine apps.

It is not added to the root `CMakeLists.txt` by default. Copy this folder to a real app name, rename the target/files/classes, then add:

```cmake
add_subdirectory(apps/your_app_name)
```

The template now follows the revived runtime pattern:

- `template_runtime` is the app-owned gameplay/runtime DLL
- `template_preview` is the thin host executable that loads that DLL through `get_game_api()`
- app-local game/tool state stays in the app folder
- gameplay code consumes `studio_runtime::RuntimeInput`
- game/world drawing returns a `FramePrototype` through `studio_runtime::RuntimeFrame`
- the preview host and Studio should both be able to load the same runtime DLL

Starter files:

- `src/runtime_entry.cpp`
- `src/template_runtime.hpp`
- `src/template_runtime.cpp`
- `src/template_game.hpp`
- `src/template_game.cpp`
- `src/template_frame_builder.hpp`
- `src/template_frame_builder.cpp`

When copying this template:

- rename both CMake targets
- rename `template_runtime.dll` and the corresponding `GAME_RUNTIME_DLL_NAME`
- keep gameplay state/rules in the app folder
- keep the preview host thin

Do not put game-specific state or rules into `src/prototypes`, and do not move
gameplay truth into the preview host just because it is convenient.
