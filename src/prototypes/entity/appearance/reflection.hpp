#pragma once

#include "color.hpp"
#include "../../math/reflectivity_math.hpp"

struct ReflectionPrototype
{
    ColorPrototype reflectionColor = ColorPrototype(1.0f, 1.0f, 1.0f, 1.0f);
    float reflectivity = 0.0f;
    float roughness = 1.0f;

    bool IsReflective() const
    {
        return reflectivity > 0.0f;
    }

    Vector3 ComputeReflectedDirection(const Vector3& incomingDirection, const Vector3& surfaceNormal) const
    {
        return ComputeReflectionVector(incomingDirection, surfaceNormal);
    }

    float ApplyReflectivityToIntensity(float incomingIntensity) const
    {
        return incomingIntensity * ClampReflectivity(reflectivity);
    }
};
