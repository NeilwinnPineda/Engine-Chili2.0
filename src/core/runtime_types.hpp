#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

enum class PressureState
{
    Normal,
    Elevated,
    High,
    Critical
};

inline const wchar_t* ToWideString(PressureState state)
{
    switch (state)
    {
    case PressureState::Normal:
        return L"Normal";
    case PressureState::Elevated:
        return L"Elevated";
    case PressureState::High:
        return L"High";
    case PressureState::Critical:
        return L"Critical";
    default:
        return L"Unknown";
    }
}

struct RuntimeControlState
{
    std::atomic<bool> shutdownRequested{false};
    std::atomic<bool> degradedMode{false};
    std::atomic<bool> emergencyMode{false};
    std::atomic<PressureState> pressureState{PressureState::Normal};
    std::atomic<PressureState> logicPressureState{PressureState::Normal};
    std::atomic<PressureState> presentationPressureState{PressureState::Normal};
    std::atomic<int> logicThrottleLevel{0};
    std::atomic<int> presentationThrottleLevel{0};
};

struct LoopHeartbeat
{
    std::atomic<std::uint64_t> tickCount{0};
    std::atomic<std::uint64_t> lastProgressTimeUs{0};
    std::atomic<bool> alive{false};
    std::atomic<bool> stalled{false};

    void MarkProgress(std::uint64_t timestampUs)
    {
        tickCount.fetch_add(1, std::memory_order_relaxed);
        lastProgressTimeUs.store(timestampUs, std::memory_order_relaxed);
        alive.store(true, std::memory_order_relaxed);
        stalled.store(false, std::memory_order_relaxed);
    }
};

struct ResourceSnapshot
{
    std::uint64_t totalPhysicalMemoryBytes = 0;
    std::uint64_t availablePhysicalMemoryBytes = 0;
    std::uint64_t totalVirtualMemoryBytes = 0;
    std::uint64_t availableVirtualMemoryBytes = 0;
    unsigned int logicalProcessors = 0;
    unsigned int physicalCores = 0;
    std::uint64_t diskFreeBytes = 0;
    std::uint64_t diskTotalBytes = 0;
    double memoryPressureRatio = 0.0;
    double diskPressureRatio = 0.0;
};

struct RuntimeStats
{
    double logicFrameMs = 0.0;
    double presentationFrameMs = 0.0;
    double logicAverageMs = 0.0;
    double presentationAverageMs = 0.0;
    double logicMaxMs = 0.0;
    double presentationMaxMs = 0.0;
    std::uint64_t uptimeFrames = 0;
    double uptimeSeconds = 0.0;
    PressureState pressureState = PressureState::Normal;
    PressureState logicPressureState = PressureState::Normal;
    PressureState presentationPressureState = PressureState::Normal;
    std::uint64_t publishedSnapshotVersion = 0;
};

struct EngineBudgetPolicy
{
    double logicWarningMs = 25.0;
    double logicCriticalMs = 50.0;
    double presentationWarningMs = 20.0;
    double presentationCriticalMs = 40.0;
    double memoryWarningRatio = 0.80;
    double memoryCriticalRatio = 0.92;
    double heartbeatWarningSeconds = 0.50;
    double heartbeatCriticalSeconds = 2.00;
};

struct PresentationSnapshot
{
    std::uint64_t version = 0;
    double totalTimeSeconds = 0.0;
    double deltaTimeSeconds = 0.0;
    std::size_t submittedItemCount = 0;
    std::size_t queuedJobs = 0;
    std::size_t activeJobs = 0;
    PressureState pressureState = PressureState::Normal;
    bool degradedMode = false;
    bool emergencyMode = false;
    int logicThrottleLevel = 0;
    int presentationThrottleLevel = 0;
};
