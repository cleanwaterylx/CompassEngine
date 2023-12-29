#include "runtime/engine.h"

namespace cw_engine
{
    bool g_is_editor_mode {false};
    std::unordered_set<std::string> g_editor_tick_component_types {};

    void cw_engine::CW_Engine::startEngine(const std::string &config_file_path)
    {
        
    }
}