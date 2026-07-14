from __future__ import annotations

import importlib.util
from pathlib import Path


MODULE_PATH = Path(__file__).resolve().parents[1] / "tools" / "ai_bridge" / "studio_mcp.py"
SPEC = importlib.util.spec_from_file_location("studio_mcp", MODULE_PATH)
assert SPEC and SPEC.loader
studio_mcp = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(studio_mcp)


FULL_COMMAND_DESCRIPTORS = [
    {
        "name": "get_status",
        "access": "read",
        "description": "Read the native Studio backend status.",
        "bridgeToolName": "studio_status",
    },
    {
        "name": "list_capabilities",
        "access": "read",
        "description": "Read the Studio command capabilities contract and mutation policy.",
        "bridgeToolName": "studio_capabilities",
    },
    {
        "name": "get_workspace_status",
        "access": "read",
        "description": "Read visible workspace panels and layout state.",
        "bridgeToolName": "workspace_status",
    },
    {
        "name": "get_project_status",
        "access": "read",
        "description": "Inspect the currently opened project and runtime artifact.",
        "bridgeToolName": "project_status",
    },
    {
        "name": "list_entities",
        "access": "read",
        "description": "List scene entities and their high-level component flags.",
        "bridgeToolName": "list_entities",
    },
    {
        "name": "get_selected_entity",
        "access": "read",
        "description": "Inspect the current centralized Studio selection.",
        "bridgeToolName": "selected_entity",
    },
    {
        "name": "get_runtime_status",
        "access": "read",
        "description": "Read play state, FPS, active project, and runtime artifact.",
        "bridgeToolName": "runtime_status",
    },
    {
        "name": "exit",
        "access": "mutating",
        "description": "Request Studio shutdown.",
    },
]


def build_capabilities_payload(
    *,
    protocol_version: str = "1",
    mutations_enabled: bool = False,
    external_read_only_bridge: bool = True,
    read: list[str] | None = None,
    mutate: list[str] | None = None,
    commands: list[dict] | None = None,
) -> dict:
    return {
        "protocolVersion": protocol_version,
        "mutationsEnabled": mutations_enabled,
        "externalReadOnlyBridge": external_read_only_bridge,
        "read": read if read is not None else [
            "get_status",
            "list_capabilities",
            "get_workspace_status",
            "get_project_status",
            "list_entities",
            "get_selected_entity",
            "get_runtime_status",
        ],
        "mutate": mutate if mutate is not None else ["exit"],
        "commands": commands if commands is not None else FULL_COMMAND_DESCRIPTORS,
    }


def build_capabilities_response(payload: dict) -> dict:
    return {
        "kind": "response",
        "request_id": "test-capabilities",
        "ok": True,
        "command": "list_capabilities",
        "data": payload,
    }


def build_project_status_data() -> dict:
    return {
        "ok": True,
        "open": True,
        "id": "build_route",
        "name": "Build Route",
        "logicalPath": "User/engine_contract_build_route_open/build_route/Project",
        "runtimeArtifact": "../Build/bin/build_route_runtime.dll",
        "activeRuntimeArtifact": "User/engine_contract_build_route_open/build_route/Export/build_route_runtime.dll",
        "builtRuntimeArtifact": "User/engine_contract_build_route_open/build_route/Build/bin/build_route_runtime.dll",
        "packagedRuntimeArtifact": "User/engine_contract_build_route_open/build_route/Export/build_route_runtime.dll",
        "packagedExecutable": "User/engine_contract_build_route_open/build_route/Export/build_route.exe",
        "exportPath": "User/engine_contract_build_route_open/build_route/Export",
        "scene": "build_route/Project/scenes/main.scene",
    }


