#include "runtime/scene/runtime_world.hpp"
#include "runtime/scene/scene_serializer.hpp"
#include "pong_game.hpp"
#include "pong_runtime.hpp"
#include "commands/command_policy.hpp"
#include "commands/command_protocol.hpp"
#include "runtime/api/game_runtime_library.hpp"
#include "studio/file_proxy.hpp"
#include "studio/studio_build_request_service.hpp"
#include "studio/studio_command_query_service.hpp"
#include "studio/studio_project_system.hpp"
#include "studio/studio_workspace_request_service.hpp"
#include "input/input_binding.h"
#include "input/input_system.h"

#include <cmath>
#include <iostream>
#include <string>

namespace
{
    int g_failures = 0;

    void Check(bool condition, const char* expression, int line)
    {
        if (condition)
        {
            return;
        }

        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        ++g_failures;
    }

    bool NearlyEqual(float left, float right)
    {
        return std::fabs(left - right) <= 0.0001f;
    }

#define CHECK(expression) Check((expression), #expression, __LINE__)

    void TestEntityLifecycle()
    {
        studio_runtime::RuntimeWorld world;

        const studio_runtime::EntityId first = world.CreateEntity("First");
        const studio_runtime::EntityId explicitId = world.CreateEntityWithId(40, "Explicit");
        const studio_runtime::EntityId next = world.CreateEntity("Next");

        CHECK(first == 1);
        CHECK(explicitId == 40);
        CHECK(next == 41);
        CHECK(world.CreateEntityWithId(40, "Duplicate") == 0);
        CHECK(world.Contains(first));
        CHECK(world.GetEntityList().size() == 3);

        world.SetName(first, "Renamed");
        const studio_runtime::NameComponent* name = world.GetName(first);
        CHECK(name != nullptr);
        CHECK(name && name->name == "Renamed");

        TransformPrototype transform;
        transform.translation = Vector3(1.0f, 2.0f, 3.0f);
        world.SetTransform(first, transform);
        const studio_runtime::TransformComponent* storedTransform = world.GetTransform(first);
        CHECK(storedTransform != nullptr);
        CHECK(storedTransform && NearlyEqual(storedTransform->transform.translation.x, 1.0f));
        CHECK(storedTransform && NearlyEqual(storedTransform->transform.translation.y, 2.0f));
        CHECK(storedTransform && NearlyEqual(storedTransform->transform.translation.z, 3.0f));

        CHECK(world.DestroyEntity(first));
        CHECK(!world.Contains(first));
        CHECK(!world.DestroyEntity(first));
        CHECK(world.GetEntityList().size() == 2);

        world.Clear();
        CHECK(world.GetEntityList().empty());
        CHECK(world.CreateEntity("AfterClear") == 1);
    }

    void TestRenderPresets()
    {
        const studio_runtime::SceneRenderSettings low =
            studio_runtime::MakeSceneRenderSettings(studio_runtime::RenderConfigurationPreset::Low);
        const studio_runtime::SceneRenderSettings balanced =
            studio_runtime::MakeSceneRenderSettings(studio_runtime::RenderConfigurationPreset::Balanced);
        const studio_runtime::SceneRenderSettings high =
            studio_runtime::MakeSceneRenderSettings(studio_runtime::RenderConfigurationPreset::High);

        CHECK(low.derivedBounce.enabled);
        CHECK(!low.tracedIndirect.enabled);
        CHECK(balanced.derivedBounce.enabled);
        CHECK(high.derivedBounce.enabled);
        CHECK(high.tracedIndirect.enabled);
        CHECK(high.derivedBounce.bounceStrength > low.derivedBounce.bounceStrength);
        CHECK(high.tracedIndirect.maxTraceDistance > low.tracedIndirect.maxTraceDistance);
    }

