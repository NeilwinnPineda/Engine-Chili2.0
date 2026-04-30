#pragma once

#include "template_game.hpp"

#include "prototypes/presentation/frame.hpp"

// Converts app-local state into generic render presentation.
// Keep UI text/status out of this builder; use NativeUiBuilder for HUD/panels.
class TemplateFrameBuilder
{
public:
    static FramePrototype Build(const TemplateGame& game);
};
