#pragma once

#include "light_visibility.hpp"

// Transitional shadow aliases:
// unified light visibility now owns policy/method/update authoring, while
// mesh/object shadow participation remains a separate reusable concern.
using ShadowBaseMethodPrototype = LightVisibilityMethodPrototype;
using ShadowRefinementMethodPrototype = LightVisibilityRefinementPrototype;
using ShadowUpdatePolicyPrototype = LightVisibilityUpdatePolicyPrototype;
using ShadowPrototype = LightVisibilityPolicyPrototype;

struct ShadowParticipationPrototype
{
    bool casts = true;
    bool receives = true;
};
