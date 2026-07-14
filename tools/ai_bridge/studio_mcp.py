#!/usr/bin/env python3
"""Read-only MCP adapter for a running Engine Studio session.

The adapter deliberately exposes a fixed tool allowlist. It cannot forward
arbitrary Studio commands and therefore cannot opt into mutation authority.
"""

from __future__ import annotations

import json
import sys
import time
import urllib.parse
import urllib.request
import uuid
from pathlib import Path
from typing import Any


STUDIO_COMMAND_URL = "http://127.0.0.1:37620/studio/bridge/command"
MAX_STUDIO_RESPONSE_BYTES = 1024 * 1024
ROOT_PATH = Path(__file__).resolve().parents[2]
AUDIT_PATH = ROOT_PATH / "User" / "studio" / "logs" / "ai_bridge_audit.jsonl"
TOOLS_PATH = Path(__file__).resolve().with_name("tools.json")


def load_tools() -> dict[str, tuple[str, str]]:
    data = json.loads(TOOLS_PATH.read_text(encoding="utf-8"))
    entries = data.get("tools")
    if not isinstance(entries, list):
        raise RuntimeError("AI bridge tool manifest must contain a 'tools' array.")

    tools: dict[str, tuple[str, str]] = {}
    for entry in entries:
        if not isinstance(entry, dict):
            raise RuntimeError("AI bridge tool manifest entries must be objects.")
        name = entry.get("name")
        command = entry.get("command")
        description = entry.get("description")
        if not isinstance(name, str) or not isinstance(command, str) or not isinstance(description, str):
            raise RuntimeError("AI bridge tool manifest entries require string name, command, and description fields.")
        tools[name] = (command, description)
    if not tools:
        raise RuntimeError("AI bridge tool manifest must declare at least one tool.")
    return tools


TOOLS = load_tools()
ALLOWED_COMMANDS = frozenset(command for command, _ in TOOLS.values())
CAPABILITIES_COMMAND = "list_capabilities"
EXPECTED_PROTOCOL_VERSION = "1"


def audit(event: dict[str, Any]) -> None:
    event = {"timestamp": time.time(), **event}
    try:
        AUDIT_PATH.parent.mkdir(parents=True, exist_ok=True)
        with AUDIT_PATH.open("a", encoding="utf-8") as output:
            output.write(json.dumps(event, separators=(",", ":")) + "\n")
    except OSError:
        # Audit failure must not corrupt the stdio protocol.
        pass


