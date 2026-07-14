#include "studio/studio_command_query_service.hpp"

#include "runtime/host/studio_runtime_host.hpp"
#include "studio/proxy_library.hpp"
#include "studio/studio_project_system.hpp"

#include "runtime/scene/runtime_world.hpp"

#include <string>
#include <utility>
#include <vector>

namespace
{
    const char* MeshName(BuiltInMeshKind mesh)
    {
        switch (mesh)
        {
        case BuiltInMeshKind::Cube: return "builtin:cube";
        case BuiltInMeshKind::Quad: return "builtin:quad";
        case BuiltInMeshKind::Octahedron: return "builtin:octahedron";
        case BuiltInMeshKind::Diamond: return "builtin:diamond";
        case BuiltInMeshKind::Triangle: return "builtin:triangle";
        case BuiltInMeshKind::None:
        default:
            return "builtin:none";
        }
    }

    std::string EscapeJson(const std::string& value)
    {
        std::string escaped;
        escaped.reserve(value.size());

        for (char ch : value)
        {
            if (ch == '\\' || ch == '"')
            {
                escaped.push_back('\\');
            }

            escaped.push_back(ch);
        }

        return escaped;
    }

    std::string BuildWorkspaceActionJsonResponse(
        bool ok,
        bool leftVisible,
        bool bottomVisible,
        bool rightVisible,
        const std::string& message)
    {
        return std::string("{\"ok\":") +
            (ok ? "true" : "false") +
            ",\"leftVisible\":" + (leftVisible ? "true" : "false") +
            ",\"bottomVisible\":" + (bottomVisible ? "true" : "false") +
            ",\"rightVisible\":" + (rightVisible ? "true" : "false") +
            ",\"message\":\"" + EscapeJson(message) + "\"}";
    }
}

void StudioCommandQueryService::Configure(
    const studio::StudioProjectSystem* projectSystem,
    const studio_runtime::StudioRuntimeHost* runtimeHost,
    const studio::ProxyLibrary* proxyLibrary,
    StringCallback proxyFolderPathProvider,
    BoolCallback coreToolsVisibleProvider,
    BoolCallback consoleVisibleProvider,
    BoolCallback explorerVisibleProvider,
    StringCallback activeScenePathProvider)
{
    m_projectSystem = projectSystem;
    m_runtimeHost = runtimeHost;
    m_proxyLibrary = proxyLibrary;
    m_proxyFolderPathProvider = std::move(proxyFolderPathProvider);
    m_coreToolsVisibleProvider = std::move(coreToolsVisibleProvider);
    m_consoleVisibleProvider = std::move(consoleVisibleProvider);
    m_explorerVisibleProvider = std::move(explorerVisibleProvider);
    m_activeScenePathProvider = std::move(activeScenePathProvider);
}

bool StudioCommandQueryService::HandleCommandQuery(
    const std::string& command,
    std::string& outDataJson,
    std::string& outMessage) const
{
    if (command == "get_workspace_status")
    {
        outDataJson = BuildWorkspaceStateJson();
        outMessage = "Workspace status read.";
        return true;
    }
    if (command == "get_project_status")
    {
        outDataJson = BuildProjectStatusJson();
        outMessage = "Project status read.";
        return true;
    }
    if (command == "list_entities")
    {
        outDataJson = BuildEntityListJson();
        outMessage = "Entity list read.";
        return true;
    }
    if (command == "get_selected_entity")
    {
        outDataJson = BuildSelectedEntityJson();
        outMessage = "Selection read.";
        return true;
    }
    if (command == "get_runtime_status")
    {
        outDataJson = BuildRuntimeStatusJson();
        outMessage = "Runtime status read.";
        return true;
    }

    return false;
}

