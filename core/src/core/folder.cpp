#include <utility>
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <system_error>

#include <windows.h>
#include <bcrypt.h>

#include "core/folder.hpp"
#include "core/filesystem.hpp"

namespace safe::core
{
    namespace {
        constexpr std::uint32_t KEY_DERIVATION_ITERATIONS = 120000;
        constexpr std::size_t SALT_SIZE = 16;
        constexpr std::size_t IV_SIZE = 16;
        constexpr std::size_t AES256_KEY_SIZE = 32;
        constexpr std::uint8_t SOURCE_KIND_FILE = 0;
        constexpr std::uint8_t SOURCE_KIND_DIRECTORY = 1;

        std::wstring BuildArchivePath(const std::wstring& basePath) {
            return basePath + L".safe";
        }

        void ClearReadOnlyAttribute(const std::filesystem::path& path) {
            const std::wstring widePath = path.wstring();
            const DWORD attrs = GetFileAttributesW(widePath.c_str());
            if (attrs == INVALID_FILE_ATTRIBUTES) return;
            if ((attrs & FILE_ATTRIBUTE_READONLY) != 0) {
                SetFileAttributesW(widePath.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
            }
        }

        void MakePathDeletable(const std::filesystem::path& path) {
            std::error_code ec;
            if (!std::filesystem::exists(path, ec) || ec) return;

            if (std::filesystem::is_directory(path, ec) && !ec) {
                for (std::filesystem::recursive_directory_iterator it(path, ec), end; it != end && !ec; it.increment(ec)) {
                    if (ec) break;
                    ClearReadOnlyAttribute(it->path());
                }
            }
            ClearReadOnlyAttribute(path);
        }

        void WriteU32(std::vector<uint8_t>& out, std::uint32_t value) {
            out.push_back(static_cast<uint8_t>(value & 0xFFu));
            out.push_back(static_cast<uint8_t>((value >> 8) & 0xFFu));
            out.push_back(static_cast<uint8_t>((value >> 16) & 0xFFu));
            out.push_back(static_cast<uint8_t>((value >> 24) & 0xFFu));
        }

        void WriteU64(std::vector<uint8_t>& out, std::uint64_t value) {
            for (int i = 0; i < 8; ++i) {
                out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFFu));
            }
        }

        bool ReadU32(const std::vector<uint8_t>& in, std::size_t& cursor, std::uint32_t& value) {
            if (cursor + 4 > in.size()) return false;
            value = static_cast<std::uint32_t>(in[cursor])
                | (static_cast<std::uint32_t>(in[cursor + 1]) << 8)
                | (static_cast<std::uint32_t>(in[cursor + 2]) << 16)
                | (static_cast<std::uint32_t>(in[cursor + 3]) << 24);
            cursor += 4;
            return true;
        }

        bool ReadU64(const std::vector<uint8_t>& in, std::size_t& cursor, std::uint64_t& value) {
            if (cursor + 8 > in.size()) return false;
            value = 0;
            for (int i = 0; i < 8; ++i) {
                value |= static_cast<std::uint64_t>(in[cursor + i]) << (8 * i);
            }
            cursor += 8;
            return true;
        }

        bool FillRandom(std::vector<uint8_t>& bytes) {
            return BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
        }

        bool DeriveKey(const std::string& password, const std::vector<uint8_t>& salt, std::vector<uint8_t>& key) {
            BCRYPT_ALG_HANDLE hPrf = nullptr;
            if (BCryptOpenAlgorithmProvider(&hPrf, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0) {
                return false;
            }

            key.resize(AES256_KEY_SIZE);
            const NTSTATUS status = BCryptDeriveKeyPBKDF2(
                hPrf,
                reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())),
                static_cast<ULONG>(password.size()),
                const_cast<PUCHAR>(salt.data()),
                static_cast<ULONG>(salt.size()),
                KEY_DERIVATION_ITERATIONS,
                key.data(),
                static_cast<ULONG>(key.size()),
                0
            );

