#include "template_runtime.hpp"

#include "template_frame_builder.hpp"

#include <algorithm>
#include <string>

void TemplateRuntime::BeginPlay(const studio_runtime::ProjectRuntimeDesc&)
{
    TemplateGameSystem::Reset(m_game);
    m_playing = true;
}

void TemplateRuntime::EndPlay()
{
    m_playing = false;
}

void TemplateRuntime::Tick(
    float deltaSeconds,
    const studio_runtime::RuntimeInput& input,
    studio_runtime::RuntimeFrame& frame)
{
    const TemplateInput gameplayInput = ReadGameplayInput(input);
    if (m_playing)
    {
        TemplateGameSystem::Step(
            m_game,
            gameplayInput,
            static_cast<float>(std::max(0.0f, deltaSeconds)));
    }

    frame.renderFrame = TemplateFrameBuilder::Build(m_game);
    frame.hasRenderFrame = true;
    frame.exitRequested = input.escapePressed;
    frame.textOutput = "Template Runtime | Score: " + std::to_string(m_game.score);
}

TemplateInput TemplateRuntime::ReadGameplayInput(const studio_runtime::RuntimeInput& input) const
{
    const float moveAxis =
        (input.rawInput.Key(InputKey::Right).down ? 1.0f : 0.0f) -
        (input.rawInput.Key(InputKey::Left).down ? 1.0f : 0.0f);

    TemplateInput gameplayInput;
    gameplayInput.moveAxis = std::clamp(moveAxis, -1.0f, 1.0f);
    gameplayInput.actionPressed = input.servePressed;
    gameplayInput.resetPressed = input.resetPressed;
    return gameplayInput;
}