    void TestSceneRoundTrip()
    {
        studio_runtime::RuntimeWorld source;
        source.SetSceneRenderSettings(
            studio_runtime::MakeSceneRenderSettings(studio_runtime::RenderConfigurationPreset::High));

        const studio_runtime::EntityId entity = source.CreateEntityWithId(77, "RoundTripCube");

        TransformPrototype transform;
        transform.translation = Vector3(2.5f, -1.0f, 4.25f);
        transform.rotationRadians = Vector3(0.1f, 0.2f, 0.3f);
        transform.scale = Vector3(1.5f, 2.0f, 0.5f);
        source.SetTransform(entity, transform);

        studio_runtime::ObjectComponent object;
        object.kind = "Cube";
        object.prototypeId = "proto.test_cube";
        object.behaviorPrototypeId = "script.rotate_self";
        object.selectable = false;
        source.SetObject(entity, object);

        studio_runtime::RenderableComponent renderable;
        renderable.mesh = BuiltInMeshKind::Cube;
        renderable.visible = false;
        renderable.material.baseLayer.albedo = ColorPrototype::FromBytes(12, 34, 56, 255);
        source.SetRenderable(entity, renderable);

        const std::string serialized = studio_runtime::SceneSerializer::SaveToText(source);
        CHECK(!serialized.empty());
        CHECK(serialized.find("RoundTripCube") != std::string::npos);
        CHECK(serialized.find("proto.test_cube") != std::string::npos);

        studio_runtime::RuntimeWorld loaded;
        std::string error;
        CHECK(studio_runtime::SceneSerializer::LoadFromText(serialized, loaded, error));
        CHECK(error.empty());
        CHECK(loaded.Contains(77));

        studio_runtime::EntityInfo info;
        CHECK(loaded.GetEntityInfo(77, info));
        CHECK(info.name == "RoundTripCube");
        CHECK(info.hasObject);
        CHECK(info.object.prototypeId == "proto.test_cube");
        CHECK(info.object.behaviorPrototypeId == "script.rotate_self");
        CHECK(!info.object.selectable);
        CHECK(info.hasRenderable);
        CHECK(!info.renderable.visible);
        CHECK(NearlyEqual(info.transform.translation.x, 2.5f));
        CHECK(NearlyEqual(info.transform.rotationRadians.z, 0.3f));
        CHECK(NearlyEqual(info.transform.scale.y, 2.0f));

        const studio_runtime::SceneRenderSettings& settings = loaded.GetSceneRenderSettings();
        CHECK(settings.derivedBounce.enabled);
        CHECK(settings.tracedIndirect.enabled);
        CHECK(NearlyEqual(settings.tracedIndirect.maxTraceDistance, 14.0f));
    }

    void TestMalformedSceneDoesNotReplaceWorld()
    {
        studio_runtime::RuntimeWorld world;
        const studio_runtime::EntityId existing = world.CreateEntity("Existing");
        std::string error;

        CHECK(!studio_runtime::SceneSerializer::LoadFromText("{not-json", world, error));
        CHECK(!error.empty());
        CHECK(world.Contains(existing));
    }

    void TestPongSimulationContract()
    {
        PongGame game;
        PongGameSystem::Reset(game);

        CHECK(game.ball.waitingForServe);
        CHECK(game.score.left == 0);
        CHECK(game.score.right == 0);

        PongInput serve;
        serve.serveRequested = true;
        PongGameSystem::Step(game, serve, 1.0f / 60.0f);
        CHECK(!game.ball.waitingForServe);

        const float initialX = game.ball.centerX;
        PongGameSystem::Step(game, PongInput{}, 1.0f / 60.0f);
        CHECK(game.ball.centerX > initialX);

        game.ball.centerX = game.rules.arenaHalfWidth + game.rules.ballHalfSize + 0.01f;
        PongGameSystem::Step(game, PongInput{}, 0.0f);
        CHECK(game.score.left == 1);
        CHECK(game.ball.waitingForServe);
        CHECK(game.serveDirection == 1);

        PongInput reset;
        reset.resetRequested = true;
        PongGameSystem::Step(game, reset, 0.0f);
        CHECK(game.score.left == 0);
        CHECK(game.score.right == 0);
    }

