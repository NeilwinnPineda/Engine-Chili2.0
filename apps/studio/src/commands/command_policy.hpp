#pragma once

#include <string_view>
#include <string>
#include <vector>

enum class StudioCommandAccess
{
    Unknown = 0,
    Read,
    Mutating
};

struct StudioCommandDescriptor
{
    std::string_view name;
    StudioCommandAccess access = StudioCommandAccess::Unknown;
    std::string_view description;
    std::string_view bridgeToolName;
};

StudioCommandAccess ClassifyStudioCommand(const std::string& command);
bool IsStudioCommandAllowed(const std::string& command, bool mutationsEnabled);
const std::vector<StudioCommandDescriptor>& GetStudioCommandDescriptors();
std::string_view GetStudioCommandAccessName(StudioCommandAccess access);
std::string BuildStudioCapabilitiesMessage(bool mutationsEnabled);
std::string BuildStudioCapabilitiesDataJson(std::string_view protocolVersion, bool mutationsEnabled);
