# AGENTS.md – NIMONSPOLY

C++17 Monopoly-like game. CMake + Raylib (GUI always on). No CLI runtime mode.

## Build

```bash
cmake -S . -B build
cmake --build build
```

- Requires CMake >= 3.16, C++17 compiler, and **Raylib** + **pkg-config**.
- Install Raylib (e.g. `brew install raylib`). Ensure `pkg-config --exists raylib` passes.
- Output executable: `bin/game` (Linux/macOS) or `bin/game.exe` (Windows).
- Run: `./bin/game` or `cmake --build build --target run`.
- Clean: `rm -rf build/ bin/`.

## GUI Mode

- **GUI is always enabled.** There is no CLI runtime mode. Raylib window opens on every run.
- CMake always defines `NIMONSPOLY_ENABLE_RAYLIB=1`.
- `include/ui/RaylibCompat.hpp` wraps `<raylib.h>` to avoid name collisions with project enums (`Color`, `RED`, `PINK`, etc.). Include it instead of raw `<raylib.h>`.

## Project Layout

| Directory | Purpose |
|-----------|---------|
| `src/` | `.cpp` source files |
| `include/` | `.hpp` headers |
| `config/` | Game configuration files |
| `data/` | Runtime data / logs |
| `docs/` | Documentation (not committed) |
| `build/` | CMake build directory (generated, gitignored) |
| `bin/` | Executable output (generated, gitignored) |

## Code Conventions

- **Headers:** Include paths relative to `include/`, e.g. `#include "ui/GUIView.hpp"`.
- **Header guards:** ALL_CAPS with underscores (e.g. `CORE_ENGINE_HEADER_GAME_ENGINE_HPP`).
- **Indentation:** Tab-based (observed in existing headers). Follow the style of the file you edit.
- **Namespace:** None by default; classes live in the global namespace.

## Architecture Notes

- `main.cpp` initializes Raylib window, `GameEngine`, `GUIView`, and `GUIInput`.
- `GameEngine` (`core/engine/`) drives the game loop; `GameState` (`core/state/`) holds runtime state.
- `GameBuilder` (`core/engine/`) is used for construction/setup.
- Tiles (`tile/`), cards (`models/cards/`), effects (`models/effects/`), and managers (`manager/`) are the main domain modules.
- `ComputerController` (`controllers/`) implements AI player logic.
- `AssetManager` (`ui/`) is a Singleton for loading fonts and textures.

## Agent Tips

- No test suite exists; verify by building and running the executable.
- When adding new `.cpp` files under `src/`, they are picked up automatically by `file(GLOB_RECURSE ...)` in `CMakeLists.txt`.
- When adding new headers under `include/`, ensure the include path in source files matches the directory structure under `include/`.
- The project uses `using namespace std;` in several headers (observed in `Enums.hpp`, `Exceptions.hpp`). Be cautious of name collisions.
- `*.pdf` and `OOPClassDesign.md` are gitignored and must not be committed.
