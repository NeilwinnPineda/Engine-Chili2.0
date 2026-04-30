#pragma once

#include "app/app_capabilities.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class SandboxApp
{
public:
    bool Run();

private:
    struct StressState
    {
        bool paused = false;
        bool cpuStressEnabled = true;
        bool jobStressEnabled = false;
        bool memoryStressEnabled = false;
        bool presentationStressEnabled = false;
        bool soundRegistered = false;
        bool startupSoundPlayed = false;
        bool loopSoundPlaying = false;
        bool soundAvailable = false;
        int targetLoadPercent = 40;
        int effectiveLogicLoadPercent = 40;
        int effectivePresentationLoadPercent = 40;
        int effectiveMemoryLoadPercent = 40;
        PressureState observedPressureState = PressureState::Normal;
        PressureState observedLogicPressureState = PressureState::Normal;
        PressureState observedPresentationPressureState = PressureState::Normal;
        bool observedDegradedMode = false;
        bool observedEmergencyMode = false;
        int observedLogicThrottleLevel = 0;
        int observedPresentationThrottleLevel = 0;
        std::uint64_t observedSnapshotVersion = 0;
        std::size_t memoryBlocksTarget = 0;
        std::size_t submittedStressJobs = 0;
        std::size_t frameStressJobs = 0;
        std::size_t frameCpuIterations = 0;
        std::size_t framePresentationPatches = 0;
        float masterVolume = 1.0f;
        bool soundMuted = false;
    };

private:
    void InitializeSandbox(AppCapabilities& capabilities);
    void UpdateFrame(AppCapabilities& capabilities);
    void UpdateStressPolicy(AppCapabilities& capabilities);
    void HandleControls(AppCapabilities& capabilities);
    void RunStressors(AppCapabilities& capabilities);
    void RunCpuStress();
    void RunJobStress(AppCapabilities& capabilities);
    void RunMemoryStress();
    void ResetStressState();
    std::wstring BuildOverlayText() const;
    static const wchar_t* PressureStateLabel(PressureState state);
    static const wchar_t* EnabledLabel(bool enabled);

private:
    static constexpr std::size_t kMemoryBlockBytes = 1024U * 1024U;
    static constexpr std::size_t kMaxMemoryBlocks = 64U;

    StressState m_state;
    std::vector<std::vector<std::byte>> m_memoryBlocks;
};
