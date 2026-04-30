#include "space_invaders_game.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    float ClampAxis(float axis)
    {
        return std::clamp(axis, -1.0f, 1.0f);
    }

    float ResolveAlienWorldX(const SpaceInvadersGame& game, const SpaceInvader& alien)
    {
        return game.fleetOffsetX + alien.localX;
    }

    bool Intersects(
        float aX,
        float aY,
        float aHalfWidth,
        float aHalfHeight,
        float bX,
        float bY,
        float bHalfWidth,
        float bHalfHeight)
    {
        return std::abs(aX - bX) <= (aHalfWidth + bHalfWidth) &&
               std::abs(aY - bY) <= (aHalfHeight + bHalfHeight);
    }

    bool HasLivingAliens(const SpaceInvadersGame& game)
    {
        for (const SpaceInvader& alien : game.aliens)
        {
            if (alien.alive)
            {
                return true;
            }
        }

        return false;
    }

    void ResolveFleetBounds(const SpaceInvadersGame& game, float& outLeft, float& outRight, float& outBottom)
    {
        outLeft = game.rules.arenaHalfWidth;
        outRight = -game.rules.arenaHalfWidth;
        outBottom = game.rules.arenaHalfHeight;

        for (const SpaceInvader& alien : game.aliens)
        {
            if (!alien.alive)
            {
                continue;
            }

            const float x = ResolveAlienWorldX(game, alien);
            outLeft = std::min(outLeft, x - game.rules.alienHalfWidth);
            outRight = std::max(outRight, x + game.rules.alienHalfWidth);
            outBottom = std::min(outBottom, alien.localY - game.rules.alienHalfHeight);
        }
    }
}

void SpaceInvadersGameSystem::Reset(SpaceInvadersGame& game)
{
    const SpaceInvadersRules rules = game.rules;
    game = SpaceInvadersGame{};
    game.rules = rules;
    game.lives = rules.startingLives;
    game.playerBullets.reserve(8U);
    ResetAliens(game);
}

void SpaceInvadersGameSystem::Step(
    SpaceInvadersGame& game,
    const SpaceInvadersInput& input,
    float deltaSeconds)
{
    if (input.resetPressed)
    {
        Reset(game);
        return;
    }

    if (game.gameOver || game.won)
    {
        return;
    }

    const float dt = std::clamp(deltaSeconds, 0.0f, 0.05f);
    game.shotCooldownRemaining = std::max(0.0f, game.shotCooldownRemaining - dt);

    const float playerLimit = game.rules.arenaHalfWidth - game.rules.playerHalfWidth;
    game.playerX = std::clamp(
        game.playerX + (ClampAxis(input.moveAxis) * game.rules.playerSpeed * dt),
        -playerLimit,
        playerLimit);

    if (input.firePressed && game.shotCooldownRemaining <= 0.0f)
    {
        SpaceBullet bullet;
        bullet.x = game.playerX;
        bullet.y = game.rules.playerY + game.rules.playerHalfHeight + game.rules.bulletHalfHeight;
        bullet.active = true;
        game.playerBullets.push_back(bullet);
        game.shotCooldownRemaining = game.rules.shotCooldownSeconds;
    }

    for (SpaceBullet& bullet : game.playerBullets)
    {
        if (!bullet.active)
        {
            continue;
        }

        bullet.y += game.rules.bulletSpeed * dt;
        if (bullet.y > game.rules.arenaHalfHeight + game.rules.bulletHalfHeight)
        {
            bullet.active = false;
        }
    }

    const float fleetSpeed = game.rules.alienSpeed + (static_cast<float>(game.score) * 0.0008f);
    game.fleetOffsetX += game.fleetDirection * fleetSpeed * dt;

    float fleetLeft = 0.0f;
    float fleetRight = 0.0f;
    float fleetBottom = 0.0f;
    ResolveFleetBounds(game, fleetLeft, fleetRight, fleetBottom);
    if (fleetRight >= game.rules.arenaHalfWidth || fleetLeft <= -game.rules.arenaHalfWidth)
    {
        game.fleetOffsetX = std::clamp(
            game.fleetOffsetX,
            -game.rules.arenaHalfWidth + (fleetRight - game.fleetOffsetX),
            game.rules.arenaHalfWidth - (fleetLeft - game.fleetOffsetX));
        game.fleetDirection *= -1.0f;
        for (SpaceInvader& alien : game.aliens)
        {
            alien.localY -= game.rules.alienDescendStep;
        }
    }

    for (SpaceBullet& bullet : game.playerBullets)
    {
        if (!bullet.active)
        {
            continue;
        }

        for (SpaceInvader& alien : game.aliens)
        {
            if (!alien.alive)
            {
                continue;
            }

            if (Intersects(
                    bullet.x,
                    bullet.y,
                    game.rules.bulletHalfWidth,
                    game.rules.bulletHalfHeight,
                    ResolveAlienWorldX(game, alien),
                    alien.localY,
                    game.rules.alienHalfWidth,
                    game.rules.alienHalfHeight))
            {
                alien.alive = false;
                bullet.active = false;
                game.score += game.rules.pointsPerAlien;
                break;
            }
        }
    }

    game.playerBullets.erase(
        std::remove_if(
            game.playerBullets.begin(),
            game.playerBullets.end(),
            [](const SpaceBullet& bullet)
            {
                return !bullet.active;
            }),
        game.playerBullets.end());

    ResolveFleetBounds(game, fleetLeft, fleetRight, fleetBottom);
    if (!HasLivingAliens(game))
    {
        game.won = true;
    }
    else if (fleetBottom <= game.rules.playerY + game.rules.playerHalfHeight)
    {
        --game.lives;
        if (game.lives <= 0)
        {
            game.gameOver = true;
        }
        else
        {
            game.fleetOffsetX = 0.0f;
            game.fleetDirection = 1.0f;
            ResetAliens(game);
            game.playerBullets.clear();
        }
    }
}

void SpaceInvadersGameSystem::ResetAliens(SpaceInvadersGame& game)
{
    const float totalWidth =
        static_cast<float>(SpaceInvadersRules::AlienColumns - 1U) * game.rules.alienSpacingX;
    const float startX = -totalWidth * 0.5f;

    for (std::size_t row = 0; row < SpaceInvadersRules::AlienRows; ++row)
    {
        for (std::size_t column = 0; column < SpaceInvadersRules::AlienColumns; ++column)
        {
            SpaceInvader& alien = game.aliens[(row * SpaceInvadersRules::AlienColumns) + column];
            alien.localX = startX + (static_cast<float>(column) * game.rules.alienSpacingX);
            alien.localY = game.rules.alienStartY - (static_cast<float>(row) * game.rules.alienSpacingY);
            alien.alive = true;
        }
    }
}
