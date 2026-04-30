#pragma once

#include "../../app/app_capabilities.hpp"

#include <windows.h>

#include <string>

class NativeLabelHost
{
public:
    bool Initialize(HWND engineWindowHandle, const NativeLabelDesc& desc);
    void Shutdown();

    bool SetBounds(const NativeControlRect& rect);
    bool SetText(const std::wstring& text);
    bool SetVisible(bool visible);
    bool SetStyle(
        std::uint32_t textColor,
        std::uint32_t backgroundColor,
        bool transparentBackground,
        bool multiline,
        bool alignLeft);

    bool IsOpen() const;

    static LRESULT CALLBACK WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Draw(HDC dc);
    RECT GetClientRectSafe() const;

private:
    HWND m_engineWindowHandle = nullptr;
    HWND m_labelWindowHandle = nullptr;
    std::wstring m_text;
    NativeControlRect m_rect{};
    bool m_visible = true;
    std::uint32_t m_textColor = 0xFFFFFFFFu;
    std::uint32_t m_backgroundColor = 0x00000000u;
    bool m_transparentBackground = true;
    bool m_multiline = false;
    bool m_alignLeft = false;
};
