#include "native_ui_module.hpp"

#include "../platform/iplatform_service.hpp"

namespace
{
    bool SameRect(const NativeControlRect& a, const NativeControlRect& b)
    {
        return a.x == b.x &&
               a.y == b.y &&
               a.width == b.width &&
               a.height == b.height;
    }
}

const char* NativeUiModule::GetName() const
{
    return "NativeUiModule";
}

bool NativeUiModule::Initialize(EngineContext& context)
{
    if (!m_platform)
    {
        m_platform = context.Platform;
    }

    m_initialized = true;
    return true;
}

void NativeUiModule::Startup(EngineContext& context)
{
    if (!m_platform)
    {
        m_platform = context.Platform;
    }
}

void NativeUiModule::Update(EngineContext& context, float deltaTime)
{
    (void)deltaTime;

    if (!m_platform)
    {
        m_platform = context.Platform;
    }
}

void NativeUiModule::Shutdown(EngineContext& context)
{
    (void)context;
    DestroyAllLabels();
    DestroyAllButtons();
    m_initialized = false;
}

NativeUiModule::ButtonHandle NativeUiModule::CreateButton(const NativeButtonDesc& desc)
{
    if (!m_initialized || !m_platform)
    {
        return 0U;
    }

    const HWND engineWindowHandle = GetEngineWindowHandle();
    if (!engineWindowHandle)
    {
        return 0U;
    }

    const ButtonHandle handle = m_nextHandle++;
    auto [iterator, inserted] = m_buttons.try_emplace(handle);
    if (!inserted)
    {
        return 0U;
    }

    iterator->second.desc = desc;
    if (!iterator->second.host.Initialize(engineWindowHandle, desc))
    {
        m_buttons.erase(iterator);
        return 0U;
    }

    return handle;
}

bool NativeUiModule::DestroyButton(ButtonHandle handle)
{
    ButtonEntry* button = FindButton(handle);
    if (!button)
    {
        return false;
    }

    button->host.Shutdown();
    m_buttons.erase(handle);
    return true;
}

void NativeUiModule::DestroyAllButtons()
{
    for (auto& [handle, button] : m_buttons)
    {
        (void)handle;
        button.host.Shutdown();
    }

    m_buttons.clear();
}

bool NativeUiModule::SetButtonBounds(ButtonHandle handle, const NativeControlRect& rect)
{
    ButtonEntry* button = FindButton(handle);
    if (!button)
    {
        return false;
    }

    button->desc.rect = rect;
    return button->host.SetBounds(rect);
}

bool NativeUiModule::SetButtonText(ButtonHandle handle, const std::wstring& text)
{
    ButtonEntry* button = FindButton(handle);
    if (!button)
    {
        return false;
    }

    button->desc.text = text;
    return button->host.SetText(text);
}

bool NativeUiModule::SetButtonVisible(ButtonHandle handle, bool visible)
{
    ButtonEntry* button = FindButton(handle);
    if (!button)
    {
        return false;
    }

    button->desc.visible = visible;
    return button->host.SetVisible(visible);
}

bool NativeUiModule::SetButtonEnabled(ButtonHandle handle, bool enabled)
{
    ButtonEntry* button = FindButton(handle);
    if (!button)
    {
        return false;
    }

    button->desc.enabled = enabled;
    return button->host.SetEnabled(enabled);
}

bool NativeUiModule::ConsumeButtonPressed(ButtonHandle handle)
{
    ButtonEntry* button = FindButton(handle);
    return button ? button->host.ConsumePressed() : false;
}

bool NativeUiModule::IsButtonOpen(ButtonHandle handle) const
{
    const ButtonEntry* button = FindButton(handle);
    return button ? button->host.IsOpen() : false;
}

NativeUiModule::LabelHandle NativeUiModule::CreateLabel(const NativeLabelDesc& desc)
{
    if (!m_initialized || !m_platform)
    {
        return 0U;
    }

    const HWND engineWindowHandle = GetEngineWindowHandle();
    if (!engineWindowHandle)
    {
        return 0U;
    }

    const LabelHandle handle = m_nextLabelHandle++;
    auto [iterator, inserted] = m_labels.try_emplace(handle);
    if (!inserted)
    {
        return 0U;
    }

    iterator->second.desc = desc;
    if (!iterator->second.host.Initialize(engineWindowHandle, desc))
    {
        m_labels.erase(iterator);
        return 0U;
    }

    return handle;
}

bool NativeUiModule::DestroyLabel(LabelHandle handle)
{
    LabelEntry* label = FindLabel(handle);
    if (!label)
    {
        return false;
    }

    label->host.Shutdown();
    m_labels.erase(handle);
    return true;
}

