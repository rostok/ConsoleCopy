# ConsoleCopy

Mouse selection and clipboard copying for Far Manager's console output screen.

Far Manager is a text-mode file and archive manager for Windows. Its two-panel interface can be hidden with **Ctrl+O** to show the output of commands run from Far. ConsoleCopy adds text selection to that screen: drag over the visible output, release the mouse button, and paste the result into another application.

ConsoleCopy is a native DLL plugin for **Far Manager 3**. It uses Far's [Plugins API](https://api.farmanager.com/) and appears in the **F11** plugin menu. For more about Far itself, see the [Far Manager website](https://www.farmanager.com/) and [source repository](https://github.com/FarGroup/FarManager).

## Features

- **Shift + left-button drag:** select a continuous range across lines.
- **Ctrl + left-button drag:** select a rectangular block.
- Highlight the selected cells while dragging.
- Copy automatically when the left mouse button is released.
- Show an information and diagnostics dialog through **F11 → ConsoleCopy**.

The selection works only while **both panels are hidden** with Ctrl+O. Normal panel navigation and mouse handling are left to Far.

## Requirements

- Windows and Far Manager **3**, with x86 or x64 matching the plugin DLL.
- Mouse support enabled in Far.
- A console host that supports the Win32 console screen-buffer APIs used by the plugin, including `ReadConsoleOutputW`.

The x86 build is for **Far Manager 3 x86**. It is not compatible with Far Manager 2.

To build from source, you also need Visual Studio with the C++ build tools and a Far Manager source tree. The installed Far directory alone does not contain the headers and MSBuild configuration used by this project.

## Install

1. Build the DLL for the architecture of your Far Manager installation.
2. Create `Plugins\ConsoleCopy` under the Far installation directory if it does not exist.
3. Copy `ConsoleCopy.dll` into that directory.
4. Restart Far.

Press **F11** in the panels window and select **ConsoleCopy**. The information dialog confirms that Far loaded the plugin. Opening this menu item is optional for normal use; the input hook loads automatically.

If the installation is under `Program Files`, copying the DLL may require administrator privileges. Close Far before replacing a DLL that is already loaded.

## Build from source

From the plugin directory, pass the **root of the Far Manager source tree** as the first argument. You can also invoke the scripts by full path from another working directory:

```bat
build-x86.bat "C:\src\FarManager"
build-x64.bat "C:\src\FarManager"
```

That directory must contain at least:

```text
FarManager/
├── _build/vc/config/common.plugins.props
├── far/common/
└── plugins/common/unicode/plugin.hpp
```

The project imports Far's MSBuild properties from this tree. Those properties supply the plugin API include directories, compiler settings, and output location. The scripts locate MSBuild with Visual Studio's `vswhere.exe`, then fall back to an `MSBuild.exe` on `PATH`.

When this plugin lives at `<Far source root>\plugins\consolecopy`, the source-root argument is optional:

```bat
build-x64.bat
```

You can also set `FAR_SOURCE_ROOT` as an environment variable. An explicit argument overrides it. The x86 script selects MSBuild platform `Win32`; the x64 script selects `x64`. Both use the `Release` configuration.

| Build | Output |
| --- | --- |
| x86 | `<Far source root>\_build\vc\_output\product\Release.Win32\Plugins\ConsoleCopy\ConsoleCopy.dll` |
| x64 | `<Far source root>\_build\vc\_output\product\Release.x64\Plugins\ConsoleCopy\ConsoleCopy.dll` |

The shared [build.bat](build.bat) checks the source tree and runs MSBuild. The architecture-specific entry points are [build-x86.bat](build-x86.bat) and [build-x64.bat](build-x64.bat).

## Use

1. Run a command from Far that prints output, for example `dir`.
2. Press **Ctrl+O** to hide both panels and show the console output.
3. Hold **Shift** or **Ctrl** and drag with the left mouse button over the text.
4. Release the button, then paste into a text editor with **Ctrl+V**.

Holding **Shift** makes a continuous selection. Holding **Ctrl** selects the same column range on each row. If both modifiers are held, the rectangular mode takes precedence.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| ConsoleCopy is missing from F11 | Check the `Plugins\ConsoleCopy\ConsoleCopy.dll` location, match the DLL architecture to Far Manager 3, and restart Far. |
| Dragging does not highlight text | Confirm that both panels are hidden with Ctrl+O, Far mouse support is enabled, and the console host passes mouse events to Far. |
| Highlighting works but copying fails | Install the latest DLL and restart Far. Check whether the completed-copy counter increases; if it does, the selection finished and the remaining issue is in clipboard handling. |
| Text cannot be read from the screen | Try a console host with Win32 `ReadConsoleOutputW` support. Some terminal environments do not expose a readable screen buffer. |

The F11 dialog reports counts for mouse events, selection attempts, successful screen reads, and completed copies, plus the last Win32 console-read error code. These values help distinguish input problems from screen-buffer or clipboard problems.

## How it works

ConsoleCopy implements Far's `ProcessConsoleInputW` hook. When a modified left click occurs while both panels are hidden, it snapshots the visible cells from `CONOUT$`, tracks the drag through console input events, and redraws the selected cells with swapped foreground and background colors. On release, it restores the original cells and sends the selected Unicode text to Far's clipboard function. Rectangular selections use Far's column clipboard format.

## Limitations

- Only the **visible console screen** is available. Older scrollback lines are outside the captured buffer.
- The plugin depends on Win32 console screen-buffer access; support varies by console host.
- The copy is based on displayed character cells, so it is not a substitute for capturing a command's original output stream.

## License and warranty

LICENSE: MIT  
WARRANTY: None
