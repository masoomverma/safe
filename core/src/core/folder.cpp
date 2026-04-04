#include "core/folder.hpp"
#include "core/filesystem.hpp"

namespace safe::core {

Folder::Folder()
    : m_id(-1)
    , m_path(L"")
    , m_lockedPath(L"")
    , m_status(FolderStatus::Error)
    , m_sizeBytes(0)
    , m_lastModified(0)
    , m_createdAt(0)
{
}

Folder::Folder(const std::wstring& folderPath)
    : m_id(-1)
    , m_path(folderPath)
    , m_lockedPath(L"")
    , m_status(FolderStatus::Unlocked)
    , m_sizeBytes(0)
    , m_lastModified(0)
    , m_createdAt(0)
{
    RefreshMetadata();
}

bool Folder::Lock(const std::string& password) {
    // Phase 5-6: Will implement compression + encryption
    (void)password; // Unused for now
    return false;
}

bool Folder::Unlock(const std::string& password) {
    // Phase 5-6: Will implement decryption + decompression
    (void)password; // Unused for now
    return false;
}

std::wstring Folder::GetDisplayName() const {
    return Filesystem::GetFileName(m_path);
}

bool Folder::RefreshMetadata() {
    if (!Filesystem::DirectoryExists(m_path)) {
        m_status = FolderStatus::Error;
        return false;
    }
    
    m_sizeBytes = Filesystem::GetDirectorySize(m_path);
    m_lastModified = Filesystem::GetLastModifiedTime(m_path);
    m_createdAt = Filesystem::GetCreationTime(m_path);
    
    // Check if locked version exists
    std::wstring lockedPath = m_path + L".locked";
    if (Filesystem::FileExists(lockedPath)) {
        m_status = FolderStatus::Locked;
        m_lockedPath = lockedPath;
    } else {
        m_status = FolderStatus::Unlocked;
    }
    
    return true;
}

bool Folder::IsValid() const {
    return !m_path.empty() && Filesystem::DirectoryExists(m_path);
}

} // namespace safe::core