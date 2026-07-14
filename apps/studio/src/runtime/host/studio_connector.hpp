#pragma once

#include "runtime/interaction/studio_camera.hpp"
#include "runtime/query/runtime_commands.hpp"
#include "runtime/query/runtime_queries.hpp"

namespace studio_runtime
{
    class StudioConnector
    {
    public:
        StudioConnector(RuntimeWorld& world, StudioCamera& camera);

        RuntimeQueries& Queries();
        const RuntimeQueries& Queries() const;
        RuntimeCommands& Commands();
        StudioCamera& Camera();

    private:
        RuntimeWorld& m_world;
        RuntimeQueries m_queries;
        RuntimeCommands m_commands;
        StudioCamera& m_camera;
    };
}
