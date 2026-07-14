#include "engine/game_runtime_api.hpp"

#include "app/app_capabilities.hpp"
#include "core/engine_core.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <cstring>
#include <filesystem>
#include <iostream>

#ifndef GAME_RUNTIME_DLL_NAME
#define GAME_RUNTIME_DLL_NAME "game_runtime.dll"
#endif

#ifndef GAME_PROJECT_ID
#define GAME_PROJECT_ID "game"
#endif

#ifndef GAME_PROJECT_NAME
#define GAME_PROJECT_NAME "Engine Game"
#endif

namespace
{
    std::string GetAdjacentRuntimePath()
    {
        char executablePath[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameA(nullptr, executablePath, MAX_PATH);
        if (length == 0U || length >= MAX_PATH)
        {
            return std::string();
        }
        return (std::filesystem::path(executablePath).parent_path() / GAME_RUNTIME_DLL_NAME).string();
    }

    void SetKeyState(
        studio_runtime::RuntimeInput& runtimeInput,
        const IAppInput& input,
        InputKey runtimeKey,
        AppKey appKey)
    {
        RawButtonState& state = runtimeInput.rawInput.keys[static_cast<std::size_t>(runtimeKey)];
        state.pressed = input.WasKeyPressed(appKey);
        state.down = input.IsKeyDown(appKey);
        state.released = input.WasKeyReleased(appKey);
    }

    studio_runtime::RuntimeInput ReadRuntimeInput(const IAppInput& input)
    {
        studio_runtime::RuntimeInput runtimeInput;
        runtimeInput.escapePressed = input.WasKeyPressed(static_cast<AppKey>(VK_ESCAPE));
        runtimeInput.servePressed = input.WasKeyPressed(AppKey::Space);
        runtimeInput.resetPressed = input.WasKeyPressed(AppKey::R);
        runtimeInput.leftUpDown = input.IsKeyDown(AppKey::W);
        runtimeInput.leftDownDown = input.IsKeyDown(AppKey::S);
        runtimeInput.rightUpDown = input.IsKeyDown(AppKey::Up);
        runtimeInput.rightDownDown = input.IsKeyDown(AppKey::Down);

        SetKeyState(runtimeInput, input, InputKey::Escape, static_cast<AppKey>(VK_ESCAPE));
        SetKeyState(runtimeInput, input, InputKey::Space, AppKey::Space);
        SetKeyState(runtimeInput, input, InputKey::W, AppKey::W);
        SetKeyState(runtimeInput, input, InputKey::A, AppKey::A);
        SetKeyState(runtimeInput, input, InputKey::S, AppKey::S);
        SetKeyState(runtimeInput, input, InputKey::L, AppKey::L);
        SetKeyState(runtimeInput, input, InputKey::R, AppKey::R);
        SetKeyState(runtimeInput, input, InputKey::Left, AppKey::Left);
        SetKeyState(runtimeInput, input, InputKey::Right, AppKey::Right);
        SetKeyState(runtimeInput, input, InputKey::Up, AppKey::Up);
        SetKeyState(runtimeInput, input, InputKey::Down, AppKey::Down);
        return runtimeInput;
    }
}

int main()
{
    const std::string runtimePath = GetAdjacentRuntimePath();
    HMODULE gameRuntimeLibrary = runtimePath.empty()
        ? nullptr
        : LoadLibraryExA(
            runtimePath.c_str(),
            nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!gameRuntimeLibrary)
    {
        std::cerr << GAME_PROJECT_NAME << ": failed to load " << GAME_RUNTIME_DLL_NAME << ".\n";
        return 1;
    }

    FARPROC rawGetGameApi = GetProcAddress(gameRuntimeLibrary, "get_game_api");
    if (!rawGetGameApi)
    {
        std::cerr << GAME_PROJECT_NAME << ": get_game_api was not exported.\n";
        FreeLibrary(gameRuntimeLibrary);
        return 1;
    }

    GetGameApiFn getGameApi = nullptr;
    static_assert(sizeof(getGameApi) == sizeof(rawGetGameApi), "Function pointer size mismatch.");
    std::memcpy(&getGameApi, &rawGetGameApi, sizeof(getGameApi));

    const GameRuntimeApi* api = getGameApi();
    if (!api ||
        api->apiVersion != studio_runtime::kGameRuntimeApiVersion ||
        api->structSize < sizeof(GameRuntimeApi) ||
        !api->createRuntime ||
        !api->destroyRuntime)
    {
        std::cerr << GAME_PROJECT_NAME << ": incompatible GameRuntime API.\n";
        FreeLibrary(gameRuntimeLibrary);
        return 1;
    }

    studio_runtime::IGameRuntime* runtime = api->createRuntime();
    if (!runtime)
    {
        std::cerr << GAME_PROJECT_NAME << ": runtime creation failed.\n";
        FreeLibrary(gameRuntimeLibrary);
        return 1;
    }

    EngineCore core;
    core.SetStartupWindowTitle(L"Engine Game Preview");
    core.SetStartupWindowSize(1280, 720);
    if (!core.Initialize())
    {
        api->destroyRuntime(runtime);
        FreeLibrary(gameRuntimeLibrary);
        return 1;
    }

    studio_runtime::ProjectRuntimeDesc project;
    project.projectId = GAME_PROJECT_ID;
    project.projectName = GAME_PROJECT_NAME;
    project.codeEntryKind = studio_runtime::ProjectCodeEntryKind::NativeArtifact;
    project.exportedArtifactPath = GAME_RUNTIME_DLL_NAME;
    runtime->BeginPlay(project);

    core.SetFrameCallback(
        [runtime, &core](AppCapabilities& capabilities)
        {
            studio_runtime::RuntimeFrame frame;
            runtime->Tick(
                static_cast<float>(capabilities.time->GetDeltaSeconds()),
                ReadRuntimeInput(*capabilities.input),
                frame);

            if (frame.hasRenderFrame && capabilities.rendering)
            {
                capabilities.rendering->SubmitFrame(frame.renderFrame);
            }
            if (frame.exitRequested)
            {
                core.RequestShutdown();
            }
        });

    const bool success = core.Run();
    runtime->EndPlay();
    api->destroyRuntime(runtime);
    core.Shutdown();
    FreeLibrary(gameRuntimeLibrary);
    return success ? 0 : 1;
}
