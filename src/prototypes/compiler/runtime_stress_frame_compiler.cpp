#include "runtime_stress_frame_compiler.hpp"

#include "../presentation/item.hpp"
#include "../presentation/pass.hpp"
#include "../presentation/view.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>

namespace
{
std::size_t ComputePatchCount(int loadPercent)
{
    if (loadPercent <= 0)
    {
        return 0U;
    }

    return std::max<std::size_t>(16U, static_cast<std::size_t>(loadPercent) * 12U);
}
}

RuntimeStressPresentationResultPrototype RuntimeStressFrameCompiler::Compile(
    const RuntimeStressPresentationOptionsPrototype& options)
{
    RuntimeStressPresentationResultPrototype result;

    if (!options.enabled || options.loadPercent <= 0)
    {
        return result;
    }

    result.patchCount = ComputePatchCount(options.loadPercent);

    PassPrototype pass;
    pass.kind = PassKind::Overlay;

    ViewPrototype view;
    view.kind = ViewKind::Overlay2D;
    view.items.reserve(result.patchCount);

    const std::size_t columns = std::max<std::size_t>(4U, result.patchCount / 8U);
    const std::size_t rows = (result.patchCount + columns - 1U) / columns;
    const float halfSize =
        std::max(0.004f, 0.025f - (static_cast<float>(options.loadPercent) * 0.00015f));

    for (std::size_t index = 0; index < result.patchCount; ++index)
    {
        const std::size_t column = index % columns;
        const std::size_t row = index / columns;

        const float x = -0.92f + (static_cast<float>(column) / std::max(1.0f, static_cast<float>(columns - 1U))) * 1.84f;
        const float y = -0.88f + (static_cast<float>(row) / std::max(1.0f, static_cast<float>(rows - 1U))) * 1.76f;

        ItemPrototype item;
        item.kind = (index % 3U == 0U) ? ItemKind::ScreenHexPatch : ItemKind::ScreenPatch;
        item.screenPatch.centerX = x;
        item.screenPatch.centerY = y;
        item.screenPatch.halfWidth = halfSize;
        item.screenPatch.halfHeight = halfSize;
        item.screenPatch.rotationRadians = static_cast<float>(index % 16U) * 0.19634954f;

        const std::uint32_t red = static_cast<std::uint32_t>((37U * static_cast<unsigned int>(index)) & 0xFFU);
        const std::uint32_t green = static_cast<std::uint32_t>((83U * static_cast<unsigned int>(index + 3U)) & 0xFFU);
        const std::uint32_t blue = static_cast<std::uint32_t>((151U * static_cast<unsigned int>(index + 7U)) & 0xFFU);
        item.screenPatch.color = 0xFF000000u | (red << 16U) | (green << 8U) | blue;
        view.items.push_back(item);
    }

    pass.views.push_back(std::move(view));
    result.frame.passes.push_back(std::move(pass));
    return result;
}
