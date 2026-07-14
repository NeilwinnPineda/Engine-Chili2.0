#pragma once

#include "engine/game_runtime_api.hpp"

#include <string>

namespace studio_runtime
{
    class GameRuntimeLibrary
    {
    public:
        GameRuntimeLibrary() = default;
        ~GameRuntimeLibrary();

        GameRuntimeLibrary(const GameRuntimeLibrary&) = delete;
        GameRuntimeLibrary& operator=(const GameRuntimeLibrary&) = delete;

        bool Load(const std::string& artifactPath, std::string& outError);
        void Unload();

        IGameRuntime* GetRuntime() const;
        const std::string& GetRuntimeName() const;
        const std::string& GetArtifactPath() const;
        bool IsLoaded() const;

    private:
        void* m_library = nullptr;
        const GameRuntimeApi* m_api = nullptr;
        IGameRuntime* m_runtime = nullptr;
        std::string m_runtimeName;
        std::string m_artifactPath;
    };
}

