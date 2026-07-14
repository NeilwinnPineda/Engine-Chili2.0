#include "native_ui_builder.hpp"

#include <utility>

NativeUiBuilder::OverlayBuilder::OverlayBuilder(NativeUiBuilder& owner)
    : m_owner(owner)
{
}

NativeUiBuilder::OverlayBuilder& NativeUiBuilder::OverlayBuilder::Line(const std::wstring& text)
{
    m_owner.m_frame.overlayLines.push_back(text);
    return *this;
}

NativeUiBuilder::OverlayBuilder& NativeUiBuilder::OverlayBuilder::Line(const wchar_t* text)
{
    return Line(text ? std::wstring(text) : std::wstring());
}

NativeUiBuilder::PanelBuilder::PanelBuilder(NativeUiBuilder& owner, std::size_t panelIndex)
    : m_owner(owner),
      m_panelIndex(panelIndex)
{
}

NativeUiBuilder::PanelBuilder& NativeUiBuilder::PanelBuilder::Visible(bool visible)
{
    m_owner.m_frame.panels[m_panelIndex].visible = visible;
    return *this;
}

NativeUiBuilder::PanelBuilder& NativeUiBuilder::PanelBuilder::Bounds(int x, int y, int width, int height)
{
    NativeUiPanel& panel = m_owner.m_frame.panels[m_panelIndex];
    panel.anchor = NativeUiAnchor::TopLeft;
    panel.offsetX = x;
    panel.offsetY = y;
    panel.width = width;
    panel.height = height;
    return *this;
}

NativeUiBuilder::PanelBuilder& NativeUiBuilder::PanelBuilder::Anchor(
    NativeUiAnchor anchor,
    int offsetX,
    int offsetY,
    int width,
    int height)
{
    NativeUiPanel& panel = m_owner.m_frame.panels[m_panelIndex];
    panel.anchor = anchor;
    panel.offsetX = offsetX;
    panel.offsetY = offsetY;
    panel.width = width;
    panel.height = height;
    return *this;
}

NativeUiBuilder::PanelBuilder& NativeUiBuilder::PanelBuilder::Colors(
    std::uint32_t textColor,
    std::uint32_t backgroundColor)
{
    NativeUiPanel& panel = m_owner.m_frame.panels[m_panelIndex];
    panel.textColor = textColor;
    panel.backgroundColor = backgroundColor;
    return *this;
}

NativeUiBuilder::PanelBuilder& NativeUiBuilder::PanelBuilder::Colors(
    const ColorPrototype& textColor,
    const ColorPrototype& backgroundColor)
{
    return Colors(textColor.ToArgb(), backgroundColor.ToArgb());
}

NativeUiBuilder::PanelBuilder& NativeUiBuilder::PanelBuilder::Row(const std::string& label, const std::string& value)
{
    m_owner.m_frame.panels[m_panelIndex].rows.push_back(NativeUiStatusRow{ label, value });
    return *this;
}

NativeUiBuilder::PanelBuilder& NativeUiBuilder::PanelBuilder::Row(const std::string& label, const char* value)
{
    return Row(label, value ? std::string(value) : std::string());
}

NativeUiBuilder::ButtonBuilder::ButtonBuilder(NativeUiBuilder& owner, std::size_t buttonIndex)
    : m_owner(owner),
      m_buttonIndex(buttonIndex)
{
}

NativeUiBuilder::ButtonBuilder& NativeUiBuilder::ButtonBuilder::Visible(bool visible)
{
    m_owner.m_frame.nativeButtons[m_buttonIndex].visible = visible;
    return *this;
}

NativeUiBuilder::ButtonBuilder& NativeUiBuilder::ButtonBuilder::Enabled(bool enabled)
{
    m_owner.m_frame.nativeButtons[m_buttonIndex].enabled = enabled;
    return *this;
}

NativeUiBuilder::ButtonBuilder& NativeUiBuilder::ButtonBuilder::Bounds(int x, int y, int width, int height)
{
    NativeUiButton& button = m_owner.m_frame.nativeButtons[m_buttonIndex];
    button.useAnchor = false;
    button.x = x;
    button.y = y;
    button.width = width;
    button.height = height;
    return *this;
}