void NativeUiModule::DestroyAllLabels()
{
    for (auto& [handle, label] : m_labels)
    {
        (void)handle;
        label.host.Shutdown();
    }

    m_labels.clear();
}

bool NativeUiModule::SetLabelBounds(LabelHandle handle, const NativeControlRect& rect)
{
    LabelEntry* label = FindLabel(handle);
    if (!label)
    {
        return false;
    }

    label->desc.rect = rect;
    return label->host.SetBounds(rect);
}

bool NativeUiModule::SetLabelText(LabelHandle handle, const std::wstring& text)
{
    LabelEntry* label = FindLabel(handle);
    if (!label)
    {
        return false;
    }

    label->desc.text = text;
    return label->host.SetText(text);
}

bool NativeUiModule::SetLabelVisible(LabelHandle handle, bool visible)
{
    LabelEntry* label = FindLabel(handle);
    if (!label)
    {
        return false;
    }

    label->desc.visible = visible;
    return label->host.SetVisible(visible);
}

bool NativeUiModule::SetLabelStyle(
    LabelHandle handle,
    std::uint32_t textColor,
    std::uint32_t backgroundColor,
    bool transparentBackground,
    bool multiline,
    bool alignLeft)
{
    LabelEntry* label = FindLabel(handle);
    if (!label)
    {
        return false;
    }

    label->desc.textColor = textColor;
    label->desc.backgroundColor = backgroundColor;
    label->desc.transparentBackground = transparentBackground;
    label->desc.multiline = multiline;
    label->desc.alignLeft = alignLeft;
    return label->host.SetStyle(textColor, backgroundColor, transparentBackground, multiline, alignLeft);
}

bool NativeUiModule::SyncLabels(const std::vector<NativeLabelDesc>& labels)
{
    bool success = true;
    for (const NativeLabelDesc& desc : labels)
    {
        if (desc.name.empty())
        {
            success = false;
            continue;
        }

        const LabelHandle existingHandle = FindLabelByName(desc.name);
        if (existingHandle == 0U)
        {
            success = CreateLabel(desc) != 0U && success;
            continue;
        }

        LabelEntry* existing = FindLabel(existingHandle);
        if (!existing)
        {
            success = false;
            continue;
        }

        if (existing->desc.text != desc.text)
        {
            success = existing->host.SetText(desc.text) && success;
            existing->desc.text = desc.text;
        }

        if (!SameRect(existing->desc.rect, desc.rect))
        {
            success = existing->host.SetBounds(desc.rect) && success;
            existing->desc.rect = desc.rect;
        }

        if (existing->desc.textColor != desc.textColor ||
            existing->desc.backgroundColor != desc.backgroundColor ||
            existing->desc.transparentBackground != desc.transparentBackground ||
            existing->desc.multiline != desc.multiline ||
            existing->desc.alignLeft != desc.alignLeft)
        {
            success = existing->host.SetStyle(
                desc.textColor,
                desc.backgroundColor,
                desc.transparentBackground,
                desc.multiline,
                desc.alignLeft) && success;
            existing->desc.textColor = desc.textColor;
            existing->desc.backgroundColor = desc.backgroundColor;
            existing->desc.transparentBackground = desc.transparentBackground;
            existing->desc.multiline = desc.multiline;
            existing->desc.alignLeft = desc.alignLeft;
        }

        if (existing->desc.visible != desc.visible)
        {
            success = existing->host.SetVisible(desc.visible) && success;
            existing->desc.visible = desc.visible;
        }
    }

    return success;
}

HWND NativeUiModule::GetEngineWindowHandle() const
{
    return m_platform ? m_platform->GetWindowHandle() : nullptr;
}

NativeUiModule::ButtonEntry* NativeUiModule::FindButton(ButtonHandle handle)
{
    const auto iterator = m_buttons.find(handle);
    return iterator != m_buttons.end() ? &iterator->second : nullptr;
}

const NativeUiModule::ButtonEntry* NativeUiModule::FindButton(ButtonHandle handle) const
{
    const auto iterator = m_buttons.find(handle);
    return iterator != m_buttons.end() ? &iterator->second : nullptr;
}

NativeUiModule::LabelEntry* NativeUiModule::FindLabel(LabelHandle handle)
{
    const auto iterator = m_labels.find(handle);
    return iterator != m_labels.end() ? &iterator->second : nullptr;
}

const NativeUiModule::LabelEntry* NativeUiModule::FindLabel(LabelHandle handle) const
{
    const auto iterator = m_labels.find(handle);
    return iterator != m_labels.end() ? &iterator->second : nullptr;
}

NativeUiModule::LabelHandle NativeUiModule::FindLabelByName(const std::string& name) const
{
    for (const auto& [handle, label] : m_labels)
    {
        if (label.desc.name == name)
        {
            return handle;
        }
    }

    return 0U;
}
