add_executable(EngineContractTests
    engine_contract_tests.cpp
    ${PROJECT_SOURCE_DIR}/apps/pong/src/pong_game.cpp
    ${PROJECT_SOURCE_DIR}/apps/pong/src/pong_frame_builder.cpp
    ${PROJECT_SOURCE_DIR}/apps/pong/runtime/pong_runtime.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/commands/command_policy.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/commands/command_protocol.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/studio/file_proxy.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/studio/studio_command_query_service.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/studio/studio_build_request_service.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/studio/studio_workspace_request_service.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/studio/studio_project_system.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/studio/studio_template_system.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/runtime/api/game_runtime_library.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/runtime/scene/default_scene_json.cpp
    ${PROJECT_SOURCE_DIR}/src/input/input_system.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/runtime/scene/runtime_world.cpp
    ${PROJECT_SOURCE_DIR}/apps/studio/src/runtime/scene/scene_serializer.cpp
)

target_include_directories(EngineContractTests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/apps/pong/src
        ${PROJECT_SOURCE_DIR}/apps/pong/runtime
        ${PROJECT_SOURCE_DIR}/apps/studio/src
)

apply_project_warnings(EngineContractTests)
enable_project_sanitizers(EngineContractTests)

set_target_properties(EngineContractTests PROPERTIES
    FOLDER "tests"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

add_test(NAME engine.contracts COMMAND EngineContractTests)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME studio.ai_bridge_contract
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_studio_mcp.py)
endif()