    void TestPongRuntimeLifecycleContract()
    {
        PongRuntime runtime;
        studio_runtime::ProjectRuntimeDesc project;
        project.projectId = "pong";
        project.projectName = "Pong Contract";
        project.codeEntryKind = studio_runtime::ProjectCodeEntryKind::NativeArtifact;

        runtime.BeginPlay(project);
        studio_runtime::RuntimeInput input;
        input.servePressed = true;
        studio_runtime::RuntimeFrame frame;
        runtime.Tick(1.0f / 60.0f, input, frame);

        CHECK(frame.hasRenderFrame);
        CHECK(!frame.renderFrame.passes.empty());
        CHECK(frame.textOutput.find("Pong Contract") != std::string::npos);
        CHECK(!frame.exitRequested);

        input = studio_runtime::RuntimeInput{};
        input.escapePressed = true;
        frame = studio_runtime::RuntimeFrame{};
        runtime.Tick(0.0f, input, frame);
        CHECK(frame.exitRequested);
        runtime.EndPlay();
    }

    void TestRuntimeLibraryFailureContract()
    {
        studio_runtime::GameRuntimeLibrary library;
        std::string error;
        CHECK(!library.Load(std::string(), error));
        CHECK(!error.empty());
        CHECK(!library.IsLoaded());
        CHECK(library.GetRuntime() == nullptr);

        error.clear();
        CHECK(!library.Load("definitely_missing_game_runtime.dll", error));
        CHECK(!error.empty());
        CHECK(!library.IsLoaded());
    }

    void TestStudioCommandPolicy()
    {
        CHECK(ClassifyStudioCommand("get_status") == StudioCommandAccess::Read);
        CHECK(ClassifyStudioCommand("exit") == StudioCommandAccess::Mutating);
        CHECK(ClassifyStudioCommand("delete_everything") == StudioCommandAccess::Unknown);
        CHECK(IsStudioCommandAllowed("get_status", false));
        CHECK(!IsStudioCommandAllowed("exit", false));
        CHECK(IsStudioCommandAllowed("exit", true));
        CHECK(!IsStudioCommandAllowed("delete_everything", true));

        StudioCommandEnvelope envelope;
        std::string error;
        CHECK(ParseStudioCommandEnvelope(
            "{\"protocol_version\":\"1\",\"kind\":\"command\",\"command\":\"get_status\",\"request_id\":\"test-1\"}",
            envelope,
            error));
        CHECK(error.empty());
        CHECK(envelope.command == "get_status");
        CHECK(envelope.requestId == "test-1");

        CHECK(!ParseStudioCommandEnvelope(
            "{\"kind\":\"command\",\"command\":\"ping\",\"command\":\"exit\"}",
            envelope,
            error));
        CHECK(!error.empty());

        const std::vector<StudioCommandDescriptor>& descriptors = GetStudioCommandDescriptors();
        CHECK(!descriptors.empty());
        CHECK(descriptors.front().name == "hello");
        CHECK(!descriptors.front().description.empty());
        CHECK(descriptors.front().bridgeToolName.empty());
        CHECK(descriptors[2].name == "get_status");
        CHECK(descriptors[2].bridgeToolName == "studio_status");
        CHECK(GetStudioCommandAccessName(descriptors.front().access) == "read");
        CHECK(BuildStudioCapabilitiesMessage(false).find("currently disabled") != std::string::npos);
        CHECK(BuildStudioCapabilitiesMessage(true).find("currently enabled") != std::string::npos);

        const std::string capabilitiesJson = BuildStudioCapabilitiesDataJson("1", false);
        const std::string capabilitiesWithMutationsJson = BuildStudioCapabilitiesDataJson("1", true);
        CHECK(capabilitiesJson.find("\"protocolVersion\":\"1\"") != std::string::npos);
        CHECK(capabilitiesJson.find("\"mutationsEnabled\":false") != std::string::npos);
        CHECK(capabilitiesWithMutationsJson.find("\"mutationsEnabled\":true") != std::string::npos);
        CHECK(capabilitiesJson.find("\"externalReadOnlyBridge\":true") != std::string::npos);
        CHECK(capabilitiesJson.find("\"read\"") != std::string::npos);
        CHECK(capabilitiesJson.find("\"mutate\"") != std::string::npos);
        CHECK(capabilitiesJson.find("\"commands\"") != std::string::npos);
        CHECK(capabilitiesJson.find("\"description\"") != std::string::npos);
        CHECK(capabilitiesJson.find("\"bridgeToolName\":\"studio_status\"") != std::string::npos);
        CHECK(capabilitiesJson.find("\"bridgeToolName\":\"runtime_status\"") != std::string::npos);
        CHECK(capabilitiesJson.find("\"get_runtime_status\"") != std::string::npos);
        CHECK(capabilitiesJson.find("\"exit\"") != std::string::npos);
    }

