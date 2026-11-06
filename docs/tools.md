# Tools

Everything this project needs installed on a computer, and how to remove it. When we add a tool, it goes here in the same change. When we stop needing one, we remove it and take it off this list.

## Rules

- Install with the system's package manager: Homebrew on macOS, the distro's on Linux, winget on Windows.
- Python packages go in a virtual environment inside the project, never globally.
- Avoid curl-pipe installers and global npm or pip installs. If there's no alternative, note why here.
- Install a tool only when a step needs it.

## Installed

### arduino-cli and the `arduino:avr` core

Compiles and flashes the UNO. The core includes avrdude, which we use for firmware backups. **Needed since:** stage 1, and through stage 2 at least.

| | macOS | Linux | Windows |
|---|---|---|---|
| Install | `brew install arduino-cli` | distro package if there is one, otherwise `brew install arduino-cli` | `winget install ArduinoSA.CLI` |
| Update | `brew upgrade arduino-cli` | same as install method | `winget upgrade ArduinoSA.CLI` |
| Uninstall | `brew uninstall arduino-cli` | same as install method | `winget uninstall ArduinoSA.CLI` |

On every system, install the core with `arduino-cli core install arduino:avr@1.8.8`, update it with `arduino-cli core upgrade`, and remove it with `arduino-cli core uninstall arduino:avr`.

The core lives in `~/Library/Arduino15` on macOS, `~/.arduino15` on Linux and `%LOCALAPPDATA%\Arduino15` on Windows. Other Arduino work on the same computer may share that folder, so remove individual cores rather than the whole folder.

We've only used the macOS commands so far.

### esptool

Reads and writes the camera board's flash. We use it for firmware backups, and in stage 3 for flashing. **Needed since:** stage 1, and through stage 3 at least.

| | macOS | Linux | Windows |
|---|---|---|---|
| Install | `brew install esptool` | distro package if there is one, such as `esptool` on Debian, Ubuntu and Fedora; otherwise a Python virtual environment | `pip install esptool` inside a Python virtual environment |
| Update | `brew upgrade esptool` | same as install method | `pip install --upgrade esptool` in the environment |
| Uninstall | `brew uninstall esptool`, then `brew autoremove` | same as install method | delete the virtual environment |

On macOS, Homebrew also installs `cffi`, `cryptography` and `libyaml` for it if they aren't already there. `brew autoremove` removes them again once nothing else needs them.

We've only used the macOS commands so far.

## Planned

| Tool | For | Install on macOS | Uninstall on macOS | When |
|---|---|---|---|---|
| Python packages for the computer tool | Stage 2 control and testing | A virtual environment in `.venv/`, added to `.gitignore` | Delete `.venv/` | Stage 2 |

Stage 1's sensor checks need neither: they're throwaway scripts using only Python's standard library, and they aren't part of the repo.
