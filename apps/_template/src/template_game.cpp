#include "template_game.hpp"

#include <algorithm>

void TemplateGameSystem::Reset(TemplateGame& game)
{
    // Preserve configurable rules across resets.
    const TemplateRules rules = game.rules;
    game = TemplateGame{};
    game.rules = rules;
}

void TemplateGameSystem::Step(TemplateGame& game, const TemplateInput& input, float deltaSeconds)
{
    if (input.resetPressed)
    {
        Reset(game);
        return;
    }

    // Clamp delta to avoid huge jumps after a breakpoint or window stall.
    const float dt = std::clamp(deltaSeconds, 0.0f, 0.05f);
    game.playerX = std::clamp(
        game.playerX + (std::clamp(input.moveAxis, -1.0f, 1.0f) * game.rules.playerSpeed * dt),
        -1.0f,
        1.0f);

    if (input.actionPressed)
    {
        ++game.score;
    }
}
