#include "studio_host.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>

namespace
{
    constexpr int kStudioTopBarHeight = 76;
    constexpr int kCoreToolsDockWidth = 208;
    constexpr unsigned short kStudioHttpPort = 37620;

    std::string EscapeJson(const std::string& value)
    {
        std::string escaped;
        escaped.reserve(value.size());
        for (const char ch : value)
        {
            if (ch == '\\' || ch == '"')
            {
                escaped.push_back('\\');
            }

            escaped.push_back(ch);
        }

        return escaped;
    }

    std::string BuildJsonResponse(bool ok, const std::string& message, const std::string& projectId = std::string(), const std::string& logicalPath = std::string())
    {
        return std::string("{\"ok\":") +
            (ok ? "true" : "false") +
            ",\"message\":\"" +
            EscapeJson(message) +
            "\",\"projectId\":\"" +
            EscapeJson(projectId) +
            "\",\"logicalPath\":\"" +
            EscapeJson(logicalPath) +
            "\"}";
    }

    int HexValue(char ch)
    {
        if (ch >= '0' && ch <= '9')
        {
            return ch - '0';
        }
        if (ch >= 'a' && ch <= 'f')
        {
            return 10 + (ch - 'a');
        }
        if (ch >= 'A' && ch <= 'F')
        {
            return 10 + (ch - 'A');
        }

        return -1;
    }

    std::string UrlDecode(const std::string& value)
    {
        std::string decoded;
        decoded.reserve(value.size());

        for (std::size_t index = 0; index < value.size(); ++index)
        {
            if (value[index] == '+' )
            {
                decoded.push_back(' ');
                continue;
            }

            if (value[index] == '%' && index + 2U < value.size())
            {
                const int high = HexValue(value[index + 1U]);
                const int low = HexValue(value[index + 2U]);
                if (high >= 0 && low >= 0)
                {
                    decoded.push_back(static_cast<char>((high << 4) | low));
                    index += 2U;
                    continue;
                }
            }

            decoded.push_back(value[index]);
        }

        return decoded;
    }

    std::unordered_map<std::string, std::string> ParseQuery(const std::string& path)
    {
        std::unordered_map<std::string, std::string> values;
        const std::size_t queryStart = path.find('?');
        if (queryStart == std::string::npos)
        {
            return values;
        }

        std::size_t cursor = queryStart + 1U;
        while (cursor < path.size())
        {
            const std::size_t separator = path.find('&', cursor);
            const std::string pair = path.substr(cursor, separator == std::string::npos ? std::string::npos : separator - cursor);
            const std::size_t equals = pair.find('=');
            if (equals != std::string::npos)
            {
                values[UrlDecode(pair.substr(0, equals))] = UrlDecode(pair.substr(equals + 1U));
            }

            if (separator == std::string::npos)
            {
                break;
            }

            cursor = separator + 1U;
        }

        return values;
    }

    std::string StripQuery(const std::string& path)
    {
        const std::size_t queryStart = path.find('?');
        return queryStart == std::string::npos ? path : path.substr(0, queryStart);
    }
}

bool StudioHost::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    if (!m_bridge.Initialize())
    {
        return false;
    }

    const HWND windowHandle = m_bridge.GetNativeWindowHandle();
    if (!windowHandle)
    {
        m_bridge.LogError("Studio: native host window is not available.");
        m_bridge.Shutdown();
        return false;
    }

    if (!InitializeStudioHttpBridge())
    {
        m_bridge.LogWarn("Studio: HTTP bridge is unavailable; HTML Studio actions will be display-only.");
    }

    if (!InitializeCoreToolsDialog())
    {
        m_bridge.LogError("Studio: failed to initialize the engine-owned CoreTools dialog.");
        m_bridge.Shutdown();
        return false;
    }

    if (!InitializeTopBarDialog())
    {
        m_bridge.LogError("Studio: failed to initialize the engine-owned Studio top bar.");
        m_bridge.Shutdown();
        return false;
    }

    if (!InitializeProjectExplorerPanel())
    {
        m_bridge.LogError("Studio: failed to initialize the project explorer panel.");
        m_bridge.Shutdown();
        return false;
    }

    m_commandRouter.Bind(&m_bridge);
    LogStudioShellStatus();
    m_initialized = true;
    return true;
}

