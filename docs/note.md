# Safe Project — Development Flow

---

## High-Level Flow

Core Engine → UI → Metadata → Features → Security → Polish

---

# PHASE 1 — RUN GUI (FIRST MILESTONE)

## Goal:

Window opens successfully

## Tasks:

* Setup Win32 window
* Setup DirectX11
* Initialize ImGui
* Create render loop

## Output:

Window opens with empty UI

---

# PHASE 2 — BASIC UI LAYOUT

## Goal:

Build visual structure of app

## Tasks:

* Sidebar (folder list)
* Main panel (details)
* Top bar (search)
* Buttons (Lock, Unlock, Open)

Note: Use dummy data (no logic yet)

## Output:

UI shows folder list + buttons

---

# PHASE 3 — CONNECT FILESYSTEM

## Goal:

UI interacts with real folders

## Tasks:

* Detect folder existence
* Show folder status
* Connect UI buttons to filesystem checks

## Output:

UI shows real folder info

---

# PHASE 4 — METADATA SYSTEM (DBMS CORE)

## Goal:

Persistent storage system

## Tasks:

* Create .meta files
* Store:

  * folder path
  * locked/unlocked
  * last modified

Location:
data/metadata/

## Output:

App remembers folders after restart

---

# PHASE 5 — COMPRESSION

## Goal:

Convert folder → single file

## Tasks:

* Compress folder
* Extract folder

Library:
miniz (later)

## Output:

Folder becomes .safe file

---

# PHASE 6 — ENCRYPTION

## Goal:

Secure locked data

## Tasks:

* Password input
* Key derivation
* Encrypt / Decrypt data

Library:
OpenSSL (later)

## Output:

Locked file is secure

---

# PHASE 7 — CONNECT UI + CORE

## Goal:

Make UI functional

## Tasks:

* Lock button → compress + encrypt
* Unlock → decrypt + extract
* Update UI state

## Output:

Buttons perform real actions

---

# PHASE 8 — FEATURES

## Goal:

Improve usability

## Add:

* Search (filter list)
* Open in Explorer
* Change password
* Show file info

## Output:

Full working application

---

# PHASE 9 — POLISH

## Goal:

Production-level feel

## Add:

* Error handling
* UI improvements
* Icons / fonts
* Edge case handling

## Output:

Smooth, stable app

---

# Final Summary

1. Run GUI
2. Build UI
3. Connect filesystem
4. Add metadata
5. Add compression
6. Add encryption
7. Connect UI
8. Add features
9. Polish

---

# Golden Rules

* Do NOT skip phases
* Do NOT add all libraries at once
* Always test after each phase
* Keep UI and core separate

---

# Current Position

You are here:

Phase 1 — Ready to start GUI

---

Next Step:
Create basic ImGui window and layout

---

# Future libs:
- SQLite (metadata DB)
- OpenSSL (encryption)
- miniz (compression)
