#include "studio/studio_template_system.hpp"

#include "studio/file_proxy.hpp"
#include "runtime/scene/default_scene_json.hpp"

#include <cctype>
#include <sstream>

namespace studio
{
    namespace
    {
        std::string JoinVirtualPath(const std::string& left, const std::string& right)
        {
            if (left.empty())
            {
                return right;
            }

            if (right.empty())
            {
                return left;
            }

            return left + "/" + right;
        }

        std::string ToPascalIdentifier(const std::string& projectId)
        {
            std::string identifier;
            bool capitalizeNext = true;

            for (const char character : projectId)
            {
                if (character == '_')
                {
                    capitalizeNext = true;
                    continue;
                }

                const unsigned char value = static_cast<unsigned char>(character);
                if (std::isalnum(value) == 0)
                {
                    capitalizeNext = true;
                    continue;
                }

                if (capitalizeNext)
                {
                    identifier += static_cast<char>(std::toupper(value));
                    capitalizeNext = false;
                }
                else
                {
                    identifier += static_cast<char>(std::tolower(value));
                }
            }

            if (identifier.empty() || std::isdigit(static_cast<unsigned char>(identifier.front())) != 0)
            {
                identifier.insert(identifier.begin(), 'G');
            }

            return identifier;
        }

        std::string BuildArcadeHeader(const std::string& className)
        {
            std::ostringstream output;
            output
                << "#pragma once\n"
                << "\n"
                << "#include \"engine/game_runtime_api.hpp\"\n"
                << "\n"
                << "class " << className << "Runtime final : public studio_runtime::IGameRuntime\n"
                << "{\n"
                << "public:\n"
                << "    void BeginPlay(const studio_runtime::ProjectRuntimeDesc& project) override;\n"
                << "    void EndPlay() override;\n"
                << "    void Tick(float deltaSeconds, const studio_runtime::RuntimeInput& input, studio_runtime::RuntimeFrame& frame) override;\n"
                << "\n"
                << "private:\n"
                << "    float m_playerX = 0.0f;\n"
                << "    int m_score = 0;\n"
                << "    bool m_playing = false;\n"
                << "};\n";
            return output.str();
        }

        std::string BuildArcadeMainSource(const std::string& projectId, const std::string& className)
        {
            std::ostringstream output;
            output
                << "#include \"" << projectId << ".hpp\"\n"
                << "\n"
                << "#include \"engine/game_runtime_api.hpp\"\n"
                << "\n"
                << "namespace\n"
                << "{\n"
                << "    studio_runtime::IGameRuntime* CreateRuntime()\n"
                << "    {\n"
                << "        return new " << className << "Runtime();\n"
                << "    }\n"
                << "\n"
                << "    void DestroyRuntime(studio_runtime::IGameRuntime* runtime)\n"
                << "    {\n"
                << "        delete runtime;\n"
                << "    }\n"
                << "}\n"
                << "\n"
                << "extern \"C\" GAME_RUNTIME_API_EXPORT const GameRuntimeApi* get_game_api()\n"
                << "{\n"
                << "    static const GameRuntimeApi api{\n"
                << "        studio_runtime::kGameRuntimeApiVersion,\n"
                << "        static_cast<std::uint32_t>(sizeof(GameRuntimeApi)),\n"
                << "        \"" << className << "Runtime\",\n"
                << "        &CreateRuntime,\n"
                << "        &DestroyRuntime\n"
                << "    };\n"
                << "    return &api;\n"
                << "}\n";
            return output.str();
        }

