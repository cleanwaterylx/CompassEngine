#pragma once

#include <filesystem>
#include <vector>

namespace Compass
{
    class FileSystem
    {
    public:
        /// @brief 获取目录下的所有文件名
        /// @param directory 
        /// @return 
        std::vector<std::filesystem::path> getFiles(const std::filesystem::path& directory);
    };
} // namespace Compass
