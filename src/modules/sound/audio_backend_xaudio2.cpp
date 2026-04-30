#include "audio_backend_xaudio2.hpp"

#include "../logger/logger_module.hpp"

#include <windows.h>
#include <xaudio2.h>

#include <algorithm>

XAudio2AudioBackend::XAudio2AudioBackend() = default;

XAudio2AudioBackend::~XAudio2AudioBackend()
{
    Shutdown();
}

bool XAudio2AudioBackend::Initialize(LoggerModule* logger)
{
    m_logger = logger;
    if (m_available)
    {
        return true;
    }

    HRESULT result = XAudio2Create(&m_engine, 0U, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(result) || m_engine == nullptr)
    {
        if (m_logger)
        {
            m_logger->Warn("XAudio2AudioBackend failed to create engine.");
        }

        return false;
    }

    result = m_engine->CreateMasteringVoice(&m_masteringVoice);
    if (FAILED(result) || m_masteringVoice == nullptr)
    {
        if (m_logger)
        {
            m_logger->Warn("XAudio2AudioBackend failed to create mastering voice.");
        }

        Shutdown();
        return false;
    }

    m_available = true;
    ApplyMasterVolume();

    if (m_logger)
    {
        m_logger->Info("XAudio2AudioBackend initialized.");
    }

    return true;
}

void XAudio2AudioBackend::Shutdown()
{
    StopAll();

    if (m_masteringVoice)
    {
        m_masteringVoice->DestroyVoice();
        m_masteringVoice = nullptr;
    }

    if (m_engine)
    {
        m_engine->Release();
        m_engine = nullptr;
    }

    m_available = false;
}

void XAudio2AudioBackend::Update()
{
    for (auto it = m_activeVoices.begin(); it != m_activeVoices.end();)
    {
        XAUDIO2_VOICE_STATE state{};
        it->second.voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        if (!it->second.looping && state.BuffersQueued == 0U)
        {
            DestroyVoice(it->second);
            it = m_activeVoices.erase(it);
            continue;
        }

        ++it;
    }
}

bool XAudio2AudioBackend::IsAvailable() const
{
    return m_available;
}

void XAudio2AudioBackend::SetMasterVolume(float volume)
{
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
    ApplyMasterVolume();
}

float XAudio2AudioBackend::GetMasterVolume() const
{
    return m_masterVolume;
}

void XAudio2AudioBackend::SetMuted(bool muted)
{
    m_muted = muted;
    ApplyMasterVolume();
}

bool XAudio2AudioBackend::IsMuted() const
{
    return m_muted;
}

bool XAudio2AudioBackend::PlayClip(
    SoundHandle handle,
    const std::string& soundId,
    const SoundClipDataPtr& clip,
    bool loop)
{
    if (!m_available || !m_engine || !clip || clip->pcmFrames.empty())
    {
        return false;
    }

    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = clip->channelCount;
    format.nSamplesPerSec = clip->sampleRate;
    format.wBitsPerSample = clip->bitsPerSample;
    format.nBlockAlign = static_cast<WORD>((format.nChannels * format.wBitsPerSample) / 8U);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    IXAudio2SourceVoice* sourceVoice = nullptr;
    HRESULT result = m_engine->CreateSourceVoice(&sourceVoice, &format);
    if (FAILED(result) || sourceVoice == nullptr)
    {
        return false;
    }

    XAUDIO2_BUFFER buffer{};
    buffer.AudioBytes = static_cast<UINT32>(clip->pcmFrames.size());
    buffer.pAudioData = reinterpret_cast<const BYTE*>(clip->pcmFrames.data());
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0U;

    result = sourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(result))
    {
        sourceVoice->DestroyVoice();
        return false;
    }

    result = sourceVoice->Start(0U);
    if (FAILED(result))
    {
        sourceVoice->DestroyVoice();
        return false;
    }

    ActiveVoice activeVoice;
    activeVoice.handle = handle;
    activeVoice.soundId = soundId;
    activeVoice.looping = loop;
    activeVoice.voice = sourceVoice;
    activeVoice.clip = clip;
    m_activeVoices.emplace(handle, std::move(activeVoice));
    return true;
}

void XAudio2AudioBackend::StopSound(SoundHandle handle)
{
    auto it = m_activeVoices.find(handle);
    if (it == m_activeVoices.end())
    {
        return;
    }

    DestroyVoice(it->second);
    m_activeVoices.erase(it);
}

void XAudio2AudioBackend::StopSoundsById(const std::string& soundId)
{
    for (auto it = m_activeVoices.begin(); it != m_activeVoices.end();)
    {
        if (it->second.soundId == soundId)
        {
            DestroyVoice(it->second);
            it = m_activeVoices.erase(it);
            continue;
        }

        ++it;
    }
}

void XAudio2AudioBackend::StopAll()
{
    for (auto& entry : m_activeVoices)
    {
        DestroyVoice(entry.second);
    }

    m_activeVoices.clear();
}

void XAudio2AudioBackend::ApplyMasterVolume()
{
    if (!m_masteringVoice)
    {
        return;
    }

    const float effectiveVolume = m_muted ? 0.0f : m_masterVolume;
    m_masteringVoice->SetVolume(effectiveVolume);
}

void XAudio2AudioBackend::DestroyVoice(ActiveVoice& activeVoice)
{
    if (!activeVoice.voice)
    {
        return;
    }

    activeVoice.voice->Stop(0U);
    activeVoice.voice->FlushSourceBuffers();
    activeVoice.voice->DestroyVoice();
    activeVoice.voice = nullptr;
    activeVoice.clip.reset();
}
