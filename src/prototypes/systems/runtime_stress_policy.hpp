#pragma once

#include "../../core/runtime_types.hpp"

#include <algorithm>
#include <cstddef>

inline int ClampRuntimeStressLoadForPressure(
    int requestedLoadPercent,
    PressureState pressureState,
    bool hardStop)
{
    if (hardStop)
    {
        return 0;
    }

    int effectiveLoadPercent = requestedLoadPercent;
    switch (pressureState)
    {
    case PressureState::Normal:
        break;
    case PressureState::Elevated:
        effectiveLoadPercent = std::min(effectiveLoadPercent, 45);
        break;
    case PressureState::High:
        effectiveLoadPercent = std::min(effectiveLoadPercent, 20);
        break;
    case PressureState::Critical:
        effectiveLoadPercent = 0;
        break;
    default:
        break;
    }

    return std::max(0, effectiveLoadPercent);
}

inline std::size_t ComputeRuntimeStressMemoryBlockTarget(
    std::size_t maxMemoryBlocks,
    bool memoryStressEnabled,
    int effectiveMemoryLoadPercent)
{
    if (!memoryStressEnabled || effectiveMemoryLoadPercent <= 0)
    {
        return 0U;
    }

    return static_cast<std::size_t>(
        (static_cast<long long>(maxMemoryBlocks) * effectiveMemoryLoadPercent) / 100);
}
