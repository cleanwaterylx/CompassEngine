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
        /// @brief ��ȡ���·��
        /// @param directory Ҫ��Ե�Ŀ¼
        /// @param file_path
        /// @return
        static const std::filesystem::path getRelativePath(const std::filesystem::path &directory,
                                                           const std::filesystem::path &file_path);

        /// @brief ��ȡ·��Ƭ��
        /// @param file_path
        /// @return
        static const std::vector<std::string> getPathSegments(const std::filesystem::path &file_path);

        /// @brief ��ȡ��չ��
        /// @param file_path
        /// @return
        static const std::tuple<std::string, std::string, std::string> getFileExtensions(const std::filesystem::path &file_path);

        /// @brief ��ȡ���ļ���
        /// @param file_full_name 
        /// @return 
        static const std::string getFilePureName(const std::string file_full_name);
    };
}