        std::string BuildArcadeSource(const std::string& projectName, const std::string& projectId, const std::string& className)
        {
            std::ostringstream output;
            output
                << "#include \"" << projectId << ".hpp\"\n"
                << "\n"
                << "#include \"prototypes/presentation/overlay_frame_builder.hpp\"\n"
                << "\n"
                << "#include <algorithm>\n"
                << "\n"
                << "void " << className << "Runtime::BeginPlay(const studio_runtime::ProjectRuntimeDesc&)\n"
                << "{\n"
                << "    m_playerX = 0.0f;\n"
                << "    m_score = 0;\n"
                << "    m_playing = true;\n"
                << "}\n"
                << "\n"
                << "void " << className << "Runtime::EndPlay()\n"
                << "{\n"
                << "    m_playing = false;\n"
                << "}\n"
                << "\n"
                << "void " << className << "Runtime::Tick(\n"
                << "    float deltaSeconds,\n"
                << "    const studio_runtime::RuntimeInput& input,\n"
                << "    studio_runtime::RuntimeFrame& frame)\n"
                << "{\n"
                << "    const float moveAxis =\n"
                << "        (input.rawInput.Key(InputKey::Right).down ? 1.0f : 0.0f) -\n"
                << "        (input.rawInput.Key(InputKey::Left).down ? 1.0f : 0.0f);\n"
                << "\n"
                << "    if (input.resetPressed)\n"
                << "    {\n"
                << "        m_playerX = 0.0f;\n"
                << "        m_score = 0;\n"
                << "    }\n"
                << "    if (input.servePressed)\n"
                << "    {\n"
                << "        ++m_score;\n"
                << "    }\n"
                << "    if (m_playing)\n"
                << "    {\n"
                << "        m_playerX = std::clamp(m_playerX + (moveAxis * deltaSeconds), -0.9f, 0.9f);\n"
                << "    }\n"
                << "\n"
                << "    OverlayFrameBuilder presentation(8U);\n"
                << "    presentation.Rect(0.0f, -0.82f, 0.94f, 0.02f, ColorPrototype::FromBytes(86, 120, 145));\n"
                << "    presentation.Rect(m_playerX, -0.70f, 0.08f, 0.08f, ColorPrototype::FromBytes(232, 243, 241));\n"
                << "    frame.renderFrame = presentation.Build();\n"
                << "    frame.hasRenderFrame = true;\n"
                << "    frame.exitRequested = input.escapePressed;\n"
                << "    frame.textOutput = \"" << projectName << " | Score: \" + std::to_string(m_score);\n"
                << "}\n";
            return output.str();
        }

        std::string BuildProjectCMake(const std::string& projectId)
        {
            std::ostringstream output;
            output
                << "cmake_minimum_required(VERSION 3.20)\n"
                << "\n"
                << "project(" << projectId << " LANGUAGES CXX)\n"
                << "\n"
                << "set(CMAKE_CXX_STANDARD 17)\n"
                << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
                << "set(CMAKE_CXX_EXTENSIONS OFF)\n"
                << "\n"
                << "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)\n"
                << "set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)\n"
                << "set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)\n"
                << "\n"
                << "get_filename_component(ENGINE_CHILI_ROOT \"${CMAKE_CURRENT_LIST_DIR}/../../..\" ABSOLUTE)\n"
                << "include(\"${ENGINE_CHILI_ROOT}/cmake/compiler_warnings.cmake\")\n"
                << "include(\"${ENGINE_CHILI_ROOT}/cmake/sanitizers.cmake\")\n"
                << "\n"
                << "add_subdirectory(\"${ENGINE_CHILI_ROOT}/src/engine\" \"${CMAKE_BINARY_DIR}/engine_runtime\")\n"
                << "\n"
                << "add_library(" << projectId << "Runtime SHARED\n"
                << "    src/runtime_entry.cpp\n"
                << "    src/" << projectId << ".cpp\n"
                << "    src/" << projectId << ".hpp\n"
                << ")\n"
                << "\n"
                << "target_include_directories(" << projectId << "Runtime\n"
                << "    PRIVATE\n"
                << "        ${CMAKE_CURRENT_SOURCE_DIR}/src\n"
                << "        ${ENGINE_CHILI_ROOT}/src\n"
                << ")\n"
                << "\n"
                << "target_link_libraries(" << projectId << "Runtime\n"
                << "    PRIVATE\n"
                << "        EngineRuntime\n"
                << ")\n"
                << "\n"
                << "target_compile_definitions(" << projectId << "Runtime PRIVATE GAME_RUNTIME_EXPORTS)\n"
                << "set_target_properties(" << projectId << "Runtime PROPERTIES OUTPUT_NAME \"" << projectId << "_runtime\" PREFIX \"\")\n"
                << "apply_project_warnings(" << projectId << "Runtime)\n"
                << "enable_project_sanitizers(" << projectId << "Runtime)\n"
                << "\n"
                << "add_executable(" << projectId << " ${ENGINE_CHILI_ROOT}/tools/game_preview/main.cpp)\n"
                << "target_link_libraries(" << projectId << " PRIVATE EngineRuntime)\n"
                << "target_compile_definitions(" << projectId << " PRIVATE\n"
                << "    GAME_RUNTIME_DLL_NAME=\\\"" << projectId << "_runtime.dll\\\"\n"
                << "    GAME_PROJECT_ID=\\\"" << projectId << "\\\"\n"
                << "    GAME_PROJECT_NAME=\\\"" << projectId << "\\\")\n"
                << "add_dependencies(" << projectId << " " << projectId << "Runtime)\n"
                << "add_custom_command(TARGET " << projectId << " POST_BUILD\n"
                << "    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EngineRuntime> $<TARGET_FILE_DIR:" << projectId << ">/$<TARGET_FILE_NAME:EngineRuntime>\n"
                << "    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:" << projectId << "Runtime> $<TARGET_FILE_DIR:" << projectId << ">/$<TARGET_FILE_NAME:" << projectId << "Runtime>)\n"
                << "apply_project_warnings(" << projectId << ")\n"
                << "enable_project_sanitizers(" << projectId << ")\n";
            return output.str();
        }