    void TestInputPriorityAndConsumption()
    {
        InputSystem input;
        input.RegisterContext("Game", 10).BindAction("Fire", BindMouseButton(InputMouseButton::Left));
        input.RegisterContext("Studio", 100).BindAction("Select", BindMouseButton(InputMouseButton::Left));

        RawInputState raw;
        RawButtonState& leftMouse = raw.mouseButtons[static_cast<std::size_t>(InputMouseButton::Left)];
        leftMouse.pressed = true;
        leftMouse.down = true;
        input.Evaluate(raw);

        CHECK(input.Pressed("Studio", "Select"));
        CHECK(!input.Pressed("Game", "Fire"));

        input.SetContextEnabled("Studio", false);
        input.Evaluate(raw);
        CHECK(input.Pressed("Game", "Fire"));

        InputSystem modifiers;
        modifiers.RegisterContext("Studio", 100).BindAction(
            "Select",
            BindMouseButton(InputMouseButton::Left, false, false, false, true, false));
        modifiers.RegisterContext("Studio", 100).BindAction(
            "MultiSelect",
            BindMouseButton(InputMouseButton::Left, false, true, false, true, false));

        modifiers.Evaluate(raw);
        CHECK(modifiers.Pressed("Studio", "Select"));
        CHECK(!modifiers.Pressed("Studio", "MultiSelect"));
        raw.keys[static_cast<std::size_t>(InputKey::Shift)].down = true;
        modifiers.Evaluate(raw);
        CHECK(!modifiers.Pressed("Studio", "Select"));
        CHECK(modifiers.Pressed("Studio", "MultiSelect"));
    }

