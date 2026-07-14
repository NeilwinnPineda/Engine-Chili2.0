#include "pong_game.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    float ClampAxis(float axis)
    {
        return std::clamp(axis, -1.0f, 1.0f);
    }

    float ClampPaddleY(const PongRules& rules, float centerY)
    {
        const float limit = rules.arenaHalfHeight - rules.paddleHalfHeight;
        return std::clamp(centerY, -limit, limit);
    }

    float ResolvePaddleX(const PongRules& rules, bool left)
    {
        const float x = rules.arenaHalfWidth - rules.paddleInset;
        return left ? -x : x;
    }

    float ResolveBallSpeed(const PongGame& game)
    {
        return std::min(
            game.rules.ballMaxSpeed,
            game.rules.ballInitialSpeed + (static_cast<float>(game.rallyCount) * game.rules.ballSpeedGain));
    }

    void NormalizeBallVelocity(PongGame& game)
    {
        const float speed = ResolveBallSpeed(game);
        const float length = std::sqrt(
            (game.ball.velocityX * game.ball.velocityX) +
            (game.ball.velocityY * game.ball.velocityY));
        if (length < 0.0001f)
        {
            game.ball.velocityX = static_cast<float>(game.serveDirection) * speed;
            game.ball.velocityY = 0.0f;
            return;
        }

        game.ball.velocityX = (game.ball.velocityX / length) * speed;
        game.ball.velocityY = (game.ball.velocityY / length) * speed;
    }

    bool IntersectsPaddle(
        const PongRules& rules,
        const PongBall& ball,
        const PongPaddle& paddle,
        bool leftPaddle)
    {
        const float paddleX = ResolvePaddleX(rules, leftPaddle);
        return std::abs(ball.centerX - paddleX) <= rules.ballHalfSize + rules.paddleHalfWidth &&
               std::abs(ball.centerY - paddle.centerY) <= rules.ballHalfSize + rules.paddleHalfHeight;
    }

    void ReflectFromPaddle(PongGame& game, const PongPaddle& paddle, bool leftPaddle)
    {
        const float side = leftPaddle ? 1.0f : -1.0f;
        const float impact = std::clamp(
            (game.ball.centerY - paddle.centerY) / game.rules.paddleHalfHeight,
            -1.0f,
            1.0f);

        game.ball.centerX =
            ResolvePaddleX(game.rules, leftPaddle) +
            (side * (game.rules.paddleHalfWidth + game.rules.ballHalfSize + 0.001f));
        game.ball.velocityX = side;
        game.ball.velocityY = impact * 0.78f;
        ++game.rallyCount;
        NormalizeBallVelocity(game);
    }

    float ComputePaddleAiAxis(const PongGame& game, const PongPaddle& paddle)
    {
        const float error = game.ball.centerY - paddle.centerY;
        const float deadZone = game.rules.ballHalfSize * 0.65f;
        return std::abs(error) > deadZone
            ? ClampAxis(error * game.rules.aiResponsiveness * 5.0f)
            : 0.0f;
    }
}

void PongGameSystem::Reset(PongGame& game)
{
    const PongRules rules = game.rules;
    game = PongGame{};
    game.rules = rules;
    game.serveDirection = 1;
    ResetBall(game, game.serveDirection);
}

void PongGameSystem::Step(PongGame& game, const PongInput& input, float deltaSeconds)
{
    if (input.resetRequested ||
        game.score.left >= game.rules.winningScore ||
        game.score.right >= game.rules.winningScore)
    {
        Reset(game);
    }

    const float dt = std::clamp(deltaSeconds, 0.0f, 0.05f);
    const float leftAxis = input.leftPaddleAiEnabled
        ? ComputePaddleAiAxis(game, game.leftPaddle)
        : ClampAxis(input.leftPaddleAxis);
    game.leftPaddle.centerY = ClampPaddleY(
        game.rules,
        game.leftPaddle.centerY + (leftAxis * game.rules.paddleSpeed * dt));

    float rightAxis = ClampAxis(input.rightPaddleAxis);
    if (input.rightPaddleAiEnabled)
    {
        rightAxis = ComputePaddleAiAxis(game, game.rightPaddle);
    }

    game.rightPaddle.centerY = ClampPaddleY(
        game.rules,
        game.rightPaddle.centerY + (rightAxis * game.rules.paddleSpeed * dt));

    if (game.ball.waitingForServe)
    {
        if (input.serveRequested)
        {
            game.ball.waitingForServe = false;
            NormalizeBallVelocity(game);
        }
        return;
    }

    game.ball.centerX += game.ball.velocityX * dt;
    game.ball.centerY += game.ball.velocityY * dt;

    const float topLimit = game.rules.arenaHalfHeight - game.rules.ballHalfSize;
    if (game.ball.centerY > topLimit)
    {
        game.ball.centerY = topLimit;
        game.ball.velocityY = -std::abs(game.ball.velocityY);
    }
    else if (game.ball.centerY < -topLimit)
    {
        game.ball.centerY = -topLimit;
        game.ball.velocityY = std::abs(game.ball.velocityY);
    }

    if (game.ball.velocityX < 0.0f && IntersectsPaddle(game.rules, game.ball, game.leftPaddle, true))
    {
        ReflectFromPaddle(game, game.leftPaddle, true);
    }
    else if (game.ball.velocityX > 0.0f && IntersectsPaddle(game.rules, game.ball, game.rightPaddle, false))
    {
        ReflectFromPaddle(game, game.rightPaddle, false);
    }

    if (game.ball.centerX < -game.rules.arenaHalfWidth - game.rules.ballHalfSize)
    {
        ++game.score.right;
        ResetBall(game, -1);
    }
    else if (game.ball.centerX > game.rules.arenaHalfWidth + game.rules.ballHalfSize)
    {
        ++game.score.left;
        ResetBall(game, 1);
    }
}

void PongGameSystem::ResetBall(PongGame& game, int serveDirection)
{
    game.rallyCount = 0U;
    game.serveDirection = serveDirection < 0 ? -1 : 1;
    game.ball.centerX = 0.0f;
    game.ball.centerY = 0.0f;
    game.ball.velocityX = static_cast<float>(game.serveDirection) * game.rules.ballInitialSpeed;
    game.ball.velocityY = 0.24f * static_cast<float>(game.serveDirection);
    game.ball.waitingForServe = true;
}
