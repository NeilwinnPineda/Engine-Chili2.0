#pragma once

#include "ui_canvas_metrics.hpp"
#include "native_ui_element.hpp"
#include "../prototypes/presentation/frame.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct NativeUiLabel
{
    std::string name;
    std::wstring text;
    int x = 0;
    int y = 0;
    int width = 96;
    int height = 24;
    NativeUiAnchor anchor = NativeUiAnchor::TopLeft;
    int offsetX = 0;
    int offsetY = 0;
    bool useAnchor = false;
    bool visible = true;
    std::uint32_t textColor = 0xFFFFFFFFu;
    std::uint32_t backgroundColor = 0x00000000u;
    bool transparentBackground = true;
    bool multiline = false;
    bool alignLeft = false;
};

struct NativeUiFrame
{
    bool hasWindowTitle = false;
    std::wstring windowTitle;

    bool hasCanvasSettings = false;
    NativeUiCanvasSettings canvasSettings{};

    bool overlayEnabled = true;
    bool hasClearColor = false;
    std::uint32_t clearColor = 0xFF000000u;

    bool hasContentFrame = false;
    FramePrototype contentFrame;

    std::vector<std::wstring> overlayLines;
    std::vector<NativeUiTextBlock> textBlocks;
    std::vector<NativeUiPanel> panels;
    std::vector<NativeUiShape> shapes;
    std::vector<NativeUiForm> forms;
    std::vector<NativeUiButton> nativeButtons;
    std::vector<NativeUiLabel> nativeLabels;
};
