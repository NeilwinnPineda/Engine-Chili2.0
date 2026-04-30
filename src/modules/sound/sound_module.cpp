#include "sound_module.hpp"

#include "../../core/engine_context.hpp"
#include "../file/ifile_service.hpp"
#include "../logger/logger_module.hpp"
#include "audio_backend_xaudio2.hpp"

#include <algorithm>
#include <cstring>

namespace
{
    template<typename T>
    bool ReadLittleEndianValue(
        const std::vector<std::byte>& bytes,
        std::size_t offset,
        T& outValue)
    {
        if (offset + sizeof(T) > bytes.size())
        {
            return false;
        }

        std::memcpy(&outValue, bytes.data() + offset, sizeof(T));
        return true;
    }
}

SoundModule::SoundModule() = default;

SoundModule::~SoundModule() = default;

const char* SoundModule::GetName() const
{
    return "Sound";
}

bool SoundModule::Initialize(EngineContext& context)
{
    if (m_initialized)
    {
        return true;
    }

    m_files = context.Files;
    m_backend = std::make_unique<XAudio2AudioBackend>();
    m_initialized = true;

    if (context.Logger)
    {
        context.Logger->Info("SoundModule initialized.");
    }

    return true;
}

void SoundModule::Startup(EngineContext& context)
{
    if (!m_initialized || m_started || !m_backend)
    {
        return;
    }

    const bool backendReady = m_backend->Initialize(context.Logger);
    m_started = true;

    if (context.Logger)
    {
        context.Logger->Info(
            std::string("SoundModule started | available = ") +
            (backendReady ? "true" : "false"));
    }
}

void SoundModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;

    if (!m_initialized || !m_started || !m_backend)
    {
        return;
    }

    ProcessQueuedCommands();
    m_backend->Update();
}

void SoundModule::Shutdown(EngineContext& context)
{
    if (!m_initialized)
    {
        return;
    }

    StopAll();
    ProcessQueuedCommands();

    if (m_backend)
    {
        m_backend->Shutdown();
        m_backend.reset();
    }

    {
        std::lock_guard<std::mutex> lock(m_commandMutex);
        m_pendingCommands.clear();
    }

    m_registeredSounds.clear();
    m_files = nullptr;
    m_started = false;
    m_initialized = false;

    if (context.Logger)
    {
        context.Logger->Info("SoundModule shutdown.");
    }
}

bool SoundModule::IsAvailable() const
{
    return m_backend && m_backend->IsAvailable();
}

bool SoundModule::RegisterSound(const SoundClipDesc& desc)
{
    if (desc.id.empty() || desc.sourcePath.empty())
    {
        return false;
    }

    SoundClipRecord& record = m_registeredSounds[desc.id];
    record.desc = desc;
    record.clip.reset();
    record.loadAttempted = false;
    record.lastError.clear();
    return true;
}

void SoundModule::SetMasterVolume(float volume)
{
    SoundCommand command;
    command.type = SoundCommandType::SetMasterVolume;
    command.volume = ClampVolume(volume);
    EnqueueCommand(std::move(command));
}

float SoundModule::GetMasterVolume() const
{
    return m_backend ? m_backend->GetMasterVolume() : 1.0f;
}

void SoundModule::SetMuted(bool muted)
{
    SoundCommand command;
    command.type = SoundCommandType::SetMuted;
    command.muted = muted;
    EnqueueCommand(std::move(command));
}

bool SoundModule::IsMuted() const
{
    return m_backend ? m_backend->IsMuted() : false;
}

void SoundModule::StopAll()
{
    SoundCommand command;
    command.type = SoundCommandType::StopAll;
    EnqueueCommand(std::move(command));
}

bool SoundModule::PlayOneShot(const std::string& soundId)
{
    return EnqueuePlayCommand(soundId, false);
}

bool SoundModule::PlayLoop(const std::string& soundId)
{
    return EnqueuePlayCommand(soundId, true);
}

void SoundModule::StopSound(const std::string& soundId)
{
    if (soundId.empty())
    {
        return;
    }

    SoundCommand command;
    command.type = SoundCommandType::StopSound;
    command.soundId = soundId;
    EnqueueCommand(std::move(command));
}

bool SoundModule::EnqueuePlayCommand(const std::string& soundId, bool loop)
{
    if (soundId.empty())
    {
        return false;
    }

    SoundCommand command;
    command.type = loop ? SoundCommandType::PlayLoop : SoundCommandType::PlayOneShot;
    command.soundId = soundId;
    EnqueueCommand(std::move(command));
    return true;
}

void SoundModule::EnqueueCommand(SoundCommand command)
{
    std::lock_guard<std::mutex> lock(m_commandMutex);
    m_pendingCommands.push_back(std::move(command));
}

