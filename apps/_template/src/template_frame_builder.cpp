#include "template_frame_builder.hpp"

#include "prototypes/presentation/item.hpp"
#include "prototypes/presentation/pass.hpp"
#include "prototypes/presentation/view.hpp"

#include <utility>

namespace
{
    ItemPrototype MakePatch(
        float centerX,
        float centerY,
        float halfWidth,
        float halfHeight,
        std::uint32_t color)
    {
        ItemPrototype item;
        item.kind = ItemKind::ScreenPatch;
        item.screenPatch.centerX = centerX;
        item.screenPatch.centerY = centerY;
        item.screenPatch.halfWidth = halfWidth;
        item.screenPatch.halfHeight = halfHeight;
        item.screenPatch.color = color;
        return item;
    }
}

FramePrototype TemplateFrameBuilder::Build(const TemplateGame& game)
{
    FramePrototype frame;

    // For simple 2D tests, use an Overlay pass/view and normalized screen-space
    // patches. This keeps game visuals in the render frame while native UI owns
    // text and panels.
    PassPrototype pass;
    pass.kind = PassKind::Overlay;

    ViewPrototype view;
    view.kind = ViewKind::Overlay2D;
    view.items.push_back(MakePatch(game.playerX, -0.55f, 0.07f, 0.05f, 0xFF56D6A3u));

    pass.views.push_back(std::move(view));
    frame.passes.push_back(std::move(pass));
    return frame;
}
