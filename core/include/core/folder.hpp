#ifndef FOLDER_HPP
#define FOLDER_HPP

#include <string>
#include <cstdint>

namespace safe::core
{

    enum class FolderStatus
    {
        Unlocked,       // Normal folder
        Locked,         // Encrypted .safe archive exists
        Processing,     // Operation in progress
        Error           // Error state
    };

    class Folder
    {
    private:
        int m_id;
        std::wstring m_path;              // Original folder path
        std::wstring m_lockedPath;        // Path to .safe archive (if locked)
        FolderStatus m_status;
        uint64_t m_sizeBytes;
        std::int64_t m_lastModified;
        std::int64_t m_createdAt;

    public:
        Folder();
        Folder(std::wstring  folderPath);

        // Getters
        [[nodiscard]] int GetId() const { return m_id; }
        [[nodiscard]] const std::wstring& GetPath() const { return m_path; }
        [[nodiscard]] const std::wstring& GetLockedPath() const { return m_lockedPath; }
        [[nodiscard]] FolderStatus GetStatus() const { return m_status; }
        [[nodiscard]] uint64_t GetSizeBytes() const { return m_sizeBytes; }
        [[nodiscard]] std::int64_t GetLastModified() const { return m_lastModified; }
        [[nodiscard]] std::int64_t GetCreatedAt() const { return m_createdAt; }

        // Setters
        void SetId(int id) { m_id = id; }
        void SetStatus(FolderStatus status) { m_status = status; }
        void SetLockedPath(const std::wstring& path) { m_lockedPath = path; }
        void SetSizeBytes(uint64_t size) { m_sizeBytes = size; }

        // Operations (Phase 5-6 implementation)
        bool Lock(const std::string& password);
        bool Unlock(const std::string& password);

        // Utilities
        [[nodiscard]] std::wstring GetDisplayName() const;
        [[nodiscard]] bool RefreshMetadata();
        [[nodiscard]] bool IsValid() const;
    };

} // namespace safe::core

#endif
