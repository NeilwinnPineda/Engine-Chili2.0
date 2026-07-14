#include "http_server.hpp"

#include "../bridge/engine_bridge.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr int kReceiveBufferSize = 4096;
    constexpr std::size_t kMaximumRequestBytes = 64U * 1024U;

    struct HttpClientConnection
    {
        SOCKET socket = INVALID_SOCKET;
        std::string requestBuffer;
    };

    std::string ToLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });

        return value;
    }

    bool SetNonBlocking(SOCKET socket)
    {
        u_long enabled = 1;
        return ioctlsocket(socket, FIONBIO, &enabled) == 0;
    }

    bool SendAll(SOCKET socket, const std::string& payload)
    {
        std::size_t totalSent = 0;
        while (totalSent < payload.size())
        {
            const int sent = send(
                socket,
                payload.data() + totalSent,
                static_cast<int>(payload.size() - totalSent),
                0);

            if (sent == SOCKET_ERROR)
            {
                return false;
            }

            totalSent += static_cast<std::size_t>(sent);
        }

        return true;
    }

    std::string BuildHttpResponse(
        int statusCode,
        const std::string& statusText,
        const std::string& contentType,
        const std::string& body,
        const std::string& extraHeaders = std::string())
    {
        return std::string("HTTP/1.1 ") +
            std::to_string(statusCode) +
            " " +
            statusText +
            "\r\nContent-Type: " +
            contentType +
            "\r\nContent-Length: " +
            std::to_string(body.size()) +
            "\r\nConnection: close\r\nCache-Control: no-store\r\n"
            "X-Content-Type-Options: nosniff\r\n"
            "Content-Security-Policy: frame-ancestors 'self'\r\n" +
            extraHeaders +
            "\r\n" +
            body;
    }

    std::string GenerateSessionSecret()
    {
        constexpr char kHex[] = "0123456789abcdef";
        std::random_device random;
        std::string secret;
        secret.reserve(64U);
        for (std::size_t index = 0U; index < 32U; ++index)
        {
            const unsigned int value = random() & 0xFFU;
            secret.push_back(kHex[(value >> 4U) & 0x0FU]);
            secret.push_back(kHex[value & 0x0FU]);
        }
        return secret;
    }

    std::string ExtractHeaderValue(const std::string& request, const std::string& headerName)
    {
        const std::string lowerRequest = ToLower(request);
        const std::string pattern = "\r\n" + ToLower(headerName) + ":";
        const std::size_t headerPosition = lowerRequest.find(pattern);
        if (headerPosition == std::string::npos)
        {
            return std::string();
        }

        std::size_t valueStart = headerPosition + pattern.size();
        while (valueStart < request.size() &&
               (request[valueStart] == ' ' || request[valueStart] == '\t'))
        {
            ++valueStart;
        }

        const std::size_t valueEnd = request.find("\r\n", valueStart);
        return request.substr(valueStart, valueEnd - valueStart);
    }

    bool IsTrustedLocalRequest(
        const std::string& request,
        const std::string& expectedHost,
        const std::string& expectedOrigin)
    {
        const std::string host = ToLower(ExtractHeaderValue(request, "Host"));
        if (host != ToLower(expectedHost))
        {
            return false;
        }

        const std::string fetchSite = ToLower(ExtractHeaderValue(request, "Sec-Fetch-Site"));
        if (fetchSite == "cross-site")
        {
            return false;
        }

        const std::string origin = ExtractHeaderValue(request, "Origin");
        return origin.empty() || ToLower(origin) == ToLower(expectedOrigin);
    }

    bool HasSessionCookie(const std::string& request, const std::string& sessionSecret)
    {
        const std::string cookie = ExtractHeaderValue(request, "Cookie");
        const std::string expected = "studio_session=" + sessionSecret;
        std::size_t cursor = 0U;
        while (cursor < cookie.size())
        {
            while (cursor < cookie.size() && (cookie[cursor] == ' ' || cookie[cursor] == ';'))
            {
                ++cursor;
            }
            const std::size_t end = cookie.find(';', cursor);
            const std::string entry = cookie.substr(
                cursor,
                end == std::string::npos ? std::string::npos : end - cursor);
            if (entry == expected)
            {
                return true;
            }
            if (end == std::string::npos)
            {
                break;
            }
            cursor = end + 1U;
        }
        return false;
    }

    std::string GetContentType(const std::string& path)
    {
        const std::size_t extensionPosition = path.find_last_of('.');
        if (extensionPosition == std::string::npos)
        {
            return "application/octet-stream";
        }

        const std::string extension = ToLower(path.substr(extensionPosition));
        if (extension == ".html")
        {
            return "text/html; charset=utf-8";
        }
        if (extension == ".js")
        {
            return "application/javascript; charset=utf-8";
        }
        if (extension == ".css")
        {
            return "text/css; charset=utf-8";
        }
        if (extension == ".json")
        {
            return "application/json; charset=utf-8";
        }
        if (extension == ".txt")
        {
            return "text/plain; charset=utf-8";
        }

        return "application/octet-stream";
    }

    bool ReadFileText(const std::string& path, std::string& outContent)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input)
        {
            outContent.clear();
            return false;
        }

        outContent.assign(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
        return true;
    }

    bool BuildResolvedPath(
        const std::string& webRoot,
        const std::string& indexFilePath,
        std::string requestedPath,
        std::string& outResolvedPath)
    {
        if (requestedPath.empty() || requestedPath == "/")
        {
            requestedPath = indexFilePath;
        }

        const std::size_t queryPosition = requestedPath.find('?');
        if (queryPosition != std::string::npos)
        {
            requestedPath = requestedPath.substr(0, queryPosition);
        }

        if (!requestedPath.empty() && requestedPath.front() == '/')
        {
            requestedPath.erase(requestedPath.begin());
        }

        if (requestedPath.find("..") != std::string::npos)
        {
            return false;
        }

        outResolvedPath = webRoot;
        if (!outResolvedPath.empty() && outResolvedPath.back() != '\\' && outResolvedPath.back() != '/')
        {
            outResolvedPath += '/';
        }

        outResolvedPath += requestedPath;
        return true;
    }
}

