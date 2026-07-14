#include "template_runtime.hpp"

#include "engine/game_runtime_api.hpp"

namespace
{
    studio_runtime::IGameRuntime* CreateRuntime()
    {
        return new TemplateRuntime();
    }

    void DestroyRuntime(studio_runtime::IGameRuntime* runtime)
    {
        delete runtime;
    }
}

extern "C" GAME_RUNTIME_API_EXPORT const GameRuntimeApi* get_game_api()
{
    static const GameRuntimeApi api{
        studio_runtime::kGameRuntimeApiVersion,
        static_cast<std::uint32_t>(sizeof(GameRuntimeApi)),
        "TemplateRuntime",
        &CreateRuntime,
        &DestroyRuntime
    };
    return &api;
}
