#include "studio/studio_project_system.hpp"

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <utility>

namespace studio
{
    namespace
    {
        constexpr int kDefaultProjectDisplayWidth = 1280;
        constexpr int kDefaultProjectDisplayHeight = 720;
        constexpr float kDefaultProjectAspectRatio = 16.0f / 9.0f;

        studio_runtime::ProjectCodeEntryKind ParseProjectCodeEntryKind(const std::string& value)
        {
            if (value == "native_artifact")
            {
                return studio_runtime::ProjectCodeEntryKind::NativeArtifact;
            }
            if (value == "lua")
            {
                return studio_runtime::ProjectCodeEntryKind::LuaScript;
            }
            if (value == "external_adapter")
            {
                return studio_runtime::ProjectCodeEntryKind::ExternalAdapter;
            }

            return studio_runtime::ProjectCodeEntryKind::NativeInProcess;
        }

        bool TryParseInteger(const std::string& text, int& outValue)
        {
            if (text.empty())
            {
                return false;
            }

            char* end = nullptr;
            const long value = std::strtol(text.c_str(), &end, 10);
            if (end == text.c_str() || (end && *end != '\0'))
            {
                return false;
            }

            outValue = static_cast<int>(value);
            return true;
        }

        bool TryParseFloat(const std::string& text, float& outValue)
        {
            if (text.empty())
            {
                return false;
            }

            char* end = nullptr;
            const float value = std::strtof(text.c_str(), &end);
            if (end == text.c_str() || (end && *end != '\0'))
            {
                return false;
            }

            outValue = value;
            return true;
        }

        bool TryParseBool(const std::string& text, bool& outValue)
        {
            if (text == "true" || text == "1" || text == "yes")
            {
                outValue = true;
                return true;
            }

            if (text == "false" || text == "0" || text == "no")
            {
                outValue = false;
                return true;
            }

            return false;
        }

        float ParseAspectRatioText(const std::string& text)
        {
            if (text.empty())
            {
                return 0.0f;
            }

            const std::size_t colon = text.find(':');
            if (colon != std::string::npos)
            {
                float left = 0.0f;
                float right = 0.0f;
                if (TryParseFloat(text.substr(0U, colon), left) &&
                    TryParseFloat(text.substr(colon + 1U), right) &&
                    right > 0.0001f)
                {
                    return left / right;
                }
            }

            float value = 0.0f;
            return TryParseFloat(text, value) ? value : 0.0f;
        }

        std::string ExtractManifestField(const std::string& manifestText, const std::string& fieldName);

        studio_runtime::ProjectDisplaySettings ParseProjectDisplaySettings(const std::string& manifestText)
        {
            studio_runtime::ProjectDisplaySettings settings;

            int width = 0;
            if (TryParseInteger(ExtractManifestField(manifestText, "display_width"), width) && width > 0)
            {
                settings.targetWidth = width;
            }

            int height = 0;
            if (TryParseInteger(ExtractManifestField(manifestText, "display_height"), height) && height > 0)
            {
                settings.targetHeight = height;
            }

            bool lockAspect = false;
            if (TryParseBool(ExtractManifestField(manifestText, "display_lock_aspect"), lockAspect))
            {
                settings.lockAspectRatio = lockAspect;
            }

            const float parsedAspect = ParseAspectRatioText(ExtractManifestField(manifestText, "display_aspect_ratio"));
            if (parsedAspect > 0.0001f)
            {
                settings.aspectRatio = parsedAspect;
            }
            else if (settings.targetWidth > 0 && settings.targetHeight > 0)
            {
                settings.aspectRatio = static_cast<float>(settings.targetWidth) / static_cast<float>(settings.targetHeight);
            }
            else
            {
                settings.targetWidth = kDefaultProjectDisplayWidth;
                settings.targetHeight = kDefaultProjectDisplayHeight;
                settings.aspectRatio = kDefaultProjectAspectRatio;
            }

            return settings;
        }

        std::string JoinVirtualPath(const std::string& left, const std::string& right)
        {
            if (left.empty())
            {
                return right;
            }

            if (right.empty())
            {
                return left;
            }

            return left + "/" + right;
        }

