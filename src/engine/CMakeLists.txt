# EngineRuntime is the reusable engine DLL. It owns engine systems and module
# coordination; game/editor behavior must live in higher-level DLLs or hosts.
get_filename_component(ENGINE_CHILI_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

file(GLOB_RECURSE ENGINE_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${ENGINE_CHILI_ROOT}/src/app/*.cpp"
    "${ENGINE_CHILI_ROOT}/src/core/*.cpp"
    "${ENGINE_CHILI_ROOT}/src/engine/*.cpp"
    "${ENGINE_CHILI_ROOT}/src/input/*.cpp"
    "${ENGINE_CHILI_ROOT}/src/modules/*.cpp"
    "${ENGINE_CHILI_ROOT}/src/native_ui/*.cpp"
    "${ENGINE_CHILI_ROOT}/src/prototypes/*.cpp"
    "${ENGINE_CHILI_ROOT}/src/systems/*.cpp"
)

file(GLOB_RECURSE ENGINE_RUNTIME_HEADERS CONFIGURE_DEPENDS
    "${ENGINE_CHILI_ROOT}/include/*.hpp"
    "${ENGINE_CHILI_ROOT}/src/app/*.hpp"
    "${ENGINE_CHILI_ROOT}/src/core/*.hpp"
    "${ENGINE_CHILI_ROOT}/src/engine/*.hpp"
    "${ENGINE_CHILI_ROOT}/src/input/*.h"
    "${ENGINE_CHILI_ROOT}/src/modules/*.hpp"
    "${ENGINE_CHILI_ROOT}/src/native_ui/*.hpp"
    "${ENGINE_CHILI_ROOT}/src/prototypes/*.hpp"
    "${ENGINE_CHILI_ROOT}/src/systems/*.hpp"
)

add_library(EngineRuntime SHARED
    ${ENGINE_RUNTIME_SOURCES}
    ${ENGINE_RUNTIME_HEADERS}
)

add_library(engine_core ALIAS EngineRuntime)

target_include_directories(EngineRuntime
    PUBLIC
        ${ENGINE_CHILI_ROOT}/include
        ${ENGINE_CHILI_ROOT}/src
)

target_compile_features(EngineRuntime PUBLIC cxx_std_17)
apply_project_warnings(EngineRuntime)
enable_project_sanitizers(EngineRuntime)

if(WIN32)
    target_link_libraries(EngineRuntime
        PUBLIC
            ole32
            d3d11
            d3dcompiler
            dxgi
    )
endif()

if(MINGW)
    target_link_libraries(EngineRuntime
        PUBLIC
            gdi32
            winmm
            xaudio2_9
    )
endif()

set_target_properties(EngineRuntime PROPERTIES
    FOLDER "runtime"
    OUTPUT_NAME "engine"
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/engine"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/engine"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/engine"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
