#pragma once

#include "runtime/runtime_game_api.hpp"
#include "runtime/proxy_prototype_resolver.hpp"
#include "runtime/runtime_registry.hpp"
#include "runtime/studio_connector.hpp"
#include "runtime/studio_interaction_controller.hpp"
#include "runtime/studio_picking_service.hpp"
#include "modules/render/render_types.hpp"
#include "input/input_system.h"
#include "prototypes/snap/snap_debug.hpp"
#include "prototypes/snap/snap_types.hpp"

#include <memory>
#include <string>

namespace studio_runtime
{
    enum class StudioRuntimePlayState
    {
        Edit,
        Playing,
        Paused,
        Step
    };

    class StudioRuntimeHost
    {
    public:
        StudioRuntimeHost();

        bool Initialize(const std::string& scenePath, std::string& outError);
        bool SaveScene(const std::string& scenePath, std::string& outError) const;
        bool Play(const ProjectRuntimeDesc& project, std::string& outError);
        void Stop();
        void TogglePause();
        void StepOnce();
        void Tick(float deltaSeconds, const RuntimeInput& input, const ViewportRect& viewportRect);

        StudioRuntimePlayState GetState() const;
        std::string GetStateName() const;
        const std::string& GetViewportText() const;
        const FramePrototype& GetRenderFrame() const;
        bool HasRenderFrame() const;
        const ProjectRuntimeDesc& GetActiveProject() const;
        StudioConnector& GetConnector();
        const StudioConnector& GetConnector() const;
        StudioInteractionController& GetInteraction();
        const StudioInteractionController& GetInteraction() const;
        const RuntimeWorld& GetWorld() const;
        void SetActiveTool(StudioTool tool);
        void ConfigurePrototypeLibrary(const std::string& proxyFolderPath);
        void SetSnapDebugVisible(bool visible);
        bool IsSnapDebugVisible() const;

    private:
        void ConfigureInputContexts();
        std::string CreateFallbackScene();
        void HandleEditInput(const RuntimeInput& input, const ViewportRect& viewportRect);
        void UpdateSnapDebug(const RuntimeInput& input, const ViewportRect& viewportRect);
        FramePrototype BuildWorldFrame() const;

    private:
        RuntimeRegistry m_registry;
        std::unique_ptr<IGameRuntime> m_runtime;
        RuntimeWorld m_world;
        RuntimeWorld m_editSnapshot;
        StudioCamera m_studioCamera;
        StudioConnector m_connector;
        StudioPickingService m_pickingService;
        StudioInteractionController m_interaction;
        ProxyPrototypeResolver m_prototypeResolver;
        InputSystem m_inputSystem;
        ProjectRuntimeDesc m_activeProject;
        FramePrototype m_renderFrame;
        SnapDebugReport m_snapDebugReport;
        SnapResult m_snapDebugResult;
        StudioRuntimePlayState m_state = StudioRuntimePlayState::Edit;
        std::string m_viewportText = "Edit Mode.";
        bool m_snapDebugVisible = false;
        bool m_hasRenderFrame = true;
    };
}