NativeUiBuilder::ButtonBuilder& NativeUiBuilder::ButtonBuilder::Anchor(
    NativeUiAnchor anchor,
    int offsetX,
    int offsetY,
    int width,
    int height)
{
    NativeUiButton& button = m_owner.m_frame.nativeButtons[m_buttonIndex];
    button.anchor = anchor;
    button.offsetX = offsetX;
    button.offsetY = offsetY;
    button.width = width;
    button.height = height;
    button.useAnchor = true;
    return *this;
}

NativeUiBuilder::FormBuilder::FormBuilder(NativeUiBuilder& owner, std::size_t formIndex)
    : m_owner(owner),
      m_formIndex(formIndex)
{
}

NativeUiBuilder::FormBuilder& NativeUiBuilder::FormBuilder::Visible(bool visible)
{
    m_owner.m_frame.forms[m_formIndex].visible = visible;
    return *this;
}

NativeUiBuilder::FormBuilder& NativeUiBuilder::FormBuilder::Bounds(int x, int y, int width, int height)
{
    NativeUiForm& form = m_owner.m_frame.forms[m_formIndex];
    form.useAnchor = false;
    form.x = x;
    form.y = y;
    form.width = width;
    form.height = height;
    return *this;
}

NativeUiBuilder::FormBuilder& NativeUiBuilder::FormBuilder::Anchor(
    NativeUiAnchor anchor,
    int offsetX,
    int offsetY,
    int width,
    int height)
{
    NativeUiForm& form = m_owner.m_frame.forms[m_formIndex];
    form.anchor = anchor;
    form.offsetX = offsetX;
    form.offsetY = offsetY;
    form.width = width;
    form.height = height;
    form.useAnchor = true;
    return *this;
}

NativeUiBuilder::FormBuilder& NativeUiBuilder::FormBuilder::Body(const std::wstring& body)
{
    m_owner.m_frame.forms[m_formIndex].body = body;
    return *this;
}

NativeUiBuilder::FormBuilder& NativeUiBuilder::FormBuilder::Body(const wchar_t* body)
{
    return Body(body ? std::wstring(body) : std::wstring());
}

NativeUiBuilder::FormBuilder& NativeUiBuilder::FormBuilder::HeaderHeight(int height)
{
    m_owner.m_frame.forms[m_formIndex].headerHeight = height;
    return *this;
}

NativeUiBuilder::FormBuilder& NativeUiBuilder::FormBuilder::Padding(int padding)
{
    m_owner.m_frame.forms[m_formIndex].padding = padding;
    return *this;
}

NativeUiBuilder::FormBuilder& NativeUiBuilder::FormBuilder::Colors(
    std::uint32_t titleTextColor,
    std::uint32_t bodyTextColor,
    std::uint32_t backgroundColor,
    std::uint32_t headerColor,
    std::uint32_t borderColor)
{
    NativeUiForm& form = m_owner.m_frame.forms[m_formIndex];
    form.titleTextColor = titleTextColor;
    form.bodyTextColor = bodyTextColor;
    form.backgroundColor = backgroundColor;
    form.headerColor = headerColor;
    form.borderColor = borderColor;
    return *this;
}

NativeUiBuilder::FormBuilder& NativeUiBuilder::FormBuilder::Colors(
    const ColorPrototype& titleTextColor,
    const ColorPrototype& bodyTextColor,
    const ColorPrototype& backgroundColor,
    const ColorPrototype& headerColor,
    const ColorPrototype& borderColor)
{
    return Colors(
        titleTextColor.ToArgb(),
        bodyTextColor.ToArgb(),
        backgroundColor.ToArgb(),
        headerColor.ToArgb(),
        borderColor.ToArgb());
}

NativeUiBuilder& NativeUiBuilder::WindowTitle(const std::wstring& title)
{
    m_frame.hasWindowTitle = true;
    m_frame.windowTitle = title;
    return *this;
}

NativeUiBuilder& NativeUiBuilder::WindowTitle(const wchar_t* title)
{
    return WindowTitle(title ? std::wstring(title) : std::wstring());
}

NativeUiBuilder& NativeUiBuilder::Canvas(int designWidth, int designHeight, bool lockAspectRatio)
{
    NativeUiCanvasSettings settings;
    settings.designWidth = designWidth;
    settings.designHeight = designHeight;
    settings.lockAspectRatio = lockAspectRatio;
    if (designWidth > 0 && designHeight > 0)
    {
        settings.aspectRatio = static_cast<float>(designWidth) / static_cast<float>(designHeight);
    }

    return Canvas(settings);
}

