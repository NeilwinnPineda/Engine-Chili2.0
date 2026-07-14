#include "command_router.hpp"
#include "command_policy.hpp"
#include "command_protocol.hpp"

#include "../bridge/engine_bridge.hpp"

#include <utility>

namespace
{
    constexpr const char* kProtocolVersion = "1";
    constexpr std::size_t kMaximumCommandBytes = 16U * 1024U;

    std::string EscapeJson(const std::string& value)
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
}

void CommandRouter::Bind(EngineBridge* bridge)
{
    m_bridge = bridge;
}

void CommandRouter::SetMutationsEnabled(bool enabled)
{
    m_mutationsEnabled = enabled;
}

void CommandRouter::SetQueryHandler(QueryHandler handler)
{
    m_queryHandler = std::move(handler);
}

std::string CommandRouter::HandleMessage(const std::string& message)
{
    return HandleMessageWithMutationPolicy(message, m_mutationsEnabled);
}

std::string CommandRouter::HandleExternalMessage(const std::string& message)
{
    return HandleMessageWithMutationPolicy(message, false);
}

std::string CommandRouter::HandleMessageWithMutationPolicy(const std::string& message, bool mutationsEnabled)
{
    if (message.empty() || message.size() > kMaximumCommandBytes)
    {
        return BuildResponse(false, "unknown", std::string(), "Command envelope size is invalid.", "bad_envelope");
    }

    if (!m_bridge)
    {
        return BuildResponse(false, "unknown", std::string(), "Studio backend is not initialized.", "backend_unavailable");
    }

    StudioCommandEnvelope envelope;
    std::string parseError;
    if (!ParseStudioCommandEnvelope(message, envelope, parseError))
    {
        return BuildResponse(false, "unknown", std::string(), parseError, "bad_envelope");
    }

    if (!envelope.protocolVersion.empty() && envelope.protocolVersion != kProtocolVersion)
    {
        return BuildResponse(false, envelope.command, envelope.requestId, "Unsupported protocol version.", "unsupported_version");
    }

    if (envelope.kind != "command")
    {
        return BuildResponse(false, envelope.command, envelope.requestId, "Only command envelopes are accepted.", "unsupported_kind");
    }

    if (envelope.command.empty())
    {
        return BuildResponse(false, envelope.command, envelope.requestId, "Command field is required.", "missing_command");
    }

    const StudioCommandAccess access = ClassifyStudioCommand(envelope.command);
    if (access == StudioCommandAccess::Unknown)
    {
        return BuildResponse(false, envelope.command, envelope.requestId, "Unknown studio command.", "unknown_command");
    }

    if (!IsStudioCommandAllowed(envelope.command, mutationsEnabled))
    {
        return BuildResponse(false, envelope.command, envelope.requestId, "Command requires explicit mutation permission.", "mutation_denied");
    }

    if (envelope.command == "hello")
    {
        const std::string helloMessage = m_bridge->BuildHelloMessage(envelope.sender);
        return BuildResponse(true, envelope.command, envelope.requestId, helloMessage);
    }

    if (envelope.command == "ping")
    {
        return BuildResponse(true, envelope.command, envelope.requestId, "pong");
    }

    if (envelope.command == "get_status")
    {
        return BuildResponse(true, envelope.command, envelope.requestId, m_bridge->BuildStatusMessage());
    }

    if (envelope.command == "list_capabilities")
    {
        return BuildResponse(
            true,
            envelope.command,
            envelope.requestId,
            BuildStudioCapabilitiesMessage(mutationsEnabled),
            "ok",
            BuildStudioCapabilitiesDataJson(kProtocolVersion, mutationsEnabled));
    }

    if (envelope.command == "exit")
    {
        m_bridge->RequestExit();
        return BuildResponse(true, envelope.command, envelope.requestId, "Studio shutdown requested by frontend.");
    }

    if (m_queryHandler)
    {
        std::string dataJson;
        std::string queryMessage;
        if (m_queryHandler(envelope.command, dataJson, queryMessage))
        {
            return BuildResponse(
                true,
                envelope.command,
                envelope.requestId,
                queryMessage,
                "ok",
                dataJson);
        }
    }

    return BuildResponse(false, envelope.command, envelope.requestId, "Command was not handled.", "not_handled");
}

std::string CommandRouter::BuildResponse(
    bool ok,
    const std::string& command,
    const std::string& requestId,
    const std::string& message,
    const std::string& code,
    const std::string& dataJson)
{
    std::string response = std::string("{\"kind\":\"response\",\"protocol_version\":\"") +
        kProtocolVersion +
        "\",\"ok\":" +
        (ok ? "true" : "false") +
        ",\"command\":\"" +
        EscapeJson(command) +
        "\",\"request_id\":\"" +
        EscapeJson(requestId) +
        "\",\"code\":\"" +
        EscapeJson(code) +
        "\",\"message\":\"" +
        EscapeJson(message) +
        "\"";
    if (!dataJson.empty())
    {
        response += ",\"data\":" + dataJson;
    }
    response += "}";
    return response;
}
