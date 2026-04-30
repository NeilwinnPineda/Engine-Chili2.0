#pragma once

#include "../presentation/frame.hpp"

#include <cstddef>

struct RuntimeStressPresentationOptionsPrototype
{
    bool enabled = false;
    int loadPercent = 0;
};

struct RuntimeStressPresentationResultPrototype
{
    FramePrototype frame;
    std::size_t patchCount = 0U;
};

class RuntimeStressFrameCompiler
{
public:
    static RuntimeStressPresentationResultPrototype Compile(
        const RuntimeStressPresentationOptionsPrototype& options);
};
