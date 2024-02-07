#include "runtime/function/framework/level/level.h"

#include "runtime/core/base/macro.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/res_type/common/level.h"

#include "runtime/engine.h"
// #include "runtime/function/character/character.h"
#include "runtime/function/framework/object/object.h"
// #include "runtime/function/particle/particle_manager.h"
// #include "runtime/function/physics/physics_manager.h"
// #include "runtime/function/physics/physics_scene.h"
#include <limits>
#include "level.h"

namespace Compass
{
    bool Level::load(const std::string &level_res_url)
    {
        LOG_INFO("loading level: {}", level_res_url);

        m_level_res_url = level_res_url;

        LevelRes level_res;
        
    }

} // namespace Compass
