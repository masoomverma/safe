# Safe - Project Overview

## 1. Purpose

Safe is a Windows desktop application for locking and unlocking files and folders into `.safe` archives through a graphical interface.

## 2. Current Functional Scope

The core workflow is implemented and operational:

1. Open a target folder
2. Search and browse items
3. Lock selected item(s)
4. Unlock selected item(s)
5. Persist metadata and restore the last opened path

## 3. Implemented Features

1. Full-screen Dear ImGui interface with TopBar, Sidebar, MainPanel, and StatusBar
2. Selection system with focus, Select mode, Ctrl+Click toggle, and Shift+Click range selection
3. Lock/Unlock operations on selected items, or on the focused item when no selection exists
4. Password modal with stable multi-select retry behavior
5. Unlock feedback rules:
   - If no item is unlocked in the current attempt: **"Wrong password. Please try again."**
   - If only part of the selection is unlocked: **"Some selected items need a different password."**
6. SQLite metadata persistence (`%LOCALAPPDATA%\Safe\safe.db`)
7. Automatic restoration of the last opened root path at startup

## 4. Technology Stack

1. C++20
2. CMake + Ninja
3. Win32 API
4. DirectX 11
5. Dear ImGui (including Win32 and DX11 backends)
6. SQLite3
7. Windows BCrypt

## 5. OpenSSL Runtime DLL Requirements (`core\build`)

Required runtime files:

1. `libcrypto-3-x64.dll`
2. `libssl-3-x64.dll`

Recommended setup process:

1. Install OpenSSL 3.x (Win64)
2. Copy the required DLL files into `core\build\`
3. Configure the project:
   - `cmake -S . -B release-build -G Ninja -DOPENSSL_DLL_DIR="core\build"`
4. Build the project:
   - `cmake --build release-build`

## 6. Project Structure

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
├── .gitignore
├── app.rc
├── CMakeLists.txt
├── Note.md
└── README.md
```