        std::string BuildManifest(const CreateProjectRequest& request, const std::string& projectId)
        {
            std::ostringstream manifest;
            manifest
                << "name = " << request.projectName << "\n"
                << "id = " << projectId << "\n"
                << "template = " << request.templateName << "\n"
                << "runtime_kind = native_artifact\n"
                << "runtime_artifact = ../Build/bin/" << projectId << "_runtime.dll\n"
                << "build_configure = cmake -S . -B ../Build -G Ninja\n"
                << "build_command = cmake --build ../Build\n"
                << "build_output = ../Build/bin/" << projectId << "_runtime.dll\n"
                << "preview_output = ../Build/bin/" << projectId << ".exe\n"
                << "default_scene = scenes/main.scene\n"
                << "source_header = src/" << projectId << ".hpp\n"
                << "source_entry = src/" << projectId << ".cpp\n"
                << "display_width = " << kDefaultProjectDisplayWidth << "\n"
                << "display_height = " << kDefaultProjectDisplayHeight << "\n"
                << "display_aspect_ratio = 16:9\n"
                << "display_lock_aspect = true\n"
                << "target_fps = 60\n";
            return manifest.str();
        }

        std::string BuildProjectJson(
            const CreateProjectRequest& request,
            const std::string& projectId,
            const std::string& assetProxyFolder)
        {
            std::ostringstream json;
            json
                << "{\n"
                << "  \"name\": \"" << request.projectName << "\",\n"
                << "  \"id\": \"" << projectId << "\",\n"
                << "  \"template\": \"" << request.templateName << "\",\n"
                << "  \"runtimeKind\": \"native_artifact\",\n"
                << "  \"runtimeArtifact\": \"../Build/bin/" << projectId << "_runtime.dll\",\n"
                << "  \"defaultScene\": \"scenes/main.scene\",\n"
                << "  \"sourceHeader\": \"src/" << projectId << ".hpp\",\n"
                << "  \"sourceEntry\": \"src/" << projectId << ".cpp\",\n"
                << "  \"displayWidth\": " << kDefaultProjectDisplayWidth << ",\n"
                << "  \"displayHeight\": " << kDefaultProjectDisplayHeight << ",\n"
                << "  \"displayAspectRatio\": \"16:9\",\n"
                << "  \"displayLockAspect\": true,\n"
                << "  \"targetFps\": 60,\n"
                << "  \"assetProxyFolder\": \"" << assetProxyFolder << "\"\n"
                << "}\n";
            return json.str();
        }

        bool IsSingleSegmentProjectId(const std::string& projectId)
        {
            return !projectId.empty() &&
                projectId.find('/') == std::string::npos &&
                projectId.find('\\') == std::string::npos &&
                projectId.find("..") == std::string::npos;
        }

        std::string ExtractManifestField(const std::string& manifestText, const std::string& fieldName)
        {
            const std::string prefix = fieldName + " = ";
            std::stringstream stream(manifestText);
            std::string line;

            while (std::getline(stream, line))
            {
                if (line.find(prefix) == 0U)
                {
                    return line.substr(prefix.size());
                }
            }

            return std::string();
        }

        std::string ExtractJsonStringField(const std::string& text, const std::string& fieldName)
        {
            const std::string key = "\"" + fieldName + "\"";
            const std::size_t keyPos = text.find(key);
            if (keyPos == std::string::npos)
            {
                return std::string();
            }

            const std::size_t colonPos = text.find(':', keyPos + key.size());
            if (colonPos == std::string::npos)
            {
                return std::string();
            }

            const std::size_t firstQuote = text.find('"', colonPos + 1U);
            if (firstQuote == std::string::npos)
            {
                return std::string();
            }

            const std::size_t secondQuote = text.find('"', firstQuote + 1U);
            if (secondQuote == std::string::npos || secondQuote <= firstQuote)
            {
                return std::string();
            }

            return text.substr(firstQuote + 1U, secondQuote - firstQuote - 1U);
        }
    }

    StudioProjectSystem::StudioProjectSystem()
        : StudioProjectSystem(FileProxy{})
    {
    }

    StudioProjectSystem::StudioProjectSystem(FileProxy files)
        : m_files(std::move(files))
        , m_templates(m_files)
    {
    }

