#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace safe {
namespace core {

class Filesystem {
public:
    // File operations
    static bool FileExists(const std::wstring& path);
    static bool DirectoryExists(const std::wstring& path);
    static bool CreateDirectory(const std::wstring& path);
    static bool DeleteFile(const std::wstring& path);
    
    // Directory operations (NEW - Phase 3)
    static std::vector<std::wstring> ListDirectories(const std::wstring& path);
    static uint64_t GetDirectorySize(const std::wstring& path);
    static bool CopyDirectory(const std::wstring& src, const std::wstring& dst);
    static bool RemoveDirectory(const std::wstring& path);
    
    // Read/Write operations
    static std::vector<uint8_t> ReadFile(const std::wstring& path);
    static bool WriteFile(const std::wstring& path, const std::vector<uint8_t>& data);
    
    // Metadata operations (NEW - Phase 4)
    static time_t GetLastModifiedTime(const std::wstring& path);
    static time_t GetCreationTime(const std::wstring& path);
    
    // Path operations
    static std::wstring GetExecutablePath();
    static std::wstring GetAppDataPath();
    static std::wstring JoinPath(const std::wstring& path1, const std::wstring& path2);
    static std::wstring GetFileName(const std::wstring& path);
    static std::wstring GetParentPath(const std::wstring& path);
};

} // namespace core
} // namespace safe
