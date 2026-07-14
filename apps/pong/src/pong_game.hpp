#pragma once

#include <cstdint>

struct PongRules
{
    float arenaHalfWidth = 1.0f;
    float arenaHalfHeight = 1.0f;
    float paddleHalfWidth = 0.035f;
    float paddleHalfHeight = 0.18f;
    float paddleInset = 0.085f;
    float paddleSpeed = 1.45f;
    float ballHalfSize = 0.030f;
    float ballInitialSpeed = 1.05f;
    float ballSpeedGain = 0.055f;
    float ballMaxSpeed = 2.10f;
    float aiResponsiveness = 0.82f;
    int winningScore = 7;
};

struct PongInput
{
    float leftPaddleAxis = 0.0f;
    float rightPaddleAxis = 0.0f;
    bool leftPaddleAiEnabled = false;
    bool rightPaddleAiEnabled = true;
    bool resetRequested = false;
    bool serveRequested = false;
};

struct PongPaddle
{
    float centerY = 0.0f;
};

struct PongBall
{
    float centerX = 0.0f;
    float centerY = 0.0f;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    bool waitingForServe = true;
};

struct PongScore
{
    int left = 0;
    int right = 0;
};

struct PongGame
{
    PongRules rules;
    PongPaddle leftPaddle;
    PongPaddle rightPaddle;
    PongBall ball;
    PongScore score;
    std::uint32_t rallyCount = 0U;
    int serveDirection = 1;
};

class PongGameSystem
{
public:
    static void Reset(PongGame& game);
    static void Step(PongGame& game, const PongInput& input, float deltaSeconds);

private:
    static void ResetBall(PongGame& game, int serveDirection);
};
