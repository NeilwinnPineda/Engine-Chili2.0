#pragma once

#include "studio/studio_build_system.hpp"

#include <functional>
#include <string>
#include <vector>

namespace studio
{
    class StudioProjectSystem;

    struct StudioBuildRouteResult
    {
        bool success = false;
        std::string message;
        std::string projectId;
        std::string logicalPath;
        std::vector<std::string> consoleLines;
    };

    class StudioBuildRequestService
    {
    public:
        using BuildInvoker = std::function<BuildAndRunProjectResult(const BuildAndRunProjectRequest&)>;

        StudioBuildRouteResult ExecuteBuildRequest(
            StudioProjectSystem& projectSystem,
            const StudioBuildSystem& buildSystem,
            bool runAfterBuild,
            bool exportAfterBuild) const;

        StudioBuildRouteResult ExecuteBuildRequestWithInvoker(
            StudioProjectSystem& projectSystem,
            BuildInvoker buildInvoker,
            bool runAfterBuild,
            bool exportAfterBuild) const;
    };
}
