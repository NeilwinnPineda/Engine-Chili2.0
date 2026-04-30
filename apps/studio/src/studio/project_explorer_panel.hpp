#pragma once

#include "app/app_capabilities.hpp"
#include "studio/file_proxy.hpp"
#include "studio/studio_project_system.hpp"

#include <string>

namespace studio
{
    class ProjectExplorerPanel
    {
    public:
        bool Open(AppCapabilities& capabilities, const std::string& contentPath);
        void Close(AppCapabilities& capabilities);

        std::string BuildTreeJson(const StudioProjectSystem& projects) const;
        bool SelectFile(const StudioProjectSystem& projects, const std::string& logicalPath, std::string& outMessage);
        const std::string& GetSelectedFileLogicalPath() const;

    private:
        std::string BuildEntryJson(const std::string& logicalPath, const std::string& displayName, bool isRoot, std::string& outError) const;
        bool IsInsideProjectRoot(const std::string& projectRoot, const std::string& logicalPath) const;

        FileProxy m_files;
        IAppUi::WebDialogHandle m_dialogHandle = 0U;
        std::string m_selectedFileLogicalPath;
    };
}
