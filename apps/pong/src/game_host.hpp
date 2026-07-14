#pragma once

#include "bridge/engine_bridge.hpp"
#include "runtime/host/studio_runtime_host.hpp"
#include "scene_manager.hpp"
#include "engine/script/script_event.hpp"

#include <string>

class GameHost
{
public:
    bool Initialize();
    void Run();
    void Shutdown();

private:
    bool LoadScene(int index);
    void RegisterScripts();
    void HandleScriptEvent(const engine::script::ScriptEvent& event);
    void TickGame();

    EngineBridge m_bridge;
    studio_runtime::StudioRuntimeHost m_runtimeHost;
    SceneManager m_sceneManager;
    int m_pendingSceneTransition = -1;
    bool m_pendingExit = false;
    bool m_initialized = false;
};
