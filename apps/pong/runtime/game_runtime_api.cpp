#include "engine/game_runtime_api.hpp"

#include "pong_runtime.hpp"

namespace
{
    studio_runtime::IGameRuntime* CreatePongRuntime()
    {
        return new PongRuntime();
    }

    void DestroyPongRuntime(studio_runtime::IGameRuntime* runtime)
    {
        delete runtime;
    }
}

extern "C" GAME_RUNTIME_API_EXPORT const GameRuntimeApi* get_game_api()
{
    static const GameRuntimeApi api{
        studio_runtime::kGameRuntimeApiVersion,
        static_cast<std::uint32_t>(sizeof(GameRuntimeApi)),
        "PongRuntime",
        &CreatePongRuntime,
        &DestroyPongRuntime
    };

    return &api;
}
