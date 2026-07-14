#include "runtime/api/game_runtime_library.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <cstring>

namespace studio_runtime
{
    GameRuntimeLibrary::~GameRuntimeLibrary()
    {
        Unload();
    }

    bool GameRuntimeLibrary::Load(const std::string& artifactPath, std::string& outError)
    {
        Unload();
        outError.clear();

        if (artifactPath.empty())
        {
            outError = "Project runtime artifact path is empty.";
            return false;
        }

        HMODULE library = LoadLibraryExA(
            artifactPath.c_str(),
            nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!library)
        {
            outError = "Failed to load project runtime DLL '" + artifactPath +
                "' (Win32 error " + std::to_string(static_cast<unsigned long>(GetLastError())) + ").";
            return false;
        }

        FARPROC rawGetApi = GetProcAddress(library, "get_game_api");
        if (!rawGetApi)
        {
            outError = "Project runtime DLL does not export get_game_api().";
            FreeLibrary(library);
            return false;
        }

        GetGameApiFn getApi = nullptr;
        static_assert(sizeof(getApi) == sizeof(rawGetApi), "Function pointer size mismatch.");
        std::memcpy(&getApi, &rawGetApi, sizeof(getApi));

        const GameRuntimeApi* api = getApi();
        if (!api ||
            api->apiVersion != kGameRuntimeApiVersion ||
            api->structSize < sizeof(GameRuntimeApi) ||
            !api->createRuntime ||
            !api->destroyRuntime)
        {
            outError = "Project runtime DLL exposes an incompatible GameRuntime API.";
            FreeLibrary(library);
            return false;
        }

        IGameRuntime* runtime = api->createRuntime();
        if (!runtime)
        {
            outError = "Project runtime DLL failed to create its runtime instance.";
            FreeLibrary(library);
            return false;
        }

        m_library = library;
        m_api = api;
        m_runtime = runtime;
        m_runtimeName = api->runtimeName ? api->runtimeName : "GameRuntime";
        m_artifactPath = artifactPath;
        return true;
    }

    void GameRuntimeLibrary::Unload()
    {
        if (m_runtime && m_api && m_api->destroyRuntime)
        {
            m_api->destroyRuntime(m_runtime);
        }

        m_runtime = nullptr;
        m_api = nullptr;
        m_runtimeName.clear();
        m_artifactPath.clear();

        if (m_library)
        {
            FreeLibrary(static_cast<HMODULE>(m_library));
            m_library = nullptr;
        }
    }

    IGameRuntime* GameRuntimeLibrary::GetRuntime() const
    {
        return m_runtime;
    }

    const std::string& GameRuntimeLibrary::GetRuntimeName() const
    {
        return m_runtimeName;
    }

    const std::string& GameRuntimeLibrary::GetArtifactPath() const
    {
        return m_artifactPath;
    }

    bool GameRuntimeLibrary::IsLoaded() const
    {
        return m_library != nullptr && m_runtime != nullptr;
    }
}
