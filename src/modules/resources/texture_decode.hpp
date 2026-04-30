#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct DecodedTextureRgba8
{
    std::vector<std::byte> pixels;
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::uint32_t rowPitch = 0U;
};

bool DecodeTextureFileRgba8(
    const std::string& resolvedPath,
    DecodedTextureRgba8& outTexture,
    std::string& outError);
