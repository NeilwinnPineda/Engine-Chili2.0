#pragma once

#include "../entity/appearance/color.hpp"
#include "math.hpp"

#include <algorithm>
#include <cmath>

inline float ClampReflectivity(float reflectivity)
{
    return std::clamp(reflectivity, 0.0f, 1.0f);
}

inline float ClampRoughness(float roughness)
{
    return std::clamp(roughness, 0.0f, 1.0f);
}

inline Vector3 ComputeReflectionVector(
    const Vector3& incomingDirection,
    const Vector3& surfaceNormal)
{
    const Vector3 incoming = Normalize(incomingDirection);
    const Vector3 normal = Normalize(surfaceNormal);
    if (Length(incoming) <= 0.000001f || Length(normal) <= 0.000001f)
    {
        return incomingDirection;
    }

    return Normalize(incoming - (normal * (2.0f * Dot(incoming, normal))));
}

inline float ComputeFresnelSchlickFactor(float cosTheta, float baseReflectivity)
{
    const float clampedCosTheta = std::clamp(cosTheta, 0.0f, 1.0f);
    const float reflectivity = ClampReflectivity(baseReflectivity);
    const float oneMinusCosTheta = 1.0f - clampedCosTheta;
    const float falloff =
        oneMinusCosTheta *
        oneMinusCosTheta *
        oneMinusCosTheta *
        oneMinusCosTheta *
        oneMinusCosTheta;
    return reflectivity + ((1.0f - reflectivity) * falloff);
}

inline float ComputeSpecularStrengthFromReflectivity(float reflectivity)
{
    const float clampedReflectivity = ClampReflectivity(reflectivity);
    return std::clamp(0.08f + (clampedReflectivity * 0.92f), 0.0f, 1.0f);
}

inline float ComputeSpecularPowerFromRoughness(float roughness)
{
    const float clampedRoughness = ClampRoughness(roughness);
    const float smoothness = 1.0f - clampedRoughness;
    return 8.0f + (smoothness * smoothness * 248.0f);
}

inline float ComputeDiffuseStrengthFromReflectivity(float reflectivity, float roughness)
{
    const float clampedReflectivity = ClampReflectivity(reflectivity);
    const float clampedRoughness = ClampRoughness(roughness);
    return std::clamp(1.0f - (clampedReflectivity * (0.55f + (0.25f * (1.0f - clampedRoughness)))), 0.05f, 1.0f);
}

inline ColorPrototype ComputeReflectedLightTint(
    const ColorPrototype& baseColor,
    const ColorPrototype& reflectionColor,
    float reflectivity)
{
    const float blend = ClampReflectivity(reflectivity);
    return ColorPrototype(
        (baseColor.r * (1.0f - blend)) + (reflectionColor.r * blend),
        (baseColor.g * (1.0f - blend)) + (reflectionColor.g * blend),
        (baseColor.b * (1.0f - blend)) + (reflectionColor.b * blend),
        baseColor.a);
}
