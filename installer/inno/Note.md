# Inno Setup Notes (`installer/inno`)

This folder contains the Windows installer script for Safe:

- **Script file:** `SafeInstaller.iss`
- **Installer system:** Inno Setup
- **Target app binary:** `Safe.exe`

## Current defaults in `SafeInstaller.iss`

1. **Source build folder**
   - `..\..\release-build`
2. **Installer output folder**
   - `..\..\release-build\installer`
3. **Installed executable**
   - `{app}\Safe.exe`

## What the installer does

1. Installs `Safe.exe` and `assets\` into `{localappdata}\Programs\Safe`
2. Creates a Start Menu shortcut
3. Optionally launches Safe after install
4. Adds `{app}` to the current user's PATH
5. On uninstall, removes PATH entry and preserves `.safe` archives

## Build reminder

Compile the app so `release-build\Safe.exe` exists, then run the Inno Setup script to generate the installer (`Safe.exe`).
