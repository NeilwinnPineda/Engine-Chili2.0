#include "studio/studio_workspace_request_service.hpp"

#include "studio/studio_project_system.hpp"

namespace studio
{
    namespace
    {
        StudioWorkspaceRouteResult BuildVisibilityResult(
            bool success,
            bool leftVisible,
            bool bottomVisible,
            bool rightVisible,
            const std::string& message)
        {
            StudioWorkspaceRouteResult result;
            result.success = success;
            result.leftVisible = leftVisible;
            result.bottomVisible = bottomVisible;
            result.rightVisible = rightVisible;
            result.message = message;
            return result;
        }
    }

    StudioWorkspaceRouteResult StudioWorkspaceRequestService::ExecuteSaveLayout(
        StudioProjectSystem& projectSystem,
        bool explorerVisible,
        bool consoleVisible) const
    {
        StudioWorkspaceRouteResult routeResult;
        const StudioProject project = projectSystem.GetCurrentProject();
        if (!project.isOpen)
        {
            routeResult.message = "No project open.";
            return routeResult;
        }

        SaveProjectRequest request;
        request.projectId = project.projectId;
        request.editorState = std::string("workspace=open\nexplorer_visible=") +
            (explorerVisible ? "true" : "false") +
            "\nconsole_visible=" +
            (consoleVisible ? "true" : "false") +
            "\nlast_project=" +
            project.projectId +
            "\n";

        const SaveProjectResult result = projectSystem.SaveProject(request);
        routeResult.success = result.success;
        routeResult.message = result.success ? "Layout saved." : result.error;
        routeResult.projectId = result.projectId;
        routeResult.logicalPath = result.logicalSavePath;
        return routeResult;
    }

    StudioWorkspaceRouteResult StudioWorkspaceRequestService::ExecuteToggleRequest(
        WorkspaceToggleTarget target,
        BoolGetter leftVisibleGetter,
        BoolGetter bottomVisibleGetter,
        BoolGetter rightVisibleGetter,
        BoolSetter leftSetter,
        BoolSetter bottomSetter,
        BoolSetter rightSetter,
        VoidCallback onLayoutChanged) const
    {
        const bool leftVisible = leftVisibleGetter ? leftVisibleGetter() : false;
        const bool bottomVisible = bottomVisibleGetter ? bottomVisibleGetter() : false;
        const bool rightVisible = rightVisibleGetter ? rightVisibleGetter() : false;

        switch (target)
        {
        case WorkspaceToggleTarget::LeftBar:
        {
            const bool nextVisible = !leftVisible;
            const bool updated = leftSetter ? leftSetter(nextVisible) : false;
            return BuildVisibilityResult(
                updated,
                updated ? nextVisible : leftVisible,
                bottomVisible,
                rightVisible,
                updated ? (nextVisible ? "Left bar shown." : "Left bar hidden.") : "Left bar unavailable.");
        }

        case WorkspaceToggleTarget::BottomBar:
        {
            const bool nextVisible = !bottomVisible;
            const bool updated = bottomSetter ? bottomSetter(nextVisible) : false;
            if (updated && onLayoutChanged)
            {
                onLayoutChanged();
            }
            return BuildVisibilityResult(
                updated,
                leftVisible,
                updated ? nextVisible : bottomVisible,
                rightVisible,
                updated ? (nextVisible ? "Bottom bar shown." : "Bottom bar hidden.") : "Bottom bar unavailable.");
        }

        case WorkspaceToggleTarget::RightBar:
        {
            const bool nextVisible = !rightVisible;
            const bool updated = rightSetter ? rightSetter(nextVisible) : false;
            if (updated && onLayoutChanged)
            {
                onLayoutChanged();
            }
            return BuildVisibilityResult(
                updated,
                leftVisible,
                bottomVisible,
                updated ? nextVisible : rightVisible,
                updated ? (nextVisible ? "Right bar shown." : "Right bar hidden.") : "Right bar unavailable.");
        }

        case WorkspaceToggleTarget::Explorer:
        {
            const bool nextVisible = !rightVisible;
            const bool updated = rightSetter ? rightSetter(nextVisible) : false;
            if (updated && onLayoutChanged)
            {
                onLayoutChanged();
            }
            return BuildVisibilityResult(
                updated,
                leftVisible,
                bottomVisible,
                updated ? nextVisible : rightVisible,
                updated ? (nextVisible ? "Explorer shown." : "Explorer hidden.") : "Explorer panel unavailable.");
        }

        case WorkspaceToggleTarget::Console:
        default:
        {
            const bool nextVisible = !bottomVisible;
            const bool updated = bottomSetter ? bottomSetter(nextVisible) : false;
            if (updated && onLayoutChanged)
            {
                onLayoutChanged();
            }
            return BuildVisibilityResult(
                updated,
                leftVisible,
                updated ? nextVisible : bottomVisible,
                rightVisible,
                updated ? (nextVisible ? "Console shown." : "Console hidden.") : "Console panel unavailable.");
        }
        }
    }
}