std::string StudioCommandQueryService::BuildWorkspaceStateJson(const std::string& message) const
{
    const bool leftVisible = m_coreToolsVisibleProvider ? m_coreToolsVisibleProvider() : false;
    const bool bottomVisible = m_consoleVisibleProvider ? m_consoleVisibleProvider() : false;
    const bool rightVisible = m_explorerVisibleProvider ? m_explorerVisibleProvider() : false;
    return BuildWorkspaceActionJsonResponse(true, leftVisible, bottomVisible, rightVisible, message);
}

std::string StudioCommandQueryService::BuildProjectStatusJson() const
{
    const studio::StudioProject project = m_projectSystem ? m_projectSystem->GetCurrentProject() : studio::StudioProject{};
    const std::string scenePath = m_activeScenePathProvider ? m_activeScenePathProvider() : std::string();
    const std::string activeRuntimeArtifact =
        !project.packagedRuntimeArtifactPath.empty() ? project.packagedRuntimeArtifactPath :
        (!project.builtRuntimeArtifactPath.empty() ? project.builtRuntimeArtifactPath : project.exportedArtifactPath);
    std::string json = "{\"ok\":true";
    json += ",\"open\":" + std::string(project.isOpen ? "true" : "false");
    json += ",\"id\":\"" + EscapeJson(project.projectId) + "\"";
    json += ",\"name\":\"" + EscapeJson(project.projectName) + "\"";
    json += ",\"logicalPath\":\"" + EscapeJson(project.logicalProjectPath) + "\"";
    json += ",\"runtimeArtifact\":\"" + EscapeJson(project.exportedArtifactPath) + "\"";
    json += ",\"activeRuntimeArtifact\":\"" + EscapeJson(activeRuntimeArtifact) + "\"";
    json += ",\"builtRuntimeArtifact\":\"" + EscapeJson(project.builtRuntimeArtifactPath) + "\"";
    json += ",\"packagedRuntimeArtifact\":\"" + EscapeJson(project.packagedRuntimeArtifactPath) + "\"";
    json += ",\"packagedExecutable\":\"" + EscapeJson(project.packagedExecutablePath) + "\"";
    json += ",\"exportPath\":\"" + EscapeJson(project.logicalExportPath) + "\"";
    json += ",\"scene\":\"" + EscapeJson(scenePath) + "\"";
    json += "}";
    return json;
}

std::string StudioCommandQueryService::BuildEntityListJson() const
{
    if (!m_runtimeHost)
    {
        return "{\"ok\":true,\"entities\":[]}";
    }

    const std::vector<studio_runtime::EntityId> ids =
        m_runtimeHost->GetConnector().Queries().GetEntityList();
    std::string json = "{\"ok\":true,\"entities\":[";
    bool first = true;
    for (const studio_runtime::EntityId id : ids)
    {
        studio_runtime::EntityInfo info;
        if (!m_runtimeHost->GetConnector().Queries().GetEntityInfo(id, info))
        {
            continue;
        }

        if (!first)
        {
            json += ",";
        }
        first = false;
        json += "{\"id\":" + std::to_string(id);
        json += ",\"name\":\"" + EscapeJson(info.name) + "\"";
        json += ",\"selected\":" + std::string(
            m_runtimeHost->GetInteraction().IsEntitySelected(id) ? "true" : "false");
        json += ",\"hasRenderable\":" + std::string(info.hasRenderable ? "true" : "false");
        json += ",\"hasLight\":" + std::string(info.hasLight ? "true" : "false");
        json += ",\"hasCamera\":" + std::string(info.hasCamera ? "true" : "false");
        json += "}";
    }
    json += "]}";
    return json;
}

