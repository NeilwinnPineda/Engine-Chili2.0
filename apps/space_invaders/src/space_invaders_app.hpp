#pragma once

#include "app/app_capabilities.hpp"
#include "space_invaders_game.hpp"

class SpaceInvadersApp
{
public:
    bool Run();

private:
    void Initialize(AppCapabilities& capabilities);
    void UpdateFrame(AppCapabilities& capabilities);
    void ApplyAppActions(const IAppInput& input);
    SpaceInvadersInput ReadGameplayInput(const IAppInput& input) const;
    SpaceInvadersInput ReadAiInput() const;
    void SimulateGame(const SpaceInvadersInput& input, const IAppTime& time);
    void PresentGame(AppCapabilities& capabilities) const;

private:
    SpaceInvadersGame m_game;
    bool m_aiEnabled = false;
};
