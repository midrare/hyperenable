# hyperenable
**hyperenable allows `ctrl+shift+alt+win` (i.e. the Office key, or the hyper key), which is normally reserved by Windows, to be freely rebound.** hyperenable does this by preemptively registering the hotkeys before `explorer.exe` can, and deregistering them afterwards. Run `hyperenable.exe --help` for cli options.

## Getting Started
  Start a Powershell instance with admin privileges.

  **To install**  

  ```powershell
  # to install
  PS> Set-ExecutionPolicy Bypass -Scope Process -Force
  PS> ./install.ps1 -action install -exe "./hyperenable.exe"
  ```

  **To uninstall**  

  ```powershell
  # to uninstall
  PS> Set-ExecutionPolicy Bypass -Scope Process -Force
  PS> ./install.ps1 -action uninstall
  ```


## Building
  1. Install Visual Studio 2022. Make sure to include MSVC C++ x64/x86 build tools
  2. Install [GNU Make](https://scoop.sh/)
  3. Start Visual Studio 2022 developer prompt
  4. `cd ~/project_dir`
  5. `make`

## Acknowledgements
Concept based on [OfficeKeyFix](https://github.com/anthonyheddings/OfficeKeyFix).

