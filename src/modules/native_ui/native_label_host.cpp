#include "native_label_host.hpp"

#include <algorithm>

namespace
{
    constexpr wchar_t kNativeLabelWindowClassName[] = L"ProjectEngineNativeLabelClass";

    COLORREF ToColorRef(std::uint32_t argb)
    {
        return RGB(
            static_cast<BYTE>((argb >> 16U) & 0xFFU),
            static_cast<BYTE>((argb >> 8U) & 0xFFU),
            static_cast<BYTE>(argb & 0xFFU));
    }

    bool EnsureNativeLabelWindowClass()
    {
        WNDCLASSEXW windowClass = {};
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = NativeLabelHost::WindowProcSetup;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.hbrBackground = nullptr;
        windowClass.lpszClassName = kNativeLabelWindowClassName;

        if (!RegisterClassExW(&windowClass))
        {
            return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
        }

        return true;
    }
}

bool NativeLabelHost::Initialize(HWND engineWindowHandle, const NativeLabelDesc& desc)
{
    Shutdown();

    if (!engineWindowHandle || !EnsureNativeLabelWindowClass())
    {
        return false;
    }

    m_engineWindowHandle = engineWindowHandle;
    m_text = desc.text;
    m_rect = desc.rect;
    m_visible = desc.visible;
    m_textColor = desc.textColor;
    m_backgroundColor = desc.backgroundColor;
    m_transparentBackground = desc.transparentBackground;
    m_multiline = desc.multiline;
    m_alignLeft = desc.alignLeft;

    m_labelWindowHandle = CreateWindowExW(
        0,
        kNativeLabelWindowClassName,
        m_text.c_str(),
        WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | (m_visible ? WS_VISIBLE : 0),
        m_rect.x,
        m_rect.y,
        std::max(1, m_rect.width),
        std::max(1, m_rect.height),
        m_engineWindowHandle,
        nullptr,
        GetModuleHandleW(nullptr),
        this);

    return m_labelWindowHandle != nullptr;
}

void NativeLabelHost::Shutdown()
{
    if (m_labelWindowHandle)
    {
        DestroyWindow(m_labelWindowHandle);
        m_labelWindowHandle = nullptr;
    }

    m_engineWindowHandle = nullptr;
}

bool NativeLabelHost::SetBounds(const NativeControlRect& rect)
{
    m_rect = rect;
    if (!m_labelWindowHandle)
    {
        return false;
    }

    return SetWindowPos(
        m_labelWindowHandle,
        HWND_TOP,
        rect.x,
        rect.y,
        std::max(1, rect.width),
        std::max(1, rect.height),
        SWP_NOACTIVATE) != FALSE;
}

bool NativeLabelHost::SetText(const std::wstring& text)
{
    m_text = text;
    if (!m_labelWindowHandle)
    {
        return false;
    }

    SetWindowTextW(m_labelWindowHandle, m_text.c_str());
    InvalidateRect(m_labelWindowHandle, nullptr, TRUE);
    return true;
}

bool NativeLabelHost::SetVisible(bool visible)
{
    m_visible = visible;
    if (!m_labelWindowHandle)
    {
        return false;
    }

    ShowWindow(m_labelWindowHandle, visible ? SW_SHOW : SW_HIDE);
    return true;
}

bool NativeLabelHost::SetStyle(
    std::uint32_t textColor,
    std::uint32_t backgroundColor,
    bool transparentBackground,
    bool multiline,
    bool alignLeft)
{
    m_textColor = textColor;
    m_backgroundColor = backgroundColor;
    m_transparentBackground = transparentBackground;
    m_multiline = multiline;
    m_alignLeft = alignLeft;
    if (m_labelWindowHandle)
    {
        InvalidateRect(m_labelWindowHandle, nullptr, TRUE);
    }

    return true;
}

bool NativeLabelHost::IsOpen() const
{
    return m_labelWindowHandle != nullptr && IsWindow(m_labelWindowHandle) != FALSE;
}

LRESULT CALLBACK NativeLabelHost::WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        NativeLabelHost* host = reinterpret_cast<NativeLabelHost*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(host));
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&NativeLabelHost::WindowProcThunk));
        return host->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK NativeLabelHost::WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    NativeLabelHost* host = reinterpret_cast<NativeLabelHost*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return host ? host->HandleMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT NativeLabelHost::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCCREATE:
        m_labelWindowHandle = hwnd;
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(hwnd, &paint);
        Draw(dc);
        EndPaint(hwnd, &paint);
        return 0;
    }

    case WM_DESTROY:
        if (m_labelWindowHandle == hwnd)
        {
            m_labelWindowHandle = nullptr;
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void NativeLabelHost::Draw(HDC dc)
{
    RECT clientRect = GetClientRectSafe();
    const int width = clientRect.right - clientRect.left;
    const int height = clientRect.bottom - clientRect.top;
    if (width <= 0 || height <= 0)
    {
        return;
    }

    if (m_transparentBackground)
    {
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        HGDIOBJ oldFont = SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, ToColorRef(m_textColor));
        RECT textRect = clientRect;
        textRect.left += m_alignLeft ? 10 : 0;
        textRect.right -= m_alignLeft ? 10 : 0;
        const UINT format =
            (m_alignLeft ? DT_LEFT : DT_CENTER) |
            (m_multiline ? DT_WORDBREAK : DT_SINGLELINE) |
            DT_END_ELLIPSIS |
            DT_NOPREFIX |
            (m_multiline ? 0U : DT_VCENTER);
        DrawTextW(
            dc,
            m_text.c_str(),
            -1,
            &textRect,
            format);
        SelectObject(dc, oldFont);
        return;
    }

    HDC memoryDc = CreateCompatibleDC(dc);
    if (!memoryDc)
    {
        return;
    }

    HBITMAP bitmap = CreateCompatibleBitmap(dc, width, height);
    if (!bitmap)
    {
        DeleteDC(memoryDc);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);
    HBRUSH backgroundBrush = CreateSolidBrush(ToColorRef(m_backgroundColor));
    FillRect(memoryDc, &clientRect, backgroundBrush);
    DeleteObject(backgroundBrush);

    HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ oldFont = SelectObject(memoryDc, font);
    SetBkMode(memoryDc, TRANSPARENT);
    SetTextColor(memoryDc, ToColorRef(m_textColor));

    RECT textRect = clientRect;
    textRect.left += m_alignLeft ? 10 : 0;
    textRect.right -= m_alignLeft ? 10 : 0;
    textRect.top += m_multiline ? 8 : 0;
    const UINT format =
        (m_alignLeft ? DT_LEFT : DT_CENTER) |
        (m_multiline ? DT_WORDBREAK : DT_SINGLELINE) |
        DT_END_ELLIPSIS |
        DT_NOPREFIX |
        (m_multiline ? 0U : DT_VCENTER);
    DrawTextW(
        memoryDc,
        m_text.c_str(),
        -1,
        &textRect,
        format);

    SelectObject(memoryDc, oldFont);
    BitBlt(dc, 0, 0, width, height, memoryDc, 0, 0, SRCCOPY);

    SelectObject(memoryDc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memoryDc);
}

RECT NativeLabelHost::GetClientRectSafe() const
{
    RECT rect{};
    if (m_labelWindowHandle)
    {
        GetClientRect(m_labelWindowHandle, &rect);
    }
    return rect;
}
