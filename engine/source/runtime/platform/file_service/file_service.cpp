#include "runtime/platform/file_service/file_service.h"
#include "file_service.h"

namespace Compass
{
    std::vector<std::filesystem::path> Compass::FileSystem::getFiles(const std::filesystem::path &directory)
    {
        std::vector<std::filesystem::path> files;
        for (auto const& directory_entry : std::filesystem::recursive_directory_iterator {directory})
        {
            if (directory_entry.is_regular_file())
            {
                files.push_back(directory_entry);
            }
        }
        return files;
    }

} // namespace Compass