NativeUiBuilder& NativeUiBuilder::Canvas(
    int designWidth,
    int designHeight,
    float aspectRatio,
    bool lockAspectRatio)
{
    NativeUiCanvasSettings settings;
    settings.designWidth = designWidth;
    settings.designHeight = designHeight;
    settings.aspectRatio = aspectRatio;
    settings.lockAspectRatio = lockAspectRatio;
    return Canvas(settings);
}

NativeUiBuilder& NativeUiBuilder::Canvas(const NativeUiCanvasSettings& settings)
{
    m_frame.hasCanvasSettings = true;
    m_frame.canvasSettings = settings;
    return *this;
}

NativeUiBuilder& NativeUiBuilder::OverlayEnabled(bool enabled)
{
    m_frame.overlayEnabled = enabled;
    return *this;
}

NativeUiBuilder& NativeUiBuilder::ClearColor(std::uint32_t color)
{
    m_frame.hasClearColor = true;
    m_frame.clearColor = color;
    return *this;
}

NativeUiBuilder& NativeUiBuilder::ClearColor(const ColorPrototype& color)
{
    return ClearColor(color.ToArgb());
}

NativeUiBuilder& NativeUiBuilder::ContentFrame(const FramePrototype& frame)
{
    m_frame.hasContentFrame = true;
    m_frame.contentFrame = frame;
    return *this;
}

NativeUiBuilder& NativeUiBuilder::ContentFrame(FramePrototype&& frame)
{
    m_frame.hasContentFrame = true;
    m_frame.contentFrame = std::move(frame);
    return *this;
}

NativeUiBuilder& NativeUiBuilder::Text(
    const std::wstring& text,
    float x,
    float y,
    float scale,
    std::uint32_t color)
{
    NativeUiTextBlock block;
    block.content = text;
    block.position = NativeUiPoint{ x, y };
    block.style.scale = scale;
    block.style.color = color;
    m_frame.textBlocks.push_back(std::move(block));
    return *this;
}

NativeUiBuilder& NativeUiBuilder::Text(
    const std::wstring& text,
    float x,
    float y,
    float scale,
    const ColorPrototype& color)
{
    return Text(text, x, y, scale, color.ToArgb());
}

NativeUiBuilder& NativeUiBuilder::Label(
    const std::string& name,
    const std::wstring& text,
    int x,
    int y,
    int width,
    int height,
    std::uint32_t color)
{
    NativeUiLabel label;
    label.name = name;
    label.text = text;
    label.x = x;
    label.y = y;
    label.width = width;
    label.height = height;
    label.textColor = color;
    label.multiline = false;
    label.alignLeft = false;
    m_frame.nativeLabels.push_back(std::move(label));
    return *this;
}

NativeUiBuilder& NativeUiBuilder::Label(
    const std::string& name,
    const std::wstring& text,
    int x,
    int y,
    int width,
    int height,
    const ColorPrototype& color)
{
    return Label(name, text, x, y, width, height, color.ToArgb());
}

NativeUiBuilder& NativeUiBuilder::AnchoredLabel(
    const std::string& name,
    const std::wstring& text,
    NativeUiAnchor anchor,
    int offsetX,
    int offsetY,
    int width,
    int height,
    std::uint32_t color)
{
    NativeUiLabel label;
    label.name = name;
    label.text = text;
    label.anchor = anchor;
    label.offsetX = offsetX;
    label.offsetY = offsetY;
    label.width = width;
    label.height = height;
    label.useAnchor = true;
    label.textColor = color;
    m_frame.nativeLabels.push_back(std::move(label));
    return *this;
}

NativeUiBuilder& NativeUiBuilder::AnchoredLabel(
    const std::string& name,
    const std::wstring& text,
    NativeUiAnchor anchor,
    int offsetX,
    int offsetY,
    int width,
    int height,
    const ColorPrototype& color)
{
    return AnchoredLabel(name, text, anchor, offsetX, offsetY, width, height, color.ToArgb());
}

NativeUiBuilder& NativeUiBuilder::Scoreboard(
    int leftScore,
    int rightScore,
    float centerX,
    float centerY,
    float scale,
    std::uint32_t color)
{
    const int width = static_cast<int>(120.0f * scale);
    const int height = static_cast<int>(28.0f * scale);
    const int x = static_cast<int>(400.0f + (centerX * 360.0f) - (static_cast<float>(width) * 0.5f));
    const int y = static_cast<int>(24.0f + ((1.0f - centerY) * 80.0f));
    Label(
        "scoreboard",
        std::to_wstring(leftScore) + L" - " + std::to_wstring(rightScore),
        x,
        y,
        width,
        height,
        color);
    return *this;
}

