#pragma once

#include "../../core/module.hpp"
#include "../../core/engine_core.hpp"
#include "native_button_host.hpp"
#include "native_label_host.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class EngineContext;
class IPlatformService;

class NativeUiModule : public IModule
{
public:
    using ButtonHandle = std::uint32_t;
    using LabelHandle = std::uint32_t;

public:
    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    ButtonHandle CreateButton(const NativeButtonDesc& desc);
    bool DestroyButton(ButtonHandle handle);
    void DestroyAllButtons();
    bool SetButtonBounds(ButtonHandle handle, const NativeControlRect& rect);
    bool SetButtonText(ButtonHandle handle, const std::wstring& text);
    bool SetButtonVisible(ButtonHandle handle, bool visible);
    bool SetButtonEnabled(ButtonHandle handle, bool enabled);
    bool ConsumeButtonPressed(ButtonHandle handle);
    bool IsButtonOpen(ButtonHandle handle) const;
    LabelHandle CreateLabel(const NativeLabelDesc& desc);
    bool DestroyLabel(LabelHandle handle);
    void DestroyAllLabels();
    bool SetLabelBounds(LabelHandle handle, const NativeControlRect& rect);
    bool SetLabelText(LabelHandle handle, const std::wstring& text);
    bool SetLabelVisible(LabelHandle handle, bool visible);
    bool SetLabelStyle(
        LabelHandle handle,
        std::uint32_t textColor,
        std::uint32_t backgroundColor,
        bool transparentBackground,
        bool multiline,
        bool alignLeft);
    bool SyncLabels(const std::vector<NativeLabelDesc>& labels);

private:
    struct ButtonEntry
    {
        NativeButtonDesc desc;
        NativeButtonHost host;
    };

    struct LabelEntry
    {
        NativeLabelDesc desc;
        NativeLabelHost host;
    };

private:
    HWND GetEngineWindowHandle() const;
    ButtonEntry* FindButton(ButtonHandle handle);
    const ButtonEntry* FindButton(ButtonHandle handle) const;
    LabelEntry* FindLabel(LabelHandle handle);
    const LabelEntry* FindLabel(LabelHandle handle) const;
    LabelHandle FindLabelByName(const std::string& name) const;

private:
    IPlatformService* m_platform = nullptr;
    std::unordered_map<ButtonHandle, ButtonEntry> m_buttons;
    std::unordered_map<LabelHandle, LabelEntry> m_labels;
    ButtonHandle m_nextHandle = 1U;
    LabelHandle m_nextLabelHandle = 1U;
    bool m_initialized = false;
};
