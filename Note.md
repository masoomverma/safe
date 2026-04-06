# Safe - Technical Notes and Current Status

## 1. Current Status

### Phase
- **Phase 3 (UI implementation)** is active.

### Working now
1. Win32 window + DirectX 11 rendering loop
2. Dear ImGui full-screen app layout (TopBar / Sidebar / MainPanel / StatusBar)
3. Item list rendering with search filter
4. Selection system:
   - Single click selection
   - **Select button mode** (toggle multi-select)
   - **Ctrl+Click** random/non-contiguous toggle selection
   - **Shift+Click** range selection using anchor index
5. Lock/Unlock state transitions (UI/state simulation)
6. Multi-item lock modal:
   - Encrypt individually (simulated by state)
   - Combine selected items into one `.safe` entry (simulated)
7. Updated UI highlight colors:
   - Selected is medium blue
   - Hover is lighter than selected

### Not implemented yet
1. Real filesystem scan/load and open dialog integration
2. Real encryption/decryption pipeline
3. SQLite persistence for items and metadata
4. Password/key-management flows

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
2. `selectedIndices` (`std::unordered_set<size_t>`)
3. Selection helpers:
   - `multiSelectMode`
   - `selectionAnchorIndex`
   - `hasSelectionAnchor`
4. UI state:
   - `searchBuffer`
   - `statusMessage`
   - lock popup state (`showLockPopup`, `combinedNameBuffer`, `selectedLockOption`)

### UI flow functions
1. `RenderMainLayout()` - top-level window sections
2. `RenderTopBar()` - search/open/select/lock/unlock controls
3. `RenderFolderList()` - list + selection behavior
4. `RenderFolderDetails()` - single vs multi-item details
5. `RenderLockPopup()` - multi-lock options and validation
6. `PerformLockOperation()` / `PerformUnlockOperation()` - operation handlers

---

## 3. Behavior Notes (Current)

1. Lock is enabled only when selection is all unlocked.
2. Unlock is enabled only when selection is all locked.
3. Mixed locked/unlocked selection disables both actions.
4. Multi-lock combine flow removes selected entries and adds one `.safe` item.
5. Selection is index-based; after list mutation, selection is cleared where needed for safety.

---

## 4. Technical Risks / Constraints

1. Index-based identity is fragile once real data loading/sorting/filtering grows.
2. No persistence yet; all state resets on restart.
3. Lock/Unlock is simulation only, so data protection is not yet real.

---

## 5. Next Steps

1. Introduce stable item IDs and a model layer (decouple view index from identity).
2. Implement filesystem-backed item loading and open flow.
3. Add SQLite metadata storage and migration-safe schema.
4. Integrate real encryption/decryption pipeline and password verification flow.
5. Add operation-level tests for selection rules and lock/unlock guards.
