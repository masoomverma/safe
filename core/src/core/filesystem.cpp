#include "core/include/core/filesystem.hpp"
#include <windows.h>
#include <fstream>
#include <shlobj.h>

// Undefine Windows macros that conflict with our method names
#ifdef CreateDirectory
#undef CreateDirectory
#endif
#ifdef DeleteFile
#undef DeleteFile
#endif
#ifdef RemoveDirectory
#undef RemoveDirectory
#endif

namespace safe {
namespace core {

bool Filesystem::FileExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool Filesystem::DirectoryExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool Filesystem::CreateDirectory(const std::wstring& path) {
    return ::CreateDirectoryW(path.c_str(), nullptr) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool Filesystem::DeleteFile(const std::wstring& path) {
    return ::DeleteFileW(path.c_str()) != 0;
}

std::vector<uint8_t> Filesystem::ReadFile(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    
    return buffer;
}

bool Filesystem::WriteFile(const std::wstring& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

std::wstring Filesystem::GetExecutablePath() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    std::wstring path(buffer);
    return path.substr(0, path.find_last_of(L"\\/"));
}

std::wstring Filesystem::GetAppDataPath() {
    wchar_t buffer[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, buffer))) {
        return std::wstring(buffer) + L"\\Safe";
    }
    return L"";
}

std::wstring Filesystem::JoinPath(const std::wstring& path1, const std::wstring& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    
    wchar_t last = path1.back();
    if (last == L'\\' || last == L'/') {
        return path1 + path2;
    }
    return path1 + L"\\" + path2;
}

// NEW - Phase 3 implementations
std::vector<std::wstring> Filesystem::ListDirectories(const std::wstring& path) {
    std::vector<std::wstring> directories;
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW((path + L"\\*").c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        return directories;
    }
    
    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::wstring name = findData.cFileName;
            if (name != L"." && name != L"..") {
                directories.push_back(JoinPath(path, name));
            }
        }
    } while (FindNextFileW(hFind, &findData) != 0);
    
    FindClose(hFind);
    return directories;
}

uint64_t Filesystem::GetDirectorySize(const std::wstring& path) {
    // Stub implementation - will need recursive calculation
    (void)path;
    return 0; // TODO: Implement recursive size calculation
}

bool Filesystem::CopyDirectory(const std::wstring& src, const std::wstring& dst) {
    // Stub implementation - Phase 5
    (void)src;
    (void)dst;
    return false; // TODO: Implement recursive copy
}

bool Filesystem::RemoveDirectory(const std::wstring& path) {
    return ::RemoveDirectoryW(path.c_str()) != 0;
}

// NEW - Phase 4 implementations
time_t Filesystem::GetLastModifiedTime(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
        return 0;
    }
    
    ULARGE_INTEGER ull;
    ull.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
    
    return static_cast<time_t>(ull.QuadPart / 10000000ULL - 11644473600ULL);
}

time_t Filesystem::GetCreationTime(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
        return 0;
    }
    
    ULARGE_INTEGER ull;
    ull.LowPart = fileInfo.ftCreationTime.dwLowDateTime;
    ull.HighPart = fileInfo.ftCreationTime.dwHighDateTime;
    
    return static_cast<time_t>(ull.QuadPart / 10000000ULL - 11644473600ULL);
}

std::wstring Filesystem::GetFileName(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

std::wstring Filesystem::GetParentPath(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L"";
    }
    return path.substr(0, pos);
}

} // namespace core
} // namespace safe