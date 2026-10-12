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
 hyperenable comes with CLI options that control some aspects of its behavior.

 ```console
 Usage: hyperenable [--help] [--version] [--timeout INT] [--config PATH] [--kill]
 
 Block default keybind registration attempts from Windows
 
 Windows reserves certain hotkeys for its own use. These include the Office key (win+shift+ctrl+alt) among many others. To unbind these hyperenable starts before explorer.exe and registers them before Windows can, like a domain squatter. After hyperenable exits, these keybinds can then rebound for your own use.
 
 Optional arguments:
   -h, --help            shows help message and exits
   -v, --version         prints version information and exits
   -t, --timeout INT     wait specified milliseconds to for explorer.exe to load. negative for no timeout [default: 15000]
   -c, --config PATH     read given config file that controls which keybinds are affected. this allows you to control which of Windows default keybinds are allowed and which ones hyperenable unbinds for you.
   --kill                unblock keybind registration attempts. a signal will be sent to a running instance of hyperenable to stop blocking keybinds. you normally do not need to call this yourself
 
 Keybind registration attempts will be blocked until hyperenable is killed, timeout elapses, or stop signal is received, whichever happens first.
 ```

 To use any of these CLI options, you need to edit the registry key at `HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\Userinit`. After installing hyperenable, you should see a `hyperenable.exe` entry appear in this value. Just add whatever CLI options you want, being careful to get the formatting right. Pay close attention to the delimiters: *the delimters are commas, not semicolons*. Also, *there's a final trailing comma at the end*. **Be sure to get this right, or Windows won't start properly.**

For an example config file, see `example_config.yaml` and read `keys.hpp`.

## Building
  1. Install Visual Studio 2022. Make sure to include MSVC C++ x64/x86 build tools
  2. Install [GNU Make](https://scoop.sh/)
  3. Start Visual Studio 2022 developer prompt
  4. `cd ~/project_dir`
  5. `make`

## Acknowledgements
Concept based on [OfficeKeyFix](https://github.com/anthonyheddings/OfficeKeyFix).
