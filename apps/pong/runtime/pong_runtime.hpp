#pragma once

#include "engine/game_runtime_api.hpp"
#include "pong_game.hpp"

class PongRuntime final : public studio_runtime::IGameRuntime
{
public:
    void BeginPlay(const studio_runtime::ProjectRuntimeDesc& project) override;
    void EndPlay() override;
    void Tick(
        float deltaSeconds,
        const studio_runtime::RuntimeInput& input,
        studio_runtime::RuntimeFrame& frame) override;

private:
    PongGame m_game;
    std::string m_projectName = "Pong";
    bool m_leftPaddleAiEnabled = false;
    bool m_rightPaddleAiEnabled = true;
    bool m_playing = false;
};