    CreateProjectResult StudioProjectSystem::CreateProject(const CreateProjectRequest& request)
    {
        CreateProjectResult result;
        result.projectId = MakeProjectId(request.projectName);

        if (request.projectName.empty())
        {
            result.error = "Project name is required.";
            return result;
        }

        if (result.projectId.empty())
        {
            result.error = "Project name does not contain any safe project id characters.";
            return result;
        }

        if (request.templateName.empty())
        {
            result.error = "Template name is required.";
            return result;
        }

        std::string error;
        if (!m_files.EnsureWorkspaceFolders(error))
        {
            result.error = error;
            return result;
        }

        if (!IsSingleSegmentProjectId(result.projectId))
        {
            result.error = "Project id must be a single safe path segment.";
            return result;
        }

        const std::string projectPath = GetProjectSourcePath(result.projectId);
        result.logicalProjectPath = JoinVirtualPath(m_files.GetLogicalRoot(), projectPath);

        if (m_files.Exists(GetProjectWorkspacePath(result.projectId)) && !request.overwrite)
        {
            result.error = "Project already exists: " + JoinVirtualPath(m_files.GetLogicalRoot(), GetProjectWorkspacePath(result.projectId));
            return result;
        }

        if (!CreateProjectFolders(result.projectId, error))
        {
            result.error = error;
            return result;
        }

        if (!WriteManifest(request, result.projectId, projectPath, error))
        {
            result.error = error;
            return result;
        }

        const std::string defaultProxyFolder = "../ChiliProxyLibrary";
        if (!m_files.WriteText(
                JoinVirtualPath(projectPath, "project.chili.json"),
                BuildProjectJson(request, result.projectId, defaultProxyFolder),
                error))
        {
            result.error = error;
            return result;
        }

        const ApplyTemplateResult templateResult = m_templates.ApplyTemplate(
            request.templateName,
            projectPath,
            request.projectName,
            result.projectId);
        if (!templateResult.success)
        {
            result.error = templateResult.error;
            return result;
        }

        result.success = true;
        result.message = "Created project '" + request.projectName + "' at " + result.logicalProjectPath + ".";
        const studio_runtime::ProjectDisplaySettings displaySettings = ParseProjectDisplaySettings(BuildManifest(request, result.projectId));
        m_currentProject.isOpen = true;
        m_currentProject.projectId = result.projectId;
        m_currentProject.projectName = request.projectName;
        m_currentProject.logicalProjectPath = result.logicalProjectPath;
        m_currentProject.projectRootPath = projectPath;
        m_currentProject.previewRuntimeName.clear();
        m_currentProject.codeEntryKind = studio_runtime::ProjectCodeEntryKind::NativeArtifact;
        m_currentProject.exportedArtifactPath = "../Build/bin/" + result.projectId + "_runtime.dll";
        m_currentProject.builtRuntimeArtifactPath.clear();
        m_currentProject.packagedRuntimeArtifactPath.clear();
        m_currentProject.packagedExecutablePath.clear();
        m_currentProject.logicalExportPath.clear();
        m_currentProject.defaultScenePath = "scenes/main.scene";
        m_currentProject.sourceEntryPath = "src/" + result.projectId + ".cpp";
        m_currentProject.displaySettings = displaySettings;
        m_currentProject.assetProxyFolder = defaultProxyFolder;
        m_currentProject.manifestText = BuildManifest(request, result.projectId);
        return result;
    }

