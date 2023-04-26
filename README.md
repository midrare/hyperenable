# hyperenable
**hyperenable allows `ctrl+shift+alt+win` (i.e. the Office key, or the hyper key), which is normally reserved by Windows, to be freely rebound.**

hyperenable does this by squatting on the affected hotkeys. `hyperenable.exe` starts before `explorer.exe` and pre-emptively registers the hotkeys for itself. Now `explorer.exe` starts, and fails to register the already-occupied hotkeys. hyperenable then unregisters the hotkeys again, letting them be freely reused.

## Installing
  1. Put `hyperenable.exe` somewhere in your system.
  2. Open `regedit.exe`. Go to `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon` and edit the `Userinit` value. Prepend (not append!) `C:\path\to\hyperenable.exe`. This lets `hyperenable.exe` start before `explorer.exe`. **Be careful with the comma delimiters. Make sure you get this right, or Windows will not start properly.**
  3. Open an `explorer.exe` window. Go to `C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup\` and create a shortcut to `C:\path\to\hyperenable.exe`. Right-click the shortcut and click "properties". In the "target" field of the shortcut, add the `--release` argument.
  4. Open `regedit.exe`. Go to `HKEY_LOCAL_MACHINE\Software\Classes\ms-officeapp\Shell\Open\Command` and edit the default (unnamed) value. (If the key doesn't exist, create it.) Set the default value to `rundll32`. You need this because, in addition to counting as a key modifier, `ctrl+shift+alt+win` also counts as a key press. This disables the key press.

## Building
  1. Install Visual Studio 2022
  2. Install [GNU Make](https://scoop.sh/)
  3. Start Visual Studio 2022 developer prompt
  4. `cd ~/project_dir`
  5. `make`

## Acknowledgements
Concept based on [OfficeKeyFix](https://github.com/anthonyheddings/OfficeKeyFix).

