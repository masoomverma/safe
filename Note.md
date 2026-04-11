# Safe - Technical Notes and Current Status

## 1. Current Status

### Phase
- **Phase 3 (UI implementation)** is active.

### Working now
1. Win32 window + DirectX 11 rendering loop
2. Dear ImGui full-screen app layout (TopBar / Sidebar / MainPanel / StatusBar)
3. Real folder picker integration (**Open**) and filesystem-backed scan/load (folders + files + `.safe`)
4. Item list rendering with search filter and real metadata backing
5. Selection system:
   - Single click selection
   - **Select button mode** (toggle multi-select)
   - **Ctrl+Click** random/non-contiguous toggle selection
   - **Shift+Click** range selection using anchor index
6. Stable item identity with path-derived IDs (selection is ID-based)
7. SQLite metadata persistence (`%APPDATA%\\Safe\\safe.db`) for lock state and password verifier
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
3. Selection helpers:
   - `multiSelectMode`
   - `selectionAnchorIndex`
   - `hasSelectionAnchor`
4. UI state:
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

1. Lock is enabled only when selection is all unlocked.
2. Unlock is enabled only when selection is all locked.
3. Mixed locked/unlocked selection disables both actions.
4. Lock/Unlock requires password confirmation and updates SQLite metadata.
5. Selection uses stable IDs and survives index reordering safely.

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
