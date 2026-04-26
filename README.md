# SnapWM

SnapWM is a small tiling window manager for Windows, built in C on top of the Win32 API.

It is designed for ultra-wide monitors: one focused window is centered at 60% width, while the remaining windows tile evenly on the left and right sides. It runs in the background and exposes the same actions through hotkeys and a system tray menu.

Inspired by the original [LightWM](https://github.com/nir9/lightwm) by nir9.

## Keybindings

- `alt+q` - quit SnapWM
- `alt+j` - focus next window
- `alt+k` - focus previous window
- `alt+f` - toggle fullscreen for the focused window
- `alt+t` - retile and center the currently focused window
- `alt+shift+t` - retile including minimized windows
- `alt+p` - pause or resume automatic tiling

SnapWM also adds a system tray icon. Right-click it to tile, focus windows, toggle fullscreen, pause tiling, or exit.

## Features

- Center-focused ultra-wide layout with evenly tiled side windows.
- Sticky center window: focus changes do not reshuffle the layout unless you press `alt+t`.
- Pixel-aligned placement using DWM extended frame bounds, so visible window chrome lines up with the tile grid.
- Black DWM borders and square-corner preference for tiled windows.
- Filters minimized, cloaked, tool, owned helper, and overlay-style windows so surfaces like input panels or Overwolf helpers do not consume tile slots.
- Skips windows that Windows refuses to move, then recalculates the layout for the remaining windows.
- Tray menu with mnemonic labels for all main actions.
- No custom workspace model; SnapWM works with the currently visible native Windows desktop.

## Build

Build from a Visual Studio 2022 developer shell:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Use `Debug` instead of `Release` for debug logging.

The binaries are written to `release` or `debug`. Keep `snapwm.dll` next to `snapwm.exe` when running.

## Startup

Run `snapwm.exe`. To start it on login, open `shell:startup` and place a shortcut there.
