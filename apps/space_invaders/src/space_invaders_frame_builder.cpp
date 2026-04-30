#include "space_invaders_frame_builder.hpp"

#include "prototypes/presentation/item.hpp"
#include "prototypes/presentation/pass.hpp"
#include "prototypes/presentation/view.hpp"

#include <utility>

namespace
{
    constexpr std::uint32_t kBoundaryColor = 0x5549E4B8u;
    constexpr std::uint32_t kPlayerColor = 0xFF5DE3A1u;
    constexpr std::uint32_t kBulletColor = 0xFFFFF6A6u;
    constexpr std::uint32_t kAlienTopColor = 0xFFFF6B9Au;
    constexpr std::uint32_t kAlienMidColor = 0xFF7CE7FFu;
    constexpr std::uint32_t kAlienLowColor = 0xFFFFC857u;

    ItemPrototype MakePatch(
        float centerX,
        float centerY,
        float halfWidth,
        float halfHeight,
        std::uint32_t color,
        float rotationRadians = 0.0f)
    {
        ItemPrototype item;
        item.kind = ItemKind::ScreenPatch;
        item.screenPatch.centerX = centerX;
        item.screenPatch.centerY = centerY;
        item.screenPatch.halfWidth = halfWidth;
        item.screenPatch.halfHeight = halfHeight;
        item.screenPatch.rotationRadians = rotationRadians;
        item.screenPatch.color = color;
        return item;
    }

    ItemPrototype MakeHex(
        float centerX,
        float centerY,
        float halfWidth,
        float halfHeight,
        std::uint32_t color)
    {
        ItemPrototype item = MakePatch(centerX, centerY, halfWidth, halfHeight, color);
        item.kind = ItemKind::ScreenHexPatch;
        return item;
    }

    std::uint32_t AlienColorForRow(std::size_t row)
    {
        if (row <= 1U)
        {
            return kAlienTopColor;
        }
        if (row <= 3U)
        {
            return kAlienMidColor;
        }
        return kAlienLowColor;
    }
}

FramePrototype SpaceInvadersFrameBuilder::Build(const SpaceInvadersGame& game)
{
    FramePrototype frame;

    PassPrototype pass;
    pass.kind = PassKind::Overlay;

    ViewPrototype view;
    view.kind = ViewKind::Overlay2D;
    view.items.reserve(96U);

    const SpaceInvadersRules& rules = game.rules;
    view.items.push_back(MakePatch(0.0f, -0.93f, rules.arenaHalfWidth, 0.006f, kBoundaryColor));
    view.items.push_back(MakePatch(-rules.arenaHalfWidth, 0.0f, 0.006f, rules.arenaHalfHeight, kBoundaryColor));
    view.items.push_back(MakePatch(rules.arenaHalfWidth, 0.0f, 0.006f, rules.arenaHalfHeight, kBoundaryColor));

    view.items.push_back(MakePatch(
        game.playerX,
        rules.playerY,
        rules.playerHalfWidth,
        rules.playerHalfHeight,
        kPlayerColor));
    view.items.push_back(MakePatch(
        game.playerX,
        rules.playerY + rules.playerHalfHeight,
        rules.playerHalfWidth * 0.36f,
        rules.playerHalfHeight * 0.62f,
        kPlayerColor));

    for (const SpaceBullet& bullet : game.playerBullets)
    {
        if (!bullet.active)
        {
            continue;
        }

        view.items.push_back(MakePatch(
            bullet.x,
            bullet.y,
            rules.bulletHalfWidth,
            rules.bulletHalfHeight,
            kBulletColor));
    }

    for (std::size_t row = 0; row < SpaceInvadersRules::AlienRows; ++row)
    {
        for (std::size_t column = 0; column < SpaceInvadersRules::AlienColumns; ++column)
        {
            const SpaceInvader& alien = game.aliens[(row * SpaceInvadersRules::AlienColumns) + column];
            if (!alien.alive)
            {
                continue;
            }

            view.items.push_back(MakeHex(
                game.fleetOffsetX + alien.localX,
                alien.localY,
                rules.alienHalfWidth,
                rules.alienHalfHeight,
                AlienColorForRow(row)));
        }
    }

    pass.views.push_back(std::move(view));
    frame.passes.push_back(std::move(pass));
    return frame;
}
