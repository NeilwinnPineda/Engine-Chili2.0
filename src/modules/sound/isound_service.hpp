#pragma once

#include <cstdint>
#include <string>

using SoundHandle = std::uint64_t;

struct SoundClipDesc
{
    std::string id;
    std::string sourcePath;
    bool stream = false;
};

class ISoundService
{
public:
    virtual ~ISoundService() = default;

    virtual bool IsAvailable() const = 0;
    virtual bool RegisterSound(const SoundClipDesc& desc) = 0;
    virtual void SetMasterVolume(float volume) = 0;
    virtual float GetMasterVolume() const = 0;
    virtual void SetMuted(bool muted) = 0;
    virtual bool IsMuted() const = 0;
    virtual void StopAll() = 0;
    virtual bool PlayOneShot(const std::string& soundId) = 0;
    virtual bool PlayLoop(const std::string& soundId) = 0;
    virtual void StopSound(const std::string& soundId) = 0;
};