std::string StudioCommandQueryService::BuildSelectedEntityJson() const
{
    if (!m_runtimeHost)
    {
        return "{\"ok\":true,\"hasSelection\":false}";
    }

    const studio_runtime::EntityId selectedEntity = m_runtimeHost->GetInteraction().GetState().selectedEntity;
    if (selectedEntity == 0)
    {
        return "{\"ok\":true,\"hasSelection\":false}";
    }

    studio_runtime::EntityInfo info;
    if (!m_runtimeHost->GetConnector().Queries().GetEntityInfo(selectedEntity, info))
    {
        return "{\"ok\":true,\"hasSelection\":false}";
    }

    const TransformPrototype& transform = info.transform;
    std::string json = "{\"ok\":true,\"hasSelection\":true";
    json += ",\"id\":" + std::to_string(info.id);
    json += ",\"selectionCount\":" + std::to_string(m_runtimeHost->GetInteraction().GetState().selectedEntities.size());
    json += ",\"name\":\"" + EscapeJson(info.name) + "\"";
    json += ",\"hasObject\":" + std::string(info.hasObject ? "true" : "false");
    if (info.hasObject)
    {
        json += ",\"object\":{";
        json += "\"kind\":\"" + EscapeJson(info.object.kind) + "\"";
        json += ",\"prototype\":\"" + EscapeJson(info.object.prototypeId) + "\"";
        json += ",\"selectable\":" + std::string(info.object.selectable ? "true" : "false");
        json += "}";
    }

    const std::string proxyFolderPath = m_proxyFolderPathProvider ? m_proxyFolderPathProvider() : std::string();
    if (m_proxyLibrary &&
        info.hasObject &&
        !info.object.prototypeId.empty() &&
        !proxyFolderPath.empty())
    {
        studio::ProxyPrototypeInfo proto;
        if (m_proxyLibrary->GetPrototypeInfoById(proxyFolderPath, info.object.prototypeId, proto) && proto.found)
        {
            json += ",\"prototypeInfo\":{";
            json += "\"id\":\"" + EscapeJson(proto.id) + "\"";
            json += ",\"name\":\"" + EscapeJson(proto.name) + "\"";
            json += ",\"type\":\"" + EscapeJson(proto.type) + "\"";
            json += ",\"version\":\"" + EscapeJson(proto.version) + "\"";
            json += ",\"mesh\":\"" + EscapeJson(proto.meshAsset) + "\"";
            json += ",\"material\":\"" + EscapeJson(proto.materialAsset) + "\"";
            json += ",\"light\":\"" + EscapeJson(proto.lightAsset) + "\"";
            json += ",\"sourcePath\":\"" + EscapeJson(proto.sourcePath) + "\"";
            json += ",\"visible\":" + std::string(proto.visible ? "true" : "false");
            json += ",\"components\":[";
            for (std::size_t i = 0; i < proto.components.size(); ++i)
            {
                if (i > 0U) json += ",";
                json += "\"" + EscapeJson(proto.components[i]) + "\"";
            }
            json += "],\"tags\":[";
            for (std::size_t i = 0; i < proto.tags.size(); ++i)
            {
                if (i > 0U) json += ",";
                json += "\"" + EscapeJson(proto.tags[i]) + "\"";
            }
            json += "],\"chain\":[";
            for (std::size_t i = 0; i < proto.chain.size(); ++i)
            {
                if (i > 0U) json += ",";
                json += "\"" + EscapeJson(proto.chain[i]) + "\"";
            }
            json += "],\"layers\":[";
            for (std::size_t i = 0; i < proto.layers.size(); ++i)
            {
                if (i > 0U) json += ",";
                json += "\"" + EscapeJson(proto.layers[i]) + "\"";
            }
            json += "]";
            if (proto.hasDefaultTransform)
            {
                json += ",\"defaultTransform\":{";
                json += "\"position\":{\"x\":" + std::to_string(proto.defaultTransform.translation.x) + ",\"y\":" + std::to_string(proto.defaultTransform.translation.y) + ",\"z\":" + std::to_string(proto.defaultTransform.translation.z) + "}";
                json += ",\"rotation\":{\"x\":" + std::to_string(proto.defaultTransform.rotationRadians.x) + ",\"y\":" + std::to_string(proto.defaultTransform.rotationRadians.y) + ",\"z\":" + std::to_string(proto.defaultTransform.rotationRadians.z) + "}";
                json += ",\"scale\":{\"x\":" + std::to_string(proto.defaultTransform.scale.x) + ",\"y\":" + std::to_string(proto.defaultTransform.scale.y) + ",\"z\":" + std::to_string(proto.defaultTransform.scale.z) + "}";
                json += "}";
            }
            json += "}";
        }
    }

    json += ",\"transform\":{";
    json += "\"position\":{\"x\":" + std::to_string(transform.translation.x) + ",\"y\":" + std::to_string(transform.translation.y) + ",\"z\":" + std::to_string(transform.translation.z) + "}";
    json += ",\"rotation\":{\"x\":" + std::to_string(transform.rotationRadians.x) + ",\"y\":" + std::to_string(transform.rotationRadians.y) + ",\"z\":" + std::to_string(transform.rotationRadians.z) + "}";
    json += ",\"scale\":{\"x\":" + std::to_string(transform.scale.x) + ",\"y\":" + std::to_string(transform.scale.y) + ",\"z\":" + std::to_string(transform.scale.z) + "}";
    json += "}";
    json += ",\"hasRenderable\":" + std::string(info.hasRenderable ? "true" : "false");
    if (info.hasRenderable)
    {
        json += ",\"renderable\":{";
        json += "\"mesh\":\"" + std::string(MeshName(info.renderable.mesh)) + "\"";
        json += ",\"material\":\"default\"";
        json += ",\"visible\":" + std::string(info.renderable.visible ? "true" : "false");
        json += "}";
    }
    json += ",\"hasLight\":" + std::string(info.hasLight ? "true" : "false");
    json += ",\"hasCamera\":" + std::string(info.hasCamera ? "true" : "false");
    if (info.hasCamera)
    {
        json += ",\"camera\":{";
        json += "\"position\":{\"x\":" + std::to_string(info.camera.camera.pose.position.x) + ",\"y\":" + std::to_string(info.camera.camera.pose.position.y) + ",\"z\":" + std::to_string(info.camera.camera.pose.position.z) + "}";
        json += ",\"target\":{\"x\":" + std::to_string(info.camera.camera.pose.target.x) + ",\"y\":" + std::to_string(info.camera.camera.pose.target.y) + ",\"z\":" + std::to_string(info.camera.camera.pose.target.z) + "}";
        json += ",\"fov\":" + std::to_string(info.camera.camera.projection.fieldOfViewDegrees);
        json += ",\"near\":" + std::to_string(info.camera.camera.projection.nearPlane);
        json += ",\"far\":" + std::to_string(info.camera.camera.projection.farPlane);
        json += ",\"enabled\":" + std::string(info.camera.camera.enabled ? "true" : "false");
        json += "}";
    }
    json += "}";
    return json;
}

std::string StudioCommandQueryService::BuildRuntimeStatusJson() const
{
    if (!m_runtimeHost)
    {
        return "{\"ok\":true,\"state\":\"Stopped\",\"fps\":\"\",\"projectId\":\"\",\"artifact\":\"\",\"viewportText\":\"\"}";
    }

    const studio_runtime::ProjectRuntimeDesc& project = m_runtimeHost->GetActiveProject();
    std::string json = "{\"ok\":true";
    json += ",\"state\":\"" + EscapeJson(m_runtimeHost->GetStateName()) + "\"";
    json += ",\"fps\":\"" + EscapeJson(m_runtimeHost->GetFpsText()) + "\"";
    json += ",\"projectId\":\"" + EscapeJson(project.projectId) + "\"";
    json += ",\"artifact\":\"" + EscapeJson(project.exportedArtifactPath) + "\"";
    json += ",\"viewportText\":\"" + EscapeJson(m_runtimeHost->GetViewportText()) + "\"";
    json += "}";
    return json;
}
