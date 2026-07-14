# Studio AI Bridge

`studio_mcp.py` is a dependency-free, read-only MCP adapter for a running
Engine Studio process. It proxies a fixed allowlist into Studio's versioned
command service on `127.0.0.1:37620`.

The MCP tool inventory is declared in `tools/ai_bridge/tools.json`, which the
adapter loads at startup.

The adapter exposes:

- `studio_status`
- `studio_capabilities`
- `workspace_status`
- `project_status`
- `list_entities`
- `selected_entity`
- `runtime_status`

It cannot forward arbitrary commands and does not expose mutation tools.
Requests and authorization outcomes are recorded in
`User/studio/logs/ai_bridge_audit.jsonl`.

Before forwarding any read tool other than `studio_capabilities`, the adapter
first asks Studio for `list_capabilities` and rejects the tool call if the
requested command is not currently reported as read-only.

Studio currently returns both legacy `read` / `mutate` name arrays and a richer
`commands` list with per-command access, descriptions, and bridge tool names.
The bridge still uses the read/mutate arrays for compatibility, but when
`commands` is present it verifies that both representations agree before
forwarding a tool call. It also verifies that each manifest tool in
`tools.json` still matches the Studio-published command descriptor, including
the exported MCP tool name and description. The capabilities payload also
reports `protocolVersion` and `mutationsEnabled` explicitly. The bridge rejects
capability payloads that do not match protocol version `1`, that report
mutation authority enabled, or that omit any read-only command required by the
MCP tool manifest. When Studio reports `externalReadOnlyBridge`, the adapter
also requires that flag to stay `true`.

Example MCP client configuration:

```json
{
  "mcpServers": {
    "engine-chili-studio": {
      "command": "python",
      "args": ["tools/ai_bridge/studio_mcp.py"]
    }
  }
}
```

Start `engine_studio.exe` before calling a tool. The adapter returns a tool
error if Studio is unavailable, returns malformed data, or does not echo the
request id.
