#pragma once

#include "app/app_capabilities.hpp"

#include <string>

namespace studio
{
    class WorkspaceSettingsDialog
    {
    public:
        bool Open(AppCapabilities& capabilities, const std::string& contentPath)
        {
            if (!capabilities.ui)
            {
                return false;
            }

            if (m_dialogHandle != 0U)
            {
                capabilities.ui->SetWebDialogVisible(m_dialogHandle, true);
                return true;
            }

            WebDialogDesc dialogDesc;
            dialogDesc.name = "WorkspaceSettingsDialog";
            dialogDesc.title = L"Configure Workspace";
            dialogDesc.contentPath = contentPath;
            dialogDesc.dockMode = WebDialogDockMode::Floating;
            dialogDesc.rect = { 220, 150, 420, 360 };
            dialogDesc.visible = true;
            dialogDesc.resizable = true;
            dialogDesc.alwaysOnTop = false;

            m_dialogHandle = capabilities.ui->CreateWebDialog(dialogDesc);
            return m_dialogHandle != 0U;
        }

        void Close(AppCapabilities& capabilities)
        {
            if (m_dialogHandle == 0U || !capabilities.ui)
            {
                return;
            }

            capabilities.ui->DestroyWebDialog(m_dialogHandle);
            m_dialogHandle = 0U;
        }

        bool IsOpen() const { return m_dialogHandle != 0U; }
        IAppUi::WebDialogHandle GetHandle() const { return m_dialogHandle; }

    private:
        IAppUi::WebDialogHandle m_dialogHandle = 0U;
    };
}
