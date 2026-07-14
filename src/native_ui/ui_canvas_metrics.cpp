#include "ui_canvas_metrics.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int kDefaultCanvasWidth = 1280;
    constexpr int kDefaultCanvasHeight = 720;
    constexpr float kDefaultCanvasAspectRatio = 16.0f / 9.0f;

    int ClampDimension(int value, int fallback)
    {
        if (value <= 0)
        {
            return fallback;
        }

        return std::clamp(value, 1, 16384);
    }

    float ResolveAspectRatio(const NativeUiCanvasSettings& settings, int designWidth, int designHeight)
    {
        if (settings.aspectRatio > 0.0001f)
        {
            return settings.aspectRatio;
        }

        return static_cast<float>(designWidth) / static_cast<float>(designHeight);
    }
}

NativeUiCanvasMetrics ResolveNativeUiCanvasMetrics(
    const NativeUiCanvasSettings& settings,
    int windowWidth,
    int windowHeight)
{
    NativeUiCanvasMetrics metrics;
    metrics.designWidth = ClampDimension(settings.designWidth, kDefaultCanvasWidth);
    metrics.designHeight = ClampDimension(settings.designHeight, kDefaultCanvasHeight);

    const int resolvedWindowWidth = ClampDimension(windowWidth, metrics.designWidth);
    const int resolvedWindowHeight = ClampDimension(windowHeight, metrics.designHeight);

    if (!settings.lockAspectRatio)
    {
        metrics.fittedWidth = resolvedWindowWidth;
        metrics.fittedHeight = resolvedWindowHeight;
        metrics.fittedX = 0;
        metrics.fittedY = 0;
        metrics.scaleX = static_cast<float>(metrics.fittedWidth) / static_cast<float>(metrics.designWidth);
        metrics.scaleY = static_cast<float>(metrics.fittedHeight) / static_cast<float>(metrics.designHeight);
        return metrics;
    }

    const float aspectRatio = ResolveAspectRatio(settings, metrics.designWidth, metrics.designHeight);
    int fittedWidth = resolvedWindowWidth;
    int fittedHeight = static_cast<int>(std::lround(static_cast<float>(fittedWidth) / aspectRatio));
    if (fittedHeight > resolvedWindowHeight)
    {
        fittedHeight = resolvedWindowHeight;
        fittedWidth = static_cast<int>(std::lround(static_cast<float>(fittedHeight) * aspectRatio));
    }

    metrics.fittedWidth = std::max(1, fittedWidth);
    metrics.fittedHeight = std::max(1, fittedHeight);
    metrics.fittedX = (resolvedWindowWidth - metrics.fittedWidth) / 2;
    metrics.fittedY = (resolvedWindowHeight - metrics.fittedHeight) / 2;
    metrics.scaleX = static_cast<float>(metrics.fittedWidth) / static_cast<float>(metrics.designWidth);
    metrics.scaleY = static_cast<float>(metrics.fittedHeight) / static_cast<float>(metrics.designHeight);
    return metrics;
}

NativeUiCanvasRect ResolveAnchoredCanvasRect(
    const NativeUiCanvasMetrics& metrics,
    NativeUiAnchor anchor,
    int offsetX,
    int offsetY,
    int width,
    int height)
{
    NativeUiCanvasRect rect;
    rect.width = std::max(1, width);
    rect.height = std::max(1, height);

    switch (anchor)
    {
    case NativeUiAnchor::TopCenter:
        rect.x = (metrics.designWidth - rect.width) / 2 + offsetX;
        rect.y = offsetY;
        break;
    case NativeUiAnchor::TopRight:
        rect.x = metrics.designWidth - rect.width - offsetX;
        rect.y = offsetY;
        break;
    case NativeUiAnchor::Center:
        rect.x = (metrics.designWidth - rect.width) / 2 + offsetX;
        rect.y = (metrics.designHeight - rect.height) / 2 + offsetY;
        break;
    case NativeUiAnchor::BottomLeft:
        rect.x = offsetX;
        rect.y = metrics.designHeight - rect.height - offsetY;
        break;
    case NativeUiAnchor::BottomCenter:
        rect.x = (metrics.designWidth - rect.width) / 2 + offsetX;
        rect.y = metrics.designHeight - rect.height - offsetY;
        break;
    case NativeUiAnchor::BottomRight:
        rect.x = metrics.designWidth - rect.width - offsetX;
        rect.y = metrics.designHeight - rect.height - offsetY;
        break;
    case NativeUiAnchor::TopLeft:
    default:
        rect.x = offsetX;
        rect.y = offsetY;
        break;
    }

    return rect;
}

NativeUiPixelRect MapCanvasRectToPixels(
    const NativeUiCanvasMetrics& metrics,
    const NativeUiCanvasRect& rect)
{
    NativeUiPixelRect pixelRect;
    pixelRect.x = metrics.fittedX + static_cast<int>(std::lround(static_cast<float>(rect.x) * metrics.scaleX));
    pixelRect.y = metrics.fittedY + static_cast<int>(std::lround(static_cast<float>(rect.y) * metrics.scaleY));
    pixelRect.width = std::max(1, static_cast<int>(std::lround(static_cast<float>(rect.width) * metrics.scaleX)));
    pixelRect.height = std::max(1, static_cast<int>(std::lround(static_cast<float>(rect.height) * metrics.scaleY)));
    return pixelRect;
}
