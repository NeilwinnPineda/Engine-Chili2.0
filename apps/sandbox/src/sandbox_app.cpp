#include "sandbox_app.hpp"

#include "core/engine_core.hpp"
#include "prototypes/compiler/runtime_stress_frame_compiler.hpp"
#include "prototypes/systems/runtime_stress_policy.hpp"

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <sstream>

namespace
{
    volatile std::uint64_t g_runtimeStressSink = 0;
    constexpr const char* kSandboxTestSoundId = "engine.test_tone";
    constexpr const char* kSandboxTestSoundPath = "library/audio/engine_test_tone.wav";

}

bool SandboxApp::Run()
{
    EngineCore core;
    if (!core.Initialize())
    {
        return false;
    }

    AppCapabilities& capabilities = core.GetAppCapabilities();
    InitializeSandbox(capabilities);

    core.SetFrameCallback(
        [this](AppCapabilities& callbackCapabilities)
        {
            UpdateFrame(callbackCapabilities);
        });

    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void SandboxApp::InitializeSandbox(AppCapabilities& capabilities)
{
    m_state = StressState{};
    m_memoryBlocks.clear();
    capabilities.logging->Info("SandboxApp: runtime stability sandbox ready.");
    capabilities.window->SetWindowTitle(L"Engine Runtime Sandbox");
    if (capabilities.sound)
    {
        SoundClipDesc clipDesc;
        clipDesc.id = kSandboxTestSoundId;
        clipDesc.sourcePath = kSandboxTestSoundPath;
        m_state.soundAvailable = capabilities.sound->IsAvailable();
        m_state.soundRegistered = capabilities.sound->RegisterSound(clipDesc);
        m_state.masterVolume = capabilities.sound->GetMasterVolume();
        m_state.soundMuted = capabilities.sound->IsMuted();
        capabilities.logging->Info(
            std::string("SandboxApp: sound registration | available=") +
            (m_state.soundAvailable ? "true" : "false") +
            " | registered=" +
            (m_state.soundRegistered ? "true" : "false"));
    }
}

void SandboxApp::UpdateFrame(AppCapabilities& capabilities)
{
    UpdateStressPolicy(capabilities);
    HandleControls(capabilities);
    RunStressors(capabilities);

    if (capabilities.sound && m_state.soundRegistered && !m_state.startupSoundPlayed)
    {
        m_state.startupSoundPlayed = capabilities.sound->PlayOneShot(kSandboxTestSoundId);
        capabilities.logging->Info(
            m_state.startupSoundPlayed
                ? "[RUNTIME-SANDBOX] Startup one-shot sound queued."
                : "[RUNTIME-SANDBOX] Startup one-shot sound failed to queue.");
    }

    const std::uint32_t clearColor =
        m_state.observedPressureState == PressureState::Critical ? 0xFF351414u :
        m_state.observedPressureState == PressureState::High ? 0xFF352814u :
        m_state.observedPressureState == PressureState::Elevated ? 0xFF1F2D14u :
        0xFF101820u;

    const RuntimeStressPresentationResultPrototype presentationStress =
        RuntimeStressFrameCompiler::Compile(
            RuntimeStressPresentationOptionsPrototype{
                m_state.presentationStressEnabled,
                m_state.effectivePresentationLoadPercent });
    m_state.framePresentationPatches = presentationStress.patchCount;

    capabilities.rendering->ClearFrame(clearColor);
    capabilities.rendering->SubmitFrame(presentationStress.frame);
    capabilities.window->SetOverlayText(BuildOverlayText());
}

void SandboxApp::UpdateStressPolicy(AppCapabilities& capabilities)
{
    if (!capabilities.runtime)
    {
        return;
    }

    m_state.observedPressureState = capabilities.runtime->GetPressureState();
    m_state.observedLogicPressureState = capabilities.runtime->GetLogicPressureState();
    m_state.observedPresentationPressureState = capabilities.runtime->GetPresentationPressureState();
    m_state.observedDegradedMode = capabilities.runtime->IsDegradedMode();
    m_state.observedEmergencyMode = capabilities.runtime->IsEmergencyMode();
    m_state.observedLogicThrottleLevel = capabilities.runtime->GetLogicThrottleLevel();
    m_state.observedPresentationThrottleLevel = capabilities.runtime->GetPresentationThrottleLevel();
    m_state.observedSnapshotVersion = capabilities.runtime->GetPublishedPresentationSnapshotVersion();
    if (capabilities.sound)
    {
        m_state.soundAvailable = capabilities.sound->IsAvailable();
        m_state.masterVolume = capabilities.sound->GetMasterVolume();
        m_state.soundMuted = capabilities.sound->IsMuted();
    }

    const bool hardStop = m_state.paused || m_state.observedEmergencyMode;
    m_state.effectiveLogicLoadPercent =
        ClampRuntimeStressLoadForPressure(m_state.targetLoadPercent, m_state.observedLogicPressureState, hardStop);
    m_state.effectivePresentationLoadPercent =
        ClampRuntimeStressLoadForPressure(m_state.targetLoadPercent, m_state.observedPresentationPressureState, hardStop);
    m_state.effectiveMemoryLoadPercent =
        ClampRuntimeStressLoadForPressure(m_state.targetLoadPercent, m_state.observedPressureState, hardStop);
    m_state.memoryBlocksTarget = ComputeRuntimeStressMemoryBlockTarget(
        kMaxMemoryBlocks,
        m_state.memoryStressEnabled,
        m_state.effectiveMemoryLoadPercent);
}

void SandboxApp::HandleControls(AppCapabilities& capabilities)
{
    if (capabilities.window->WasKeyPressed('R'))
    {
        ResetStressState();
        capabilities.logging->Info("[RUNTIME-SANDBOX] Stress state reset.");
    }

    if (capabilities.window->WasKeyPressed(' '))
    {
        m_state.paused = !m_state.paused;
        capabilities.logging->Info(
            m_state.paused
                ? "[RUNTIME-SANDBOX] Stress paused."
                : "[RUNTIME-SANDBOX] Stress resumed.");
    }

    if (capabilities.window->WasKeyPressed('1'))
    {
        m_state.cpuStressEnabled = !m_state.cpuStressEnabled;
        capabilities.logging->Info(
            m_state.cpuStressEnabled
                ? "[RUNTIME-SANDBOX] CPU stress enabled."
                : "[RUNTIME-SANDBOX] CPU stress disabled.");
    }

    if (capabilities.window->WasKeyPressed('2'))
    {
        m_state.jobStressEnabled = !m_state.jobStressEnabled;
        capabilities.logging->Info(
            m_state.jobStressEnabled
                ? "[RUNTIME-SANDBOX] Job stress enabled."
                : "[RUNTIME-SANDBOX] Job stress disabled.");
    }

    if (capabilities.window->WasKeyPressed('3'))
    {
        m_state.memoryStressEnabled = !m_state.memoryStressEnabled;
        capabilities.logging->Info(
            m_state.memoryStressEnabled
                ? "[RUNTIME-SANDBOX] Memory stress enabled."
                : "[RUNTIME-SANDBOX] Memory stress disabled.");
    }

    if (capabilities.window->WasKeyPressed('4'))
    {
        m_state.presentationStressEnabled = !m_state.presentationStressEnabled;
        capabilities.logging->Info(
            m_state.presentationStressEnabled
                ? "[RUNTIME-SANDBOX] Presentation stress enabled."
                : "[RUNTIME-SANDBOX] Presentation stress disabled.");
    }

    if (capabilities.window->WasKeyPressed('5') && capabilities.sound && m_state.soundRegistered)
    {
        const bool played = capabilities.sound->PlayOneShot(kSandboxTestSoundId);
        capabilities.logging->Info(
            played
                ? "[RUNTIME-SANDBOX] Played one-shot test sound."
                : "[RUNTIME-SANDBOX] Failed to play one-shot test sound.");
    }

    if (capabilities.window->WasKeyPressed('6') && capabilities.sound && m_state.soundRegistered)
    {
        if (!m_state.loopSoundPlaying)
        {
            m_state.loopSoundPlaying = capabilities.sound->PlayLoop(kSandboxTestSoundId);
            capabilities.logging->Info(
                m_state.loopSoundPlaying
                    ? "[RUNTIME-SANDBOX] Started looped test sound."
                    : "[RUNTIME-SANDBOX] Failed to start looped test sound.");
        }
        else
        {
            capabilities.sound->StopSound(kSandboxTestSoundId);
            m_state.loopSoundPlaying = false;
            capabilities.logging->Info("[RUNTIME-SANDBOX] Stopped looped test sound.");
        }
    }

    if (capabilities.window->WasKeyPressed('7') && capabilities.sound)
    {
        m_state.masterVolume = std::max(0.0f, m_state.masterVolume - 0.1f);
        capabilities.sound->SetMasterVolume(m_state.masterVolume);
    }

    if (capabilities.window->WasKeyPressed('8') && capabilities.sound)
    {
        m_state.masterVolume = std::min(1.0f, m_state.masterVolume + 0.1f);
        capabilities.sound->SetMasterVolume(m_state.masterVolume);
    }

    if (capabilities.window->WasKeyPressed('9') && capabilities.sound)
    {
        m_state.soundMuted = !m_state.soundMuted;
        capabilities.sound->SetMuted(m_state.soundMuted);
    }

    if (capabilities.window->WasKeyPressed('0') && capabilities.sound)
    {
        capabilities.sound->StopAll();
        m_state.loopSoundPlaying = false;
        capabilities.logging->Info("[RUNTIME-SANDBOX] Stopped all sounds.");
    }

    if (capabilities.window->WasKeyPressed(VK_UP))
    {
        m_state.targetLoadPercent = std::min(100, m_state.targetLoadPercent + 10);
    }

    if (capabilities.window->WasKeyPressed(VK_DOWN))
    {
        m_state.targetLoadPercent = std::max(0, m_state.targetLoadPercent - 10);
    }
}

void SandboxApp::RunStressors(AppCapabilities& capabilities)
{
    m_state.frameStressJobs = 0;
    m_state.frameCpuIterations = 0;
    m_state.framePresentationPatches = 0U;

    if (m_state.effectiveLogicLoadPercent <= 0 &&
        m_state.effectivePresentationLoadPercent <= 0 &&
        m_state.effectiveMemoryLoadPercent <= 0)
    {
        RunMemoryStress();
        return;
    }

    if (m_state.cpuStressEnabled)
    {
        RunCpuStress();
    }

    if (m_state.jobStressEnabled)
    {
        RunJobStress(capabilities);
    }

    RunMemoryStress();
}

void SandboxApp::RunCpuStress()
{
    const double budgetMs = 0.10 * static_cast<double>(m_state.effectiveLogicLoadPercent);
    const auto startTime = std::chrono::steady_clock::now();
    std::uint64_t accumulator = 0;

    while (std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count() < budgetMs)
    {
        accumulator = (accumulator * 1664525ULL) + 1013904223ULL + static_cast<std::uint64_t>(m_state.frameCpuIterations);
        accumulator ^= accumulator >> 13U;
        ++m_state.frameCpuIterations;
    }

    g_runtimeStressSink ^= accumulator;
}

void SandboxApp::RunJobStress(AppCapabilities& capabilities)
{
    if (!capabilities.jobs)
    {
        return;
    }

    const unsigned int workerCount = capabilities.jobs->GetWorkerCount();
    const std::size_t jobCount = std::max<std::size_t>(
        1U,
        static_cast<std::size_t>((static_cast<unsigned long long>(std::max(1U, workerCount)) *
            static_cast<unsigned long long>(m_state.effectiveLogicLoadPercent)) / 100ULL));
    const double jobBudgetMs = 0.03 * static_cast<double>(m_state.effectiveLogicLoadPercent);

    for (std::size_t index = 0; index < jobCount; ++index)
    {
        const bool submitted = capabilities.jobs->Submit(
            [jobBudgetMs]()
            {
                const auto startTime = std::chrono::steady_clock::now();
                std::uint64_t accumulator = 0;
                std::size_t iterations = 0;
                while (std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count() < jobBudgetMs)
                {
                    accumulator = (accumulator * 1103515245ULL) + 12345ULL + iterations;
                    accumulator ^= accumulator << 7U;
                    ++iterations;
                }

                g_runtimeStressSink ^= accumulator;
            });

        if (submitted)
        {
            ++m_state.submittedStressJobs;
            ++m_state.frameStressJobs;
        }
    }
}

void SandboxApp::RunMemoryStress()
{
    while (m_memoryBlocks.size() < m_state.memoryBlocksTarget)
    {
        m_memoryBlocks.emplace_back(kMemoryBlockBytes, std::byte{ 0x3F });
    }

    while (m_memoryBlocks.size() > m_state.memoryBlocksTarget)
    {
        m_memoryBlocks.pop_back();
    }
}

void SandboxApp::ResetStressState()
{
    m_state = StressState{};
    m_memoryBlocks.clear();
}

std::wstring SandboxApp::BuildOverlayText() const
{
    std::wostringstream overlay;
    overlay << L"RUNTIME STABILITY SANDBOX\n";
    overlay << L"goal: adversarial but policy-compliant engine validation\n";
    overlay << L"pressure: " << PressureStateLabel(m_state.observedPressureState)
            << L"  degraded=" << EnabledLabel(m_state.observedDegradedMode)
            << L"  emergency=" << EnabledLabel(m_state.observedEmergencyMode) << L"\n";
    overlay << L"domain pressure: logic=" << PressureStateLabel(m_state.observedLogicPressureState)
            << L" presentation=" << PressureStateLabel(m_state.observedPresentationPressureState) << L"\n";
    overlay << L"throttle: logic=" << m_state.observedLogicThrottleLevel
            << L" presentation=" << m_state.observedPresentationThrottleLevel
            << L" snapshot=" << m_state.observedSnapshotVersion << L"\n";
    overlay << L"stress: paused=" << EnabledLabel(m_state.paused)
            << L"  target=" << m_state.targetLoadPercent << L"%\n";
    overlay << L"effective load: logic=" << m_state.effectiveLogicLoadPercent
            << L"% presentation=" << m_state.effectivePresentationLoadPercent
            << L"% memory=" << m_state.effectiveMemoryLoadPercent << L"%\n";
    overlay << L"cpu: " << EnabledLabel(m_state.cpuStressEnabled)
            << L"  frame-iterations=" << m_state.frameCpuIterations << L"\n";
    overlay << L"jobs: " << EnabledLabel(m_state.jobStressEnabled)
            << L"  frame-submitted=" << m_state.frameStressJobs
            << L"  total-submitted=" << m_state.submittedStressJobs << L"\n";
    overlay << L"memory: " << EnabledLabel(m_state.memoryStressEnabled)
            << L"  blocks=" << m_memoryBlocks.size()
            << L"/" << kMaxMemoryBlocks
            << L"  approx=" << (m_memoryBlocks.size() * kMemoryBlockBytes) / (1024U * 1024U) << L" MB\n";
    overlay << L"presentation: " << EnabledLabel(m_state.presentationStressEnabled)
            << L"  frame-patches=" << m_state.framePresentationPatches << L"\n";
    overlay << L"sound: registered=" << EnabledLabel(m_state.soundRegistered)
            << L" available=" << EnabledLabel(m_state.soundAvailable)
            << L" volume=" << m_state.masterVolume
            << L" muted=" << EnabledLabel(m_state.soundMuted)
            << L" loop=" << EnabledLabel(m_state.loopSoundPlaying) << L"\n";
    overlay << L"policy: sandbox always obeys runtime degradation and emergency rules\n";
    overlay << L"[1] cpu  [2] jobs  [3] memory  [4] presentation  [5] one-shot  [6] loop\n";
    overlay << L"[7/8] volume  [9] mute  [0] stop all  [up/down] load  [space] pause  [R] reset  [Esc] quit";
    return overlay.str();
}

const wchar_t* SandboxApp::PressureStateLabel(PressureState state)
{
    switch (state)
    {
    case PressureState::Normal:
        return L"normal";
    case PressureState::Elevated:
        return L"elevated";
    case PressureState::High:
        return L"high";
    case PressureState::Critical:
        return L"critical";
    default:
        return L"unknown";
    }
}

const wchar_t* SandboxApp::EnabledLabel(bool enabled)
{
    return enabled ? L"on" : L"off";
}
