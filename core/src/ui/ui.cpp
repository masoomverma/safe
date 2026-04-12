#include "ui/ui.hpp"
#include "core/filesystem.hpp"
#include "core/folder.hpp"
#include "sqlite3.h"

#include "imgui.h"

#include <windows.h>
#include <shlobj.h>
#include <bcrypt.h>

// Avoid Windows API macro collisions with core::Filesystem methods.
#ifdef CreateDirectory
#undef CreateDirectory
#endif
#ifdef DeleteFile
#undef DeleteFile
#endif
#ifdef RemoveDirectory
#undef RemoveDirectory
#endif

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <cstdint>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <ranges>

namespace safe::ui
{
    namespace {
        constexpr float TOPBAR_HEIGHT = 41.0f;
        constexpr float ROOTPATHBAR_HEIGHT = 28.0f;
        constexpr float STATUSBAR_HEIGHT = 30.0f;
        constexpr float SIDEBAR_WIDTH = 200.0f;
        constexpr float BUTTON_WIDTH = 80.0f;
        constexpr float SELECT_BUTTON_WIDTH = 90.0f;
        constexpr float BUTTON_SPACING = 10.0f;
        constexpr float SEARCH_WIDTH = 220.0f;
        constexpr size_t SEARCH_BUFFER_SIZE = 128;
        constexpr size_t PASSWORD_BUFFER_SIZE = 128;
        constexpr int DB_SCHEMA_VERSION = 5;
        constexpr int DB_DEFAULT_CRYPTO_VERSION = 1;
        constexpr int DB_DEFAULT_KDF_ITERATIONS = 120000;
        constexpr size_t PASSWORD_SALT_SIZE = 16;
        constexpr size_t PASSWORD_VERIFIER_SIZE = 32;
    }

    struct Item {
        std::string id;
        std::wstring path;       // Logical path for lock/unlock operations.
        std::wstring sourcePath; // Physical scanned path on disk.
        std::string name;
        bool isFolder{};
        bool isLocked{};
        uint64_t sizeBytes{};
        time_t lastModified{};
        std::string passwordHash;
        std::string passwordSalt;
        int kdfIterations{DB_DEFAULT_KDF_ITERATIONS};
    };

    struct PersistedState {
        bool found{false};
        bool isFolder{true};
        std::string passwordHash;
        std::string passwordSalt;
        int kdfIterations{DB_DEFAULT_KDF_ITERATIONS};
    };

    static std::vector<Item> items;
    static std::unordered_set<std::string> selectedItemIds;
    static std::string focusedItemId;
    static bool multiSelectMode = false;
    static size_t selectionAnchorIndex = 0;
    static bool hasSelectionAnchor = false;

    static std::wstring openedRootPath;
    static char searchBuffer[SEARCH_BUFFER_SIZE] = "";
    static std::string statusMessage = "Ready";
    static bool showPasswordPopup = false;
    static bool passwordPopupNeedsOpen = false;
    static bool passwordModeIsLock = true;
    static char passwordBuffer[PASSWORD_BUFFER_SIZE] = "";
    static bool passwordInputNeedsFocus = false;
    static double passwordRevealUntil = 0.0;
    static std::string passwordPopupError;
    static std::vector<std::string> passwordPopupTargetItemIds;
    static std::string unlockPasswordMismatchMessage;
    static std::string openedRootSignature;
    static double lastAutoRefreshAtSeconds = 0.0;
    constexpr double AUTO_REFRESH_INTERVAL_SECONDS = 1.0;

    static sqlite3* g_db = nullptr;

    static std::string ToLower(const std::string& str);
    static std::string WideToUtf8(const std::wstring& wstr);
    static std::wstring Utf8ToWide(const std::string& str);
    static std::string FormatBytes(uint64_t bytes);
    static std::string FormatDateTime(time_t timestamp);
    static bool GenerateRandomBytes(std::vector<uint8_t>& bytes);
    static bool DerivePasswordVerifier(const std::string& password, const std::vector<uint8_t>& salt, int iterations, std::vector<uint8_t>& verifierOut);
    static std::string BytesToHex(const std::vector<uint8_t>& bytes);
    static bool HexToBytes(const std::string& hex, std::vector<uint8_t>& bytes);
    static bool ConstantTimeEqual(const std::vector<uint8_t>& lhs, const std::vector<uint8_t>& rhs);
    static std::string HashString(const std::string& value);
    static std::string BuildStableItemId(const std::wstring& path);
    static std::vector<std::wstring> ListSafeArchives(const std::wstring& rootPath);
    static bool HasSafeArchiveExtension(const std::wstring& fileName);
    static std::wstring StripSafeArchiveExtension(const std::wstring& archivePath);
    static bool IsValidIndex(size_t index);
    static const Item* FindItemById(const std::string& id);
    static std::wstring OpenFolderDialog();
    static bool InitializePersistence();
    static void ShutdownPersistence();
    static bool RunMigrations();
    static int GetSchemaVersion();
    static bool SetSchemaVersion(int version);
    static bool HasColumn(const char* tableName, const char* columnName);
    static bool RunMigrationV1();
    static bool RunMigrationV2();
    static bool RunMigrationV3();
    static bool RunMigrationV4();
    static bool RunMigrationV5();
    static std::wstring LoadLastOpenedRootPath();
    static bool SaveLastOpenedRootPath(const std::wstring& rootPath);
    static PersistedState ReadPersistedState(const std::string& itemId, const std::wstring& path);
    static bool UpsertPersistedState(const Item& item);
    static bool LoadItemsFromPath(const std::wstring& rootPath);
    static void ResetSelection();
    static std::vector<size_t> GetSelectedIndices();
    static std::vector<size_t> GetOperationIndices();
    static std::vector<size_t> ResolveIndicesForItemIds(const std::vector<std::string>& itemIds);
    static bool ItemMatchesFilter(const Item& item, const std::string& lowerSearchText, bool hasSearchFilter);
    static void OpenPasswordPopup(bool forLockOperation);
    static void ClosePasswordPopupAsCancelled();

    static void RenderMainLayout();
    static void RenderTopBar();
    static void RenderMainPanel();
    static void RenderStatusBar();
    static void RenderFolderList();
    static void RenderFolderDetails();
    static void RenderPasswordPopup();
    static void PerformLockOperation();
    static void PerformUnlockOperation();
    static bool ApplyLockOperation(const std::string& password);
    static bool ApplyUnlockOperation(const std::string& password);
    static std::string BuildRootSnapshotSignature(const std::wstring& rootPath);
    static void TryAutoRefreshOpenedRoot();

    bool UI::s_initialized = false;

