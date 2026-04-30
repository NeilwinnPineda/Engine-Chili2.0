#include "native_ui_compiler.hpp"

#include "../prototypes/presentation/item.hpp"
#include "../prototypes/presentation/pass.hpp"
#include "../prototypes/presentation/view.hpp"

#include <cmath>
#include <sstream>
#include <utility>

namespace
{
    std::wstring ToWideString(const std::string& value)
    {
        return std::wstring(value.begin(), value.end());
    }

    ItemPrototype MakePatch(
        float centerX,
        float centerY,
        float halfWidth,
        float halfHeight,
        std::uint32_t color,
        ItemKind kind = ItemKind::ScreenPatch,
        float rotationRadians = 0.0f)
    {
        ItemPrototype item;
        item.kind = kind;
        item.screenPatch.centerX = centerX;
        item.screenPatch.centerY = centerY;
        item.screenPatch.halfWidth = halfWidth;
        item.screenPatch.halfHeight = halfHeight;
        item.screenPatch.rotationRadians = rotationRadians;
        item.screenPatch.color = color;
        return item;
    }

    void AppendLineShape(const NativeUiShape& shape, ViewPrototype& view)
    {
        const float dx = shape.end.x - shape.start.x;
        const float dy = shape.end.y - shape.start.y;
        const float length = std::sqrt((dx * dx) + (dy * dy));
        if (length <= 0.0001f)
        {
            return;
        }

        const float centerX = (shape.start.x + shape.end.x) * 0.5f;
        const float centerY = (shape.start.y + shape.end.y) * 0.5f;
        const float rotation = std::atan2(dy, dx);
        view.items.push_back(MakePatch(
            centerX,
            centerY,
            length * 0.5f,
            shape.thickness,
            shape.color,
            ItemKind::ScreenPatch,
            rotation));
    }

    void AppendShape(const NativeUiShape& shape, ViewPrototype& view)
    {
        switch (shape.kind)
        {
        case NativeUiShapeKind::Rect:
            view.items.push_back(MakePatch(
                shape.center.x,
                shape.center.y,
                shape.halfWidth,
                shape.halfHeight,
                shape.color,
                ItemKind::ScreenPatch,
                shape.rotationRadians));
            break;
        case NativeUiShapeKind::Hex:
            view.items.push_back(MakePatch(
                shape.center.x,
                shape.center.y,
                shape.halfWidth,
                shape.halfHeight,
                shape.color,
                ItemKind::ScreenHexPatch,
                shape.rotationRadians));
            break;
        case NativeUiShapeKind::Diamond:
            view.items.push_back(MakePatch(
                shape.center.x,
                shape.center.y,
                shape.halfWidth,
                shape.halfHeight,
                shape.color,
                ItemKind::ScreenPatch,
                0.78539816f + shape.rotationRadians));
            break;
        case NativeUiShapeKind::Line:
            AppendLineShape(shape, view);
            break;
        default:
            break;
        }
    }

    void AppendOverlayText(const NativeUiFrame& frame, std::wostringstream& overlay)
    {
        bool needsNewLine = false;
        const auto appendLine =
            [&overlay, &needsNewLine](const std::wstring& line)
            {
                if (needsNewLine)
                {
                    overlay << L"\n";
                }
                overlay << line;
                needsNewLine = true;
            };

        for (const std::wstring& line : frame.overlayLines)
        {
            appendLine(line);
        }

        for (const NativeUiTextBlock& block : frame.textBlocks)
        {
            if (block.includeInOverlay)
            {
                appendLine(block.content);
            }
        }

        for (const NativeUiPanel& panel : frame.panels)
        {
            if (!panel.visible)
            {
                continue;
            }

            appendLine(ToWideString(panel.title));
            for (const NativeUiStatusRow& row : panel.rows)
            {
                appendLine(L"  " + ToWideString(row.label) + L": " + ToWideString(row.value));
            }
        }
    }
}

NativeUiCompiledFrame NativeUiCompiler::Compile(const NativeUiFrame& frame)
{
    NativeUiCompiledFrame compiled;
    if (frame.hasContentFrame)
    {
        compiled.presentationFrame = frame.contentFrame;
        compiled.hasPresentationFrame = true;
    }

    if (!frame.shapes.empty())
    {
        if (!compiled.hasPresentationFrame)
        {
            compiled.presentationFrame = FramePrototype{};
            compiled.hasPresentationFrame = true;
        }

        PassPrototype pass;
        pass.kind = PassKind::Overlay;

        ViewPrototype view;
        view.kind = ViewKind::Overlay2D;
        view.items.reserve(frame.shapes.size());
        for (const NativeUiShape& shape : frame.shapes)
        {
            AppendShape(shape, view);
        }

        pass.views.push_back(std::move(view));
        compiled.presentationFrame.passes.push_back(std::move(pass));
    }

    std::wostringstream overlay;
    AppendOverlayText(frame, overlay);
    compiled.overlayText = overlay.str();
    return compiled;
}
