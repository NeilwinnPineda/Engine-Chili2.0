#pragma once

#include "app/app_capabilities.hpp"
#include "pong_game.hpp"

#include <string>

class PongApp
{
public:
    bool Run();

private:
    void Initialize(AppCapabilities& capabilities);
    void UpdateFrame(AppCapabilities& capabilities);
    void ApplyAppActions(const IAppInput& input);
    PongInput ReadGameplayInput(const IAppInput& input) const;
    void SimulateGame(const PongInput& input, const IAppTime& time);
    void PresentGame(AppCapabilities& capabilities) const;

private:
    PongGame m_game;
    bool m_leftPaddleAiEnabled = false;
    bool m_rightPaddleAiEnabled = true;
};
