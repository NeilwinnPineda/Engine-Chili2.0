#pragma once

#include "native_ui_style.hpp"

struct NativeUiCanvasSettings
{
    int designWidth = 1280;
    int designHeight = 720;
    float aspectRatio = 16.0f / 9.0f;
    bool lockAspectRatio = true;
};

struct NativeUiCanvasMetrics
{
    int designWidth = 1280;
    int designHeight = 720;
    int fittedX = 0;
    int fittedY = 0;
    int fittedWidth = 1280;
    int fittedHeight = 720;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

struct NativeUiCanvasRect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct NativeUiPixelRect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

NativeUiCanvasMetrics ResolveNativeUiCanvasMetrics(
    const NativeUiCanvasSettings& settings,
    int windowWidth,
    int windowHeight);

NativeUiCanvasRect ResolveAnchoredCanvasRect(
    const NativeUiCanvasMetrics& metrics,
    NativeUiAnchor anchor,
    int offsetX,
    int offsetY,
    int width,
    int height);

NativeUiPixelRect MapCanvasRectToPixels(
    const NativeUiCanvasMetrics& metrics,
    const NativeUiCanvasRect& rect);
