# Safe - Technical Notes and Current Status

## 1. Current Status

### Phase
- **Phase 3 (UI implementation)** is active.

### Working now
1. Win32 window + DirectX 11 rendering loop
2. Dear ImGui full-screen app layout (TopBar / Sidebar / MainPanel / StatusBar)
3. Real folder picker integration (**Open**) and filesystem-backed scan/load (folders + files + `.safe`)
4. Item list rendering with search filter and real metadata backing
5. Selection and focus system:
   - Single click **focuses** an item and shows details (no selection on plain click)
   - **Select button mode** (toggle multi-select)
   - **Ctrl+Click** random/non-contiguous toggle selection
   - **Shift+Click** range selection using anchor index
   - Lock/Unlock actions work for selected items, or for the single focused item when nothing is selected
6. Stable item identity with path-derived IDs (selection is ID-based)
7. SQLite metadata persistence (`%LOCALAPPDATA%\\Safe\\safe.db`) for lock state and password verifier
8. Last opened root path persistence and auto-restore at app startup
9. Migration-safe SQLite schema versioning via `PRAGMA user_version` (v1->v5 path)
10. Real item lock/unlock pipeline (files + folders):
    - Selected content is serialized and encrypted into `.safe` archives
    - Unlock decrypts and restores original file/folder content
    - Archive format uses PBKDF2(SHA-256) + AES-256-CBC
11. Password-gated lock/unlock flow via modal popup
12. Updated UI highlight colors:
    - Selected is medium blue
    - Hover is lighter than selected

### Not implemented yet
1. Advanced key-management lifecycle (recovery/rotation/credential reset).
2. Compression and large-item streaming pipeline for performance.
3. Automated regression tests around selection, migration, and lock/unlock behavior.

---

## 2. Technical Architecture

## Entry and platform layer
- **File:** `core/src/main.cpp`
- Responsibilities:
  1. Register Win32 class and create main window
  2. Initialize D3D11 device/swap chain/render target
  3. Initialize Dear ImGui context and DX11/Win32 backends
  4. Apply theme/style values
  5. Run render loop and message pump
  6. Shutdown UI and graphics resources cleanly

## UI/state layer
- **File:** `core/src/ui/ui.cpp`
- Namespace: `safe::ui`
- Public API:
  - `UI::Initialize()`
  - `UI::Render()`
  - `UI::Cleanup()`

### Main state currently in memory
1. `items` (`std::vector<Item>`) with fields:
   - `name`
   - `isFolder`
   - `isLocked`
2. `selectedItemIds` (`std::unordered_set<std::string>`)
3. `focusedItemId` (`std::string`) for single-click details focus
4. Selection helpers:
   - `multiSelectMode`
   - `selectionAnchorIndex`
   - `hasSelectionAnchor`
5. UI state:
   - `searchBuffer`
   - `statusMessage`
   - password popup state (`showPasswordPopup`, `passwordModeIsLock`)

### UI flow functions
1. `RenderMainLayout()` - top-level window sections
2. `RenderTopBar()` - search/open/select/lock/unlock controls
3. `RenderFolderList()` - list + selection behavior
4. `RenderFolderDetails()` - single vs multi-item details
5. `RenderPasswordPopup()` - password prompt and validation
6. `PerformLockOperation()` / `PerformUnlockOperation()` - operation handlers

---

## 3. Behavior Notes (Current)

1. Single click focuses an item and shows its details without selecting it.
2. Selection is performed through Select mode, Ctrl+Click, or Shift+Click.
3. Lock is enabled only when operation targets are all unlocked.
4. Unlock is enabled only when operation targets are all locked.
5. Mixed locked/unlocked targets disable both actions.
6. Lock/Unlock requires password confirmation and updates SQLite metadata.
7. Selection uses stable IDs and survives index reordering safely.

---

## 4. Technical Risks / Constraints

1. Archive packing currently serializes file data in-memory; very large files/folders may need streaming.
2. Password verifier storage is stronger (salted PBKDF2) but still local metadata-based.
3. File scan is currently root-level only; deep recursive listing is not yet implemented in UI.

---

## 5. Next Steps

1. Add operation-level tests for selection rules and lock/unlock guards.
2. Optimize encryption pipeline for large files/directories (streaming, chunking, optional compression).
3. Add operation-level tests for lock/unlock verifier and migration paths.
4. Build and ship the new per-user Inno Setup installer (`safe.exe`) for user-scope installs only (adds/removes user PATH entry and preserves `.safe` archives on uninstall).

---

## 6. AI Assistance

Development included AI-assisted implementation and cleanup support with:
1. **ChatGPT (GPT-5.3-Codex)**
2. **GitHub Copilot**
3. Unlock reliability fixes:
   - Successful decrypt/restore is treated as success even if archive cleanup fails
   - Plaintext entries are preferred over stale `.safe` entries when both exist
