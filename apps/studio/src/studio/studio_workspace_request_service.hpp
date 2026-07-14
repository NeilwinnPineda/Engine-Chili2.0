#pragma once

#include <functional>
#include <string>

namespace studio
{
    class StudioProjectSystem;

    enum class WorkspaceToggleTarget
    {
        LeftBar = 0,
        BottomBar,
        RightBar,
        Explorer,
        Console
    };

    struct StudioWorkspaceRouteResult
    {
        bool success = false;
        bool leftVisible = false;
        bool bottomVisible = false;
        bool rightVisible = false;
        std::string message;
        std::string projectId;
        std::string logicalPath;
    };

    class StudioWorkspaceRequestService
    {
    public:
        using BoolGetter = std::function<bool()>;
        using BoolSetter = std::function<bool(bool)>;
        using VoidCallback = std::function<void()>;

        StudioWorkspaceRouteResult ExecuteSaveLayout(
            StudioProjectSystem& projectSystem,
            bool explorerVisible,
            bool consoleVisible) const;

        StudioWorkspaceRouteResult ExecuteToggleRequest(
            WorkspaceToggleTarget target,
            BoolGetter leftVisibleGetter,
            BoolGetter bottomVisibleGetter,
            BoolGetter rightVisibleGetter,
            BoolSetter leftSetter,
            BoolSetter bottomSetter,
            BoolSetter rightSetter,
            VoidCallback onLayoutChanged) const;
    };
}
