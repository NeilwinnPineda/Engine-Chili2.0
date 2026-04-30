#pragma once

#include "../../core/module.hpp"
#include "audio_backend.hpp"
#include "isound_service.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class EngineContext;
class IFileService;

class SoundModule final : public IModule, public ISoundService
{
public:
    SoundModule();
    ~SoundModule() override;

    const char* GetName() const override;
    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    bool IsAvailable() const override;
    bool RegisterSound(const SoundClipDesc& desc) override;
    void SetMasterVolume(float volume) override;
    float GetMasterVolume() const override;
    void SetMuted(bool muted) override;
    bool IsMuted() const override;
    void StopAll() override;
    bool PlayOneShot(const std::string& soundId) override;
    bool PlayLoop(const std::string& soundId) override;
    void StopSound(const std::string& soundId) override;

private:
    enum class SoundCommandType : unsigned char
    {
        PlayOneShot,
        PlayLoop,
        StopSound,
        StopAll,
        SetMasterVolume,
        SetMuted
    };

    struct SoundCommand
    {
        SoundCommandType type = SoundCommandType::PlayOneShot;
        std::string soundId;
        float volume = 1.0f;
        bool muted = false;
    };

    struct SoundClipRecord
    {
        SoundClipDesc desc;
        SoundClipDataPtr clip;
        bool loadAttempted = false;
        std::string lastError;
    };

private:
    bool EnqueuePlayCommand(const std::string& soundId, bool loop);
    void EnqueueCommand(SoundCommand command);
    void ProcessQueuedCommands();
    bool EnsureSoundLoaded(const std::string& soundId);
    bool LoadSoundClip(SoundClipRecord& record);
    static bool DecodeWaveBytes(
        const std::vector<std::byte>& bytes,
        SoundClipData& outClip,
        std::string& outError);
    static float ClampVolume(float volume);

private:
    bool m_initialized = false;
    bool m_started = false;
    IFileService* m_files = nullptr;
    std::unique_ptr<IAudioBackend> m_backend;
    std::unordered_map<std::string, SoundClipRecord> m_registeredSounds;
    std::mutex m_commandMutex;
    std::vector<SoundCommand> m_pendingCommands;
};