    OpenProjectResult StudioProjectSystem::OpenProject(const OpenProjectRequest& request)
    {
        OpenProjectResult result;
        result.projectId = MakeProjectId(request.projectId);

        if (result.projectId.empty() || !IsSingleSegmentProjectId(result.projectId))
        {
            result.error = "A valid project id is required.";
            return result;
        }

        std::string error;
        if (!m_files.EnsureWorkspaceFolders(error))
        {
            result.error = error;
            return result;
        }

        const std::string projectPath = GetProjectSourcePath(result.projectId);
        const std::string manifestPath = JoinVirtualPath(projectPath, "project.enginegame");
        const std::string projectJsonPath = JoinVirtualPath(projectPath, "project.chili.json");
        result.logicalProjectPath = JoinVirtualPath(m_files.GetLogicalRoot(), projectPath);

        if (!m_files.Exists(manifestPath))
        {
            result.error = "Project manifest was not found: " + JoinVirtualPath(m_files.GetLogicalRoot(), manifestPath);
            return result;
        }

        if (!m_files.ReadText(manifestPath, result.manifestText, error))
        {
            result.error = error;
            return result;
        }

        result.projectName = ExtractManifestField(result.manifestText, "name");
        if (result.projectName.empty())
        {
            result.projectName = result.projectId;
        }
        result.codeEntryKind = ParseProjectCodeEntryKind(ExtractManifestField(result.manifestText, "runtime_kind"));
        result.previewRuntimeName = ExtractManifestField(result.manifestText, "runtime");
        if (result.previewRuntimeName.empty() &&
            result.codeEntryKind == studio_runtime::ProjectCodeEntryKind::NativeInProcess)
        {
            result.previewRuntimeName = "StudioPreviewRuntime";
        }
        result.exportedArtifactPath = ExtractManifestField(result.manifestText, "runtime_artifact");
        result.scriptEntryPath = ExtractManifestField(result.manifestText, "script_entry");
        result.adapterExecutablePath = ExtractManifestField(result.manifestText, "adapter_entry");
        result.defaultScenePath = ExtractManifestField(result.manifestText, "default_scene");
        result.sourceEntryPath = ExtractManifestField(result.manifestText, "source_entry");
        result.displaySettings = ParseProjectDisplaySettings(result.manifestText);
        result.assetProxyFolder = "../ChiliProxyLibrary";
        std::string projectJsonText;
        if (m_files.Exists(projectJsonPath) && m_files.ReadText(projectJsonPath, projectJsonText, error))
        {
            const std::string jsonProxyFolder = ExtractJsonStringField(projectJsonText, "assetProxyFolder");
            if (!jsonProxyFolder.empty())
            {
                result.assetProxyFolder = jsonProxyFolder;
            }
        }

        result.success = true;
        result.message = "Opened project '" + result.projectName + "' at " + result.logicalProjectPath + ".";
        m_currentProject.isOpen = true;
        m_currentProject.projectId = result.projectId;
        m_currentProject.projectName = result.projectName;
        m_currentProject.logicalProjectPath = result.logicalProjectPath;
        m_currentProject.projectRootPath = projectPath;
        m_currentProject.previewRuntimeName = result.previewRuntimeName;
        m_currentProject.codeEntryKind = result.codeEntryKind;
        m_currentProject.exportedArtifactPath = result.exportedArtifactPath;
        m_currentProject.builtRuntimeArtifactPath.clear();
        m_currentProject.packagedRuntimeArtifactPath.clear();
        m_currentProject.packagedExecutablePath.clear();
        m_currentProject.logicalExportPath.clear();
        m_currentProject.scriptEntryPath = result.scriptEntryPath;
        m_currentProject.adapterExecutablePath = result.adapterExecutablePath;
        m_currentProject.defaultScenePath = result.defaultScenePath;
        m_currentProject.sourceEntryPath = result.sourceEntryPath;
        m_currentProject.displaySettings = result.displaySettings;
        m_currentProject.assetProxyFolder = result.assetProxyFolder;
        m_currentProject.manifestText = result.manifestText;
        return result;
    }

    SaveProjectResult StudioProjectSystem::SaveProject(const SaveProjectRequest& request)
    {
        SaveProjectResult result;
        result.projectId = MakeProjectId(request.projectId);

        if (result.projectId.empty() || !IsSingleSegmentProjectId(result.projectId))
        {
            result.error = "A valid project id is required.";
            return result;
        }

        std::string error;
        if (!m_files.EnsureWorkspaceFolders(error))
        {
            result.error = error;
            return result;
        }

        const std::string projectPath = GetProjectSourcePath(result.projectId);
        const std::string manifestPath = JoinVirtualPath(projectPath, "project.enginegame");
        result.logicalProjectPath = JoinVirtualPath(m_files.GetLogicalRoot(), projectPath);

        if (!m_files.Exists(manifestPath))
        {
            result.error = "Cannot save because project manifest was not found: " + JoinVirtualPath(m_files.GetLogicalRoot(), manifestPath);
            return result;
        }

        const std::string savePath = JoinVirtualPath(GetProjectWorkspacePath(result.projectId), "Project/studio.session");
        if (!m_files.WriteText(savePath, request.editorState, error))
        {
            result.error = error;
            return result;
        }

        result.logicalSavePath = JoinVirtualPath(m_files.GetLogicalRoot(), savePath);
        result.success = true;
        result.message = "Saved project session for '" + result.projectId + "' to " + result.logicalSavePath + ".";
        return result;
    }

