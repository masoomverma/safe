# Installer Notes

Current installer script: `installer\inno\SafeInstaller.iss`

## Installer system and paths

- **Installer system:** Inno Setup
- **Source build folder:** `..\..\release-build`
- **Installer output folder:** `..\..\release-build\installer`
- **Installed executable path:** `{app}\Safe.exe`

## Behavior

1. **Per-user install only** (`PrivilegesRequired=lowest`).
2. Installs to `%LOCALAPPDATA%\Programs\Safe`.
3. Installs `Safe.exe` and app assets (including `assets\fonts\Inter-Regular.ttf`).
4. Adds `{app}` to current user `PATH` after install.
5. Removes that same `PATH` entry during uninstall.
6. Preserves `.safe` archived files/folders under install location on uninstall.
7. Prompts whether to remove `%LOCALAPPDATA%\Safe` user data (including `safe.db`) on uninstall.
8. Runs the app after installation (`postinstall` run step).

## Shortcut and icon policy

- A Start Menu group is created (`DefaultGroupName=Safe`, `DisableProgramGroupPage=yes`).
- A Start Menu shortcut to `Safe.exe` and an uninstall shortcut are created in that group.
- A desktop shortcut is available as an optional task (`desktopicon`), unchecked by default.
- Installed app metadata includes an uninstall display icon (`UninstallDisplayIcon={app}\Safe.exe`).

## Build reminder

Compile the app so `release-build\Safe.exe` exists, then run the Inno Setup script to generate the installer.
