#pragma once

#include "space_invaders_game.hpp"

#include "prototypes/presentation/frame.hpp"

class SpaceInvadersFrameBuilder
{
public:
    static FramePrototype Build(const SpaceInvadersGame& game);
};
