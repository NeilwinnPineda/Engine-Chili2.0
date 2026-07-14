#pragma once

#include <functional>
#include <string>

namespace studio
{
    class ProxyLibrary;
    class StudioProjectSystem;
}

namespace studio_runtime
{
    class StudioRuntimeHost;
}

class StudioCommandQueryService
{
public:
    using BoolCallback = std::function<bool()>;
    using StringCallback = std::function<std::string()>;

    void Configure(
        const studio::StudioProjectSystem* projectSystem,
        const studio_runtime::StudioRuntimeHost* runtimeHost,
        const studio::ProxyLibrary* proxyLibrary,
        StringCallback proxyFolderPathProvider,
        BoolCallback coreToolsVisibleProvider,
        BoolCallback consoleVisibleProvider,
        BoolCallback explorerVisibleProvider,
        StringCallback activeScenePathProvider);

    bool HandleCommandQuery(
        const std::string& command,
        std::string& outDataJson,
        std::string& outMessage) const;

    std::string BuildWorkspaceStateJson(const std::string& message = std::string()) const;
    std::string BuildProjectStatusJson() const;
    std::string BuildEntityListJson() const;
    std::string BuildSelectedEntityJson() const;
    std::string BuildRuntimeStatusJson() const;

private:
    const studio::StudioProjectSystem* m_projectSystem = nullptr;
    const studio_runtime::StudioRuntimeHost* m_runtimeHost = nullptr;
    const studio::ProxyLibrary* m_proxyLibrary = nullptr;
    StringCallback m_proxyFolderPathProvider;
    BoolCallback m_coreToolsVisibleProvider;
    BoolCallback m_consoleVisibleProvider;
    BoolCallback m_explorerVisibleProvider;
    StringCallback m_activeScenePathProvider;
};
