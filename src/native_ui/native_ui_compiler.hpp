#pragma once

#include "native_ui_frame.hpp"

#include <string>

struct NativeUiCompiledFrame
{
    bool hasPresentationFrame = false;
    FramePrototype presentationFrame;
    std::wstring overlayText;
};

class NativeUiCompiler
{
public:
    static NativeUiCompiledFrame Compile(const NativeUiFrame& frame);
};
