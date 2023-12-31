#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace Compass
{
    extern bool g_is_editor_mode;
    extern std::unordered_set<std::string> g_editor_tick_component_types; // 需要tick的compoent类型

    class CompassEngine
    {
        friend class CompassEditor;

        static const float s_fps_aplha;

    public:
        void startEngine(const std::string& config_file_path);
        void shutdownEngine();

        void initialize();
        void clear();
        
        bool isQuit() const {return m_is_quit;};
        void run();
        bool tickOneFrame(float delta_time);

        int getFPS() const {return m_fps;}

    protected:
        void logicalTick(float delta_time);
        bool renderTick(float delta_time);

        void calculateFPS(float calculateFPS);
        float calculateDeltaTime();

    protected:
        bool m_is_quit {false};

        std::chrono::steady_clock::time_point m_last_tick_time_point {std::chrono::steady_clock::now()};

        float m_average_duration {0.f};
        int m_frame_count {0};
        int m_fps {0};
    };
}