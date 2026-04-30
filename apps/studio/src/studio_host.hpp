#pragma once

#include "bridge/engine_bridge.hpp"
#include "commands/command_router.hpp"
#include "studio/file_management_dialog.hpp"
#include "studio/new_project_dialog.hpp"
#include "studio/project_explorer_panel.hpp"
#include "transport/http_server.hpp"

class StudioHost
{
public:
    bool Initialize();
    void Run();
    void Shutdown();

private:
    void LogStudioShellStatus();
    bool InitializeStudioHttpBridge();
    bool InitializeTopBarDialog();
    bool InitializeCoreToolsDialog();
    bool InitializeProjectExplorerPanel();
    bool HandleStudioHttpRequest(const std::string& path, std::string& outContentType, std::string& outBody);
    bool OpenFileManagementDialog();
    bool OpenNewProjectDialog();
    std::string GetCoreToolsRuntimeRootPath() const;
    std::string GetFileManagementDialogContentPath() const;
    std::string GetNewProjectDialogContentPath() const;
    std::string GetProjectExplorerContentPath() const;

private:
    EngineBridge m_bridge;
    CommandRouter m_commandRouter;
    HttpServer m_httpServer;
    studio::FileManagementDialog m_fileManagementDialog;
    studio::NewProjectDialog m_newProjectDialog;
    studio::ProjectExplorerPanel m_projectExplorerPanel;
    studio::StudioProjectSystem m_projectSystem;
    EngineCore::WebDialogHandle m_topBarDialogHandle = 0U;
    EngineCore::WebDialogHandle m_coreToolsDialogHandle = 0U;
    bool m_initialized = false;
};
