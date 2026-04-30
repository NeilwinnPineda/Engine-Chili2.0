#pragma once

#include "isound_service.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class LoggerModule;

struct SoundClipData
{
    std::uint16_t channelCount = 0;
    std::uint32_t sampleRate = 0;
    std::uint16_t bitsPerSample = 0;
    std::vector<std::byte> pcmFrames;
};

using SoundClipDataPtr = std::shared_ptr<SoundClipData>;

class IAudioBackend
{
public:
    virtual ~IAudioBackend() = default;

    virtual bool Initialize(LoggerModule* logger) = 0;
    virtual void Shutdown() = 0;
    virtual void Update() = 0;
    virtual bool IsAvailable() const = 0;
    virtual void SetMasterVolume(float volume) = 0;
    virtual float GetMasterVolume() const = 0;
    virtual void SetMuted(bool muted) = 0;
    virtual bool IsMuted() const = 0;
    virtual bool PlayClip(
        SoundHandle handle,
        const std::string& soundId,
        const SoundClipDataPtr& clip,
        bool loop) = 0;
    virtual void StopSound(SoundHandle handle) = 0;
    virtual void StopSoundsById(const std::string& soundId) = 0;
    virtual void StopAll() = 0;
};
