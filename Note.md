# Safe - Project Notes

## Purpose

Safe is a Windows desktop app for locking and unlocking files/folders into encrypted `.safe` archives.

## Current state

- Current app version: `1.1.3`
- Main workflow is implemented: open folder, browse/search, lock, unlock, and persist state.

## Implemented capabilities

1. Full-screen Dear ImGui UI (TopBar, Sidebar, MainPanel, StatusBar)
2. Selection behavior: single click, Select mode, Ctrl+Click toggle, Shift+Click range
3. Lock/Unlock operations for selected items or focused item
4. Live refresh of opened root content
5. Metadata persistence in `%LOCALAPPDATA%\Safe\safe.db`
6. Startup restore of last opened root path

## Technology

1. C++20
2. CMake + Ninja
3. Win32 API
4. DirectX 11
5. Dear ImGui (Win32 + DX11 backends)
6. SQLite3
7. Windows BCrypt

## OpenSSL runtime DLLs (`core\build`)

Required DLLs:

1. `libcrypto-3-x64.dll`
2. `libssl-3-x64.dll`

Setup:

1. Install OpenSSL 3.x (Win64)
2. Copy both DLLs into `core\build\`
3. Configure:
   - `cmake -S . -B release-build -G Ninja -DOPENSSL_DLL_DIR="core\build"`
4. Build:
   - `cmake --build release-build`

## Repository layout

```text
Safe
├── assets
│   └── icons
├── core
│   ├── build
│   ├── include
│   │   ├── core
│   │   └── ui
│   ├── libs
│   │   ├── imgui
│   │   │   └── backends
│   │   └── sqlite3
│   └── src
│       ├── core
│       └── ui
├── installer
│   └── inno
├── app.rc.in
├── CMakeLists.txt
├── README.md
└── Note.md
```
