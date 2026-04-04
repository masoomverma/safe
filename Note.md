# Safe — Secure Folder Management System

---

## 1. Overview

**Safe** is a Windows-based secure folder management system that allows users to lock, unlock, and manage folders through a graphical user interface. The system is designed using **DBMS-inspired principles**, where each folder is treated as a structured record and managed through a local storage system.

Safe combines file system operations, encryption-based security, metadata management, and an intuitive GUI to provide a comprehensive solution for protecting sensitive folders.

---

## 2. Core Idea

Safe is not just a folder locker — it's a **structured system** that integrates:

### Key Components:
- **File System Operations** → Folder compression and extraction
- **Encryption-Based Security** → Password-protected AES-256 encryption using OpenSSL
- **Metadata Management** → SQLite database for tracking folder records (DBMS concept)
- **Graphical User Interface** → Built with Dear ImGui and DirectX 11

### How It Works:
The application behaves like a **mini database** where:
- Each folder is a **record** with metadata (name, path, status, timestamp)
- Operations act as **transactions** (lock, unlock, password change)
- Data is **persisted** across sessions using SQLite
- **ACID principles** ensure data integrity during operations

---

## 3. Dependencies

This project is **Windows-specific** and requires the following dependencies:

### Core Dependencies:

| Dependency     | Version | Included in Project?       | Purpose              |
|----------------|---------|----------------------------|----------------------|
| **Dear ImGui** | Latest  | Yes (`core/libs/imgui/`)   | GUI framework        |
| **SQLite3**    | 3.x     | Yes (`core/libs/sqlite3/`) | Metadata storage     |
| **OpenSSL**    | 3.x     | No - External              | Encryption (AES-256) |
| **DirectX 11** | -       | Windows Built-in           | Graphics rendering   |
| **Win32 API**  | -       | Windows Built-in           | Window management    |

### Platform Requirements:
- **Operating System:** Windows 10/11 (64-bit)
- **Compiler:** MSVC (Visual Studio 2019+) or Clang/MinGW with Windows SDK
- **Build System:** CMake 3.20+
- **C++ Standard:** C++17 OR 17++

### OpenSSL Installation:

