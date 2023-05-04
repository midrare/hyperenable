# hyperenable
**hyperenable allows `ctrl+shift+alt+win` (i.e. the Office key, or the hyper key), which is normally reserved by Windows, to be freely rebound.** hyperenable does this by preemptively registering the hotkeys before `explorer.exe` can, and deregistering them afterwards. Run `hyperenable.exe --help` for cli options.

## Installing
  1. Put `hyperenable.exe` somewhere in your system. I recommend `%ProgramFiles%/hyperenable/hyperenable.exe` so it can be protected by admin rights. This is important because we'll be launching it through `Userinit`.
  2. In Registry Editor, go to `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon` and edit the `Userinit` value. Prepend (not append!) `C:\path\to\hyperenable.exe`. This lets `hyperenable.exe` start before `explorer.exe`. **Be careful with the comma delimiters. Make sure you get this right, or Windows won't start properly.**
  3. Make a shortcut to `hyperenable.exe` and put it in `C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup\`. Right-click the shortcut and open "properties". Open the "shortcut" tab. In the "target" field, add `--release`. The entire "target" field should look like this: `"C:\path\to\hyperenable.exe" --release`.
  4. In Registry Editor, go to `HKEY_LOCAL_MACHINE\Software\Classes\ms-officeapp\Shell\Open\Command` and edit the default (unnamed) value. (If the key doesn't exist, create it.) Set the value to `rundll32`. You need this because, in addition to counting as a key modifier, `ctrl+shift+alt+win` also counts as a key press. This disables the key press.

## Building
  1. Install Visual Studio 2022
  2. Install [GNU Make](https://scoop.sh/)
  3. Start Visual Studio 2022 developer prompt
  4. `cd ~/project_dir`
  5. `make`

## Acknowledgements
Concept based on [OfficeKeyFix](https://github.com/anthonyheddings/OfficeKeyFix).