void StudioHost::Run()
{
    if (!m_initialized)
    {
        return;
    }

    m_bridge.LogInfo("Studio: native host entering main loop.");
    m_bridge.LogInfo("Studio: close the native window or press Escape to stop the studio host.");

    while (!m_bridge.ShouldExit())
    {
        if (!m_bridge.Tick())
        {
            break;
        }

        m_httpServer.Tick(m_bridge);
    }
}

void StudioHost::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    if (m_topBarDialogHandle != 0U)
    {
        m_bridge.GetCapabilities().ui->DestroyWebDialog(m_topBarDialogHandle);
        m_topBarDialogHandle = 0U;
    }

    if (m_coreToolsDialogHandle != 0U)
    {
        m_bridge.GetCapabilities().ui->DestroyWebDialog(m_coreToolsDialogHandle);
        m_coreToolsDialogHandle = 0U;
    }

    m_projectExplorerPanel.Close(m_bridge.GetCapabilities());
    m_newProjectDialog.Close(m_bridge.GetCapabilities());
    m_fileManagementDialog.Close(m_bridge.GetCapabilities());
    m_httpServer.Stop(m_bridge);

    m_bridge.Shutdown();
    m_initialized = false;
}

void StudioHost::LogStudioShellStatus()
{
    m_bridge.LogInfo("Studio: shell boot complete.");
    m_bridge.LogInfo("Studio: native outer window is active.");
    m_bridge.LogInfo(
        std::string("Studio: CoreTools entry = ") +
        m_bridge.GetCoreToolsContentPath());
    m_bridge.LogInfo(
        std::string("Studio: top bar entry = ") +
        m_bridge.GetStudioTopBarContentPath());
    m_bridge.LogInfo("Studio: shell chrome = docked-top and docked-left via engine web dialog API.");
    m_bridge.LogInfo("Studio: File actions are driven by HTML dialogs through the Studio HTTP bridge.");
}

bool StudioHost::InitializeStudioHttpBridge()
{
    HttpServerConfig config;
    config.host = "127.0.0.1";
    config.port = kStudioHttpPort;
    config.webRootPath = GetCoreToolsRuntimeRootPath();
    config.indexFilePath = "studio-top.html";

    m_httpServer.SetRequestHandler(
        [this](const std::string& path, std::string& outContentType, std::string& outBody)
        {
            return HandleStudioHttpRequest(path, outContentType, outBody);
        });

    return m_httpServer.Start(config, m_bridge);
}

bool StudioHost::InitializeTopBarDialog()
{
    WebDialogDesc dialogDesc;
    dialogDesc.name = "StudioTopBar";
    dialogDesc.title = L"Studio Top Bar";
    dialogDesc.contentPath = m_bridge.GetStudioTopBarContentPath();
    dialogDesc.dockMode = WebDialogDockMode::Top;
    dialogDesc.dockSize = kStudioTopBarHeight;
    dialogDesc.visible = true;
    dialogDesc.resizable = false;

    m_topBarDialogHandle = m_bridge.GetCapabilities().ui->CreateWebDialog(dialogDesc);
    return m_topBarDialogHandle != 0U;
}

bool StudioHost::InitializeCoreToolsDialog()
{
    WebDialogDesc dialogDesc;
    dialogDesc.name = "CoreTools";
    dialogDesc.title = L"CoreTools";
    dialogDesc.contentPath = m_bridge.GetCoreToolsContentPath();
    dialogDesc.dockMode = WebDialogDockMode::Left;
    dialogDesc.dockSize = kCoreToolsDockWidth;
    dialogDesc.visible = true;
    dialogDesc.resizable = false;

    m_coreToolsDialogHandle = m_bridge.GetCapabilities().ui->CreateWebDialog(dialogDesc);
    return m_coreToolsDialogHandle != 0U;
}

bool StudioHost::InitializeProjectExplorerPanel()
{
    return m_projectExplorerPanel.Open(m_bridge.GetCapabilities(), GetProjectExplorerContentPath());
}

