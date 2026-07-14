#pragma once

#include "engine/game_runtime_api.hpp"
#include "template_game.hpp"

class TemplateRuntime final : public studio_runtime::IGameRuntime
{
public:
    void BeginPlay(const studio_runtime::ProjectRuntimeDesc& project) override;
    void EndPlay() override;
    void Tick(
        float deltaSeconds,
        const studio_runtime::RuntimeInput& input,
        studio_runtime::RuntimeFrame& frame) override;

private:
    TemplateInput ReadGameplayInput(const studio_runtime::RuntimeInput& input) const;

private:
    TemplateGame m_game;
    bool m_playing = false;
};
