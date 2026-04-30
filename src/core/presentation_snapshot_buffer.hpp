#pragma once

#include "runtime_types.hpp"

#include <array>
#include <mutex>

class PresentationSnapshotBuffer
{
public:
    void Publish(const PresentationSnapshot& snapshot)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const std::size_t nextIndex = (m_frontIndex + 1U) % m_snapshots.size();
        m_snapshots[nextIndex] = snapshot;
        m_frontIndex = nextIndex;
    }

    PresentationSnapshot ReadLatest() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_snapshots[m_frontIndex];
    }

private:
    mutable std::mutex m_mutex;
    std::array<PresentationSnapshot, 2> m_snapshots{};
    std::size_t m_frontIndex = 0;
};
