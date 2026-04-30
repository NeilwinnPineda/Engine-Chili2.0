#pragma once

#include "audio_backend.hpp"

#include <memory>
#include <string>
#include <unordered_map>

struct IXAudio2;
struct IXAudio2MasteringVoice;
struct IXAudio2SourceVoice;

class XAudio2AudioBackend final : public IAudioBackend
{
public:
    XAudio2AudioBackend();
    ~XAudio2AudioBackend() override;

    bool Initialize(LoggerModule* logger) override;
    void Shutdown() override;
    void Update() override;
    bool IsAvailable() const override;
    void SetMasterVolume(float volume) override;
    float GetMasterVolume() const override;
    void SetMuted(bool muted) override;
    bool IsMuted() const override;
    bool PlayClip(
        SoundHandle handle,
        const std::string& soundId,
        const SoundClipDataPtr& clip,
        bool loop) override;
    void StopSound(SoundHandle handle) override;
    void StopSoundsById(const std::string& soundId) override;
    void StopAll() override;

private:
    struct ActiveVoice
    {
        SoundHandle handle = 0;
        std::string soundId;
        bool looping = false;
        IXAudio2SourceVoice* voice = nullptr;
        SoundClipDataPtr clip;
    };

private:
    void ApplyMasterVolume();
    void DestroyVoice(ActiveVoice& activeVoice);

private:
    LoggerModule* m_logger = nullptr;
    IXAudio2* m_engine = nullptr;
    IXAudio2MasteringVoice* m_masteringVoice = nullptr;
    bool m_available = false;
    float m_masterVolume = 1.0f;
    bool m_muted = false;
    std::unordered_map<SoundHandle, ActiveVoice> m_activeVoices;
};
