add_executable(engine_space_invaders
    src/main.cpp
    src/space_invaders_app.cpp
    src/space_invaders_app.hpp
    src/space_invaders_frame_builder.cpp
    src/space_invaders_frame_builder.hpp
    src/space_invaders_game.cpp
    src/space_invaders_game.hpp
)

target_include_directories(engine_space_invaders
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(engine_space_invaders
    PRIVATE
        engine_core
)

apply_project_warnings(engine_space_invaders)
enable_project_sanitizers(engine_space_invaders)

set_target_properties(engine_space_invaders PROPERTIES
    FOLDER "apps"
)
