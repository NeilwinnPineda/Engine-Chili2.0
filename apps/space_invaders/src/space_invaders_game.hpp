#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

struct SpaceInvadersRules
{
    static constexpr std::size_t AlienColumns = 10U;
    static constexpr std::size_t AlienRows = 5U;

    float arenaHalfWidth = 1.0f;
    float arenaHalfHeight = 1.0f;
    float playerY = -0.84f;
    float playerHalfWidth = 0.060f;
    float playerHalfHeight = 0.035f;
    float playerSpeed = 1.35f;
    float bulletSpeed = 1.72f;
    float bulletHalfWidth = 0.010f;
    float bulletHalfHeight = 0.030f;
    float shotCooldownSeconds = 0.22f;
    float alienHalfWidth = 0.045f;
    float alienHalfHeight = 0.030f;
    float alienSpacingX = 0.145f;
    float alienSpacingY = 0.105f;
    float alienStartY = 0.62f;
    float alienSpeed = 0.18f;
    float alienDescendStep = 0.055f;
    int pointsPerAlien = 10;
    int startingLives = 3;
};

struct SpaceInvadersInput
{
    float moveAxis = 0.0f;
    bool firePressed = false;
    bool resetPressed = false;
};

struct SpaceInvader
{
    float localX = 0.0f;
    float localY = 0.0f;
    bool alive = true;
};

struct SpaceBullet
{
    float x = 0.0f;
    float y = 0.0f;
    bool active = false;
};

struct SpaceInvadersGame
{
    SpaceInvadersRules rules;
    float playerX = 0.0f;
    float fleetOffsetX = 0.0f;
    float fleetDirection = 1.0f;
    float shotCooldownRemaining = 0.0f;
    int score = 0;
    int lives = 3;
    bool won = false;
    bool gameOver = false;
    std::array<SpaceInvader, SpaceInvadersRules::AlienColumns * SpaceInvadersRules::AlienRows> aliens{};
    std::vector<SpaceBullet> playerBullets;
};

class SpaceInvadersGameSystem
{
public:
    static void Reset(SpaceInvadersGame& game);
    static void Step(SpaceInvadersGame& game, const SpaceInvadersInput& input, float deltaSeconds);

private:
    static void ResetAliens(SpaceInvadersGame& game);
};
