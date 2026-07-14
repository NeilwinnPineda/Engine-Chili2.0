#include "command_protocol.hpp"

#include <cctype>
#include <unordered_map>

namespace
{
    void SkipWhitespace(const std::string& text, std::size_t& cursor)
    {
        while (cursor < text.size() &&
               std::isspace(static_cast<unsigned char>(text[cursor])) != 0)
        {
            ++cursor;
        }
    }

    bool ReadJsonString(
        const std::string& text,
        std::size_t& cursor,
        std::string& outValue,
        std::string& outError)
    {
        if (cursor >= text.size() || text[cursor] != '"')
        {
            outError = "Expected a JSON string.";
            return false;
        }

        ++cursor;
        outValue.clear();
        while (cursor < text.size())
        {
            const char value = text[cursor++];
            if (value == '"')
            {
                return true;
            }

            if (static_cast<unsigned char>(value) < 0x20U)
            {
                outError = "JSON strings cannot contain control characters.";
                return false;
            }

            if (value != '\\')
            {
                outValue.push_back(value);
                continue;
            }

            if (cursor >= text.size())
            {
                outError = "Incomplete JSON escape sequence.";
                return false;
            }

            const char escaped = text[cursor++];
            switch (escaped)
            {
            case '"': outValue.push_back('"'); break;
            case '\\': outValue.push_back('\\'); break;
            case '/': outValue.push_back('/'); break;
            case 'b': outValue.push_back('\b'); break;
            case 'f': outValue.push_back('\f'); break;
            case 'n': outValue.push_back('\n'); break;
            case 'r': outValue.push_back('\r'); break;
            case 't': outValue.push_back('\t'); break;
            default:
                outError = "Unsupported JSON escape sequence.";
                return false;
            }
        }

        outError = "Unterminated JSON string.";
        return false;
    }
}

bool ParseStudioCommandEnvelope(
    const std::string& message,
    StudioCommandEnvelope& outEnvelope,
    std::string& outError)
{
    outEnvelope = StudioCommandEnvelope{};
    outError.clear();

    std::size_t cursor = 0U;
    SkipWhitespace(message, cursor);
    if (cursor >= message.size() || message[cursor++] != '{')
    {
        outError = "Command envelope must be a JSON object.";
        return false;
    }

    std::unordered_map<std::string, std::string> fields;
    SkipWhitespace(message, cursor);
    while (cursor < message.size() && message[cursor] != '}')
    {
        std::string name;
        std::string value;
        if (!ReadJsonString(message, cursor, name, outError))
        {
            return false;
        }

        SkipWhitespace(message, cursor);
        if (cursor >= message.size() || message[cursor++] != ':')
        {
            outError = "Expected ':' after command field name.";
            return false;
        }

        SkipWhitespace(message, cursor);
        if (!ReadJsonString(message, cursor, value, outError))
        {
            return false;
        }

        if (!fields.emplace(name, value).second)
        {
            outError = "Duplicate command fields are not allowed.";
            return false;
        }

        SkipWhitespace(message, cursor);
        if (cursor < message.size() && message[cursor] == ',')
        {
            ++cursor;
            SkipWhitespace(message, cursor);
            if (cursor >= message.size() || message[cursor] == '}')
            {
                outError = "Trailing commas are not allowed in command envelopes.";
                return false;
            }
            continue;
        }

        break;
    }

    if (cursor >= message.size() || message[cursor++] != '}')
    {
        outError = "Command envelope is missing its closing brace.";
        return false;
    }

    SkipWhitespace(message, cursor);
    if (cursor != message.size())
    {
        outError = "Unexpected data follows the command envelope.";
        return false;
    }

    const auto readField = [&fields](const char* name) -> std::string
    {
        const auto value = fields.find(name);
        return value == fields.end() ? std::string() : value->second;
    };

    outEnvelope.protocolVersion = readField("protocol_version");
    outEnvelope.kind = readField("kind");
    outEnvelope.command = readField("command");
    outEnvelope.sender = readField("sender");
    outEnvelope.requestId = readField("request_id");
    return true;
}