    StudioProject StudioProjectSystem::GetCurrentProject() const
    {
        return m_currentProject;
    }

    std::string StudioProjectSystem::GetCurrentProjectRoot() const
    {
        return m_currentProject.isOpen ? GetProjectSourcePath(m_currentProject.projectId) : std::string();
    }

    void StudioProjectSystem::RecordBuildOutputs(
        const std::string& builtRuntimeArtifactPath,
        const std::string& packagedRuntimeArtifactPath,
        const std::string& packagedExecutablePath,
        const std::string& logicalExportPath)
    {
        if (!m_currentProject.isOpen)
        {
            return;
        }

        m_currentProject.builtRuntimeArtifactPath = builtRuntimeArtifactPath;
        m_currentProject.packagedRuntimeArtifactPath = packagedRuntimeArtifactPath;
        m_currentProject.packagedExecutablePath = packagedExecutablePath;
        m_currentProject.logicalExportPath = logicalExportPath;
    }

    std::string StudioProjectSystem::MakeProjectId(const std::string& projectName)
    {
        std::string projectId;
        bool needsSeparator = false;

        for (const char character : projectName)
        {
            const unsigned char value = static_cast<unsigned char>(character);
            if (std::isalnum(value) != 0)
            {
                if (needsSeparator && !projectId.empty())
                {
                    projectId += '_';
                }

                projectId += static_cast<char>(std::tolower(value));
                needsSeparator = false;
            }
            else
            {
                needsSeparator = !projectId.empty();
            }
        }

        return projectId;
    }

    std::string StudioProjectSystem::GetProjectWorkspacePath(const std::string& projectId)
    {
        return projectId;
    }

    std::string StudioProjectSystem::GetProjectSourcePath(const std::string& projectId)
    {
        return JoinVirtualPath(GetProjectWorkspacePath(projectId), "Project");
    }

    std::string StudioProjectSystem::GetProjectBuildPath(const std::string& projectId)
    {
        return JoinVirtualPath(GetProjectWorkspacePath(projectId), "Build");
    }

    std::string StudioProjectSystem::GetProjectCachePath(const std::string& projectId)
    {
        return JoinVirtualPath(GetProjectWorkspacePath(projectId), "Cache");
    }

    std::string StudioProjectSystem::GetProjectLogsPath(const std::string& projectId)
    {
        return JoinVirtualPath(GetProjectWorkspacePath(projectId), "Logs");
    }

    bool StudioProjectSystem::CreateProjectFolders(const std::string& projectId, std::string& outError) const
    {
        const std::string projectPath = GetProjectSourcePath(projectId);
        return m_files.CreateDirectory(GetProjectWorkspacePath(projectId), outError) &&
            m_files.CreateDirectory(projectPath, outError) &&
            m_files.CreateDirectory(GetProjectBuildPath(projectId), outError) &&
            m_files.CreateDirectory(GetProjectCachePath(projectId), outError) &&
            m_files.CreateDirectory(GetProjectLogsPath(projectId), outError) &&
            m_files.CreateDirectory(JoinVirtualPath(projectPath, "src"), outError) &&
            m_files.CreateDirectory(JoinVirtualPath(projectPath, "assets"), outError) &&
            m_files.CreateDirectory(JoinVirtualPath(projectPath, "scripts"), outError) &&
            m_files.CreateDirectory(JoinVirtualPath(projectPath, "scenes"), outError) &&
            m_files.CreateDirectory(JoinVirtualPath(projectPath, "config"), outError);
    }

    bool StudioProjectSystem::WriteManifest(
        const CreateProjectRequest& request,
        const std::string& projectId,
        const std::string& projectPath,
        std::string& outError) const
    {
        return m_files.WriteText(
            JoinVirtualPath(projectPath, "project.enginegame"),
            BuildManifest(request, projectId),
            outError);
    }

    CreateProjectResult CreateExampleProjectForSmokeTest()
    {
        StudioProjectSystem projects;

        CreateProjectRequest request;
        request.projectName = "Pong";
        request.templateName = "Arcade2D";
        request.overwrite = true;

        return projects.CreateProject(request);
    }
}
