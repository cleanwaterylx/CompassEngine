#include "global_context.h"

#include "runtime/core/log/log_system.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/platform/file_service/file_service.h"


namespace Compass
{
    void Compass::RuntimeGlobalContext::startSystems(const std::string &config_file_path)
    {
        m_config_manager = std::make_shared<ConfigManager>();
        m_config_manager->initialize(config_file_path);

        m_asset_manager = std::make_shared<AssetManager>();

        m_file_system = std::make_shared<FileSystem>();

        m_logger_system = std::make_shared<LogSystem>();

        // todo world_manager
        







    }
}