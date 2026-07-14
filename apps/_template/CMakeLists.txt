# Rename these targets/files to your real app/runtime names.
add_library(template_runtime SHARED
    src/runtime_entry.cpp
    src/template_runtime.cpp
    src/template_runtime.hpp
    src/template_frame_builder.cpp
    src/template_frame_builder.hpp
    src/template_game.cpp
    src/template_game.hpp
)

target_include_directories(template_runtime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(template_runtime
    PRIVATE
        EngineRuntime
)

target_compile_definitions(template_runtime
    PRIVATE
        GAME_RUNTIME_EXPORTS
)

apply_project_warnings(template_runtime)
enable_project_sanitizers(template_runtime)

set_target_properties(template_runtime PROPERTIES
    FOLDER "apps"
    OUTPUT_NAME "template_runtime"
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/apps/template"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/apps/template"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/apps/template"
)

add_executable(template_preview
    ${PROJECT_SOURCE_DIR}/tools/game_preview/main.cpp
)

target_compile_definitions(template_preview
    PRIVATE
        GAME_RUNTIME_DLL_NAME=\"template_runtime.dll\"
        GAME_PROJECT_ID=\"template\"
        GAME_PROJECT_NAME=\"App Template\"
)

target_link_libraries(template_preview
    PRIVATE
        EngineRuntime
)

add_dependencies(template_preview template_runtime)

if(WIN32)
    add_custom_command(TARGET template_preview POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:EngineRuntime>"
            "$<TARGET_FILE_DIR:template_preview>/$<TARGET_FILE_NAME:EngineRuntime>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:template_runtime>"
            "$<TARGET_FILE_DIR:template_preview>/$<TARGET_FILE_NAME:template_runtime>")
endif()

apply_project_warnings(template_preview)
enable_project_sanitizers(template_preview)

set_target_properties(template_preview PROPERTIES
    FOLDER "apps"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/apps/template"
)
