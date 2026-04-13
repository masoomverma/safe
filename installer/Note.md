# Installer (.iss) Notes

Current installer script: `installer\inno\SafeInstaller.iss`

## Behavior

1. **Per-user install only** (`PrivilegesRequired=lowest`).
2. Installs to `%LOCALAPPDATA%\Programs\Safe`.
3. Installs:
   - `Safe.exe`
   - `assets\fonts\Inter-Regular.ttf`
4. Adds `{app}` to current user `PATH` after install.
5. Removes that same `PATH` entry during uninstall.
6. Preserves `.safe` archived files/folders under install location on uninstall.
7. Prompts whether to remove `%LOCALAPPDATA%\Safe` user data (including `safe.db`) on uninstall.
8. Runs the app after installation (`postinstall` run step).

## Shortcut/Icon policy

- No Start Menu/Desktop shortcut entries are created by the installer (`[Icons]` section is intentionally omitted).
- Icon-related setup entries are intentionally not used (`DefaultGroupName`, `DisableProgramGroupPage`, and `UninstallDisplayIcon` are omitted).
