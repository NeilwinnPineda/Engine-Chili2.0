#pragma once

#include "native_ui_style.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct NativeUiPoint
{
    float x = 0.0f;
    float y = 0.0f;
};

struct NativeUiTextBlock
{
    std::wstring content;
    NativeUiPoint position{};
    NativeUiAnchor anchor = NativeUiAnchor::TopLeft;
    NativeUiStyle style{};
    bool includeInOverlay = true;
};

struct NativeUiStatusRow
{
    std::string label;
    std::string value;
};

struct NativeUiPanel
{
    std::string title;
    bool visible = true;
    NativeUiAnchor anchor = NativeUiAnchor::TopLeft;
    int offsetX = 16;
    int offsetY = 16;
    int width = 280;
    int height = 128;
    std::uint32_t textColor = 0xFFE8F3F1u;
    std::uint32_t backgroundColor = 0xCC07100Fu;
    std::vector<NativeUiStatusRow> rows;
};

enum class NativeUiShapeKind : unsigned char
{
    Rect,
    Line,
    Hex,
    Diamond
};

struct NativeUiShape
{
    NativeUiShapeKind kind = NativeUiShapeKind::Rect;
    NativeUiPoint start{};
    NativeUiPoint end{};
    NativeUiPoint center{};
    float halfWidth = 0.1f;
    float halfHeight = 0.1f;
    float thickness = 0.01f;
    float rotationRadians = 0.0f;
    std::uint32_t color = 0xFFFFFFFFu;
};