    bool UI::Initialize() {
        if (!InitializePersistence()) {
            statusMessage = "Failed to initialize metadata storage";
            return false;
        }

        bool loaded = false;
        const std::wstring lastRootPath = LoadLastOpenedRootPath();
        if (!lastRootPath.empty()) {
            loaded = LoadItemsFromPath(lastRootPath);
        }

        if (!loaded) {
            statusMessage = "Open a folder to start";
        }
        s_initialized = true;
        return true;
    }

    void UI::Render() {
        if (!s_initialized) {
            return;
        }
        TryAutoRefreshOpenedRoot();
        RenderMainLayout();
    }

    void UI::Cleanup() {
        s_initialized = false;
        ResetSelection();
        items.clear();
        openedRootSignature.clear();
        lastAutoRefreshAtSeconds = 0.0;
        ShutdownPersistence();
    }

    static std::string ToLower(const std::string& str) {
        std::string result = str;
        std::ranges::transform(result, result.begin(),
                               [](const unsigned char c) {
                                   if (c >= 'A' && c <= 'Z') {
                                       return static_cast<char>(c - 'A' + 'a');
                                   }
                                   return static_cast<char>(c);
                               });
        return result;
    }

    static std::string WideToUtf8(const std::wstring& wstr) {
        if (wstr.empty()) return {};
        const int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string out(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), out.data(), size, nullptr, nullptr);
        return out;
    }

    static std::wstring Utf8ToWide(const std::string& str) {
        if (str.empty()) return {};
        const int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
        if (size <= 0) return {};
        std::wstring out(static_cast<size_t>(size), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), out.data(), size);
        return out;
    }

    static std::string FormatBytes(uint64_t bytes) {
        constexpr double KiB = 1024.0;
        constexpr double MiB = KiB * 1024.0;
        constexpr double GiB = MiB * 1024.0;

        std::ostringstream out;
        out << std::fixed << std::setprecision(2);
        if (bytes >= static_cast<uint64_t>(GiB)) {
            out << (static_cast<double>(bytes) / GiB) << " GiB";
        } else if (bytes >= static_cast<uint64_t>(MiB)) {
            out << (static_cast<double>(bytes) / MiB) << " MiB";
        } else if (bytes >= static_cast<uint64_t>(KiB)) {
            out << (static_cast<double>(bytes) / KiB) << " KiB";
        } else {
            out.str("");
            out.clear();
            out << bytes << " B";
        }
        return out.str();
    }

    static std::string FormatDateTime(time_t timestamp) {
        if (timestamp <= 0) return "--";

        std::tm tmLocal{};
        if (localtime_s(&tmLocal, &timestamp) != 0) {
            return "--";
        }

        std::ostringstream out;
        out << std::put_time(&tmLocal, "%Y-%m-%d %H:%M:%S");
        return out.str();
    }

    static bool GenerateRandomBytes(std::vector<uint8_t>& bytes) {
        if (bytes.empty()) return true;
        return BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
    }

    static bool DerivePasswordVerifier(const std::string& password, const std::vector<uint8_t>& salt, int iterations, std::vector<uint8_t>& verifierOut) {
        BCRYPT_ALG_HANDLE hPrf = nullptr;
        if (BCryptOpenAlgorithmProvider(&hPrf, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0) {
            return false;
        }

        verifierOut.assign(PASSWORD_VERIFIER_SIZE, 0);
        const NTSTATUS status = BCryptDeriveKeyPBKDF2(
            hPrf,
            reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())),
            static_cast<ULONG>(password.size()),
            const_cast<PUCHAR>(salt.data()),
            static_cast<ULONG>(salt.size()),
            static_cast<ULONGLONG>(iterations),
            verifierOut.data(),
            static_cast<ULONG>(verifierOut.size()),
            0
        );

        BCryptCloseAlgorithmProvider(hPrf, 0);
        return status == 0;
    }

    static std::string BytesToHex(const std::vector<uint8_t>& bytes) {
        static constexpr char kHexChars[] = "0123456789abcdef";
        std::string hex;
        hex.resize(bytes.size() * 2);
        for (size_t i = 0; i < bytes.size(); ++i) {
            hex[2 * i] = kHexChars[(bytes[i] >> 4) & 0x0F];
            hex[2 * i + 1] = kHexChars[bytes[i] & 0x0F];
        }
        return hex;
    }

    static bool HexToBytes(const std::string& hex, std::vector<uint8_t>& bytes) {
        if ((hex.size() % 2) != 0) return false;
        auto hexValue = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
            if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
            return -1;
        };

        bytes.clear();
        bytes.reserve(hex.size() / 2);
        for (size_t i = 0; i < hex.size(); i += 2) {
            const int hi = hexValue(hex[i]);
            const int lo = hexValue(hex[i + 1]);
            if (hi < 0 || lo < 0) return false;
            bytes.push_back(static_cast<uint8_t>((hi << 4) | lo));
        }
        return true;
    }

    static bool ConstantTimeEqual(const std::vector<uint8_t>& lhs, const std::vector<uint8_t>& rhs) {
        if (lhs.size() != rhs.size()) return false;
        uint8_t diff = 0;
        for (size_t i = 0; i < lhs.size(); ++i) {
            diff |= static_cast<uint8_t>(lhs[i] ^ rhs[i]);
        }
        return diff == 0;
    }

    static std::string HashString(const std::string& value) {
        // FNV-1a 64-bit
        uint64_t hash = 14695981039346656037ULL;
        for (const unsigned char ch : value) {
            hash ^= ch;
            hash *= 1099511628211ULL;
        }
        std::ostringstream out;
        out << std::hex << std::setfill('0') << std::setw(16) << hash;
        return out.str();
    }

    static std::string BuildStableItemId(const std::wstring& path) {
        return HashString(WideToUtf8(path));
    }

    static std::vector<std::wstring> ListSafeArchives(const std::wstring& rootPath) {
        std::vector<std::wstring> archives;
        WIN32_FIND_DATAW findData;
        const std::wstring pattern = core::Filesystem::JoinPath(rootPath, L"*.safe");
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE) {
            return archives;
        }

        do {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;
            archives.push_back(core::Filesystem::JoinPath(rootPath, findData.cFileName));
        } while (FindNextFileW(hFind, &findData) != 0);

        FindClose(hFind);
        return archives;
    }

    static bool HasSafeArchiveExtension(const std::wstring& fileName) {
        constexpr const wchar_t* extension = L".safe";
        constexpr size_t extensionLength = 5;
        if (fileName.size() <= extensionLength) return false;
        return fileName.compare(fileName.size() - extensionLength, extensionLength, extension) == 0;
    }

    static std::wstring StripSafeArchiveExtension(const std::wstring& archivePath) {
        if (!HasSafeArchiveExtension(archivePath)) return archivePath;
        return archivePath.substr(0, archivePath.size() - 5);
    }

    static std::string BuildRootSnapshotSignature(const std::wstring& rootPath) {
        if (rootPath.empty() || !core::Filesystem::DirectoryExists(rootPath)) {
            return {};
        }

        std::vector<std::string> entries;
        const auto directories = core::Filesystem::ListDirectories(rootPath);
        const auto files = core::Filesystem::ListFiles(rootPath);
        const auto archives = ListSafeArchives(rootPath);
        entries.reserve(directories.size() + files.size() + archives.size());

        for (const auto& directoryPath : directories) {
            entries.push_back(
                "D|" + WideToUtf8(directoryPath) + "|" +
                std::to_string(core::Filesystem::GetLastModifiedTime(directoryPath))
            );
        }

        for (const auto& filePath : files) {
            if (HasSafeArchiveExtension(filePath)) {
                continue;
            }
            entries.push_back(
                "F|" + WideToUtf8(filePath) + "|" +
                std::to_string(core::Filesystem::GetLastModifiedTime(filePath)) + "|" +
                std::to_string(core::Filesystem::GetFileSize(filePath))
            );
        }

        for (const auto& archivePath : archives) {
            const std::wstring logicalPath = StripSafeArchiveExtension(archivePath);
            entries.push_back(
                "A|" + WideToUtf8(logicalPath) + "|" +
                std::to_string(core::Filesystem::GetLastModifiedTime(archivePath)) + "|" +
                std::to_string(core::Filesystem::GetFileSize(archivePath))
            );
        }

        std::ranges::sort(entries);
        std::ostringstream joined;
        for (const auto& entry : entries) {
            joined << entry << '\n';
        }
        return HashString(joined.str());
    }

    static bool IsValidIndex(size_t index) {
        return index < items.size();
    }

    static const Item* FindItemById(const std::string& id) {
        auto it = std::ranges::find_if(items, [&](const Item& item) { return item.id == id; });
        return it == items.end() ? nullptr : &(*it);
    }

    static std::wstring OpenFolderDialog() {
        BROWSEINFOW bi = {};
        bi.lpszTitle = L"Select folder to open";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_NEWDIALOGSTYLE;
        PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
        if (!pidl) return L"";

        wchar_t path[MAX_PATH] = {0};
        const bool ok = SHGetPathFromIDListW(pidl, path) != 0;

        IMalloc* allocator = nullptr;
        if (SUCCEEDED(SHGetMalloc(&allocator)) && allocator) {
            allocator->Free(pidl);
            allocator->Release();
        }

        return ok ? std::wstring(path) : L"";
    }

    static bool InitializePersistence() {
        if (g_db) return true;

        const std::wstring appDataPath = core::Filesystem::GetAppDataPath();
        if (appDataPath.empty()) return false;
        if (!core::Filesystem::DirectoryExists(appDataPath) && !(core::Filesystem::CreateDirectory)(appDataPath)) {
            return false;
        }

        const std::wstring dbPath = core::Filesystem::JoinPath(appDataPath, L"safe.db");
        const std::string dbPathUtf8 = WideToUtf8(dbPath);
        if (sqlite3_open(dbPathUtf8.c_str(), &g_db) != SQLITE_OK) {
            if (g_db) {
                sqlite3_close(g_db);
                g_db = nullptr;
            }
            return false;
        }
        return RunMigrations();
    }

    static void ShutdownPersistence() {
        if (g_db) {
            sqlite3_close(g_db);
            g_db = nullptr;
        }
    }

    static int GetSchemaVersion() {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(g_db, "PRAGMA user_version;", -1, &stmt, nullptr) != SQLITE_OK) {
            return -1;
        }

        int version = -1;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            version = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        return version;
    }

    static bool SetSchemaVersion(int version) {
        const std::string sql = "PRAGMA user_version = " + std::to_string(version) + ";";
        return sqlite3_exec(g_db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
    }

    static bool HasColumn(const char* tableName, const char* columnName) {
        const std::string pragma = "PRAGMA table_info(" + std::string(tableName) + ");";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(g_db, pragma.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        bool found = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* currentName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (currentName && std::string(currentName) == columnName) {
                found = true;
                break;
            }
        }
        sqlite3_finalize(stmt);
        return found;
    }

    static bool RunMigrationV1() {
        static constexpr const char* sql =
            "CREATE TABLE IF NOT EXISTS item_metadata ("
            "item_id TEXT PRIMARY KEY,"
            "path TEXT NOT NULL UNIQUE,"
            "is_locked INTEGER NOT NULL DEFAULT 0,"
            "password_hash TEXT,"
            "updated_at INTEGER NOT NULL"
            ");";
        if (sqlite3_exec(g_db, sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
            return false;
        }
        return SetSchemaVersion(1);
    }

    static bool RunMigrationV2() {
        if (!HasColumn("item_metadata", "crypto_version")) {
            static constexpr const char* addCryptoVersion =
                "ALTER TABLE item_metadata ADD COLUMN crypto_version INTEGER NOT NULL DEFAULT 1;";
            if (sqlite3_exec(g_db, addCryptoVersion, nullptr, nullptr, nullptr) != SQLITE_OK) {
                return false;
            }
        }

        if (!HasColumn("item_metadata", "kdf_iterations")) {
            static constexpr const char* addKdfIterations =
                "ALTER TABLE item_metadata ADD COLUMN kdf_iterations INTEGER NOT NULL DEFAULT 120000;";
            if (sqlite3_exec(g_db, addKdfIterations, nullptr, nullptr, nullptr) != SQLITE_OK) {
                return false;
            }
        }

        return SetSchemaVersion(2);
    }

    static bool RunMigrationV3() {
        if (!HasColumn("item_metadata", "password_salt")) {
            static constexpr const char* addPasswordSalt =
                "ALTER TABLE item_metadata ADD COLUMN password_salt TEXT NOT NULL DEFAULT '';";
            if (sqlite3_exec(g_db, addPasswordSalt, nullptr, nullptr, nullptr) != SQLITE_OK) {
                return false;
            }
        }
        return SetSchemaVersion(3);
    }

    static bool RunMigrationV4() {
        if (!HasColumn("item_metadata", "item_kind")) {
            static constexpr const char* addItemKind =
                "ALTER TABLE item_metadata ADD COLUMN item_kind INTEGER NOT NULL DEFAULT 1;";
            if (sqlite3_exec(g_db, addItemKind, nullptr, nullptr, nullptr) != SQLITE_OK) {
                return false;
            }
        }
        return SetSchemaVersion(4);
    }

    static bool RunMigrationV5() {
        static constexpr const char* sql =
            "CREATE TABLE IF NOT EXISTS app_state ("
            "state_key TEXT PRIMARY KEY,"
            "state_value TEXT NOT NULL"
            ");";
        if (sqlite3_exec(g_db, sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
            return false;
        }
        return SetSchemaVersion(5);
    }

    static bool RunMigrations() {
        int version = GetSchemaVersion();
        if (version < 0) return false;

        if (sqlite3_exec(g_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            return false;
        }

        bool ok = true;
        if (version < 1) {
            ok = RunMigrationV1();
            version = ok ? 1 : version;
        }
        if (ok && version < 2) {
            ok = RunMigrationV2();
            version = ok ? 2 : version;
        }
        if (ok && version < 3) {
            ok = RunMigrationV3();
            version = ok ? 3 : version;
        }
        if (ok && version < 4) {
            ok = RunMigrationV4();
            version = ok ? 4 : version;
        }
        if (ok && version < 5) {
            ok = RunMigrationV5();
            version = ok ? 5 : version;
        }
        if (ok && version != DB_SCHEMA_VERSION) {
            ok = SetSchemaVersion(DB_SCHEMA_VERSION);
        }

        if (ok) {
            ok = sqlite3_exec(g_db, "COMMIT;", nullptr, nullptr, nullptr) == SQLITE_OK;
        } else {
            sqlite3_exec(g_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        }

        return ok;
    }

    static PersistedState ReadPersistedState(const std::string& itemId, const std::wstring& path) {
        PersistedState state;
        if (!g_db) return state;

        static constexpr const char* sql =
            "SELECT COALESCE(item_kind, 1), COALESCE(password_hash, ''), COALESCE(password_salt, ''), COALESCE(kdf_iterations, 120000) "
            "FROM item_metadata WHERE item_id = ?1 OR path = ?2 LIMIT 1;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return state;

        const std::string pathUtf8 = WideToUtf8(path);
        sqlite3_bind_text(stmt, 1, itemId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, pathUtf8.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            state.found = true;
            state.isFolder = sqlite3_column_int(stmt, 0) != 0;
            const auto* hashText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            state.passwordHash = hashText ? std::string(hashText) : std::string{};
            const auto* saltText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            state.passwordSalt = saltText ? std::string(saltText) : std::string{};
            state.kdfIterations = sqlite3_column_int(stmt, 3);
            if (state.kdfIterations <= 0) {
                state.kdfIterations = DB_DEFAULT_KDF_ITERATIONS;
            }
        }

        sqlite3_finalize(stmt);
        return state;
    }

    static bool UpsertPersistedState(const Item& item) {
        if (!g_db) return false;
        static constexpr const char* cleanupSql =
            "DELETE FROM item_metadata WHERE path = ?1 AND item_id != ?2;";
        sqlite3_stmt* cleanupStmt = nullptr;
        if (sqlite3_prepare_v2(g_db, cleanupSql, -1, &cleanupStmt, nullptr) != SQLITE_OK) return false;
        const std::string pathUtf8 = WideToUtf8(item.path);
        sqlite3_bind_text(cleanupStmt, 1, pathUtf8.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(cleanupStmt, 2, item.id.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(cleanupStmt) != SQLITE_DONE) {
            sqlite3_finalize(cleanupStmt);
            return false;
        }
        sqlite3_finalize(cleanupStmt);

        static constexpr const char* sql =
            "INSERT INTO item_metadata(item_id, path, is_locked, item_kind, password_hash, password_salt, updated_at, crypto_version, kdf_iterations) "
            "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9) "
            "ON CONFLICT(item_id) DO UPDATE SET "
            "path=excluded.path, is_locked=excluded.is_locked, item_kind=excluded.item_kind, password_hash=excluded.password_hash, password_salt=excluded.password_salt, "
            "updated_at=excluded.updated_at, crypto_version=excluded.crypto_version, kdf_iterations=excluded.kdf_iterations;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

        sqlite3_bind_text(stmt, 1, item.id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, pathUtf8.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, item.isLocked ? 1 : 0);
        sqlite3_bind_int(stmt, 4, item.isFolder ? 1 : 0);
        sqlite3_bind_text(stmt, 5, item.passwordHash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, item.passwordSalt.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 7, static_cast<sqlite3_int64>(std::time(nullptr)));
        sqlite3_bind_int(stmt, 8, DB_DEFAULT_CRYPTO_VERSION);
        sqlite3_bind_int(stmt, 9, item.kdfIterations > 0 ? item.kdfIterations : DB_DEFAULT_KDF_ITERATIONS);

        const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return ok;
    }

    static std::wstring LoadLastOpenedRootPath() {
        if (!g_db) return L"";

        static constexpr const char* sql =
            "SELECT state_value FROM app_state WHERE state_key = ?1 LIMIT 1;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return L"";
        }

        sqlite3_bind_text(stmt, 1, "last_opened_root", -1, SQLITE_STATIC);
        std::wstring rootPath;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            if (const auto* valueText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); valueText) {
                rootPath = Utf8ToWide(std::string(valueText));
            }
        }
        sqlite3_finalize(stmt);
        return rootPath;
    }

    static bool SaveLastOpenedRootPath(const std::wstring& rootPath) {
        if (!g_db || rootPath.empty()) return false;

        static constexpr const char* sql =
            "INSERT INTO app_state(state_key, state_value) VALUES(?1, ?2) "
            "ON CONFLICT(state_key) DO UPDATE SET state_value = excluded.state_value;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        const std::string rootPathUtf8 = WideToUtf8(rootPath);
        sqlite3_bind_text(stmt, 1, "last_opened_root", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, rootPathUtf8.c_str(), -1, SQLITE_TRANSIENT);

        const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return ok;
    }

    static bool LoadItemsFromPath(const std::wstring& rootPath) {
        if (!core::Filesystem::DirectoryExists(rootPath)) return false;

        std::unordered_map<std::string, Item> loadedById;
        const auto directories = core::Filesystem::ListDirectories(rootPath);
        const auto files = core::Filesystem::ListFiles(rootPath);
        const auto archives = ListSafeArchives(rootPath);
        loadedById.reserve(directories.size() + files.size() + archives.size());

        for (const auto& directoryPath : directories) {
            Item item;
            item.path = directoryPath;
            item.sourcePath = directoryPath;
            item.id = BuildStableItemId(directoryPath);
            item.name = WideToUtf8(core::Filesystem::GetFileName(directoryPath));
            item.isFolder = true;
            item.isLocked = false;
            item.sizeBytes = core::Filesystem::GetDirectorySize(directoryPath);
            item.lastModified = core::Filesystem::GetLastModifiedTime(directoryPath);

            const PersistedState persisted = ReadPersistedState(item.id, item.path);
            item.passwordHash = persisted.passwordHash;
            item.passwordSalt = persisted.passwordSalt;
            item.kdfIterations = persisted.kdfIterations;
            loadedById[item.id] = std::move(item);
        }

        for (const auto& filePath : files) {
            if (HasSafeArchiveExtension(filePath)) {
                continue;
            }

            Item item;
            item.path = filePath;
            item.sourcePath = filePath;
            item.id = BuildStableItemId(filePath);
            item.name = WideToUtf8(core::Filesystem::GetFileName(filePath));
            item.isFolder = false;
            item.isLocked = false;
            item.sizeBytes = core::Filesystem::GetFileSize(filePath);
            item.lastModified = core::Filesystem::GetLastModifiedTime(filePath);
            const PersistedState persisted = ReadPersistedState(item.id, item.path);
            item.passwordHash = persisted.passwordHash;
            item.passwordSalt = persisted.passwordSalt;
            item.kdfIterations = persisted.kdfIterations;
            loadedById[item.id] = std::move(item);
        }

        for (const auto& archivePath : archives) {
            const std::wstring logicalPath = StripSafeArchiveExtension(archivePath);
            Item item;
            item.path = logicalPath;
            item.sourcePath = archivePath;
            item.id = BuildStableItemId(logicalPath);
            if (loadedById.contains(item.id)) {
                // Prefer plaintext entries when both plaintext and .safe exist
                // (e.g., archive cleanup failure after successful unlock).
                continue;
            }
            item.name = WideToUtf8(core::Filesystem::GetFileName(logicalPath));
            item.isFolder = true;
            item.isLocked = true;
            item.sizeBytes = core::Filesystem::GetFileSize(archivePath);
            item.lastModified = core::Filesystem::GetLastModifiedTime(archivePath);

            const PersistedState persisted = ReadPersistedState(item.id, item.path);
            if (persisted.found) {
                item.isFolder = persisted.isFolder;
                item.passwordHash = persisted.passwordHash;
                item.passwordSalt = persisted.passwordSalt;
                item.kdfIterations = persisted.kdfIterations;
            }
            loadedById[item.id] = std::move(item);
        }

        std::vector<Item> loaded;
        loaded.reserve(loadedById.size());
        for (auto& [id, item] : loadedById) {
            (void)id;
            loaded.push_back(std::move(item));
        }

        std::ranges::sort(loaded, [](const Item& a, const Item& b) { return a.name < b.name; });
        items = std::move(loaded);
        openedRootPath = rootPath;
        openedRootSignature = BuildRootSnapshotSignature(rootPath);
        lastAutoRefreshAtSeconds = ImGui::GetTime();
        ResetSelection();

        for (const Item& item : items) {
            UpsertPersistedState(item);
        }

        statusMessage = "Loaded " + std::to_string(items.size()) + " item(s)";
        SaveLastOpenedRootPath(rootPath);
        return true;
    }

    static void TryAutoRefreshOpenedRoot() {
        if (openedRootPath.empty() || showPasswordPopup) {
            return;
        }

        const double now = ImGui::GetTime();
        if (now - lastAutoRefreshAtSeconds < AUTO_REFRESH_INTERVAL_SECONDS) {
            return;
        }
        lastAutoRefreshAtSeconds = now;

        const std::string latestSignature = BuildRootSnapshotSignature(openedRootPath);
        if (latestSignature.empty()) {
            return;
        }
        if (openedRootSignature.empty()) {
            openedRootSignature = latestSignature;
            return;
        }
        if (latestSignature == openedRootSignature) {
            return;
        }

        if (LoadItemsFromPath(openedRootPath)) {
            statusMessage = "Detected filesystem changes and refreshed items";
        }
    }

    static void ResetSelection() {
        selectedItemIds.clear();
        focusedItemId.clear();
        multiSelectMode = false;
        hasSelectionAnchor = false;
        selectionAnchorIndex = 0;
    }

    static std::vector<size_t> GetSelectedIndices() {
        std::vector<size_t> out;
        out.reserve(selectedItemIds.size());
        for (size_t i = 0; i < items.size(); ++i) {
            if (selectedItemIds.contains(items[i].id)) {
                out.push_back(i);
            }
        }
        return out;
    }

    static std::vector<size_t> GetOperationIndices() {
        std::vector<size_t> indices = GetSelectedIndices();
        if (!indices.empty()) {
            return indices;
        }

        if (focusedItemId.empty()) {
            return {};
        }

        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].id == focusedItemId) {
                return { i };
            }
        }
        return {};
    }

    static std::vector<size_t> ResolveIndicesForItemIds(const std::vector<std::string>& itemIds) {
        std::vector<size_t> indices;
        indices.reserve(itemIds.size());
        for (const std::string& id : itemIds) {
            for (size_t i = 0; i < items.size(); ++i) {
                if (items[i].id == id) {
                    indices.push_back(i);
                    break;
                }
            }
        }
        return indices;
    }

    static bool ItemMatchesFilter(const Item& item, const std::string& lowerSearchText, bool hasSearchFilter) {
        if (!hasSearchFilter) return true;
        return ToLower(item.name).find(lowerSearchText) != std::string::npos;
    }

    static void OpenPasswordPopup(bool forLockOperation) {
        passwordModeIsLock = forLockOperation;
        passwordBuffer[0] = '\0';
        passwordPopupError.clear();
        unlockPasswordMismatchMessage.clear();
        passwordInputNeedsFocus = true;
        passwordRevealUntil = 0.0;
        passwordPopupTargetItemIds.clear();
        for (const size_t idx : GetOperationIndices()) {
            if (IsValidIndex(idx)) {
                passwordPopupTargetItemIds.push_back(items[idx].id);
            }
        }
        showPasswordPopup = true;
        passwordPopupNeedsOpen = true;
    }

    static void ClosePasswordPopupAsCancelled() {
        showPasswordPopup = false;
        passwordPopupNeedsOpen = false;
        passwordPopupTargetItemIds.clear();
        unlockPasswordMismatchMessage.clear();
        passwordPopupError.clear();
        passwordInputNeedsFocus = false;
        passwordRevealUntil = 0.0;
        statusMessage = passwordModeIsLock ? "Lock cancelled" : "Unlock cancelled";
        ImGui::CloseCurrentPopup();
    }

    static void RenderMainLayout()
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGui::Begin("Safe", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoMove
                     );

        const float availableHeight = ImGui::GetContentRegionAvail().y - 8.0f;
        const float middleHeight = availableHeight - TOPBAR_HEIGHT - ROOTPATHBAR_HEIGHT - STATUSBAR_HEIGHT;

        // Top Bar
        ImGui::BeginChild("TopBar", ImVec2(0, TOPBAR_HEIGHT), true, 
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        RenderTopBar();
        ImGui::EndChild();

        // Middle Area
        ImGui::BeginChild("Middle", ImVec2(0, middleHeight), true);

        // Sidebar (Left)
        ImGui::BeginChild("Sidebar", ImVec2(SIDEBAR_WIDTH, 0), true);
        ImGui::Text("Items");
        ImGui::Separator();
        RenderFolderList();
        ImGui::EndChild();

        ImGui::SameLine();

        // Main Panel (Right)
        ImGui::BeginChild("MainPanel", ImVec2(0, 0), true);
        ImGui::Text("Item Info");
        ImGui::Separator();
        RenderMainPanel();
        ImGui::EndChild();

        ImGui::EndChild();

        // Root Path Bar
        ImGui::BeginChild("RootPathBar", ImVec2(0, ROOTPATHBAR_HEIGHT), true,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (!openedRootPath.empty()) {
            ImGui::Text("Root: %s", WideToUtf8(openedRootPath).c_str());
        } else {
            ImGui::TextDisabled("Root: None");
        }
        ImGui::EndChild();

        // Status Bar
        ImGui::BeginChild("StatusBar", ImVec2(0, STATUSBAR_HEIGHT), true);
        RenderStatusBar();
        ImGui::EndChild();

        ImGui::End();
        
        RenderPasswordPopup();
    }

    static void RenderTopBar()
    {
        const float height = ImGui::GetContentRegionAvail().y;
        const float itemHeight = ImGui::GetFrameHeight();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (height - itemHeight) * 0.01f);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));

        ImGui::SetNextItemWidth(SEARCH_WIDTH);
        ImGui::InputTextWithHint("##search", "Search...", searchBuffer, SEARCH_BUFFER_SIZE);

        ImGui::SameLine(0, BUTTON_SPACING);

        if (ImGui::Button("Open", ImVec2(BUTTON_WIDTH, 0)))
        {
            const std::wstring selectedFolder = OpenFolderDialog();
            if (selectedFolder.empty()) {
                statusMessage = "Open cancelled";
            } else if (!LoadItemsFromPath(selectedFolder)) {
                statusMessage = "Failed to load selected folder";
            }
        }

        ImGui::SameLine(0, BUTTON_SPACING);

        const bool stylePushed = multiSelectMode;
        if (stylePushed)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.70f, 0.95f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.62f, 0.90f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.52f, 0.82f, 1.0f));
        }

        if (ImGui::Button(multiSelectMode ? "Select ON" : "Select", ImVec2(SELECT_BUTTON_WIDTH, 0)))
        {
            multiSelectMode = !multiSelectMode;
            statusMessage = multiSelectMode
                ? "Select mode enabled: click to multi-select (Ctrl+Click also works)"
                : "Select mode disabled (Ctrl+Click still works)";
        }

        if (stylePushed)
        {
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine(0, BUTTON_SPACING);

        bool anyLocked = false;
        bool anyUnlocked = false;
        bool hasInvalidSelection = false;

        const std::vector<size_t> operationIndices = GetOperationIndices();
        for (const size_t idx : operationIndices) {
            if (!IsValidIndex(idx)) {
                hasInvalidSelection = true;
                continue;
            }
            if (items[idx].isLocked) anyLocked = true;
            else anyUnlocked = true;
        }

        if (hasInvalidSelection)
        {
            statusMessage = "Invalid selection detected";
        }

        const bool hasOperationTargets = !operationIndices.empty();
        const bool canLock = hasOperationTargets && anyUnlocked && !anyLocked;
        const bool canUnlock = hasOperationTargets && anyLocked && !anyUnlocked;

        if (!canLock) ImGui::BeginDisabled();

        if (ImGui::Button("Lock", ImVec2(BUTTON_WIDTH, 0)))
        {
            PerformLockOperation();
        }

        if (!canLock) ImGui::EndDisabled();

        ImGui::SameLine(0, BUTTON_SPACING);

        if (!canUnlock) ImGui::BeginDisabled();

        if (ImGui::Button("Unlock", ImVec2(BUTTON_WIDTH, 0)))
        {
            PerformUnlockOperation();
        }

        if (!canUnlock) ImGui::EndDisabled();

        if (hasOperationTargets && anyLocked && anyUnlocked)
        {
            statusMessage = "Mixed selection: Select either locked or unlocked items";
        }

        ImGui::PopStyleVar();
    }

    /**
     * Main Panel Section
     * Displays folder/file details
     */
    static void RenderMainPanel()
    {
        RenderFolderDetails();
    }

    /**
     * Status Bar Section
     * Displays current status message to user
     */
    static void RenderStatusBar()
    {
        ImGui::Text("Status: %s", statusMessage.c_str());
    }

    static void RenderFolderList()
    {
        const bool hasSearchFilter = searchBuffer[0] != '\0';
        const std::string lowerSearchText = hasSearchFilter ? ToLower(searchBuffer) : "";

        for (size_t i = 0; i < items.size(); i++)
        {
            const auto& item = items[i];
            if (!ItemMatchesFilter(item, lowerSearchText, hasSearchFilter)) continue;
            const bool selected = selectedItemIds.contains(item.id);
            const bool focused = focusedItemId == item.id;
            const bool highlighted = selected || (selectedItemIds.empty() && focused);
            const char* icon = item.isLocked ? "[L] " : (item.isFolder ? "[D] " : "[F] ");
            const std::string label = std::string(icon) + item.name;

            ImGui::PushID(static_cast<int>(i));
            const bool clicked = ImGui::Selectable(label.c_str(), highlighted);
            ImGui::PopID();

            if (clicked)
            {
                focusedItemId = item.id;
                const bool shiftDown = ImGui::GetIO().KeyShift || ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
                const bool ctrlDown = ImGui::GetIO().KeyCtrl || ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);

                if (shiftDown)
                {
                    if (!hasSelectionAnchor || !IsValidIndex(selectionAnchorIndex))
                    {
                        selectionAnchorIndex = i;
                        hasSelectionAnchor = true;
                        selectedItemIds.clear();
                        selectedItemIds.insert(item.id);
                    }
                    else
                    {
                        const size_t rangeStart = std::min(selectionAnchorIndex, i);
                        const size_t rangeEnd = std::max(selectionAnchorIndex, i);
                        selectedItemIds.clear();
                        for (size_t idx = rangeStart; idx <= rangeEnd; ++idx)
                        {
                            selectedItemIds.insert(items[idx].id);
                        }
                    }
                }
                else if (ctrlDown || multiSelectMode)
                {
                    if (selected)
                    {
                        selectedItemIds.erase(item.id);
                    }
                    else
                    {
                        selectedItemIds.insert(item.id);
                    }
                    selectionAnchorIndex = i;
                    hasSelectionAnchor = true;
                }
                else
                {
                    // Plain click only focuses item for details; selection is modifier/select-mode only.
                    if (!selectedItemIds.empty()) {
                        selectedItemIds.clear();
                    }
                    selectionAnchorIndex = i;
                    hasSelectionAnchor = true;
                }
            }
        }
    }

    static void RenderFolderDetails()
    {
        if (selectedItemIds.empty())
        {
            if (!focusedItemId.empty()) {
                if (const Item* focusedItem = FindItemById(focusedItemId); focusedItem) {
                    ImGui::Text("Name: %s", focusedItem->name.c_str());
                    ImGui::Text("Type: %s", focusedItem->isFolder ? "Folder" : "File");
                    ImGui::Text("Status: %s", focusedItem->isLocked ? "Locked" : "Unlocked");
                    ImGui::Text("Size: %s", FormatBytes(focusedItem->sizeBytes).c_str());
                    ImGui::Text("Path: %s", WideToUtf8(focusedItem->path).c_str());
                    ImGui::Text("Source Path: %s", WideToUtf8(focusedItem->sourcePath).c_str());
                    ImGui::Text("Last Modified: %s", FormatDateTime(focusedItem->lastModified).c_str());
                    ImGui::Text("Item ID: %s", focusedItem->id.c_str());
                    ImGui::Spacing();
                    ImGui::Separator();
                    return;
                }
                focusedItemId.clear();
            }
            ImGui::Text("No item selected");
            ImGui::TextDisabled("Select a file or folder to view details");
            return;
        }

        if (selectedItemIds.size() == 1)
        {
            const std::string& selectedId = *selectedItemIds.begin();
            const Item* item = FindItemById(selectedId);
            if (!item) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Invalid selection");
                selectedItemIds.clear();
                return;
            }

            ImGui::Text("Name: %s", item->name.c_str());
            ImGui::Text("Type: %s", item->isFolder ? "Folder" : "File");
            ImGui::Text("Status: %s", item->isLocked ? "Locked" : "Unlocked");
            ImGui::Text("Size: %s", FormatBytes(item->sizeBytes).c_str());
            ImGui::Text("Path: %s", WideToUtf8(item->path).c_str());
            ImGui::Text("Source Path: %s", WideToUtf8(item->sourcePath).c_str());
            ImGui::Text("Last Modified: %s", FormatDateTime(item->lastModified).c_str());
            ImGui::Text("Item ID: %s", item->id.c_str());
            ImGui::Spacing();
            ImGui::Separator();
            return;
        }

        // Multiple selection
        size_t folderCount = 0;
        size_t fileCount = 0;
        uint64_t totalSize = 0;

        for (const size_t idx : GetSelectedIndices())
        {
            if (!IsValidIndex(idx)) continue;
            
            if (items[idx].isFolder) folderCount++;
            else fileCount++;
            totalSize += items[idx].sizeBytes;
        }

        ImGui::Text("Selected Items: %zu", selectedItemIds.size());
        ImGui::Text("Folders: %zu", folderCount);
        ImGui::Text("Files: %zu", fileCount);
        ImGui::Text("Total Size: %s", FormatBytes(totalSize).c_str());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Actions will apply to all selected items");
    }

    static void RenderPasswordPopup()
    {
        if (!showPasswordPopup) {
            passwordPopupTargetItemIds.clear();
            unlockPasswordMismatchMessage.clear();
            return;
        }

        const char* popupTitle = passwordModeIsLock ? "Lock Password" : "Unlock Password";
        if (passwordPopupNeedsOpen) {
            ImGui::OpenPopup(popupTitle);
            passwordPopupNeedsOpen = false;
        }
        ImGui::SetNextWindowSize(ImVec2(460, 182), ImGuiCond_Appearing);

        if (ImGui::BeginPopupModal(popupTitle, &showPasswordPopup, ImGuiWindowFlags_NoResize))
        {
            const bool cancelPressed = ImGui::IsKeyPressed(ImGuiKey_Escape);
            ImGui::PushStyleColor(ImGuiCol_NavHighlight, ImVec4(0, 0, 0, 0));
            const float contentStartX = ImGui::GetCursorPosX();
            const float contentWidth = std::max(120.0f, ImGui::GetContentRegionAvail().x);
            const float contentRightX = contentStartX + contentWidth;

            ImGui::SetCursorPosX(contentStartX);
            ImGui::PushTextWrapPos(contentRightX);
            ImGui::TextUnformatted("Enter password for the selected items.");
            ImGui::TextDisabled(passwordModeIsLock ? "Mode: Lock" : "Mode: Unlock");
            ImGui::PopTextWrapPos();
            ImGui::Spacing();
            ImGui::SetCursorPosX(contentStartX);
            const float rowSpacing = ImGui::GetStyle().ItemSpacing.x;
            constexpr float toggleButtonWidth = 72.0f;
            const float inputWidth = std::max(80.0f, contentWidth - toggleButtonWidth - rowSpacing);
            ImGui::SetNextItemWidth(inputWidth);
            if (passwordInputNeedsFocus) {
                ImGui::SetKeyboardFocusHere();
                passwordInputNeedsFocus = false;
            }
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 9.0f));
            const bool passwordVisible = ImGui::GetTime() < passwordRevealUntil;
            const ImGuiInputTextFlags passwordFlags =
                (passwordVisible ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password) |
                ImGuiInputTextFlags_EnterReturnsTrue;
            const bool submittedWithEnter = ImGui::InputTextWithHint("##password", "Password", passwordBuffer, PASSWORD_BUFFER_SIZE, passwordFlags);
            ImGui::SameLine(0.0f, rowSpacing);
            if (ImGui::Button("Show##toggle-password", ImVec2(toggleButtonWidth, 0.0f))) {
                passwordRevealUntil = ImGui::GetTime() + 0.3;
                passwordInputNeedsFocus = true;
            }
            ImGui::PopStyleVar();
            ImGui::SetCursorPosX(contentStartX);
            if (!passwordPopupError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.85f, 0.20f, 0.20f, 1.0f), "%s", passwordPopupError.c_str());
            }
            ImGui::Spacing();
            ImGui::SetCursorPosX(contentStartX);
            ImGui::Separator();
            ImGui::Spacing();

            // Buttons
            const float spacing = ImGui::GetStyle().ItemSpacing.x;
            const float buttonWidth = (contentWidth - spacing) * 0.5f;
            const float totalWidth = (buttonWidth * 2) + spacing;
            ImGui::SetCursorPosX(contentStartX + ((contentWidth - totalWidth) * 0.5f));

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 9.0f));
            const bool submitClicked = ImGui::Button("OK", ImVec2(buttonWidth, 0));
            if (submitClicked || submittedWithEnter)
            {
                if (passwordBuffer[0] == '\0') {
                    statusMessage = "Password is required";
                    passwordPopupError = "Password is required";
                    passwordInputNeedsFocus = true;
                } else {
                    const std::string password(passwordBuffer);
                    if (const bool ok = passwordModeIsLock ? ApplyLockOperation(password) : ApplyUnlockOperation(password); ok) {
                        passwordPopupError.clear();
                        statusMessage = passwordModeIsLock ? "Locked selected item(s)" : "Unlocked selected item(s)";
                        showPasswordPopup = false;
                        passwordPopupNeedsOpen = false;
                        passwordPopupTargetItemIds.clear();
                        unlockPasswordMismatchMessage.clear();
                        passwordRevealUntil = 0.0;
                    } else if (!passwordModeIsLock) {
                        passwordBuffer[0] = '\0';
                        passwordPopupError = unlockPasswordMismatchMessage.empty()
                            ? "Wrong password. Please try again."
                            : unlockPasswordMismatchMessage;
                        passwordInputNeedsFocus = true;
                        passwordRevealUntil = 0.0;
                        statusMessage = passwordPopupError;
                    } else {
                        passwordPopupError = "Lock failed for one or more items";
                        passwordRevealUntil = 0.0;
                        statusMessage = "Lock failed for one or more items";
                    }
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)) || cancelPressed)
            {
                ClosePasswordPopupAsCancelled();
            }
            ImGui::PopStyleVar();

            ImGui::PopStyleColor();
            ImGui::EndPopup();
        }
    }

    static void PerformLockOperation()
    {
        const std::vector<size_t> operationIndices = GetOperationIndices();
        if (operationIndices.empty())
        {
            statusMessage = "No item selected";
            return;
        }

        bool anyLocked = false;
        for (const size_t idx : operationIndices)
        {
            if (!IsValidIndex(idx)) continue;
            if (items[idx].isLocked)
            {
                anyLocked = true;
                break;
            }
        }

        if (anyLocked)
        {
            statusMessage = "Cannot lock: Some items are already locked";
            return;
        }

        OpenPasswordPopup(true);
    }

    static void PerformUnlockOperation()
    {
        const std::vector<size_t> operationIndices = GetOperationIndices();
        if (operationIndices.empty())
        {
            statusMessage = "No item selected";
            return;
        }

        bool allLocked = true;
        for (const size_t idx : operationIndices)
        {
            if (!IsValidIndex(idx)) continue;
            if (!items[idx].isLocked)
            {
                allLocked = false;
                break;
            }
        }

        if (!allLocked)
        {
            statusMessage = "Cannot unlock: Not all selected items are locked";
            return;
        }

        OpenPasswordPopup(false);
    }

    static bool ApplyLockOperation(const std::string& password) {
        const std::vector<size_t> targetIndices = passwordPopupTargetItemIds.empty()
            ? GetOperationIndices()
            : ResolveIndicesForItemIds(passwordPopupTargetItemIds);
        if (targetIndices.empty()) {
            return false;
        }
        bool allCoreOpsSucceeded = true;
        bool anyChanged = false;
        for (const size_t idx : targetIndices) {
            if (!IsValidIndex(idx)) {
                allCoreOpsSucceeded = false;
                continue;
            }

            std::vector<uint8_t> salt(PASSWORD_SALT_SIZE);
            std::vector<uint8_t> verifier;
            const bool verifierReady =
                GenerateRandomBytes(salt) &&
                DerivePasswordVerifier(password, salt, DB_DEFAULT_KDF_ITERATIONS, verifier);

            core::Folder folder(items[idx].path);
            if (!folder.Lock(password)) {
                const std::wstring archivePath = items[idx].path + L".safe";
                const bool archiveExists = core::Filesystem::FileExists(archivePath);
                const bool plaintextExists =
                    core::Filesystem::DirectoryExists(items[idx].path) ||
                    core::Filesystem::FileExists(items[idx].path);
                if (!(archiveExists && !plaintextExists)) {
                    allCoreOpsSucceeded = false;
                    continue;
                }
            }

            items[idx].isLocked = true;
            if (verifierReady) {
                items[idx].passwordHash = BytesToHex(verifier);
                items[idx].passwordSalt = BytesToHex(salt);
            } else {
                items[idx].passwordHash.clear();
                items[idx].passwordSalt.clear();
            }
            items[idx].kdfIterations = DB_DEFAULT_KDF_ITERATIONS;
            anyChanged = true;
            UpsertPersistedState(items[idx]);
        }
        if (anyChanged && !openedRootPath.empty()) {
            LoadItemsFromPath(openedRootPath);
        }
        return allCoreOpsSucceeded;
    }

    static bool ApplyUnlockOperation(const std::string& password) {
        const std::vector<size_t> targetIndices = passwordPopupTargetItemIds.empty()
            ? GetOperationIndices()
            : ResolveIndicesForItemIds(passwordPopupTargetItemIds);
        if (targetIndices.empty()) {
            return false;
        }
        unlockPasswordMismatchMessage.clear();
        bool allCoreOpsSucceeded = true;
        bool anyChanged = false;
        bool hasPasswordMismatch = false;
        bool anyUnlockedThisAttempt = false;
        for (const size_t idx : targetIndices) {
            if (!IsValidIndex(idx)) {
                allCoreOpsSucceeded = false;
                continue;
            }

            // Skip items that are already unlocked from prior attempts in the same popup session.
            if (!items[idx].isLocked) {
                continue;
            }

            // Treat persisted verifier metadata as advisory only. The encrypted archive
            // remains the source of truth for password validity.
            if (!items[idx].passwordHash.empty() && !items[idx].passwordSalt.empty()) {
                std::vector<uint8_t> salt;
                std::vector<uint8_t> storedVerifier;
                if (HexToBytes(items[idx].passwordSalt, salt) && HexToBytes(items[idx].passwordHash, storedVerifier) &&
                    !salt.empty() && !storedVerifier.empty()) {
                    std::vector<uint8_t> computedVerifier;
                    const int iterations = items[idx].kdfIterations > 0 ? items[idx].kdfIterations : DB_DEFAULT_KDF_ITERATIONS;
                    if (DerivePasswordVerifier(password, salt, iterations, computedVerifier)) {
                        (void)ConstantTimeEqual(storedVerifier, computedVerifier);
                    }
                }
            }

            core::Folder folder(items[idx].path);
            if (!folder.Unlock(password)) {
                const std::wstring archivePath = items[idx].path + L".safe";
                const bool archiveExists = core::Filesystem::FileExists(archivePath);
                const bool plaintextExists =
                    core::Filesystem::DirectoryExists(items[idx].path) ||
                    core::Filesystem::FileExists(items[idx].path);
                if (archiveExists || !plaintextExists) {
                    allCoreOpsSucceeded = false;
                    hasPasswordMismatch = true;
                    continue;
                }
            }

            items[idx].isLocked = false;
            items[idx].passwordHash.clear();
            items[idx].passwordSalt.clear();
            anyChanged = true;
            anyUnlockedThisAttempt = true;
            UpsertPersistedState(items[idx]);
        }
        if (anyChanged && !openedRootPath.empty()) {
            LoadItemsFromPath(openedRootPath);
        }
        if (!allCoreOpsSucceeded && hasPasswordMismatch && anyUnlockedThisAttempt) {
            unlockPasswordMismatchMessage = "Some selected items need a different password.";
        }
        return allCoreOpsSucceeded;
    }
}
