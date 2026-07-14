#pragma once

#include <string>

struct StudioCommandEnvelope
{
    std::string protocolVersion;
    std::string kind;
    std::string command;
    std::string sender;
    std::string requestId;
};

bool ParseStudioCommandEnvelope(
    const std::string& message,
    StudioCommandEnvelope& outEnvelope,
    std::string& outError);