**Option 1: Download Pre-built Binaries (Recommended)**
1. Visit: https://slproweb.com/products/Win32OpenSSL.html
2. Download: **Win64 OpenSSL v3.x.x Light**
3. Install to default location (`C:\Program Files\OpenSSL-Win64\`)
4. Copy DLLs to project (see section 5 for placement)

**Option 2: Using vcpkg**
```bash
vcpkg install openssl:x64-windows
```

**Option 3: Using Chocolatey**
```bash
choco install openssl
```

---

## 4. Project Structure

```
Safe/                                    # Root directory
│
├── CMakeLists.txt                       # CMake build configuration
├── app.rc                               # Windows resource file (application icon)
├── Note.md                              # This documentation file
│
├── core/                                # Core application code
│   │
│   ├── build/                           # OpenSSL DLLs go here
│   │   ├── libcrypto-3-x64.dll         # OpenSSL crypto library (REQUIRED)
│   │   └── libssl-3-x64.dll            # OpenSSL SSL/TLS library (REQUIRED)
│   │
│   ├── include/                         # Header files (.hpp, .h)
│   │   │
│   │   ├── core/                        # Core module headers
│   │   │   ├── cli.hpp                  # Command-line interface
│   │   │   ├── filesystem.hpp           # File system operations
│   │   │   ├── folder.hpp               # Folder management
│   │   │   ├── renderer.hpp             # Rendering interface
│   │   │   └── types.hpp                # Custom type definitions
│   │   │
│   │   └── ui/                          # UI module headers
│   │       └── ui.hpp                   # User interface declarations
│   │
│   ├── src/                             # Source files (.cpp)
│   │   │
│   │   ├── main.cpp                     # Application entry point (WinMain)
│   │   │
│   │   ├── core/                        # Core module implementations
│   │   │   ├── cli.cpp                  # CLI implementation
│   │   │   ├── filesystem.cpp           # File system logic
│   │   │   ├── folder.cpp               # Folder operations
│   │   │   ├── renderer.cpp             # Renderer interface
│   │   │   ├── renderer_impl.cpp        # Renderer implementation
│   │   │   └── types.cpp                # Type implementations
│   │   │
│   │   └── ui/                          # UI module implementations
│   │       └── ui.cpp                   # User interface logic
│   │
│   └── libs/                            # Third-party libraries (included)
│       │
│       ├── imgui/                       # Dear ImGui (Immediate Mode GUI)
│       │   ├── imgui.cpp
│       │   ├── imgui.h
│       │   ├── imgui_demo.cpp
│       │   ├── imgui_draw.cpp
│       │   ├── imgui_internal.h
│       │   ├── imgui_tables.cpp
│       │   ├── imgui_widgets.cpp
│       │   ├── imconfig.h
│       │   ├── imstb_rectpack.h
│       │   ├── imstb_textedit.h
│       │   ├── imstb_truetype.h
│       │   │
│       │   └── backends/                # Platform-specific backends
│       │       ├── imgui_impl_dx11.cpp  # DirectX 11 backend
│       │       ├── imgui_impl_dx11.h
│       │       ├── imgui_impl_win32.cpp # Win32 backend
│       │       └── imgui_impl_win32.h
│       │
│       └── sqlite3/                     # SQLite3 embedded database
│           ├── sqlite3.c                # SQLite implementation
│           └── sqlite3.h                # SQLite header
│
├── assets/                              # Application resources
│   │
│   ├── icons/                           # Application icons
│   │   ├── Safe.png                     # PNG icon
│   │   └── safe.ico                     # Windows ICO icon
│   │
│   └── fonts/                           # Custom fonts (optional)
│
├── data/                                # Runtime application data
│   │
│   ├── cache/                           # Temporary cache files
│   └── metadata/                        # Folder metadata storage
│
├── docs/                                # Additional documentation
│   └── note.md                          # Development flow notes
│
├── debug-build/                         # CMake build output (generated)
│   ├── Safe.exe                         # Compiled executable
│   ├── Safe.pdb                         # Debug symbols
│   ├── libcrypto-3-x64.dll             # Copied during build
│   ├── libssl-3-x64.dll                # Copied during build
│   └── [other build artifacts]
│
└── .idea/                               # JetBrains CLion IDE config (optional)
```

---

## 5. OpenSSL DLL Placement

### ⚠️ IMPORTANT: Where to Put OpenSSL DLLs

After installing OpenSSL, you **MUST** copy the DLL files to the project:

### Step 1: Locate OpenSSL DLLs 

(**Note:** Path might be different, find it manually or use **AI, everybody's favorite part**)

**Default installation path:**
```
C:\Program Files\OpenSSL-Win64\bin\
├── libcrypto-3-x64.dll
└── libssl-3-x64.dll
```

**vcpkg path:**
```
[vcpkg-root]\installed\x64-windows\bin\
├── libcrypto-3-x64.dll
└── libssl-3-x64.dll
```

### Step 2: Copy DLLs to Project

**From the root directory of the Safe project**, place the DLLs here:

```
Safe/                           ← You are here (root)
└── core/
    └── build/                  ← Make build folder and place DLLs in build
        ├── libcrypto-3-x64.dll ← Copy here
        └── libssl-3-x64.dll    ← Copy here
```
(**Note:** ***Safe/core/build*** is different from the build you are building in your IDE)

### Verification:

After copying, verify the files exist:

```cmd
dir core\build\*.dll
```

**Expected output:**
```
libcrypto-3-x64.dll
libssl-3-x64.dll
```

## 6. Current Status

### Development Phase: **Phase 1 Complete ✅**

**Completed Features:**
- ✅ ImGui window rendering with DirectX 11
- ✅ Win32 window creation and message handling
- ✅ Dark/Light theme auto-detection from Windows settings
- ✅ Responsive UI with proper window resize handling
- ✅ Comprehensive error handling for DirectX operations
- ✅ Memory leak prevention in render target management
- ✅ Null pointer safety checks throughout rendering pipeline

**Current Milestone:**
The application successfully opens a window and renders the Dear ImGui demo interface.

### Next Steps (Phase 2):
- 📋 Design main UI layout (sidebar + main panel + top bar)
- 📋 Create folder list UI component
- 📋 Add action buttons (Lock, Unlock, Open, Search)
- 📋 Implement basic navigation
---
**Last Updated:** 30 Mar 2026  
**Version:** 0.1.0-alpha  
**Status:** Phase 1 Complete  
**Ongoing:** Phase 2