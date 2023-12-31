#include "global_context.h"

namespace Compass
{
    void Compass::RuntimeGlobalContext::startSystems(const std::string &config_file_path)
    {
        m_config_manager = std::make_shared<ConfigManager>();
        
    }
}