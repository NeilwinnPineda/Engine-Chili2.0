#pragma once

#include <cstdint>

// Light visibility is a lane of the unified light system.
// Concrete shadow techniques are realizations of this visibility contract.

enum class LightVisibilityMethodPrototype : unsigned char
{
    None = 0,
    RasterCubemap,
    RasterShadowMap,
    Traced
};

enum class LightVisibilityRefinementPrototype : unsigned char
{
    None = 0
};

enum class LightVisibilityUpdatePolicyPrototype : unsigned char
{
    Continuous = 0
};

struct LightVisibilityPolicyPrototype
{
    bool enabled = false;
    LightVisibilityMethodPrototype method = LightVisibilityMethodPrototype::None;
    LightVisibilityRefinementPrototype refinement = LightVisibilityRefinementPrototype::None;
    LightVisibilityUpdatePolicyPrototype updatePolicy = LightVisibilityUpdatePolicyPrototype::Continuous;
    std::uint32_t resolution = 512U;
    float depthBias = 0.015f;
    float normalBias = 0.0f;
    float nearPlane = 0.1f;
    float farPlane = 10.0f;
    float refinementInfluence = 0.0f;

    bool IsRasterCubemapEnabled() const
    {
        return enabled && method == LightVisibilityMethodPrototype::RasterCubemap;
    }
};
