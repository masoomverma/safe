#ifndef FOLDER_HPP
#define FOLDER_HPP

#include <string>
#include <cstdint>
#include <ctime>

namespace safe::core
{

    enum class FolderStatus
    {
        Unlocked,       // Normal folder
        Locked,         // Encrypted .locked file exists
        Processing,     // Operation in progress
        Error           // Error state
    };

    class Folder
    {
    private:
        int m_id;
        std::wstring m_path;              // Original folder path
        std::wstring m_lockedPath;        // Path to .locked file (if locked)
        FolderStatus m_status;
        uint64_t m_sizeBytes;
        time_t m_lastModified;
        time_t m_createdAt;

    public:
        Folder();
        Folder(const std::wstring& folderPath);

        // Getters
        int GetId() const { return m_id; }
        const std::wstring& GetPath() const { return m_path; }
        const std::wstring& GetLockedPath() const { return m_lockedPath; }
        FolderStatus GetStatus() const { return m_status; }
        uint64_t GetSizeBytes() const { return m_sizeBytes; }
        time_t GetLastModified() const { return m_lastModified; }
        time_t GetCreatedAt() const { return m_createdAt; }

        // Setters
        void SetId(int id) { m_id = id; }
        void SetStatus(FolderStatus status) { m_status = status; }
        void SetLockedPath(const std::wstring& path) { m_lockedPath = path; }
        void SetSizeBytes(uint64_t size) { m_sizeBytes = size; }

        // Operations (Phase 5-6 implementation)
        bool Lock(const std::string& password);
        bool Unlock(const std::string& password);

        // Utilities
        std::wstring GetDisplayName() const;
        bool RefreshMetadata();
        bool IsValid() const;
    };

} // namespace safe::core

#endif
