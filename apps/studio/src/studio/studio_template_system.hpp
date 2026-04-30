#pragma once

#include <string>

namespace studio
{
    class FileProxy;

    struct ApplyTemplateResult
    {
        bool success = false;
        std::string message;
        std::string error;
    };

    class StudioTemplateSystem
    {
    public:
        explicit StudioTemplateSystem(const FileProxy& files);

        ApplyTemplateResult ApplyTemplate(
            const std::string& templateName,
            const std::string& targetProjectPath,
            const std::string& projectName,
            const std::string& projectId) const;

    private:
        ApplyTemplateResult ApplyArcade2DTemplate(
            const std::string& targetProjectPath,
            const std::string& projectName,
            const std::string& projectId) const;

        const FileProxy& m_files;
    };
}
