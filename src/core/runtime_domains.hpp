#pragma once

#include "presentation_snapshot_buffer.hpp"
#include "runtime_types.hpp"

#include <functional>

class RuntimeSupervisor
{
public:
    void Tick(
        std::uint64_t currentTimeUs,
        const EngineBudgetPolicy& budgetPolicy,
        const ResourceSnapshot& resourceSnapshot,
        double logicFrameMs,
        double presentationFrameMs,
        RuntimeControlState& controlState,
        LoopHeartbeat& managementHeartbeat,
        const LoopHeartbeat& logicHeartbeat,
        const LoopHeartbeat& presentationHeartbeat) const;
};

class LogicRuntime
{
public:
    using TickFunction = std::function<void()>;

    void Tick(
        std::uint64_t currentTimeUs,
        double frameDurationMs,
        RuntimeStats& runtimeStats,
        LoopHeartbeat& logicHeartbeat,
        TickFunction tickFunction) const;
};

class PresentationRuntime
{
public:
    using TickFunction = std::function<void(const PresentationSnapshot&)>;

    void Tick(
        std::uint64_t currentTimeUs,
        double frameDurationMs,
        const PresentationSnapshotBuffer& snapshotBuffer,
        RuntimeStats& runtimeStats,
        LoopHeartbeat& presentationHeartbeat,
        TickFunction tickFunction) const;
};
