#pragma once

#include <atomic>
#include <limits>

namespace Compass
{
    using ParticleEmitterID = std::size_t;

    constexpr ParticleEmitterID k_invalid_particle_emitter_id = std::numeric_limits<std::size_t>::max();

    class ParticleEmitterIDAllocator
    {
    public:
        static ParticleEmitterID alloc();

    private:
        static std::atomic<ParticleEmitterID> m_next_id;
    };
} // namespace Compass