NativeUiBuilder& NativeUiBuilder::Scoreboard(
    int leftScore,
    int rightScore,
    float centerX,
    float centerY,
    float scale,
    const ColorPrototype& color)
{
    return Scoreboard(leftScore, rightScore, centerX, centerY, scale, color.ToArgb());
}

NativeUiBuilder& NativeUiBuilder::Rect(
    float centerX,
    float centerY,
    float halfWidth,
    float halfHeight,
    std::uint32_t color)
{
    NativeUiShape shape;
    shape.kind = NativeUiShapeKind::Rect;
    shape.center = NativeUiPoint{ centerX, centerY };
    shape.halfWidth = halfWidth;
    shape.halfHeight = halfHeight;
    shape.color = color;
    m_frame.shapes.push_back(shape);
    return *this;
}

NativeUiBuilder& NativeUiBuilder::Rect(
    float centerX,
    float centerY,
    float halfWidth,
    float halfHeight,
    const ColorPrototype& color)
{
    return Rect(centerX, centerY, halfWidth, halfHeight, color.ToArgb());
}

NativeUiBuilder& NativeUiBuilder::Hex(
    float centerX,
    float centerY,
    float halfWidth,
    float halfHeight,
    std::uint32_t color)
{
    NativeUiShape shape;
    shape.kind = NativeUiShapeKind::Hex;
    shape.center = NativeUiPoint{ centerX, centerY };
    shape.halfWidth = halfWidth;
    shape.halfHeight = halfHeight;
    shape.color = color;
    m_frame.shapes.push_back(shape);
    return *this;
}

NativeUiBuilder& NativeUiBuilder::Hex(
    float centerX,
    float centerY,
    float halfWidth,
    float halfHeight,
    const ColorPrototype& color)
{
    return Hex(centerX, centerY, halfWidth, halfHeight, color.ToArgb());
}

NativeUiBuilder& NativeUiBuilder::Line(
    float startX,
    float startY,
    float endX,
    float endY,
    float thickness,
    std::uint32_t color)
{
    NativeUiShape shape;
    shape.kind = NativeUiShapeKind::Line;
    shape.start = NativeUiPoint{ startX, startY };
    shape.end = NativeUiPoint{ endX, endY };
    shape.thickness = thickness;
    shape.color = color;
    m_frame.shapes.push_back(shape);
    return *this;
}

NativeUiBuilder& NativeUiBuilder::Line(
    float startX,
    float startY,
    float endX,
    float endY,
    float thickness,
    const ColorPrototype& color)
{
    return Line(startX, startY, endX, endY, thickness, color.ToArgb());
}

NativeUiBuilder::OverlayBuilder NativeUiBuilder::Overlay()
{
    return OverlayBuilder(*this);
}

NativeUiBuilder::PanelBuilder NativeUiBuilder::Panel(const std::string& title)
{
    NativeUiPanel panel;
    panel.title = title;
    m_frame.panels.push_back(std::move(panel));
    return PanelBuilder(*this, m_frame.panels.size() - 1U);
}

NativeUiBuilder::ButtonBuilder NativeUiBuilder::Button(const std::string& name, const std::wstring& text)
{
    NativeUiButton button;
    button.name = name;
    button.text = text;
    m_frame.nativeButtons.push_back(std::move(button));
    return ButtonBuilder(*this, m_frame.nativeButtons.size() - 1U);
}

NativeUiBuilder::ButtonBuilder NativeUiBuilder::Button(const std::string& name, const wchar_t* text)
{
    return Button(name, text ? std::wstring(text) : std::wstring());
}

NativeUiBuilder::FormBuilder NativeUiBuilder::Form(const std::string& name, const std::wstring& title)
{
    NativeUiForm form;
    form.name = name;
    form.title = title;
    m_frame.forms.push_back(std::move(form));
    return FormBuilder(*this, m_frame.forms.size() - 1U);
}

NativeUiBuilder::FormBuilder NativeUiBuilder::Form(const std::string& name, const wchar_t* title)
{
    return Form(name, title ? std::wstring(title) : std::wstring());
}

NativeUiFrame NativeUiBuilder::Build() const
{
    return m_frame;
}