void SoundModule::ProcessQueuedCommands()
{
    if (!m_backend)
    {
        return;
    }

    std::vector<SoundCommand> commands;
    {
        std::lock_guard<std::mutex> lock(m_commandMutex);
        commands.swap(m_pendingCommands);
    }

    for (const SoundCommand& command : commands)
    {
        switch (command.type)
        {
        case SoundCommandType::PlayOneShot:
        case SoundCommandType::PlayLoop:
        {
            if (!EnsureSoundLoaded(command.soundId))
            {
                continue;
            }

            const auto recordIt = m_registeredSounds.find(command.soundId);
            if (recordIt == m_registeredSounds.end() || !recordIt->second.clip)
            {
                continue;
            }

            static SoundHandle nextHandle = 1U;
            const SoundHandle handle = nextHandle++;
            m_backend->PlayClip(
                handle,
                command.soundId,
                recordIt->second.clip,
                command.type == SoundCommandType::PlayLoop);
            break;
        }

        case SoundCommandType::StopSound:
            m_backend->StopSoundsById(command.soundId);
            break;

        case SoundCommandType::StopAll:
            m_backend->StopAll();
            break;

        case SoundCommandType::SetMasterVolume:
            m_backend->SetMasterVolume(command.volume);
            break;

        case SoundCommandType::SetMuted:
            m_backend->SetMuted(command.muted);
            break;
        }
    }
}

bool SoundModule::EnsureSoundLoaded(const std::string& soundId)
{
    auto recordIt = m_registeredSounds.find(soundId);
    if (recordIt == m_registeredSounds.end())
    {
        return false;
    }

    SoundClipRecord& record = recordIt->second;
    if (record.clip)
    {
        return true;
    }

    if (record.loadAttempted)
    {
        return false;
    }

    return LoadSoundClip(record);
}

bool SoundModule::LoadSoundClip(SoundClipRecord& record)
{
    record.loadAttempted = true;
    record.lastError.clear();

    if (!m_files)
    {
        record.lastError = "File service unavailable.";
        return false;
    }

    std::vector<std::byte> bytes;
    if (!m_files->ReadBinaryFile(record.desc.sourcePath, bytes))
    {
        record.lastError = "Failed to read sound file.";
        return false;
    }

    auto clip = std::make_shared<SoundClipData>();
    if (!DecodeWaveBytes(bytes, *clip, record.lastError))
    {
        return false;
    }

    record.clip = std::move(clip);
    return true;
}

bool SoundModule::DecodeWaveBytes(
    const std::vector<std::byte>& bytes,
    SoundClipData& outClip,
    std::string& outError)
{
    if (bytes.size() < 44U)
    {
        outError = "Wave file too small.";
        return false;
    }

    const char* riff = reinterpret_cast<const char*>(bytes.data());
    if (std::memcmp(riff, "RIFF", 4) != 0 || std::memcmp(riff + 8, "WAVE", 4) != 0)
    {
        outError = "Unsupported wave header.";
        return false;
    }

    bool foundFmt = false;
    bool foundData = false;
    std::uint16_t audioFormat = 0;
    std::size_t dataOffset = 0;
    std::uint32_t dataSize = 0;

    std::size_t offset = 12U;
    while (offset + 8U <= bytes.size())
    {
        const char* chunkId = reinterpret_cast<const char*>(bytes.data() + offset);
        std::uint32_t chunkSize = 0;
        if (!ReadLittleEndianValue(bytes, offset + 4U, chunkSize))
        {
            outError = "Failed to read wave chunk size.";
            return false;
        }

        const std::size_t chunkDataOffset = offset + 8U;
        if (chunkDataOffset + chunkSize > bytes.size())
        {
            outError = "Corrupt wave chunk bounds.";
            return false;
        }

        if (std::memcmp(chunkId, "fmt ", 4) == 0)
        {
            if (chunkSize < 16U)
            {
                outError = "Wave fmt chunk too small.";
                return false;
            }

            if (!ReadLittleEndianValue(bytes, chunkDataOffset + 0U, audioFormat) ||
                !ReadLittleEndianValue(bytes, chunkDataOffset + 2U, outClip.channelCount) ||
                !ReadLittleEndianValue(bytes, chunkDataOffset + 4U, outClip.sampleRate) ||
                !ReadLittleEndianValue(bytes, chunkDataOffset + 14U, outClip.bitsPerSample))
            {
                outError = "Failed to read wave fmt chunk.";
                return false;
            }

            foundFmt = true;
        }
        else if (std::memcmp(chunkId, "data", 4) == 0)
        {
            dataOffset = chunkDataOffset;
            dataSize = chunkSize;
            foundData = true;
        }

        offset = chunkDataOffset + chunkSize;
        if ((chunkSize & 1U) != 0U)
        {
            ++offset;
        }
    }

    if (!foundFmt || !foundData)
    {
        outError = "Missing fmt or data chunk.";
        return false;
    }

    if (audioFormat != 1U)
    {
        outError = "Only PCM wave files are supported.";
        return false;
    }

    if (outClip.channelCount == 0U || outClip.sampleRate == 0U || outClip.bitsPerSample == 0U)
    {
        outError = "Invalid PCM wave format.";
        return false;
    }

    outClip.pcmFrames.resize(dataSize);
    std::memcpy(outClip.pcmFrames.data(), bytes.data() + dataOffset, dataSize);
    return true;
}

float SoundModule::ClampVolume(float volume)
{
    return std::clamp(volume, 0.0f, 1.0f);
}
