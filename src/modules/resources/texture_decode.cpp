#include "texture_decode.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#endif

#include <string>

namespace
{
#if defined(_WIN32)
template<typename T>
void SafeReleaseTextureDecodeCom(T*& value)
{
    if (value)
    {
        value->Release();
        value = nullptr;
    }
}

std::wstring ToWidePath(const std::string& value)
{
    if (value.empty())
    {
        return std::wstring();
    }

    const int wideLength = MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        nullptr,
        0);
    if (wideLength <= 0)
    {
        return std::wstring(value.begin(), value.end());
    }

    std::wstring wideValue(static_cast<std::size_t>(wideLength), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        wideValue.data(),
        wideLength);
    return wideValue;
}
#endif
}

bool DecodeTextureFileRgba8(
    const std::string& resolvedPath,
    DecodedTextureRgba8& outTexture,
    std::string& outError)
{
    outTexture = DecodedTextureRgba8{};
    outError.clear();

#if !defined(_WIN32)
    outError = "Texture decode is only implemented on Windows right now.";
    return false;
#else
    const HRESULT coInitResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool shouldUninitialize = SUCCEEDED(coInitResult);
    if (FAILED(coInitResult) && coInitResult != RPC_E_CHANGED_MODE)
    {
        outError = "CoInitializeEx failed for texture decode.";
        return false;
    }

    IWICImagingFactory* imagingFactory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;

    const std::wstring widePath = ToWidePath(resolvedPath);
    HRESULT result = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&imagingFactory));
    if (FAILED(result))
    {
        outError = "Failed to create WIC imaging factory.";
        if (shouldUninitialize)
        {
            CoUninitialize();
        }
        return false;
    }

    result = imagingFactory->CreateDecoderFromFilename(
        widePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder);
    if (FAILED(result))
    {
        outError = "Failed to open texture file with WIC.";
        SafeReleaseTextureDecodeCom(imagingFactory);
        if (shouldUninitialize)
        {
            CoUninitialize();
        }
        return false;
    }

    result = decoder->GetFrame(0, &frame);
    if (FAILED(result))
    {
        outError = "Failed to read first texture frame.";
        SafeReleaseTextureDecodeCom(decoder);
        SafeReleaseTextureDecodeCom(imagingFactory);
        if (shouldUninitialize)
        {
            CoUninitialize();
        }
        return false;
    }

    result = imagingFactory->CreateFormatConverter(&converter);
    if (FAILED(result))
    {
        outError = "Failed to create texture format converter.";
        SafeReleaseTextureDecodeCom(frame);
        SafeReleaseTextureDecodeCom(decoder);
        SafeReleaseTextureDecodeCom(imagingFactory);
        if (shouldUninitialize)
        {
            CoUninitialize();
        }
        return false;
    }

    result = converter->Initialize(
        frame,
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom);
    if (FAILED(result))
    {
        outError = "Failed to convert texture to RGBA8.";
        SafeReleaseTextureDecodeCom(converter);
        SafeReleaseTextureDecodeCom(frame);
        SafeReleaseTextureDecodeCom(decoder);
        SafeReleaseTextureDecodeCom(imagingFactory);
        if (shouldUninitialize)
        {
            CoUninitialize();
        }
        return false;
    }

    UINT width = 0U;
    UINT height = 0U;
    result = converter->GetSize(&width, &height);
    if (FAILED(result) || width == 0U || height == 0U)
    {
        outError = "Texture decode produced invalid dimensions.";
        SafeReleaseTextureDecodeCom(converter);
        SafeReleaseTextureDecodeCom(frame);
        SafeReleaseTextureDecodeCom(decoder);
        SafeReleaseTextureDecodeCom(imagingFactory);
        if (shouldUninitialize)
        {
            CoUninitialize();
        }
        return false;
    }

    const std::uint32_t rowPitch = width * 4U;
    const std::size_t pixelByteCount = static_cast<std::size_t>(rowPitch) * static_cast<std::size_t>(height);
    outTexture.pixels.resize(pixelByteCount);
    result = converter->CopyPixels(
        nullptr,
        rowPitch,
        static_cast<UINT>(pixelByteCount),
        reinterpret_cast<BYTE*>(outTexture.pixels.data()));
    if (FAILED(result))
    {
        outError = "Failed to copy decoded texture pixels.";
        outTexture = DecodedTextureRgba8{};
        SafeReleaseTextureDecodeCom(converter);
        SafeReleaseTextureDecodeCom(frame);
        SafeReleaseTextureDecodeCom(decoder);
        SafeReleaseTextureDecodeCom(imagingFactory);
        if (shouldUninitialize)
        {
            CoUninitialize();
        }
        return false;
    }

    outTexture.width = static_cast<std::uint32_t>(width);
    outTexture.height = static_cast<std::uint32_t>(height);
    outTexture.rowPitch = rowPitch;

    SafeReleaseTextureDecodeCom(converter);
    SafeReleaseTextureDecodeCom(frame);
    SafeReleaseTextureDecodeCom(decoder);
    SafeReleaseTextureDecodeCom(imagingFactory);
    if (shouldUninitialize)
    {
        CoUninitialize();
    }
    return true;
#endif
}