    void TestStudioProjectRuntimeArtifactState()
    {
        const std::string workspaceRoot = "User/engine_contract_workspace";
        studio::StudioProjectSystem projects{ studio::FileProxy(workspaceRoot) };

        studio::CreateProjectRequest request;
        request.projectName = "Artifact Contract";
        request.templateName = "Arcade2D";
        request.overwrite = true;

        const studio::CreateProjectResult created = projects.CreateProject(request);
        CHECK(created.success);

        projects.RecordBuildOutputs(
            "User/engine_contract_workspace/artifact_contract/Build/bin/artifact_contract_runtime.dll",
            "User/engine_contract_workspace/artifact_contract/Export/artifact_contract_runtime.dll",
            "User/engine_contract_workspace/artifact_contract/Export/artifact_contract.exe",
            "User/engine_contract_workspace/artifact_contract/Export");

        const studio::StudioProject project = projects.GetCurrentProject();
        CHECK(project.isOpen);
        CHECK(project.exportedArtifactPath == "../Build/bin/artifact_contract_runtime.dll");
        CHECK(project.builtRuntimeArtifactPath.find("Build/bin/artifact_contract_runtime.dll") != std::string::npos);
        CHECK(project.packagedRuntimeArtifactPath.find("Export/artifact_contract_runtime.dll") != std::string::npos);
        CHECK(project.packagedExecutablePath.find("Export/artifact_contract.exe") != std::string::npos);
        CHECK(project.logicalExportPath.find("Export") != std::string::npos);

        StudioCommandQueryService queries;
        queries.Configure(
            &projects,
            nullptr,
            nullptr,
            {},
            {},
            {},
            {},
            []() { return std::string("artifact_contract/Project/scenes/main.scene"); });

        const std::string projectStatusJson = queries.BuildProjectStatusJson();
        CHECK(projectStatusJson.find("\"runtimeArtifact\":\"../Build/bin/artifact_contract_runtime.dll\"") != std::string::npos);
        CHECK(projectStatusJson.find("\"activeRuntimeArtifact\":\"User/engine_contract_workspace/artifact_contract/Export/artifact_contract_runtime.dll\"") != std::string::npos);
        CHECK(projectStatusJson.find("\"builtRuntimeArtifact\":\"User/engine_contract_workspace/artifact_contract/Build/bin/artifact_contract_runtime.dll\"") != std::string::npos);
        CHECK(projectStatusJson.find("\"packagedRuntimeArtifact\":\"User/engine_contract_workspace/artifact_contract/Export/artifact_contract_runtime.dll\"") != std::string::npos);
        CHECK(projectStatusJson.find("\"packagedExecutable\":\"User/engine_contract_workspace/artifact_contract/Export/artifact_contract.exe\"") != std::string::npos);
        CHECK(projectStatusJson.find("\"exportPath\":\"User/engine_contract_workspace/artifact_contract/Export\"") != std::string::npos);
        CHECK(projectStatusJson.find("\"scene\":\"artifact_contract/Project/scenes/main.scene\"") != std::string::npos);

        projects.RecordBuildOutputs(
            "User/engine_contract_workspace/artifact_contract/Build/bin/artifact_contract_runtime.dll",
            std::string(),
            std::string(),
            std::string());

        const std::string builtOnlyStatusJson = queries.BuildProjectStatusJson();
        CHECK(builtOnlyStatusJson.find("\"activeRuntimeArtifact\":\"User/engine_contract_workspace/artifact_contract/Build/bin/artifact_contract_runtime.dll\"") != std::string::npos);
        CHECK(builtOnlyStatusJson.find("\"packagedRuntimeArtifact\":\"\"") != std::string::npos);
        CHECK(builtOnlyStatusJson.find("\"packagedExecutable\":\"\"") != std::string::npos);
    }

