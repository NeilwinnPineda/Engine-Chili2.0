#include "pong_frame_builder.hpp"

#include "prototypes/presentation/item.hpp"
#include "prototypes/presentation/pass.hpp"
#include "prototypes/presentation/view.hpp"

#include <cmath>
#include <utility>

namespace
{
    constexpr std::uint32_t kDimWhite = 0x668EA8A2u;
    constexpr std::uint32_t kLeftAccent = 0xFF56D6A3u;
    constexpr std::uint32_t kRightAccent = 0xFFFFC857u;
    constexpr std::uint32_t kBallColor = 0xFFF4F7F5u;
    constexpr std::uint32_t kServeColor = 0xFF7DE2D1u;

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
        std::uint32_t color,
        float rotationRadians = 0.0f)
    {
        ItemPrototype item = MakePatch(centerX, centerY, halfWidth, halfHeight, color, rotationRadians);
        item.kind = ItemKind::ScreenHexPatch;
        return item;
    }
}

FramePrototype PongFrameBuilder::Build(const PongGame& game)
{
    FramePrototype frame;

    PassPrototype pass;
    pass.kind = PassKind::Overlay;

    ViewPrototype view;
    view.kind = ViewKind::Overlay2D;
    view.items.reserve(64U);

    const PongRules& rules = game.rules;
    view.items.push_back(MakePatch(0.0f, rules.arenaHalfHeight, rules.arenaHalfWidth, 0.006f, kDimWhite));
    view.items.push_back(MakePatch(0.0f, -rules.arenaHalfHeight, rules.arenaHalfWidth, 0.006f, kDimWhite));

    for (int index = 0; index < 11; ++index)
    {
        const float y = -0.88f + (static_cast<float>(index) * 0.176f);
        view.items.push_back(MakePatch(0.0f, y, 0.007f, 0.050f, kDimWhite));
    }

    const float leftX = -rules.arenaHalfWidth + rules.paddleInset;
    const float rightX = rules.arenaHalfWidth - rules.paddleInset;
    view.items.push_back(MakePatch(
        leftX,
        game.leftPaddle.centerY,
        rules.paddleHalfWidth,
        rules.paddleHalfHeight,
        kLeftAccent));
    view.items.push_back(MakePatch(
        rightX,
        game.rightPaddle.centerY,
        rules.paddleHalfWidth,
        rules.paddleHalfHeight,
        kRightAccent));

    const std::uint32_t ballColor = game.ball.waitingForServe ? kServeColor : kBallColor;
    const float pulse = game.ball.waitingForServe
        ? 1.0f + (0.18f * std::sin(static_cast<float>(game.rallyCount) * 0.37f))
        : 1.0f;
    view.items.push_back(MakeHex(
        game.ball.centerX,
        game.ball.centerY,
        rules.ballHalfSize * pulse,
        rules.ballHalfSize * pulse,
        ballColor,
        0.52359877f));

    pass.views.push_back(std::move(view));
    frame.passes.push_back(std::move(pass));
    return frame;
}
