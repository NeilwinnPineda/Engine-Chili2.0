add_executable(engine_pong
    src/main.cpp
    src/pong_app.cpp
    src/pong_app.hpp
    src/pong_frame_builder.cpp
    src/pong_frame_builder.hpp
    src/pong_game.cpp
    src/pong_game.hpp
)

target_include_directories(engine_pong
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(engine_pong
    PRIVATE
        engine_core
)

apply_project_warnings(engine_pong)
enable_project_sanitizers(engine_pong)

set_target_properties(engine_pong PROPERTIES
    FOLDER "apps"
)
