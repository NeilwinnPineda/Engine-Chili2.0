#include "space_invaders_app.hpp"

#include "core/engine_core.hpp"
#include "native_ui/native_ui_builder.hpp"
#include "space_invaders_frame_builder.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr std::uint32_t kClearColor = 0xFF050A12u;

    float ReadAxis(const IAppInput& input, AppKey positive, AppKey negative)
    {
        float axis = 0.0f;
        if (input.IsKeyDown(positive))
        {
            axis += 1.0f;
        }
        if (input.IsKeyDown(negative))
        {
            axis -= 1.0f;
        }

        return std::clamp(axis, -1.0f, 1.0f);
    }

    bool WasPressed(const IAppInput& input, AppKey key)
    {
        return input.WasKeyPressed(key);
    }
}

bool SpaceInvadersApp::Run()
{
    EngineCore core;
    if (!core.Initialize())
    {
        return false;
    }

    AppCapabilities& capabilities = core.GetAppCapabilities();
    Initialize(capabilities);

    core.SetFrameCallback(
        [this](AppCapabilities& callbackCapabilities)
        {
            UpdateFrame(callbackCapabilities);
        });

    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void SpaceInvadersApp::Initialize(AppCapabilities& capabilities)
{
    SpaceInvadersGameSystem::Reset(m_game);
    if (capabilities.logging)
    {
        capabilities.logging->Info("SpaceInvadersApp: app-local Space Invaders test ready.");
    }
}

void SpaceInvadersApp::UpdateFrame(AppCapabilities& capabilities)
{
    ApplyAppActions(*capabilities.input);

    const SpaceInvadersInput input = m_aiEnabled
        ? ReadAiInput()
        : ReadGameplayInput(*capabilities.input);
    SimulateGame(input, *capabilities.time);
    PresentGame(capabilities);
}

void SpaceInvadersApp::ApplyAppActions(const IAppInput& input)
{
    if (WasPressed(input, AppKey::A))
    {
        m_aiEnabled = !m_aiEnabled;
    }
}

SpaceInvadersInput SpaceInvadersApp::ReadGameplayInput(const IAppInput& input) const
{
    SpaceInvadersInput gameplayInput;
    gameplayInput.moveAxis = ReadAxis(input, AppKey::Right, AppKey::Left);
    gameplayInput.firePressed = input.WasKeyPressed(AppKey::Space);
    gameplayInput.resetPressed = input.WasKeyPressed(AppKey::R);
    return gameplayInput;
}

SpaceInvadersInput SpaceInvadersApp::ReadAiInput() const
{
    SpaceInvadersInput input;

    float targetX = 0.0f;
    float bestDistance = std::numeric_limits<float>::max();
    bool foundTarget = false;

    for (const SpaceInvader& alien : m_game.aliens)
    {
        if (!alien.alive)
        {
            continue;
        }

        const float alienX = m_game.fleetOffsetX + alien.localX;
        const float distance = std::abs(alienX - m_game.playerX);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            targetX = alienX;
            foundTarget = true;
        }
    }

    if (foundTarget)
    {
        const float error = targetX - m_game.playerX;
        if (std::abs(error) > 0.018f)
        {
            input.moveAxis = error > 0.0f ? 1.0f : -1.0f;
        }

        input.firePressed = std::abs(error) < 0.052f;
    }

    return input;
}

void SpaceInvadersApp::SimulateGame(const SpaceInvadersInput& input, const IAppTime& time)
{
    SpaceInvadersGameSystem::Step(
        m_game,
        input,
        static_cast<float>(std::max(0.0, time.GetDeltaSeconds())));
}

void SpaceInvadersApp::PresentGame(AppCapabilities& capabilities) const
{
    NativeUiBuilder ui;
    ui.WindowTitle(L"Engine Space Invaders")
        .OverlayEnabled(false)
        .ClearColor(kClearColor)
        .ContentFrame(SpaceInvadersFrameBuilder::Build(m_game));

    ui.Panel("Space Invaders")
        .Anchor(NativeUiAnchor::TopLeft, 16, 16, 300, 126)
        .Colors(0xFFE8F3F1u, 0xDD050A12u)
        .Row("Score", m_game.score)
        .Row("Lives", m_game.lives)
        .Row("AI", m_aiEnabled ? "on" : "off")
        .Row("Aliens", m_game.won ? "cleared" : "active")
        .Row("State", m_game.gameOver ? "game over" : m_game.won ? "won" : "live")
        .Row("Controls", "Left/Right, Space, A, R, Esc");

    capabilities.nativeUi->Submit(ui.Build());
}
