#pragma once

#include <cstdint>

enum class NativeUiTextAlign : unsigned char
{
    Left,
    Center,
    Right
};

enum class NativeUiAnchor : unsigned char
{
    TopLeft,
    TopCenter,
    TopRight,
    Center,
    BottomLeft,
    BottomCenter,
    BottomRight
};

struct NativeUiStyle
{
    std::uint32_t color = 0xFFFFFFFFu;
    float scale = 1.0f;
    NativeUiTextAlign align = NativeUiTextAlign::Left;
};