            BCryptCloseAlgorithmProvider(hPrf, 0);
            return status == 0;
        }

        bool CreateAesKey(BCRYPT_ALG_HANDLE& hAlg, BCRYPT_KEY_HANDLE& hKey, std::vector<uint8_t>& keyObject, const std::vector<uint8_t>& key) {
            hAlg = nullptr;
            hKey = nullptr;
            if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0) != 0) return false;
            if (BCryptSetProperty(
                    hAlg,
                    BCRYPT_CHAINING_MODE,
                    reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_CBC)),
                    static_cast<ULONG>(sizeof(BCRYPT_CHAIN_MODE_CBC)),
                    0) != 0) {
                BCryptCloseAlgorithmProvider(hAlg, 0);
                return false;
            }

            ULONG keyObjectSize = 0;
            ULONG resultSize = 0;
            if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&keyObjectSize), sizeof(keyObjectSize), &resultSize, 0) != 0) {
                BCryptCloseAlgorithmProvider(hAlg, 0);
                return false;
            }

            keyObject.resize(keyObjectSize);
            if (BCryptGenerateSymmetricKey(hAlg, &hKey, keyObject.data(), keyObjectSize, const_cast<PUCHAR>(key.data()), static_cast<ULONG>(key.size()), 0) != 0) {
                BCryptCloseAlgorithmProvider(hAlg, 0);
                return false;
            }
            return true;
        }

        bool EncryptAesCbc(const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv, const std::vector<uint8_t>& plaintext, std::vector<uint8_t>& ciphertext) {
            BCRYPT_ALG_HANDLE hAlg = nullptr;
            BCRYPT_KEY_HANDLE hKey = nullptr;
            std::vector<uint8_t> keyObject;
            if (!CreateAesKey(hAlg, hKey, keyObject, key)) return false;

            std::vector<uint8_t> ivCopy = iv;
            ULONG requiredSize = 0;
            if (BCryptEncrypt(
                    hKey,
                    const_cast<PUCHAR>(plaintext.data()),
                    static_cast<ULONG>(plaintext.size()),
                    nullptr,
                    ivCopy.data(),
                    static_cast<ULONG>(ivCopy.size()),
                    nullptr,
                    0,
                    &requiredSize,
                    BCRYPT_BLOCK_PADDING) != 0) {
                BCryptDestroyKey(hKey);
                BCryptCloseAlgorithmProvider(hAlg, 0);
                return false;
            }

            ciphertext.resize(requiredSize);
            ivCopy = iv;
            const NTSTATUS status = BCryptEncrypt(
                hKey,
                const_cast<PUCHAR>(plaintext.data()),
                static_cast<ULONG>(plaintext.size()),
                nullptr,
                ivCopy.data(),
                static_cast<ULONG>(ivCopy.size()),
                ciphertext.data(),
                requiredSize,
                &requiredSize,
                BCRYPT_BLOCK_PADDING
            );

            BCryptDestroyKey(hKey);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            if (status != 0) return false;
            ciphertext.resize(requiredSize);
            return true;
        }

        bool DecryptAesCbc(const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv, const std::vector<uint8_t>& ciphertext, std::vector<uint8_t>& plaintext) {
            BCRYPT_ALG_HANDLE hAlg = nullptr;
            BCRYPT_KEY_HANDLE hKey = nullptr;
            std::vector<uint8_t> keyObject;
            if (!CreateAesKey(hAlg, hKey, keyObject, key)) return false;

            std::vector<uint8_t> ivCopy = iv;
            ULONG requiredSize = 0;
            if (BCryptDecrypt(
                    hKey,
                    const_cast<PUCHAR>(ciphertext.data()),
                    static_cast<ULONG>(ciphertext.size()),
                    nullptr,
                    ivCopy.data(),
                    static_cast<ULONG>(ivCopy.size()),
                    nullptr,
                    0,
                    &requiredSize,
                    BCRYPT_BLOCK_PADDING) != 0) {
                BCryptDestroyKey(hKey);
                BCryptCloseAlgorithmProvider(hAlg, 0);
                return false;
            }

            plaintext.resize(requiredSize);
            ivCopy = iv;
            const NTSTATUS status = BCryptDecrypt(
                hKey,
                const_cast<PUCHAR>(ciphertext.data()),
                static_cast<ULONG>(ciphertext.size()),
                nullptr,
                ivCopy.data(),
                static_cast<ULONG>(ivCopy.size()),
                plaintext.data(),
                requiredSize,
                &requiredSize,
                BCRYPT_BLOCK_PADDING
            );

            BCryptDestroyKey(hKey);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            if (status != 0) return false;
            plaintext.resize(requiredSize);
            return true;
        }

        bool SerializeInputPath(const std::filesystem::path& inputPath, std::uint8_t sourceKind, std::vector<uint8_t>& archive) {
            std::vector<std::filesystem::path> files;

            if (sourceKind == SOURCE_KIND_DIRECTORY) {
                std::error_code ec;
                if (!std::filesystem::exists(inputPath, ec) || ec) return false;
                if (!std::filesystem::is_directory(inputPath, ec) || ec) return false;

                for (std::filesystem::recursive_directory_iterator it(inputPath, std::filesystem::directory_options::none, ec), end;
                     it != end;
                     it.increment(ec)) {
                    if (ec) return false;
                    if (it->is_regular_file(ec) && !ec) {
                        files.push_back(it->path());
                    } else if (ec) {
                        return false;
                    }
                }
            } else {
                std::error_code ec;
                if (!std::filesystem::exists(inputPath, ec) || ec) return false;
                if (!std::filesystem::is_regular_file(inputPath, ec) || ec) return false;
                files.push_back(inputPath);
            }

            archive.clear();
            archive.insert(archive.end(), {'S', 'A', 'F', 'E', 'A', 'R', 'C', '1'});
            WriteU32(archive, static_cast<std::uint32_t>(files.size()));

            for (const auto& filePath : files) {
                std::u8string relativePath;
                if (sourceKind == SOURCE_KIND_DIRECTORY) {
                    std::error_code relativeError;
                    const std::filesystem::path relative = std::filesystem::relative(filePath, inputPath, relativeError);
                    if (relativeError || relative.empty()) return false;
                    relativePath = relative.generic_u8string();
                } else {
                    relativePath = filePath.filename().generic_u8string();
                }

                std::ifstream input(filePath, std::ios::binary | std::ios::ate);
                if (!input.is_open()) return false;

                const auto length = input.tellg();
                if (length < 0) return false;
                input.seekg(0, std::ios::beg);

                std::vector<uint8_t> fileBytes(static_cast<std::size_t>(length));
                if (length > 0) {
                    input.read(reinterpret_cast<char*>(fileBytes.data()), length);
                    if (!input.good() && !input.eof()) return false;
                }

                WriteU32(archive, static_cast<std::uint32_t>(relativePath.size()));
                for (const char8_t ch : relativePath) {
                    archive.push_back(static_cast<uint8_t>(ch));
                }
                WriteU64(archive, static_cast<std::uint64_t>(fileBytes.size()));
                archive.insert(archive.end(), fileBytes.begin(), fileBytes.end());
            }

            return true;
        }

        bool DeserializeArchive(const std::vector<uint8_t>& archive, const std::filesystem::path& outputPath, std::uint8_t sourceKind) {
            std::size_t cursor = 0;
            if (archive.size() < 12) return false;
            const std::string magic(archive.begin(), archive.begin() + 8);
            if (magic != "SAFEARC1") return false;
            cursor += 8;

            std::uint32_t fileCount = 0;
            if (!ReadU32(archive, cursor, fileCount)) return false;
            if (sourceKind == SOURCE_KIND_FILE && fileCount != 1) return false;

            for (std::uint32_t i = 0; i < fileCount; ++i) {
                std::uint32_t pathLen = 0;
                if (!ReadU32(archive, cursor, pathLen)) return false;
                if (cursor + pathLen > archive.size()) return false;

                std::u8string relativePathUtf8;
                relativePathUtf8.reserve(pathLen);
                for (std::uint32_t offset = 0; offset < pathLen; ++offset) {
                    relativePathUtf8.push_back(static_cast<char8_t>(archive[cursor + offset]));
                }
                cursor += pathLen;

                std::uint64_t fileSize = 0;
                if (!ReadU64(archive, cursor, fileSize)) return false;
                if (cursor + fileSize > archive.size()) return false;

                std::filesystem::path destination;
                if (sourceKind == SOURCE_KIND_DIRECTORY) {
                    destination = outputPath / std::filesystem::path(relativePathUtf8);
                } else {
                    destination = outputPath;
                }

                if (!destination.parent_path().empty()) {
                    std::error_code createError;
                    std::filesystem::create_directories(destination.parent_path(), createError);
                    if (createError) return false;
                }

                std::ofstream output(destination, std::ios::binary | std::ios::trunc);
                if (!output.is_open()) return false;
                if (fileSize > 0) {
                    output.write(reinterpret_cast<const char*>(archive.data() + cursor), static_cast<std::streamsize>(fileSize));
                    if (!output.good()) return false;
                }

                cursor += static_cast<std::size_t>(fileSize);
            }

            return cursor == archive.size();
        }

        bool BuildEncryptedContainer(const std::string& password, std::uint8_t sourceKind, const std::vector<uint8_t>& archive, std::vector<uint8_t>& container) {
            std::vector<uint8_t> salt(SALT_SIZE);
            std::vector<uint8_t> iv(IV_SIZE);
            if (!FillRandom(salt) || !FillRandom(iv)) return false;

            std::vector<uint8_t> key;
            if (!DeriveKey(password, salt, key)) return false;

            std::vector<uint8_t> ciphertext;
            if (!EncryptAesCbc(key, iv, archive, ciphertext)) return false;

            container.clear();
            container.insert(container.end(), {'S', 'A', 'F', 'E', 'E', 'N', 'C', '1'});
            WriteU32(container, 2u);
            container.push_back(sourceKind);
            container.insert(container.end(), salt.begin(), salt.end());
            container.insert(container.end(), iv.begin(), iv.end());
            WriteU64(container, static_cast<std::uint64_t>(ciphertext.size()));
            container.insert(container.end(), ciphertext.begin(), ciphertext.end());
            return true;
        }

        bool ExtractEncryptedContainer(const std::string& password, const std::vector<uint8_t>& container, std::uint8_t& sourceKind, std::vector<uint8_t>& archive) {
            std::size_t cursor = 0;
            if (container.size() < 8 + 4 + SALT_SIZE + IV_SIZE + 8) return false;

            const std::string magic(container.begin(), container.begin() + 8);
            if (magic != "SAFEENC1") return false;
            cursor += 8;

            std::uint32_t version = 0;
            if (!ReadU32(container, cursor, version)) return false;
            if (version != 1u && version != 2u) return false;

            sourceKind = SOURCE_KIND_DIRECTORY;
            if (version == 2u) {
                if (cursor >= container.size()) return false;
                sourceKind = container[cursor++];
                if (sourceKind != SOURCE_KIND_DIRECTORY && sourceKind != SOURCE_KIND_FILE) return false;
            }

            std::vector<uint8_t> salt(container.begin() + static_cast<std::ptrdiff_t>(cursor), container.begin() + static_cast<std::ptrdiff_t>(cursor + SALT_SIZE));
            cursor += SALT_SIZE;
            std::vector<uint8_t> iv(container.begin() + static_cast<std::ptrdiff_t>(cursor), container.begin() + static_cast<std::ptrdiff_t>(cursor + IV_SIZE));
            cursor += IV_SIZE;

            std::uint64_t cipherSize = 0;
            if (!ReadU64(container, cursor, cipherSize)) return false;
            if (cursor + cipherSize > container.size()) return false;

            std::vector<uint8_t> ciphertext(container.begin() + static_cast<std::ptrdiff_t>(cursor), container.begin() + static_cast<std::ptrdiff_t>(cursor + cipherSize));

            std::vector<uint8_t> key;
            if (!DeriveKey(password, salt, key)) return false;
            return DecryptAesCbc(key, iv, ciphertext, archive);
        }
    }

    Folder::Folder()
        : m_id(-1)
        , m_status(FolderStatus::Error)
        , m_sizeBytes(0)
        , m_lastModified(0)
        , m_createdAt(0)
    {
    }

    Folder::Folder(std::wstring folderPath)
        : m_id(-1)
        , m_path(std::move(folderPath))
        , m_status(FolderStatus::Unlocked)
        , m_sizeBytes(0)
        , m_lastModified(0)
        , m_createdAt(0)
    {
        (void)RefreshMetadata();
    }

    bool Folder::Lock(const std::string& password) {
        if (password.empty()) return false;

        const std::filesystem::path inputPath(m_path);
        std::error_code typeError;
        const bool exists = std::filesystem::exists(inputPath, typeError);
        if (typeError || !exists) return false;
        const bool isDirectory = std::filesystem::is_directory(inputPath, typeError);
        if (typeError) return false;
        const bool isFile = std::filesystem::is_regular_file(inputPath, typeError);
        if (typeError) return false;
        if (!isDirectory && !isFile) return false;

        const std::uint8_t sourceKind = isDirectory ? SOURCE_KIND_DIRECTORY : SOURCE_KIND_FILE;

        std::vector<uint8_t> archive;
        if (!SerializeInputPath(inputPath, sourceKind, archive)) return false;

        std::vector<uint8_t> container;
        if (!BuildEncryptedContainer(password, sourceKind, archive, container)) return false;

        const std::wstring archivePath = BuildArchivePath(m_path);
        if (const bool archiveAlreadyExists = Filesystem::FileExists(archivePath); archiveAlreadyExists) {
            // Recover from stale archives left behind by interrupted lock/unlock attempts.
            if (!Filesystem::DeleteFile(archivePath)) {
                m_status = FolderStatus::Error;
                return false;
            }
        }

        m_status = FolderStatus::Processing;
        m_lockedPath = archivePath;
        const std::wstring tempArchivePath = m_lockedPath + L".tmp";

        // Clean up stale temp/output files from interrupted attempts.
        if (Filesystem::FileExists(tempArchivePath)) {
            Filesystem::DeleteFile(tempArchivePath);
        }

        if (!Filesystem::WriteFile(tempArchivePath, container)) {
            m_status = FolderStatus::Error;
            return false;
        }

        if (Filesystem::FileExists(m_lockedPath) && !Filesystem::DeleteFile(m_lockedPath)) {
            Filesystem::DeleteFile(tempArchivePath);
            m_status = FolderStatus::Error;
            return false;
        }

        std::error_code renameError;
        std::filesystem::rename(tempArchivePath, m_lockedPath, renameError);
        if (renameError) {
            Filesystem::DeleteFile(tempArchivePath);
            m_status = FolderStatus::Error;
            return false;
        }

        bool removedOriginal = false;
        if (isDirectory) {
            MakePathDeletable(inputPath);
            std::error_code removeError;
            std::filesystem::remove_all(inputPath, removeError);
            removedOriginal = !removeError;
        } else {
            removedOriginal = Filesystem::DeleteFile(m_path);
        }

        if (!removedOriginal) {
            // Roll back archive if plaintext content could not be removed.
            Filesystem::DeleteFile(m_lockedPath);
            m_lockedPath.clear();
            m_status = FolderStatus::Error;
            return false;
        }

        m_status = FolderStatus::Locked;
        return true;
    }

    bool Folder::Unlock(const std::string& password) {
        if (password.empty()) return false;

        const std::wstring archivePath = m_lockedPath.empty() ? BuildArchivePath(m_path) : m_lockedPath;
        if (!Filesystem::FileExists(archivePath)) return false;

        const std::vector<uint8_t> container = Filesystem::ReadFile(archivePath);
        if (container.empty()) {
            m_status = FolderStatus::Error;
            return false;
        }

        std::uint8_t sourceKind = SOURCE_KIND_DIRECTORY;
        std::vector<uint8_t> archive;
        if (!ExtractEncryptedContainer(password, container, sourceKind, archive)) return false;

        m_status = FolderStatus::Processing;
        const std::filesystem::path outputPath(m_path);
        std::error_code outputStateError;
        const bool outputExists = std::filesystem::exists(outputPath, outputStateError);
        if (outputStateError) {
            m_status = FolderStatus::Error;
            return false;
        }

        if (sourceKind == SOURCE_KIND_DIRECTORY) {
            if (outputExists && !std::filesystem::is_directory(outputPath, outputStateError)) {
                if (outputStateError) {
                    m_status = FolderStatus::Error;
                    return false;
                }
                if (!Filesystem::DeleteFile(outputPath.wstring())) {
                    m_status = FolderStatus::Error;
                    return false;
                }
            }
            std::error_code createError;
            std::filesystem::create_directories(outputPath, createError);
            if (createError) {
                m_status = FolderStatus::Error;
                return false;
            }
        } else {
            if (outputExists && std::filesystem::is_directory(outputPath, outputStateError)) {
                if (outputStateError) {
                    m_status = FolderStatus::Error;
                    return false;
                }
                MakePathDeletable(outputPath);
                std::error_code removeDirError;
                std::filesystem::remove_all(outputPath, removeDirError);
                if (removeDirError) {
                    m_status = FolderStatus::Error;
                    return false;
                }
            }
            if (!outputPath.parent_path().empty()) {
                std::error_code createError;
                std::filesystem::create_directories(outputPath.parent_path(), createError);
                if (createError) {
                    m_status = FolderStatus::Error;
                    return false;
                }
            }
        }

        if (!DeserializeArchive(archive, outputPath, sourceKind)) {
            m_status = FolderStatus::Error;
            return false;
        }

        // Archive cleanup failure should not negate a successful decrypt/restore.
        // The plaintext data is already restored at this point.
        (void)Filesystem::DeleteFile(archivePath);

        m_lockedPath.clear();
        m_status = FolderStatus::Unlocked;
        return true;
    }

    std::wstring Folder::GetDisplayName() const {
        return Filesystem::GetFileName(m_path);
    }

    bool Folder::RefreshMetadata() {
        const std::wstring archivePath = BuildArchivePath(m_path);
        if (Filesystem::FileExists(archivePath)) {
            m_status = FolderStatus::Locked;
            m_lockedPath = archivePath;
            const std::filesystem::path archiveFsPath(archivePath);
            std::error_code sizeError;
            m_sizeBytes = std::filesystem::file_size(archiveFsPath, sizeError);
            if (sizeError) m_sizeBytes = 0;
            m_lastModified = static_cast<std::int64_t>(Filesystem::GetLastModifiedTime(archivePath));
            m_createdAt = static_cast<std::int64_t>(Filesystem::GetCreationTime(archivePath));
            return true;
        }

        if (Filesystem::DirectoryExists(m_path)) {
            m_sizeBytes = Filesystem::GetDirectorySize(m_path);
            m_lastModified = static_cast<std::int64_t>(Filesystem::GetLastModifiedTime(m_path));
            m_createdAt = static_cast<std::int64_t>(Filesystem::GetCreationTime(m_path));
            m_lockedPath.clear();
            m_status = FolderStatus::Unlocked;
            return true;
        }

        if (Filesystem::FileExists(m_path)) {
            m_sizeBytes = Filesystem::GetFileSize(m_path);
            m_lastModified = static_cast<std::int64_t>(Filesystem::GetLastModifiedTime(m_path));
            m_createdAt = static_cast<std::int64_t>(Filesystem::GetCreationTime(m_path));
            m_lockedPath.clear();
            m_status = FolderStatus::Unlocked;
            return true;
        }

        m_status = FolderStatus::Error;
        return false;
    }

    bool Folder::IsValid() const {
        if (m_path.empty()) return false;
        if (Filesystem::DirectoryExists(m_path)) return true;
        if (Filesystem::FileExists(m_path)) return true;
        const std::wstring archivePath = BuildArchivePath(m_path);
        if (Filesystem::FileExists(archivePath)) return true;
        if (!m_lockedPath.empty() && Filesystem::FileExists(m_lockedPath)) return true;
        return false;
    }

} // namespace safe::core
