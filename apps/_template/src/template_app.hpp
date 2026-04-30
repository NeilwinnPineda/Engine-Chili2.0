#pragma once

#include "app/app_capabilities.hpp"
#include "template_game.hpp"

class TemplateApp
{
public:
    bool Run();

private:
    void Initialize(AppCapabilities& capabilities);
    void UpdateFrame(AppCapabilities& capabilities);
    TemplateInput ReadGameplayInput(const IAppInput& input) const;
    void SimulateGame(const TemplateInput& input, const IAppTime& time);
    void PresentGame(AppCapabilities& capabilities) const;

private:
    TemplateGame m_game;
};