def main() -> int:
    manifest = studio_mcp.json.loads(studio_mcp.TOOLS_PATH.read_text(encoding="utf-8"))
    assert isinstance(manifest.get("tools"), list)
    assert len(manifest["tools"]) == len(studio_mcp.TOOLS)
    assert "exit" not in {command for command, _ in studio_mcp.TOOLS.values()}
    assert "list_entities" in studio_mcp.TOOLS
    assert "studio_capabilities" in studio_mcp.TOOLS
    assert "list_capabilities" in studio_mcp.ALLOWED_COMMANDS

    read, mutate = studio_mcp.extract_capabilities({
        "data": build_capabilities_payload()
    })
    assert "get_status" in read
    assert "exit" in mutate

    listed = studio_mcp.dispatch({"jsonrpc": "2.0", "id": 1, "method": "tools/list"})
    assert listed and len(listed["result"]["tools"]) == len(studio_mcp.TOOLS)
    assert all(
        tool["inputSchema"]["additionalProperties"] is False
        for tool in listed["result"]["tools"]
    )

    def fake_raw_call(command: str) -> dict:
        if command == "list_capabilities":
            return build_capabilities_response(build_capabilities_payload())
        if command == "get_project_status":
            return {
                "kind": "response",
                "request_id": "test-project",
                "ok": True,
                "command": command,
                "data": build_project_status_data(),
            }
        return {
            "kind": "response",
            "request_id": "test",
            "ok": True,
            "command": command,
            "data": {"state": "Edit"},
        }

    studio_mcp.raw_call_studio = fake_raw_call
    called = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 2,
        "method": "tools/call",
        "params": {"name": "runtime_status", "arguments": {}},
    })
    assert called and called["result"]["isError"] is False
    assert called["result"]["structuredContent"]["command"] == "get_runtime_status"

    capabilities = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 4,
        "method": "tools/call",
        "params": {"name": "studio_capabilities", "arguments": {}},
    })
    assert capabilities and capabilities["result"]["isError"] is False
    assert capabilities["result"]["structuredContent"]["command"] == "list_capabilities"

    project_status = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 13,
        "method": "tools/call",
        "params": {"name": "project_status", "arguments": {}},
    })
    assert project_status and project_status["result"]["isError"] is False
    project_content = project_status["result"]["structuredContent"]
    assert project_content["command"] == "get_project_status"
    project_data = project_content["data"]
    assert project_data["runtimeArtifact"] == "../Build/bin/build_route_runtime.dll"
    assert project_data["activeRuntimeArtifact"].endswith("/Export/build_route_runtime.dll")
    assert project_data["builtRuntimeArtifact"].endswith("/Build/bin/build_route_runtime.dll")
    assert project_data["packagedRuntimeArtifact"].endswith("/Export/build_route_runtime.dll")
    assert project_data["packagedExecutable"].endswith("/Export/build_route.exe")
    assert project_data["exportPath"].endswith("/Export")

    studio_mcp.raw_call_studio = lambda command: build_capabilities_response(
        build_capabilities_payload(
            read=["get_status"],
            mutate=["exit"],
            commands=[
                {
                    "name": "get_status",
                    "access": "read",
                    "description": "Read the native Studio backend status.",
                    "bridgeToolName": "studio_status",
                },
                {
                    "name": "exit",
                    "access": "mutating",
                    "description": "Request Studio shutdown.",
                },
            ],
        )
    )
    denied = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 5,
        "method": "tools/call",
        "params": {"name": "runtime_status", "arguments": {}},
    })
    assert denied and denied["result"]["isError"] is True
    assert "do not authorize" in denied["result"]["content"][0]["text"]

    studio_mcp.raw_call_studio = lambda command: build_capabilities_response(
        build_capabilities_payload(
            read=["get_status", "get_runtime_status"],
            mutate=["exit"],
            commands=[
                {
                    "name": "get_status",
                    "access": "read",
                    "description": "Read the native Studio backend status.",
                    "bridgeToolName": "studio_status",
                },
                {
                    "name": "exit",
                    "access": "mutating",
                    "description": "Request Studio shutdown.",
                },
            ],
        )
    )
    inconsistent = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 6,
        "method": "tools/call",
        "params": {"name": "runtime_status", "arguments": {}},
    })
    assert inconsistent and inconsistent["result"]["isError"] is True
    assert "did not match" in inconsistent["result"]["content"][0]["text"]

    studio_mcp.raw_call_studio = lambda command: build_capabilities_response(
        build_capabilities_payload(protocol_version="2")
    )
    protocol_error = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 7,
        "method": "tools/call",
        "params": {"name": "runtime_status", "arguments": {}},
    })
    assert protocol_error and protocol_error["result"]["isError"] is True
    assert "unsupported protocolVersion" in protocol_error["result"]["content"][0]["text"]

    studio_mcp.raw_call_studio = lambda command: build_capabilities_response(
        build_capabilities_payload(mutations_enabled=True)
    )
    mutation_policy_error = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 8,
        "method": "tools/call",
        "params": {"name": "runtime_status", "arguments": {}},
    })
    assert mutation_policy_error and mutation_policy_error["result"]["isError"] is True
    assert "mutation authority enabled" in mutation_policy_error["result"]["content"][0]["text"]

    studio_mcp.raw_call_studio = lambda command: build_capabilities_response(
        build_capabilities_payload(
            commands=[
                {
                    "name": "get_status",
                    "access": "read",
                    "description": "Read the native Studio backend status.",
                    "bridgeToolName": "studio_status",
                },
                {
                    "name": "get_runtime_status",
                    "access": "read",
                    "description": "Read play state, FPS, active project, and runtime artifact.",
                    "bridgeToolName": "runtime_status",
                },
                {
                    "name": "exit",
                    "access": "mutating",
                    "description": "Request Studio shutdown.",
                },
            ]
        )
    )
    missing_manifest_command = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 9,
        "method": "tools/call",
        "params": {"name": "runtime_status", "arguments": {}},
    })
    assert missing_manifest_command and missing_manifest_command["result"]["isError"] is True
    assert "did not publish required read-only bridge commands" in missing_manifest_command["result"]["content"][0]["text"]

    studio_mcp.raw_call_studio = lambda command: build_capabilities_response(
        build_capabilities_payload(external_read_only_bridge=False)
    )
    bridge_policy_error = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 10,
        "method": "tools/call",
        "params": {"name": "runtime_status", "arguments": {}},
    })
    assert bridge_policy_error and bridge_policy_error["result"]["isError"] is True
    assert "not permanently read-only" in bridge_policy_error["result"]["content"][0]["text"]

    studio_mcp.raw_call_studio = lambda command: build_capabilities_response(
        build_capabilities_payload(
            commands=[
                {
                    "name": "get_status",
                    "access": "read",
                    "description": "Read the native Studio backend status.",
                    "bridgeToolName": "wrong_tool_name",
                },
                *FULL_COMMAND_DESCRIPTORS[1:],
            ]
        )
    )
    wrong_bridge_tool_name = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 11,
        "method": "tools/call",
        "params": {"name": "runtime_status", "arguments": {}},
    })
    assert wrong_bridge_tool_name and wrong_bridge_tool_name["result"]["isError"] is True
    assert "expected bridge tool name" in wrong_bridge_tool_name["result"]["content"][0]["text"]

    studio_mcp.raw_call_studio = lambda command: build_capabilities_response(
        build_capabilities_payload(
            commands=[
                {
                    "name": "get_status",
                    "access": "read",
                    "description": "Wrong description",
                    "bridgeToolName": "studio_status",
                },
                *FULL_COMMAND_DESCRIPTORS[1:],
            ]
        )
    )
    wrong_bridge_description = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 12,
        "method": "tools/call",
        "params": {"name": "runtime_status", "arguments": {}},
    })
    assert wrong_bridge_description and wrong_bridge_description["result"]["isError"] is True
    assert "expected bridge tool description" in wrong_bridge_description["result"]["content"][0]["text"]

    rejected = studio_mcp.dispatch({
        "jsonrpc": "2.0",
        "id": 3,
        "method": "tools/call",
        "params": {"name": "exit", "arguments": {}},
    })
    assert rejected and rejected["error"]["code"] == -32602
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
