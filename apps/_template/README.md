# App Starter Template

This folder is a commented starter shape for new engine apps.

It is not added to the root `CMakeLists.txt` by default. Copy this folder to a real app name, rename the target/files/classes, then add:

```cmake
add_subdirectory(apps/your_app_name)
```

The template follows the current app pattern:

- app-local game/tool state stays in the app folder
- app code reads input intent through `AppCapabilities::input`
- app code reads frame time through `AppCapabilities::time`
- game/world drawing returns a `FramePrototype`
- HUD/status/control text goes through `NativeUiBuilder`
- the app submits one native UI frame through `capabilities.nativeUi->Submit(...)`

Do not put game-specific state or rules into `src/prototypes`.
