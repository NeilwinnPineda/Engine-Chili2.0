#include "studio/studio_build_request_service.hpp"

#include "studio/studio_project_system.hpp"

#include <string>

namespace studio
{
    StudioBuildRouteResult StudioBuildRequestService::ExecuteBuildRequest(
        StudioProjectSystem& projectSystem,
        const StudioBuildSystem& buildSystem,
        bool runAfterBuild,
        bool exportAfterBuild) const
    {
        return ExecuteBuildRequestWithInvoker(
            projectSystem,
            [&buildSystem](const BuildAndRunProjectRequest& request)
            {
                return buildSystem.BuildAndRunProject(request);
            },
            runAfterBuild,
            exportAfterBuild);
    }

    StudioBuildRouteResult StudioBuildRequestService::ExecuteBuildRequestWithInvoker(
        StudioProjectSystem& projectSystem,
        BuildInvoker buildInvoker,
        bool runAfterBuild,
        bool exportAfterBuild) const
    {
        StudioBuildRouteResult routeResult;

        const StudioProject project = projectSystem.GetCurrentProject();
        if (!project.isOpen)
        {
            routeResult.message = "No project open.";
            routeResult.consoleLines.push_back("Build request rejected: no project open.");
            return routeResult;
        }

        BuildAndRunProjectRequest request;
        request.projectId = project.projectId;
        request.runAfterBuild = runAfterBuild;
        request.exportAfterBuild = exportAfterBuild;

        routeResult.projectId = request.projectId;
        routeResult.consoleLines.push_back("Build requested for project '" + request.projectId + "'.");

        if (!buildInvoker)
        {
            routeResult.message = "Build invoker is not configured.";
            routeResult.consoleLines.push_back("Build failed for '" + request.projectId + "': Build invoker is not configured.");
            return routeResult;
        }

        const BuildAndRunProjectResult result = buildInvoker(request);
        routeResult.success = result.success;
        routeResult.message = result.success ? result.message : result.error;
        routeResult.logicalPath = result.logicalExportPath.empty() ? result.logicalBuildPath : result.logicalExportPath;

        if (!result.success)
        {
            routeResult.consoleLines.push_back(
                "Build failed for '" + request.projectId +
                "' (configureExit=" + std::to_string(result.configureExitCode) +
                ", buildExit=" + std::to_string(result.buildExitCode) +
                "): " + (result.error.empty() ? "Unknown error." : result.error));
            return routeResult;
        }

        projectSystem.RecordBuildOutputs(
            result.builtRuntimeOutputPath,
            result.packagedRuntimeOutputPath,
            result.packagedExecutablePath,
            result.logicalExportPath);

        routeResult.consoleLines.push_back(result.message);
        if (!result.logicalExportPath.empty())
        {
            routeResult.consoleLines.push_back("Export output: " + result.logicalExportPath);
        }
        if (!result.runtimeOutputPath.empty())
        {
            routeResult.consoleLines.push_back("Runtime output: " + result.runtimeOutputPath);
        }
        if (!result.executablePath.empty() && request.runAfterBuild)
        {
            routeResult.consoleLines.push_back("Preview executable: " + result.executablePath);
        }

        return routeResult;
    }
}
