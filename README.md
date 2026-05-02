# SloppyFocus

A tiny Win32 tray utility that turns on **focus-follows-mouse** in
Windows 10 / 11 — the focus is given to the window the cursor hovers over,
without raising it. No installer, no admin, single static-CRT binary.

## Behavior

* The cursor's window receives keyboard focus after a configurable hover
  delay (default 200 ms).
* Optionally raise the window to the top instead of just transferring focus.
* Optionally turn the system setting back off when SloppyFocus exits, so the
  feature is only active while the tool runs.

Under the hood SloppyFocus toggles the documented `SPI_SETACTIVEWINDOWTRACKING`
system parameter — there is no hook, no polling, no global state of its own.

## Run

```
SloppyFocus.exe
```

A tray icon appears. Left-click to toggle focus-follows-mouse on/off.
Right-click for the menu:

* **Enabled** — toggle the feature.
* **Settings...** — hover delay, raise-on-focus, autostart, disable-on-exit.
* **Start with Windows** — toggles the `Run` registry entry.
* **Exit**

Launching `SloppyFocus.exe` while it is already running re-opens the Settings
dialog of the existing instance.

## Build

Requires Visual Studio 2022 with the **Desktop development with C++** workload
(MSVC v143 toolset, Windows 10/11 SDK).

```cmd
build.bat
```

The script locates VS2022 via `vswhere`, calls `vcvarsall x64`, runs `rc` and
`cl` directly, and emits `SloppyFocus.exe` next to `build.bat`. Intermediate
files go to `build\`.

## Settings storage

* App settings: `HKCU\SOFTWARE\Baltazar Studios, LLC\SloppyFocus`
* Autostart:    `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\SloppyFocus`
* Focus-follows-mouse + hover delay: standard Windows
  `SPI_SETACTIVEWINDOWTRACKING` / `SPI_SETACTIVEWNDTRKTIMEOUT` (registry-backed).

## Layout

```
src\main.cpp        WinMain, tray, settings dialog, message loop
src\settings.h      Registry helpers (autostart, disable-on-exit)
src\spi.h           SPI_*ACTIVEWINDOWTRACKING wrappers
src\version.h       Version + release URL (single source of truth)
src\resource.h      Resource IDs
res\app.rc          Icon, menu, dialog, version info
res\app.manifest    PerMonitorV2 DPI, asInvoker, comctl6, Win10/11
res\app.ico         Application icon
res\make_icon.ps1   Regenerates app.ico
build.bat           Direct-cl build via vswhere + vcvarsall
.github\workflows\  GitHub Actions CI (Windows runner)
```

## Releases

Latest builds: <https://github.com/gdevic/SloppyFocus/releases>.
Tray menu → **About...** shows the running version and a link.

## License

Copyright (c) 2026 Baltazar Studios, LLC.

Licensed under [Creative Commons Attribution-NonCommercial-ShareAlike 4.0
International](LICENSE) (CC BY-NC-SA 4.0).
