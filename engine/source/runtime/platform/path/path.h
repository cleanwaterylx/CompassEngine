#pragma once

#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

namespace Compass
{
    class Path
    {
    public:
        /// @brief 获取相对路径
        /// @param directory 要相对的目录
        /// @param file_path
        /// @return
        static const std::filesystem::path getRelativePath(const std::filesystem::path &directory,
                                                           const std::filesystem::path &file_path);

        /// @brief 获取路径片段
        /// @param file_path
        /// @return
        static const std::vector<std::string> getPathSegments(const std::filesystem::path &file_path);

        /// @brief 获取扩展名
        /// @param file_path
        /// @return
        static const std::tuple<std::string, std::string, std::string> getFileExtensions(const std::filesystem::path &file_path);

        /// @brief 获取纯文件名
        /// @param file_full_name 
        /// @return 
        static const std::string getFilePureName(const std::string file_full_name);
    };
}
