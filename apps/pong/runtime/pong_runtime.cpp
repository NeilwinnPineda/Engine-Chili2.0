#include "pong_runtime.hpp"

#include "pong_frame_builder.hpp"

void PongRuntime::BeginPlay(const studio_runtime::ProjectRuntimeDesc& project)
{
    m_projectName = project.projectName.empty() ? "Pong" : project.projectName;
    m_leftPaddleAiEnabled = false;
    m_rightPaddleAiEnabled = true;
    PongGameSystem::Reset(m_game);
    m_playing = true;
}

void PongRuntime::EndPlay()
{
    m_playing = false;
}

void PongRuntime::Tick(
    float deltaSeconds,
    const studio_runtime::RuntimeInput& input,
    studio_runtime::RuntimeFrame& frame)
{
    if (input.rawInput.Key(InputKey::A).pressed)
    {
        m_rightPaddleAiEnabled = !m_rightPaddleAiEnabled;
    }
    if (input.rawInput.Key(InputKey::L).pressed)
    {
        m_leftPaddleAiEnabled = !m_leftPaddleAiEnabled;
    }

    PongInput pongInput;
    pongInput.leftPaddleAxis = (input.leftUpDown ? 1.0f : 0.0f) - (input.leftDownDown ? 1.0f : 0.0f);
    pongInput.rightPaddleAxis = (input.rightUpDown ? 1.0f : 0.0f) - (input.rightDownDown ? 1.0f : 0.0f);
    pongInput.leftPaddleAiEnabled = m_leftPaddleAiEnabled;
    pongInput.rightPaddleAiEnabled = m_rightPaddleAiEnabled;
    pongInput.resetRequested = input.resetPressed;
    pongInput.serveRequested = input.servePressed;

    if (m_playing)
    {
        PongGameSystem::Step(m_game, pongInput, deltaSeconds);
    }

    frame.renderFrame = PongFrameBuilder::Build(m_game);
    frame.hasRenderFrame = true;
    frame.exitRequested = input.escapePressed;
    frame.textOutput = m_projectName + " | " +
        std::to_string(m_game.score.left) + " - " +
        std::to_string(m_game.score.right) +
        (m_game.ball.waitingForServe ? " | ready" : " | live");
}
