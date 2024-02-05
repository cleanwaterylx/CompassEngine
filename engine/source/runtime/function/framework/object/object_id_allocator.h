#pragma once

#include <atomic>
#include <limits>

namespace Compass
{
    using GObjectID = std::size_t;  // ±ðÃû

    constexpr GObjectID k_invalid_gobject_id = std::numeric_limits<std::size_t>::max();

    class ObjectIDAllocator
    {
    public:
        static GObjectID alloc();

    private:
        static std::atomic<GObjectID> m_next_id;
    };

} // namespace Compass
