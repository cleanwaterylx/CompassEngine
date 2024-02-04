#pragma once

#include <filesystem>
#include <vector>

namespace Compass
{
    class FileSystem
    {
    public:
        /// @brief ��ȡĿ¼�µ������ļ���
        /// @param directory 
        /// @return 
        std::vector<std::filesystem::path> getFiles(const std::filesystem::path& directory);
    };
} // namespace Compass
