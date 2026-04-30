#pragma once

// App-local rules/state/input live here. Keep this folder specific to the app.
// Only move something into src/prototypes when it is a reusable engine contract,
// not just useful for one game.

struct TemplateRules
{
    float playerSpeed = 1.0f;
};

struct TemplateInput
{
    float moveAxis = 0.0f;
    bool actionPressed = false;
    bool resetPressed = false;
};

struct TemplateGame
{
    TemplateRules rules;
    float playerX = 0.0f;
    int score = 0;
};

class TemplateGameSystem
{
public:
    static void Reset(TemplateGame& game);
    static void Step(TemplateGame& game, const TemplateInput& input, float deltaSeconds);
};