def raw_call_studio(command: str) -> dict[str, Any]:
    if command not in ALLOWED_COMMANDS:
        raise ValueError("Command is outside the read-only AI bridge allowlist.")

    request_id = str(uuid.uuid4())
    envelope = {
        "protocol_version": "1",
        "kind": "command",
        "command": command,
        "sender": "studio_mcp",
        "request_id": request_id,
    }
    query = urllib.parse.urlencode(
        {"message": json.dumps(envelope, separators=(",", ":"))}
    )

    audit({"event": "tool_request", "request_id": request_id, "command": command})
    try:
        with urllib.request.urlopen(f"{STUDIO_COMMAND_URL}?{query}", timeout=3.0) as response:
            payload = response.read(MAX_STUDIO_RESPONSE_BYTES + 1)
    except Exception as error:
        audit({
            "event": "tool_result",
            "request_id": request_id,
            "command": command,
            "ok": False,
            "error": str(error),
        })
        raise RuntimeError(f"Studio bridge is unavailable: {error}") from error

    if len(payload) > MAX_STUDIO_RESPONSE_BYTES:
        raise RuntimeError("Studio response exceeded the AI bridge size limit.")

    try:
        result = json.loads(payload.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise RuntimeError("Studio returned an invalid command response.") from error

    if not isinstance(result, dict) or result.get("kind") != "response":
        raise RuntimeError("Studio returned an unexpected response envelope.")
    if result.get("request_id") != request_id:
        raise RuntimeError("Studio response request id did not match the request.")

    audit({
        "event": "tool_result",
        "request_id": request_id,
        "command": command,
        "ok": bool(result.get("ok")),
        "code": result.get("code", "unknown"),
    })
    return result


def extract_capabilities(result: dict[str, Any]) -> tuple[set[str], set[str]]:
    data = result.get("data")
    if not isinstance(data, dict):
        raise RuntimeError("Studio capabilities response did not include a data object.")

    protocol_version = data.get("protocolVersion")
    if not isinstance(protocol_version, str):
        raise RuntimeError("Studio capabilities response did not include a protocolVersion string.")
    if protocol_version != EXPECTED_PROTOCOL_VERSION:
        raise RuntimeError("Studio capabilities response used an unsupported protocolVersion.")

    mutations_enabled = data.get("mutationsEnabled")
    if not isinstance(mutations_enabled, bool):
        raise RuntimeError("Studio capabilities response did not include a boolean mutationsEnabled field.")
    if mutations_enabled:
        raise RuntimeError("Studio capabilities reported mutation authority enabled for the read-only bridge.")

    external_read_only_bridge = data.get("externalReadOnlyBridge")
    if external_read_only_bridge is not None:
        if not isinstance(external_read_only_bridge, bool):
            raise RuntimeError("Studio capabilities response contained a non-boolean externalReadOnlyBridge field.")
        if not external_read_only_bridge:
            raise RuntimeError("Studio capabilities reported that the external bridge is not permanently read-only.")

    read = data.get("read")
    mutate = data.get("mutate")
    if not isinstance(read, list) or not isinstance(mutate, list):
        raise RuntimeError("Studio capabilities response did not include read/mutate lists.")
    if not all(isinstance(entry, str) for entry in read + mutate):
        raise RuntimeError("Studio capabilities response contained a non-string command name.")

    commands = data.get("commands")
    if commands is not None:
        if not isinstance(commands, list):
            raise RuntimeError("Studio capabilities response 'commands' field must be a list when present.")

        described_read: set[str] = set()
        described_mutate: set[str] = set()
        described_bridge_tools: dict[str, str] = {}
        for entry in commands:
            if not isinstance(entry, dict):
                raise RuntimeError("Studio capabilities response contained a non-object command descriptor.")

            name = entry.get("name")
            access = entry.get("access")
            description = entry.get("description")
            if not isinstance(name, str) or not isinstance(access, str) or not isinstance(description, str):
                raise RuntimeError("Studio command descriptors require string name, access, and description fields.")

            bridge_tool_name = entry.get("bridgeToolName")
            if bridge_tool_name is not None and not isinstance(bridge_tool_name, str):
                raise RuntimeError("Studio command descriptors contained a non-string bridgeToolName field.")

            if access == "read":
                described_read.add(name)
            elif access == "mutating":
                described_mutate.add(name)
            else:
                raise RuntimeError("Studio command descriptors contained an unsupported access value.")

            if bridge_tool_name is not None:
                described_bridge_tools[name] = bridge_tool_name

        if set(read) != described_read or set(mutate) != described_mutate:
            raise RuntimeError("Studio capabilities arrays did not match the structured command descriptors.")

        missing_manifest_commands = sorted(ALLOWED_COMMANDS - described_read)
        if missing_manifest_commands:
            raise RuntimeError(
                "Studio command descriptors did not publish required read-only bridge commands: " +
                ", ".join(missing_manifest_commands)
            )

        for tool_name, (command_name, description) in TOOLS.items():
            if described_bridge_tools.get(command_name) != tool_name:
                raise RuntimeError(
                    "Studio command descriptors did not publish the expected bridge tool name for command: " +
                    command_name
                )

            matching_entry = next((entry for entry in commands if entry.get("name") == command_name), None)
            if matching_entry is None or matching_entry.get("description") != description:
                raise RuntimeError(
                    "Studio command descriptors did not publish the expected bridge tool description for command: " +
                    command_name
                )

    return set(read), set(mutate)


def call_studio(command: str) -> dict[str, Any]:
    if command == CAPABILITIES_COMMAND:
        return raw_call_studio(command)

    capability_result = raw_call_studio(CAPABILITIES_COMMAND)
    read_commands, mutate_commands = extract_capabilities(capability_result)
    if command in mutate_commands or command not in read_commands:
        raise RuntimeError("Studio capabilities do not authorize this read-only bridge command.")

    return raw_call_studio(command)


def tool_descriptions() -> list[dict[str, Any]]:
    return [
        {
            "name": name,
            "description": description,
            "inputSchema": {
                "type": "object",
                "properties": {},
                "additionalProperties": False,
            },
        }
        for name, (_, description) in TOOLS.items()
    ]


def result_message(request_id: Any, result: Any = None, error: Any = None) -> dict[str, Any]:
    message: dict[str, Any] = {"jsonrpc": "2.0", "id": request_id}
    if error is not None:
        message["error"] = error
    else:
        message["result"] = result
    return message


def dispatch(message: dict[str, Any]) -> dict[str, Any] | None:
    method = message.get("method")
    request_id = message.get("id")
    params = message.get("params") if isinstance(message.get("params"), dict) else {}

    if method == "notifications/initialized":
        return None
    if method == "initialize":
        requested_version = params.get("protocolVersion", "2024-11-05")
        return result_message(request_id, {
            "protocolVersion": requested_version,
            "capabilities": {"tools": {"listChanged": False}},
            "serverInfo": {"name": "engine-chili-studio", "version": "0.1.0"},
        })
    if method == "ping":
        return result_message(request_id, {})
    if method == "tools/list":
        return result_message(request_id, {"tools": tool_descriptions()})
    if method == "tools/call":
        tool_name = params.get("name")
        arguments = params.get("arguments", {})
        if tool_name not in TOOLS:
            return result_message(request_id, error={"code": -32602, "message": "Unknown tool."})
        if arguments not in ({}, None):
            return result_message(request_id, error={"code": -32602, "message": "This read-only tool takes no arguments."})

        command = TOOLS[tool_name][0]
        try:
            studio_result = call_studio(command)
            text = json.dumps(studio_result, ensure_ascii=False, separators=(",", ":"))
            return result_message(request_id, {
                "content": [{"type": "text", "text": text}],
                "structuredContent": studio_result,
                "isError": not bool(studio_result.get("ok")),
            })
        except Exception as error:
            return result_message(request_id, {
                "content": [{"type": "text", "text": str(error)}],
                "isError": True,
            })

    if request_id is None:
        return None
    return result_message(request_id, error={"code": -32601, "message": "Method not found."})


def main() -> int:
    audit({"event": "bridge_start", "mode": "read_only"})
    for line in sys.stdin:
        if not line.strip():
            continue
        try:
            message = json.loads(line)
            if not isinstance(message, dict):
                raise ValueError("JSON-RPC message must be an object.")
            response = dispatch(message)
        except Exception as error:
            response = result_message(None, error={"code": -32700, "message": str(error)})

        if response is not None:
            sys.stdout.write(json.dumps(response, separators=(",", ":")) + "\n")
            sys.stdout.flush()

    audit({"event": "bridge_stop"})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
