#pragma once

#include <string>
#include <functional>

class EngineBridge;

class CommandRouter
{
public:
    using QueryHandler = std::function<bool(
        const std::string& command,
        std::string& outDataJson,
        std::string& outMessage)>;

    void Bind(EngineBridge* bridge);
    void SetMutationsEnabled(bool enabled);
    void SetQueryHandler(QueryHandler handler);
    std::string HandleMessage(const std::string& message);
    std::string HandleExternalMessage(const std::string& message);

private:
    std::string HandleMessageWithMutationPolicy(const std::string& message, bool mutationsEnabled);
    static std::string BuildResponse(
        bool ok,
        const std::string& command,
        const std::string& requestId,
        const std::string& message,
        const std::string& code = "ok",
        const std::string& dataJson = std::string());

private:
    EngineBridge* m_bridge = nullptr;
    bool m_mutationsEnabled = false;
    QueryHandler m_queryHandler;
};
