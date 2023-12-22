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

 **Options**  
 hyperenable comes with CLI options that control some aspects of its behavior. These include:

  - `-c, --config PATH`  
    Read given config file that controls which keybinds are affected. This is so you can choose which of Windows' default keybinds are allowed to go through and which ones hyperenables unbinds for you. See the included `example_config.yaml`.
  - `-r, --run PATH`
    Run a program after hyperenable has made sure it's safe to rebind the affected keyboard shortcuts. **I highly recommend putting this exe in the same folder as hyperenable.exe (i.e. `$ProgramFiles/hyperenable/config.yaml`), so it can be protected by admin rights.** Using this option prevents timing problems in which a later hotkey setup app fails to bind shortcuts because hyperenable hasn't finished running yet.

 To use any of these CLI options, you need to edit the registry key at `HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\Userinit`. After installing hyperenable, you should see a `hyperenable.exe` entry appear in this value. Just add whatever CLI options you want, being careful to get the formatting right. Pay close attention to the delimiters: *the delimters are commas, not semicolons*. Also, *there's a final trailing comma at the end*. **Be sure to get this right, or Windows won't start properly.**

## Building
  1. Install Visual Studio 2022. Make sure to include MSVC C++ x64/x86 build tools
  2. Install [GNU Make](https://scoop.sh/)
  3. Start Visual Studio 2022 developer prompt
  4. `cd ~/project_dir`
  5. `make`

## Acknowledgements
Concept based on [OfficeKeyFix](https://github.com/anthonyheddings/OfficeKeyFix).

