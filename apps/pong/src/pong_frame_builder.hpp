#pragma once

#include "pong_game.hpp"

#include "prototypes/presentation/frame.hpp"

class PongFrameBuilder
{
public:
    static FramePrototype Build(const PongGame& game);
};