bool StudioHost::HandleStudioHttpRequest(const std::string& path, std::string& outContentType, std::string& outBody)
{
    const std::string route = StripQuery(path);
    outContentType = "application/json; charset=utf-8";

    if (route == "/studio/open-file-management")
    {
        const bool opened = OpenFileManagementDialog();
        outBody = BuildJsonResponse(opened, opened ? "File dialog opened." : "Failed to open File dialog.");
        return true;
    }

    if (route == "/studio/open-new-project")
    {
        const bool opened = OpenNewProjectDialog();
        outBody = BuildJsonResponse(opened, opened ? "New Project dialog opened." : "Failed to open New Project dialog.");
        return true;
    }

    if (route == "/studio/open-project")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        studio::OpenProjectRequest request;
        const auto projectId = query.find("projectId");
        request.projectId = projectId == query.end() ? std::string("pong") : projectId->second;

        const studio::OpenProjectResult result = m_projectSystem.OpenProject(request);
        outBody = BuildJsonResponse(
            result.success,
            result.success ? result.message : result.error,
            result.projectId,
            result.logicalProjectPath);
        return true;
    }

    if (route == "/studio/save-project")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        studio::SaveProjectRequest request;
        const auto projectId = query.find("projectId");
        request.projectId = projectId == query.end() ? std::string("pong") : projectId->second;
        request.editorState = "workspace=open\nlast_project=" + request.projectId + "\n";

        const studio::SaveProjectResult result = m_projectSystem.SaveProject(request);
        outBody = BuildJsonResponse(
            result.success,
            result.success ? result.message : result.error,
            result.projectId,
            result.logicalSavePath);
        return true;
    }

    if (route == "/studio/create-project")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        studio::CreateProjectRequest request;
        const auto projectName = query.find("projectName");
        const auto templateName = query.find("templateName");
        request.projectName = projectName == query.end() ? std::string() : projectName->second;
        request.templateName = templateName == query.end() ? std::string("Arcade2D") : templateName->second;
        request.overwrite = false;

        const studio::CreateProjectResult result = m_projectSystem.CreateProject(request);
        outBody = BuildJsonResponse(
            result.success,
            result.success ? result.message : result.error,
            result.projectId,
            result.logicalProjectPath);
        return true;
    }

    if (route == "/studio/project-explorer/tree")
    {
        outBody = m_projectExplorerPanel.BuildTreeJson(m_projectSystem);
        return true;
    }

    if (route == "/studio/project-explorer/select")
    {
        const std::unordered_map<std::string, std::string> query = ParseQuery(path);
        const auto selectedPath = query.find("path");
        std::string message;
        const bool selected = m_projectExplorerPanel.SelectFile(
            m_projectSystem,
            selectedPath == query.end() ? std::string() : selectedPath->second,
            message);
        outBody = BuildJsonResponse(
            selected,
            message,
            m_projectSystem.GetCurrentProject().projectId,
            m_projectExplorerPanel.GetSelectedFileLogicalPath());
        return true;
    }

    return false;
}

bool StudioHost::OpenFileManagementDialog()
{
    if (!m_fileManagementDialog.Open(m_bridge.GetCapabilities(), GetFileManagementDialogContentPath()))
    {
        m_bridge.LogError("Studio: failed to open File dialog.");
        return false;
    }

    m_bridge.LogInfo("Studio: File dialog opened.");
    return true;
}

bool StudioHost::OpenNewProjectDialog()
{
    if (!m_newProjectDialog.Open(m_bridge.GetCapabilities(), GetNewProjectDialogContentPath()))
    {
        m_bridge.LogError("Studio: failed to open New Project dialog.");
        return false;
    }

    m_bridge.LogInfo("Studio: New Project dialog opened.");
    return true;
}

std::string StudioHost::GetCoreToolsRuntimeRootPath() const
{
    if (!m_bridge.IsInitialized())
    {
        return "apps/studio/coretools/runtime";
    }

    return m_bridge.GetCapabilities().resources->GetAbsolutePath("apps/studio/coretools/runtime");
}

std::string StudioHost::GetFileManagementDialogContentPath() const
{
    if (!m_bridge.IsInitialized())
    {
        return "apps/studio/coretools/runtime/file-management.html";
    }

    return m_bridge.GetCapabilities().resources->GetAbsolutePath("apps/studio/coretools/runtime/file-management.html");
}

std::string StudioHost::GetNewProjectDialogContentPath() const
{
    if (!m_bridge.IsInitialized())
    {
        return "apps/studio/coretools/runtime/new-project.html";
    }

    return m_bridge.GetCapabilities().resources->GetAbsolutePath("apps/studio/coretools/runtime/new-project.html");
}

std::string StudioHost::GetProjectExplorerContentPath() const
{
    if (!m_bridge.IsInitialized())
    {
        return "apps/studio/coretools/runtime/project-explorer.html";
    }

    return m_bridge.GetCapabilities().resources->GetAbsolutePath("apps/studio/coretools/runtime/project-explorer.html");
}
