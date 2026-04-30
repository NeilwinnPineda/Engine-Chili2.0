#include "runtime_domains.hpp"

#include <algorithm>

namespace
{
    PressureState ClassifyDomainPressure(double frameMs, double warningMs, double criticalMs)
    {
        if (frameMs >= criticalMs)
        {
            return PressureState::Critical;
        }

        if (frameMs >= criticalMs * 0.75)
        {
            return PressureState::High;
        }

        if (frameMs >= warningMs)
        {
            return PressureState::Elevated;
        }

        return PressureState::Normal;
    }

    PressureState MaxPressureState(PressureState a, PressureState b)
    {
        return (static_cast<int>(a) >= static_cast<int>(b)) ? a : b;
    }

    int ThrottleLevelFromPressure(PressureState state)
    {
        switch (state)
        {
        case PressureState::Normal:
            return 0;
        case PressureState::Elevated:
            return 1;
        case PressureState::High:
            return 2;
        case PressureState::Critical:
            return 3;
        default:
            return 0;
        }
    }

    PressureState PressureFromHeartbeatAge(
        double heartbeatAgeSeconds,
        double warningSeconds,
        double criticalSeconds)
    {
        if (heartbeatAgeSeconds >= criticalSeconds)
        {
            return PressureState::Critical;
        }

        if (heartbeatAgeSeconds >= warningSeconds)
        {
            return PressureState::Elevated;
        }

        return PressureState::Normal;
    }

    void UpdateRollingAverage(double sampleMs, double& rollingAverageMs)
    {
        if (rollingAverageMs <= 0.0)
        {
            rollingAverageMs = sampleMs;
            return;
        }

        rollingAverageMs = (rollingAverageMs * 0.90) + (sampleMs * 0.10);
    }

    double CalculateHeartbeatAgeSeconds(std::uint64_t currentTimeUs, const LoopHeartbeat& heartbeat)
    {
        const std::uint64_t lastProgressUs = heartbeat.lastProgressTimeUs.load(std::memory_order_relaxed);
        if (currentTimeUs <= lastProgressUs)
        {
            return 0.0;
        }

        return static_cast<double>(currentTimeUs - lastProgressUs) / 1000000.0;
    }
}

void RuntimeSupervisor::Tick(
    std::uint64_t currentTimeUs,
    const EngineBudgetPolicy& budgetPolicy,
    const ResourceSnapshot& resourceSnapshot,
    double logicFrameMs,
    double presentationFrameMs,
    RuntimeControlState& controlState,
    LoopHeartbeat& managementHeartbeat,
    const LoopHeartbeat& logicHeartbeat,
    const LoopHeartbeat& presentationHeartbeat) const
{
    const double logicHeartbeatAgeSeconds = CalculateHeartbeatAgeSeconds(currentTimeUs, logicHeartbeat);
    const double presentationHeartbeatAgeSeconds = CalculateHeartbeatAgeSeconds(currentTimeUs, presentationHeartbeat);

    PressureState logicPressureState =
        ClassifyDomainPressure(logicFrameMs, budgetPolicy.logicWarningMs, budgetPolicy.logicCriticalMs);
    PressureState presentationPressureState =
        ClassifyDomainPressure(
            presentationFrameMs,
            budgetPolicy.presentationWarningMs,
            budgetPolicy.presentationCriticalMs);
    logicPressureState = MaxPressureState(
        logicPressureState,
        PressureFromHeartbeatAge(
            logicHeartbeatAgeSeconds,
            budgetPolicy.heartbeatWarningSeconds,
            budgetPolicy.heartbeatCriticalSeconds));
    presentationPressureState = MaxPressureState(
        presentationPressureState,
        PressureFromHeartbeatAge(
            presentationHeartbeatAgeSeconds,
            budgetPolicy.heartbeatWarningSeconds,
            budgetPolicy.heartbeatCriticalSeconds));

    PressureState infrastructurePressureState = PressureState::Normal;
    if (resourceSnapshot.memoryPressureRatio >= budgetPolicy.memoryWarningRatio)
    {
        infrastructurePressureState = PressureState::Elevated;
    }
    if (resourceSnapshot.memoryPressureRatio >= budgetPolicy.memoryCriticalRatio * 0.90)
    {
        infrastructurePressureState = PressureState::High;
    }
    if (resourceSnapshot.memoryPressureRatio >= budgetPolicy.memoryCriticalRatio)
    {
        infrastructurePressureState = PressureState::Critical;
    }

    logicPressureState = MaxPressureState(logicPressureState, infrastructurePressureState);
    presentationPressureState = MaxPressureState(presentationPressureState, infrastructurePressureState);
    const PressureState sharedPressureState =
        MaxPressureState(logicPressureState, presentationPressureState);

    const bool degradedMode =
        sharedPressureState == PressureState::High || sharedPressureState == PressureState::Critical;
    const bool emergencyMode = sharedPressureState == PressureState::Critical;

    controlState.pressureState.store(sharedPressureState, std::memory_order_relaxed);
    controlState.logicPressureState.store(logicPressureState, std::memory_order_relaxed);
    controlState.presentationPressureState.store(presentationPressureState, std::memory_order_relaxed);
    controlState.degradedMode.store(degradedMode, std::memory_order_relaxed);
    controlState.emergencyMode.store(emergencyMode, std::memory_order_relaxed);
    controlState.logicThrottleLevel.store(ThrottleLevelFromPressure(logicPressureState), std::memory_order_relaxed);
    controlState.presentationThrottleLevel.store(
        ThrottleLevelFromPressure(presentationPressureState),
        std::memory_order_relaxed);

    managementHeartbeat.MarkProgress(currentTimeUs);
}

void LogicRuntime::Tick(
    std::uint64_t currentTimeUs,
    double frameDurationMs,
    RuntimeStats& runtimeStats,
    LoopHeartbeat& logicHeartbeat,
    TickFunction tickFunction) const
{
    if (tickFunction)
    {
        tickFunction();
    }

    const double frameMs = std::max(0.0, frameDurationMs);
    runtimeStats.logicFrameMs = frameMs;
    UpdateRollingAverage(frameMs, runtimeStats.logicAverageMs);
    runtimeStats.logicMaxMs = std::max(runtimeStats.logicMaxMs, frameMs);
    logicHeartbeat.MarkProgress(currentTimeUs);
}

void PresentationRuntime::Tick(
    std::uint64_t currentTimeUs,
    double frameDurationMs,
    const PresentationSnapshotBuffer& snapshotBuffer,
    RuntimeStats& runtimeStats,
    LoopHeartbeat& presentationHeartbeat,
    TickFunction tickFunction) const
{
    const PresentationSnapshot snapshot = snapshotBuffer.ReadLatest();

    if (tickFunction)
    {
        tickFunction(snapshot);
    }

    const double frameMs = std::max(0.0, frameDurationMs);
    runtimeStats.presentationFrameMs = frameMs;
    UpdateRollingAverage(frameMs, runtimeStats.presentationAverageMs);
    runtimeStats.presentationMaxMs = std::max(runtimeStats.presentationMaxMs, frameMs);
    presentationHeartbeat.MarkProgress(currentTimeUs);
}
