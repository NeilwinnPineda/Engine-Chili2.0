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

struct NativeUiButton
{
    std::string name;
    std::wstring text;
    bool visible = true;
    bool enabled = true;
    NativeUiAnchor anchor = NativeUiAnchor::TopLeft;
    int x = 0;
    int y = 0;
    int width = 120;
    int height = 36;
    int offsetX = 0;
    int offsetY = 0;
    bool useAnchor = false;
};

struct NativeUiForm
{
    std::string name;
    std::wstring title;
    std::wstring body;
    bool visible = true;
    NativeUiAnchor anchor = NativeUiAnchor::TopLeft;
    int x = 0;
    int y = 0;
    int width = 320;
    int height = 200;
    int offsetX = 0;
    int offsetY = 0;
    bool useAnchor = false;
    int headerHeight = 36;
    int padding = 14;
    std::uint32_t titleTextColor = 0xFFF7F0E8u;
    std::uint32_t bodyTextColor = 0xFFE6DDD2u;
    std::uint32_t backgroundColor = 0xD9181412u;
    std::uint32_t headerColor = 0xEE2B221Eu;
    std::uint32_t borderColor = 0xFF8A6D57u;
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