    void TestStudioBuildRequestService()
    {
        studio::StudioBuildRequestService buildRequests;

        {
            studio::StudioProjectSystem closedProjects{ studio::FileProxy("User/engine_contract_build_route_closed") };
            const studio::StudioBuildRouteResult closedResult = buildRequests.ExecuteBuildRequestWithInvoker(
                closedProjects,
                [](const studio::BuildAndRunProjectRequest&)
                {
                    return studio::BuildAndRunProjectResult{};
                },
                true,
                true);
            CHECK(!closedResult.success);
            CHECK(closedResult.message == "No project open.");
            CHECK(closedResult.consoleLines.size() == 1);
            CHECK(closedResult.consoleLines.front().find("no project open") != std::string::npos);
        }

        studio::StudioProjectSystem projects{ studio::FileProxy("User/engine_contract_build_route_open") };
        studio::CreateProjectRequest request;
        request.projectName = "Build Route";
        request.templateName = "Arcade2D";
        request.overwrite = true;
        const studio::CreateProjectResult created = projects.CreateProject(request);
        CHECK(created.success);

        const studio::StudioBuildRouteResult successResult = buildRequests.ExecuteBuildRequestWithInvoker(
            projects,
            [](const studio::BuildAndRunProjectRequest& request)
            {
                studio::BuildAndRunProjectResult result;
                result.success = true;
                result.projectId = request.projectId;
                result.logicalBuildPath = "User/engine_contract_build_route_open/build_route/Build";
                result.builtRuntimeOutputPath = "User/engine_contract_build_route_open/build_route/Build/bin/build_route_runtime.dll";
                result.runtimeOutputPath = result.builtRuntimeOutputPath;
                result.packagedRuntimeOutputPath = "User/engine_contract_build_route_open/build_route/Export/build_route_runtime.dll";
                result.executablePath = "User/engine_contract_build_route_open/build_route/Export/build_route.exe";
                result.packagedExecutablePath = result.executablePath;
                result.logicalExportPath = "User/engine_contract_build_route_open/build_route/Export";
                result.message = "Exported project 'build_route' to User/engine_contract_build_route_open/build_route/Export.";
                return result;
            },
            true,
            true);

        CHECK(successResult.success);
        CHECK(successResult.projectId == "build_route");
        CHECK(successResult.logicalPath == "User/engine_contract_build_route_open/build_route/Export");
        CHECK(successResult.consoleLines.size() >= 5);
        CHECK(successResult.consoleLines[0].find("Build requested for project 'build_route'.") != std::string::npos);
        CHECK(successResult.consoleLines[1].find("Exported project 'build_route'") != std::string::npos);
        CHECK(successResult.consoleLines[2].find("Export output: User/engine_contract_build_route_open/build_route/Export") != std::string::npos);
        CHECK(successResult.consoleLines[3].find("Runtime output: User/engine_contract_build_route_open/build_route/Build/bin/build_route_runtime.dll") != std::string::npos);
        CHECK(successResult.consoleLines[4].find("Preview executable: User/engine_contract_build_route_open/build_route/Export/build_route.exe") != std::string::npos);

        const studio::StudioProject updatedProject = projects.GetCurrentProject();
        CHECK(updatedProject.builtRuntimeArtifactPath.find("Build/bin/build_route_runtime.dll") != std::string::npos);
        CHECK(updatedProject.packagedRuntimeArtifactPath.find("Export/build_route_runtime.dll") != std::string::npos);
        CHECK(updatedProject.packagedExecutablePath.find("Export/build_route.exe") != std::string::npos);
        CHECK(updatedProject.logicalExportPath.find("Export") != std::string::npos);

        const studio::StudioBuildRouteResult failureResult = buildRequests.ExecuteBuildRequestWithInvoker(
            projects,
            [](const studio::BuildAndRunProjectRequest& request)
            {
                studio::BuildAndRunProjectResult result;
                result.success = false;
                result.projectId = request.projectId;
                result.logicalBuildPath = "User/engine_contract_build_route_open/build_route/Build";
                result.error = "Project build failed with exit code 7.";
                result.configureExitCode = 0;
                result.buildExitCode = 7;
                return result;
            },
            false,
            false);

        CHECK(!failureResult.success);
        CHECK(failureResult.message == "Project build failed with exit code 7.");
        CHECK(failureResult.logicalPath == "User/engine_contract_build_route_open/build_route/Build");
        CHECK(failureResult.consoleLines.size() == 2);
        CHECK(failureResult.consoleLines[0].find("Build requested for project 'build_route'.") != std::string::npos);
        CHECK(failureResult.consoleLines[1].find("configureExit=0, buildExit=7") != std::string::npos);
    }