struct HttpServerState
{
    SOCKET listener = INVALID_SOCKET;
    std::vector<HttpClientConnection> clients;
    bool wsaStarted = false;
    std::string expectedHost;
    std::string expectedOrigin;
    std::string sessionSecret;
};

HttpServer::HttpServer()
    : m_state(std::make_unique<HttpServerState>())
{
}

HttpServer::~HttpServer() = default;

bool HttpServer::Start(const HttpServerConfig& config, EngineBridge& bridge)
{
    if (m_running)
    {
        return true;
    }

    if (config.host.empty() || config.webRootPath.empty())
    {
        bridge.LogError("Studio: HTTP server configuration is incomplete.");
        return false;
    }

    m_baseUrl = "http://" + config.host + ":" + std::to_string(config.port);
    m_state->expectedHost = config.host + ":" + std::to_string(config.port);
    m_state->expectedOrigin = m_baseUrl;
    m_state->sessionSecret = GenerateSessionSecret();
    m_webRootPath = config.webRootPath;
    m_indexFilePath = config.indexFilePath.empty() ? std::string("index.html") : config.indexFilePath;

    WSADATA wsaData = {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        bridge.LogError("Studio: HTTP server failed to initialize Winsock.");
        return false;
    }

    m_state->wsaStarted = true;
    m_state->listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_state->listener == INVALID_SOCKET)
    {
        bridge.LogError("Studio: HTTP server failed to create listener socket.");
        Stop(bridge);
        return false;
    }

    if (!SetNonBlocking(m_state->listener))
    {
        bridge.LogError("Studio: HTTP server failed to set non-blocking mode.");
        Stop(bridge);
        return false;
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(config.port);
    address.sin_addr.s_addr = inet_addr(config.host.c_str());

    if (bind(m_state->listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
    {
        bridge.LogError("Studio: HTTP server failed to bind listener socket.");
        Stop(bridge);
        return false;
    }

    if (listen(m_state->listener, SOMAXCONN) == SOCKET_ERROR)
    {
        bridge.LogError("Studio: HTTP server failed to listen on localhost.");
        Stop(bridge);
        return false;
    }

    m_running = true;

    bridge.LogInfo(
        std::string("Studio: HTTP server bound to ") +
        m_baseUrl +
        " serving " +
        m_webRootPath);

    return true;
}

void HttpServer::Stop(EngineBridge& bridge)
{
    if (m_state)
    {
        for (HttpClientConnection& client : m_state->clients)
        {
            if (client.socket != INVALID_SOCKET)
            {
                closesocket(client.socket);
                client.socket = INVALID_SOCKET;
            }
        }

        m_state->clients.clear();

        if (m_state->listener != INVALID_SOCKET)
        {
            closesocket(m_state->listener);
            m_state->listener = INVALID_SOCKET;
        }

        if (m_state->wsaStarted)
        {
            WSACleanup();
            m_state->wsaStarted = false;
        }
    }

    if (!m_running)
    {
        return;
    }

    bridge.LogInfo("Studio: HTTP server stopped.");
    m_running = false;
}

void HttpServer::Tick(EngineBridge& bridge)
{
    if (!m_running || !m_state || m_state->listener == INVALID_SOCKET)
    {
        return;
    }

    for (;;)
    {
        SOCKET clientSocket = accept(m_state->listener, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET)
        {
            const int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK)
            {
                bridge.LogWarn("Studio: HTTP server accept failed.");
            }

            break;
        }

        if (!SetNonBlocking(clientSocket))
        {
            closesocket(clientSocket);
            bridge.LogWarn("Studio: HTTP server rejected client after non-blocking setup failure.");
            continue;
        }

        m_state->clients.push_back({ clientSocket, std::string() });
    }

    std::vector<HttpClientConnection> activeClients;
    activeClients.reserve(m_state->clients.size());

    for (HttpClientConnection& client : m_state->clients)
    {
        char buffer[kReceiveBufferSize] = {};
        const int received = recv(client.socket, buffer, sizeof(buffer), 0);

        if (received == 0)
        {
            closesocket(client.socket);
            continue;
        }

        if (received == SOCKET_ERROR)
        {
            const int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK)
            {
                activeClients.push_back(std::move(client));
                continue;
            }

            closesocket(client.socket);
            continue;
        }

        client.requestBuffer.append(buffer, static_cast<std::size_t>(received));

        if (client.requestBuffer.size() > kMaximumRequestBytes)
        {
            SendAll(client.socket, BuildHttpResponse(
                413,
                "Content Too Large",
                "text/plain; charset=utf-8",
                "Request headers are too large."));
            closesocket(client.socket);
            continue;
        }

        const std::size_t headerEnd = client.requestBuffer.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
        {
            activeClients.push_back(std::move(client));
            continue;
        }

        const std::size_t requestLineEnd = client.requestBuffer.find("\r\n");
        const std::string requestLine = client.requestBuffer.substr(0, requestLineEnd);
        const std::size_t methodEnd = requestLine.find(' ');
        const std::size_t pathEnd = (methodEnd == std::string::npos) ? std::string::npos : requestLine.find(' ', methodEnd + 1);

        std::string response;
        if (!IsTrustedLocalRequest(
                client.requestBuffer,
                m_state->expectedHost,
                m_state->expectedOrigin))
        {
            response = BuildHttpResponse(403, "Forbidden", "text/plain; charset=utf-8", "Untrusted request origin.");
        }
        else if (methodEnd == std::string::npos || pathEnd == std::string::npos || requestLine.substr(0, methodEnd) != "GET")
        {
            response = BuildHttpResponse(405, "Method Not Allowed", "text/plain; charset=utf-8", "Only GET is supported.");
        }
        else
        {
            const std::string requestedPath = requestLine.substr(methodEnd + 1, pathEnd - methodEnd - 1);
            std::string resolvedPath;
            std::string handledContentType;
            std::string handledBody;
            const bool studioRoute = requestedPath.rfind("/studio/", 0U) == 0U;
            const bool readOnlyBridgeRoute = requestedPath.rfind("/studio/bridge/command", 0U) == 0U;
            if (studioRoute &&
                !readOnlyBridgeRoute &&
                !HasSessionCookie(client.requestBuffer, m_state->sessionSecret))
            {
                response = BuildHttpResponse(401, "Unauthorized", "text/plain; charset=utf-8", "Studio session cookie is required.");
            }
            else if (m_requestHandler && m_requestHandler(requestedPath, handledContentType, handledBody))
            {
                response = BuildHttpResponse(200, "OK", handledContentType, handledBody);
            }
            else if (!BuildResolvedPath(m_webRootPath, m_indexFilePath, requestedPath, resolvedPath))
            {
                response = BuildHttpResponse(403, "Forbidden", "text/plain; charset=utf-8", "Forbidden path.");
            }
            else
            {
                std::string body;
                if (!ReadFileText(resolvedPath, body))
                {
                    response = BuildHttpResponse(404, "Not Found", "text/plain; charset=utf-8", "File not found.");
                }
                else
                {
                    response = BuildHttpResponse(
                        200,
                        "OK",
                        GetContentType(resolvedPath),
                        body,
                        "Set-Cookie: studio_session=" + m_state->sessionSecret +
                            "; HttpOnly; SameSite=Strict; Path=/\r\n");
                }
            }
        }

        SendAll(client.socket, response);
        closesocket(client.socket);
    }

    m_state->clients = std::move(activeClients);
}

bool HttpServer::IsRunning() const
{
    return m_running;
}

const std::string& HttpServer::GetBaseUrl() const
{
    return m_baseUrl;
}

const std::string& HttpServer::GetWebRootPath() const
{
    return m_webRootPath;
}

void HttpServer::SetRequestHandler(RequestHandler handler)
{
    m_requestHandler = std::move(handler);
}
