#include "runtime/function/framework/object/object_id_allocator.h"

#include "core/base/macro.h"

namespace Compass
{
    std::atomic<GObjectID> ObjectIDAllocator::m_next_id {0};

    GObjectID Compass::ObjectIDAllocator::alloc()
    {
        std::atomic<GObjectID> new_object_ret = m_next_id.load();  // ¶ÁÈ¡id
        m_next_id++;

        if(m_next_id >= k_invalid_gobject_id)
        {
            LOG_FATAL("gobject id overflow");
        }

        return new_object_ret;
    }
} // namespace Compass
