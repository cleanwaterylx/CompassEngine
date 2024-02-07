#include "engine.h"

namespace Compass
{
    bool g_is_editor_mode{false};
    // editor中注册需要tick的组件
    std::unordered_set<std::string> g_editor_tick_component_types{};

    void CompassEngine::startEngine(const std::string &config_file_path)
    {
        
    }

}
