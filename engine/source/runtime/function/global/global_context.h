#pragma once

#include <memory>
#include <string>

namespace Compass
{
    class LogSystem;
    class InputSystem;
    class PhysicsManager;
    class FileSystem;
    class AssetManager;
    class ConfigManager;
    class WorldManager;
    class RenderSystem;
    class WindowSystem;
    class ParticleManager;
    class DebugDrawManager;
    class RenderDebugConfig;

    struct EngineInitParams;

    // Manage the lifetime global system
    class RuntimeGlobalContext
    {
    public:
        void startSystems(const std::string &config_file_path);
        void shutdownSystems();

    public:
        std::shared_ptr<LogSystem>         m_logger_system;
        std::shared_ptr<InputSystem>       m_input_system;
        std::shared_ptr<FileSystem>        m_file_system;
        std::shared_ptr<AssetManager>      m_asset_manager;
        std::shared_ptr<ConfigManager>     m_config_manager;
        std::shared_ptr<WorldManager>      m_world_manager;
        std::shared_ptr<PhysicsManager>    m_physics_manager;
        std::shared_ptr<WindowSystem>      m_window_system;
        std::shared_ptr<RenderSystem>      m_render_system;
        std::shared_ptr<ParticleManager>   m_particle_manager;
        std::shared_ptr<DebugDrawManager>  m_debugdraw_manager;
        std::shared_ptr<RenderDebugConfig> m_render_debug_config;
    };
    //定义全局管理变量
    extern RuntimeGlobalContext g_runtime_global_context;
}