        std::string BuildMainScene(const std::string&)
        {
            // Default scene template is reusable editor/runtime composition only.
            // It must stay free of gameplay behavior.
            return studio_runtime::BuildDefaultSceneTemplateSceneJson();
        }

        std::string BuildGameConfig()
        {
            return "target_fps = 60\nstartup_scene = scenes/main.scene\n";
        }
    }

    StudioTemplateSystem::StudioTemplateSystem(const FileProxy& files)
        : m_files(files)
    {
    }

    ApplyTemplateResult StudioTemplateSystem::ApplyTemplate(
        const std::string& templateName,
        const std::string& targetProjectPath,
        const std::string& projectName,
        const std::string& projectId) const
    {
        if (templateName == "Arcade2D")
        {
            return ApplyArcade2DTemplate(targetProjectPath, projectName, projectId);
        }

        ApplyTemplateResult result;
        result.error = "Unknown project template: " + templateName;
        return result;
    }

    ApplyTemplateResult StudioTemplateSystem::ApplyArcade2DTemplate(
        const std::string& targetProjectPath,
        const std::string& projectName,
        const std::string& projectId) const
    {
        ApplyTemplateResult result;
        const std::string className = ToPascalIdentifier(projectId);
        std::string error;

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "CMakeLists.txt"),
                BuildProjectCMake(projectId),
                error))
        {
            result.error = error;
            return result;
        }

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "src/runtime_entry.cpp"),
                BuildArcadeMainSource(projectId, className),
                error))
        {
            result.error = error;
            return result;
        }

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "src/" + projectId + ".hpp"),
                BuildArcadeHeader(className),
                error))
        {
            result.error = error;
            return result;
        }

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "src/" + projectId + ".cpp"),
                BuildArcadeSource(projectName, projectId, className),
                error))
        {
            result.error = error;
            return result;
        }

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "scenes/main.scene"),
                BuildMainScene(projectName),
                error))
        {
            result.error = error;
            return result;
        }

        if (!m_files.WriteText(
                JoinVirtualPath(targetProjectPath, "config/game.config"),
                BuildGameConfig(),
                error))
        {
            result.error = error;
            return result;
        }

        result.success = true;
        result.message = "Applied Arcade2D template.";
        return result;
    }
}
