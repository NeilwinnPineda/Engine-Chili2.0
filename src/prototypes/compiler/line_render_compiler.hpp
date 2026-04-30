#pragma once

#include "../../modules/render/render_frame_data.hpp"
#include "../entity/geometry/line.hpp"

#include <cstdint>
#include <vector>

class LineRenderCompiler
{
public:
    static void Append(
        const LinePrototype& line,
        std::uint32_t color,
        float thickness,
        float fallbackLength,
        std::vector<RenderItemData>& outItems);
};