    void TestStudioWorkspaceRequestService()
    {
        studio::StudioWorkspaceRequestService workspaceRequests;

        {
            studio::StudioProjectSystem closedProjects{ studio::FileProxy("User/engine_contract_workspace_closed") };
            const studio::StudioWorkspaceRouteResult saveClosedResult =
                workspaceRequests.ExecuteSaveLayout(closedProjects, true, true);
            CHECK(!saveClosedResult.success);
            CHECK(saveClosedResult.message == "No project open.");
        }

        studio::StudioProjectSystem projects{ studio::FileProxy("User/engine_contract_workspace_open") };
        studio::CreateProjectRequest request;
        request.projectName = "Workspace Route";
        request.templateName = "Arcade2D";
        request.overwrite = true;
        const studio::CreateProjectResult created = projects.CreateProject(request);
        CHECK(created.success);

        const studio::StudioWorkspaceRouteResult saveOpenResult =
            workspaceRequests.ExecuteSaveLayout(projects, false, true);
        CHECK(saveOpenResult.success);
        CHECK(saveOpenResult.message == "Layout saved.");
        CHECK(saveOpenResult.projectId == "workspace_route");
        CHECK(saveOpenResult.logicalPath.find("studio.session") != std::string::npos);

        bool leftVisible = true;
        bool bottomVisible = false;
        bool rightVisible = true;
        bool layoutChanged = false;

        const studio::StudioWorkspaceRouteResult bottomToggleResult =
            workspaceRequests.ExecuteToggleRequest(
                studio::WorkspaceToggleTarget::BottomBar,
                [&leftVisible]() { return leftVisible; },
                [&bottomVisible]() { return bottomVisible; },
                [&rightVisible]() { return rightVisible; },
                {},
                [&bottomVisible](bool visible)
                {
                    bottomVisible = visible;
                    return true;
                },
                {},
                [&layoutChanged]() { layoutChanged = true; });
        CHECK(bottomToggleResult.success);
        CHECK(bottomToggleResult.leftVisible);
        CHECK(bottomToggleResult.bottomVisible);
        CHECK(bottomToggleResult.rightVisible);
        CHECK(bottomToggleResult.message == "Bottom bar shown.");
        CHECK(layoutChanged);

        layoutChanged = false;
        const studio::StudioWorkspaceRouteResult explorerToggleResult =
            workspaceRequests.ExecuteToggleRequest(
                studio::WorkspaceToggleTarget::Explorer,
                [&leftVisible]() { return leftVisible; },
                [&bottomVisible]() { return bottomVisible; },
                [&rightVisible]() { return rightVisible; },
                {},
                {},
                [&rightVisible](bool visible)
                {
                    rightVisible = visible;
                    return true;
                },
                [&layoutChanged]() { layoutChanged = true; });
        CHECK(explorerToggleResult.success);
        CHECK(!explorerToggleResult.rightVisible);
        CHECK(explorerToggleResult.message == "Explorer hidden.");
        CHECK(layoutChanged);

        const studio::StudioWorkspaceRouteResult leftToggleFailureResult =
            workspaceRequests.ExecuteToggleRequest(
                studio::WorkspaceToggleTarget::LeftBar,
                [&leftVisible]() { return leftVisible; },
                [&bottomVisible]() { return bottomVisible; },
                [&rightVisible]() { return rightVisible; },
                [](bool)
                {
                    return false;
                },
                {},
                {},
                {});
        CHECK(!leftToggleFailureResult.success);
        CHECK(leftToggleFailureResult.leftVisible == leftVisible);
        CHECK(leftToggleFailureResult.message == "Left bar unavailable.");
    }
}

int main()
{
    TestEntityLifecycle();
    TestRenderPresets();
    TestSceneRoundTrip();
    TestMalformedSceneDoesNotReplaceWorld();
    TestPongSimulationContract();
    TestPongRuntimeLifecycleContract();
    TestRuntimeLibraryFailureContract();
    TestStudioCommandPolicy();
    TestInputPriorityAndConsumption();
    TestStudioProjectRuntimeArtifactState();
    TestStudioBuildRequestService();
    TestStudioWorkspaceRequestService();

    if (g_failures != 0)
    {
        std::cerr << g_failures << " contract test(s) failed.\n";
        return 1;
    }

    std::cout << "All engine contract tests passed.\n";
    return 0;
}
