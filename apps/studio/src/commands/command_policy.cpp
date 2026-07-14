#include "command_policy.hpp"

#include <string>

namespace
{
    std::string EscapeJson(std::string_view value)
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

    const std::vector<StudioCommandDescriptor>& CommandDescriptors()
    {
        static const std::vector<StudioCommandDescriptor> descriptors = {
            { "hello", StudioCommandAccess::Read, "Build a frontend greeting from the native Studio backend.", "" },
            { "ping", StudioCommandAccess::Read, "Perform a minimal Studio bridge liveness check.", "" },
            { "get_status", StudioCommandAccess::Read, "Read the native Studio backend status.", "studio_status" },
            { "list_capabilities", StudioCommandAccess::Read, "Read the Studio command capabilities contract and mutation policy.", "studio_capabilities" },
            { "get_workspace_status", StudioCommandAccess::Read, "Read visible workspace panels and layout state.", "workspace_status" },
            { "get_project_status", StudioCommandAccess::Read, "Inspect the currently opened project and runtime artifact.", "project_status" },
            { "list_entities", StudioCommandAccess::Read, "List scene entities and their high-level component flags.", "list_entities" },
            { "get_selected_entity", StudioCommandAccess::Read, "Inspect the current centralized Studio selection.", "selected_entity" },
            { "get_runtime_status", StudioCommandAccess::Read, "Read play state, FPS, active project, and runtime artifact.", "runtime_status" },
            { "exit", StudioCommandAccess::Mutating, "Request Studio shutdown.", "" }
        };
        return descriptors;
    }
}

StudioCommandAccess ClassifyStudioCommand(const std::string& command)
{
    for (const StudioCommandDescriptor& descriptor : CommandDescriptors())
    {
        if (descriptor.name == command)
        {
            return descriptor.access;
        }
    }

    return StudioCommandAccess::Unknown;
}

bool IsStudioCommandAllowed(const std::string& command, bool mutationsEnabled)
{
    const StudioCommandAccess access = ClassifyStudioCommand(command);
    return access == StudioCommandAccess::Read ||
        (access == StudioCommandAccess::Mutating && mutationsEnabled);
}

const std::vector<StudioCommandDescriptor>& GetStudioCommandDescriptors()
{
    return CommandDescriptors();
}

std::string_view GetStudioCommandAccessName(StudioCommandAccess access)
{
    switch (access)
    {
    case StudioCommandAccess::Read:
        return "read";
    case StudioCommandAccess::Mutating:
        return "mutating";
    case StudioCommandAccess::Unknown:
    default:
        return "unknown";
    }
}

std::string BuildStudioCapabilitiesMessage(bool mutationsEnabled)
{
    if (mutationsEnabled)
    {
        return "Studio inspection commands are available and mutation commands are currently enabled.";
    }

    return "Read-only Studio inspection commands are available; mutation commands are currently disabled.";
}

std::string BuildStudioCapabilitiesDataJson(std::string_view protocolVersion, bool mutationsEnabled)
{
    std::string json = "{\"protocolVersion\":\"" + EscapeJson(protocolVersion) + "\"";
    json += ",\"mutationsEnabled\":";
    json += mutationsEnabled ? "true" : "false";
    json += ",\"externalReadOnlyBridge\":true";
    json += ",\"read\":[";
    bool firstRead = true;
    bool firstMutating = true;
    std::string mutateEntries;
    std::string commandEntries;
    bool firstCommand = true;

    for (const StudioCommandDescriptor& descriptor : CommandDescriptors())
    {
        if (descriptor.access == StudioCommandAccess::Read)
        {
            if (!firstRead)
            {
                json += ",";
            }
            firstRead = false;
            json += "\"" + std::string(descriptor.name) + "\"";
        }
        else if (descriptor.access == StudioCommandAccess::Mutating)
        {
            if (!firstMutating)
            {
                mutateEntries += ",";
            }
            firstMutating = false;
            mutateEntries += "\"" + std::string(descriptor.name) + "\"";
        }

        if (!firstCommand)
        {
            commandEntries += ",";
        }
        firstCommand = false;
        commandEntries += "{\"name\":\"" + EscapeJson(descriptor.name) + "\"";
        commandEntries += ",\"access\":\"" + EscapeJson(GetStudioCommandAccessName(descriptor.access)) + "\"";
        commandEntries += ",\"description\":\"" + EscapeJson(descriptor.description) + "\"";
        if (!descriptor.bridgeToolName.empty())
        {
            commandEntries += ",\"bridgeToolName\":\"" + EscapeJson(descriptor.bridgeToolName) + "\"";
        }
        commandEntries += "}";
    }

    json += "],\"mutate\":[";
    json += mutateEntries;
    json += "],\"commands\":[";
    json += commandEntries;
    json += "]}";
    return json;
}
