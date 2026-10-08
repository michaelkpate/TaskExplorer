# Performance dashboard validation

Open **View → Performance Dashboard** (Ctrl+Shift+D) in a successfully built TaskExplorer.
The window monitors this PC, independently of the selected cluster host. It displays
CPU utilization, physical RAM used/usable capacity, selected GPU utilization, and
dedicated VRAM used/capacity. The RAM detail also shows available memory, in GiB.
Shared GPU memory is shown separately.
Graphs retain 60 seconds; the local counters are read once per second. Pausing the
main application's collection freezes counter values. The dashboard does not start
another hardware collector. Window geometry is saved, and Always on top applies
only to this window.

## Compile check

The object-only target builds the actual dashboard source without linking the Windows
backend. Run from the repository root, supplying a Qt 6.5+ SDK:

```powershell
cmake -S tests/dashboard -B build-dashboard-check -DCMAKE_PREFIX_PATH="path/to/Qt/msvc2022_64" -DCMAKE_TRY_COMPILE_CONFIGURATION=Release
cmake --build build-dashboard-check --config Release
```

Verified on 2026-10-07 with Qt 6.8.3 and MSVC 19.51. This validates compilation,
not linking, rendering, or live hardware accuracy.

## Live acceptance checks (not yet run)

- All four cards fit at the default size and remain legible when resized or scaled.
- CPU/RAM trends track the existing performance views under idle and load.
- Switching between the integrated and discrete GPUs updates both GPU cards and
  resets their history. An idle adapter must not inherit another adapter's load.
- Opening the adapter dropdown for several seconds does not reset its selection.
- Adapter names have no trailing replacement glyphs; RAM's available figure agrees
  with the platform's available-pages/kernel estimate.
- Missing utilization counters show Unavailable; integrated GPUs without dedicated
  capacity do not fabricate a VRAM percentage or divide by zero.
- Always on top works; closing/reopening preserves geometry; repeated menu activation
  focuses the existing window; closing TaskExplorer closes the dashboard.
- Hiding TaskExplorer to the tray or minimizing its main window leaves the independent
  performance dashboard visible and sampling.

## Full application build status

The complete Windows Release build succeeds with Qt 6.8.3 and MSVC 19.51. The
executable is `Bin/windows-/Release/TaskExplorer.exe`; Qt runtime DLLs and platform
plugins are deployed beside it. On 2026-10-07 the app was launched successfully and
its main window remained open during startup verification. The application manifest
enables Common Controls 6 because the bundled phlib uses `TaskDialogIndirect`.

The dashboard object target and complete application both compile. The live visual
and hardware-accuracy checks above still need a person to inspect the running
dashboard on supported hardware. The minimize/ownership change is built as a
side-by-side executable at `Bin/windows-/ReleaseTelemetry/TaskExplorer-Telemetry.exe`
so the currently open executable did not need to be closed or overwritten. This
build also trims the Windows adapter-name terminator and shows available RAM.
