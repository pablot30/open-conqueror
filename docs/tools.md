# Tools

Everything this project needs installed on a computer, and how to remove it. When we add a tool, it goes here in the same change. When we stop needing one, we remove it and take it off this list.

## Repository setup

After cloning, turn on the project's git hooks once:

```
git config core.hooksPath .githooks
```

The pre-push hook refuses direct pushes to `main`, force pushes, and deleting `main` or a `release/` branch. Deleting merged work branches is allowed. GitHub can only enforce this itself on public repos, or on a paid plan. Once the repo is public, apply `.github/rulesets/protect-main.json` with:

```
gh api repos/pablot30/open-conqueror/rulesets --method POST --input .github/rulesets/protect-main.json
```

## Rules

- Install with the system's package manager: Homebrew on macOS, the distro's on Linux, winget on Windows.
- Python packages go in a virtual environment inside the project, never globally.
- Avoid curl-pipe installers and global npm or pip installs. If there's no alternative, note why here.
- Install a tool only when a step needs it.

## Installed

### arduino-cli, its cores and the Servo library

arduino-cli compiles and flashes both firmwares. Its cores add each board's compiler and upload tools; the AVR core also includes avrdude, which we use for firmware backups. **Needed since:** stage 1, and through stage 2 at least. The `arduino:renesas_uno` core is for the Wi-Fi dongle, an Arduino UNO R4 WiFi ([firmware/dongle](../firmware/dongle/README.md)), until our camera firmware joins the home Wi-Fi in stage 3.

| | macOS | Linux | Windows |
|---|---|---|---|
| Install | `brew install arduino-cli` | distro package if there is one, otherwise `brew install arduino-cli` | `winget install ArduinoSA.CLI` |
| Update | `brew upgrade arduino-cli` | same as install method | `winget upgrade ArduinoSA.CLI` |
| Uninstall | `brew uninstall arduino-cli` | same as install method | `winget uninstall ArduinoSA.CLI` |

**Cores and libraries: one copy each, managed by arduino-cli.** The versions we build with are pinned in `scripts/dev.py`: `arduino:avr` 1.8.8, `arduino:renesas_uno` 1.6.0 and the Servo library 1.3.0. On every system:

| | Command |
|---|---|
| Install or fix the pinned versions | `python scripts/dev.py setup` |
| Update | change the pin in `scripts/dev.py`, then run `python scripts/dev.py setup` |
| Uninstall | `arduino-cli core uninstall arduino:avr`, `arduino-cli core uninstall arduino:renesas_uno`, `arduino-cli lib uninstall Servo` |

`dev.py build` checks the installed versions and stops if they differ, rather than downloading its own copy. We don't use arduino-cli build profiles (`sketch.yaml` with `platforms:`): they keep a second copy of every core in an `internal` folder, about 1 GB for our two boards.

Where arduino-cli keeps things:

| | macOS | Linux | Windows |
|---|---|---|---|
| Cores and their tools | `~/Library/Arduino15` | `~/.arduino15` | `%LOCALAPPDATA%\Arduino15` |
| Libraries | `~/Documents/Arduino/libraries` | `~/Arduino/libraries` | `Documents\Arduino\libraries` |
| Build cache, cleared by arduino-cli after 30 days | `~/Library/Caches/arduino` | `~/.cache/arduino` | `%LOCALAPPDATA%\arduino` |

`arduino-cli cache clean` empties its download cache (`staging` in the cores folder). Other Arduino work on the same computer may share these folders, so remove individual cores and libraries rather than whole folders. To remove arduino-cli completely, uninstall it and then delete the three folders above.

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

### Python 3.10 or newer

Runs `scripts/dev.py`, which builds, tests and flashes the firmware, and the `conq` command from the host package. **Needed since:** stage 2.

| | macOS | Linux | Windows |
|---|---|---|---|
| Install | `brew install python` | distro package, such as `python3` | `winget install Python.Python.3.12` |
| Update | `brew upgrade python` | same as install method | `winget upgrade Python.Python.3.12` |
| Uninstall | `brew uninstall python` | same as install method | `winget uninstall Python.Python.3.12` |

The host package in `host/` provides the `conq` command. Its one dependency, pyserial, is pinned in `host/pyproject.toml`. Install it in editable mode into a virtual environment in the repo's `.venv/` folder, which git ignores:

```
python -m venv .venv
.venv/bin/pip install -e host          (macOS and Linux)
.venv\Scripts\pip install -e host      (Windows)
```

Then run `.venv/bin/conq`, or `.venv\Scripts\conq` on Windows, or activate the environment and run `conq`. Editable mode means changes to the code take effect without reinstalling. To remove it, delete `.venv/`.

### CMake and a C++ compiler

Build and run the firmware's unit tests on the computer. **Needed since:** stage 2.

| | macOS | Linux | Windows |
|---|---|---|---|
| Install | `brew install cmake`, and the compiler from `xcode-select --install` | distro packages, such as `cmake` and `g++` | `winget install Kitware.CMake` and `winget install Microsoft.VisualStudio.2022.BuildTools`, choosing the C++ workload |
| Update | `brew upgrade cmake` | same as install method | `winget upgrade` with the same IDs |
| Uninstall | `brew uninstall cmake` | same as install method | `winget uninstall` with the same IDs |

### cppcheck, optional

CI runs it on every pull request, so it doesn't need to be on your computer. Install it only to run `python scripts/dev.py lint` locally: `brew install cppcheck` on macOS, the distro's `cppcheck` package on Linux, or `winget install Cppcheck.Cppcheck` on Windows. Remove it the same way.

## Planned

Nothing yet.
